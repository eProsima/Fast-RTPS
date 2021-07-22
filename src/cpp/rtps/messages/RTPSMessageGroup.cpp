// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file RTPSMessageGroup.cpp
 *
 */

#include <algorithm>

#include <fastdds/dds/log/Log.hpp>
#include <fastdds/rtps/messages/RTPSMessageCreator.h>
#include <fastdds/rtps/messages/RTPSMessageGroup.h>
#include <fastdds/rtps/reader/RTPSReader.h>
#include <fastdds/rtps/writer/RTPSWriter.h>

#include <rtps/messages/RTPSGapBuilder.hpp>
#include <rtps/messages/RTPSMessageGroup_t.hpp>
#include <rtps/participant/RTPSParticipantImpl.h>

#include <statistics/rtps/messages/RTPSStatisticsMessages.hpp>

namespace eprosima {
namespace fastrtps {
namespace rtps {

static bool append_message(
        CDRMessage_t* full_msg,
        CDRMessage_t* submsg)
{
#ifndef FASTDDS_STATISTICS
    return CDRMessage::appendMsg(full_msg, submsg);
#else
    // Keep room for the statistics submessage by reducing max_size while appending submessage
    full_msg->max_size -= eprosima::fastdds::statistics::rtps::statistics_submessage_length;
    bool ret_val = CDRMessage::appendMsg(full_msg, submsg);
    full_msg->max_size += eprosima::fastdds::statistics::rtps::statistics_submessage_length;
    return ret_val;
#endif // FASTDDS_STATISTICS
}

bool sort_changes_group (
        CacheChange_t* c1,
        CacheChange_t* c2)
{
    return(c1->sequenceNumber < c2->sequenceNumber);
}

bool sort_SeqNum(
        const SequenceNumber_t& s1,
        const SequenceNumber_t& s2)
{
    return(s1 < s2);
}

typedef std::pair<SequenceNumber_t, SequenceNumberSet_t> pair_T;

bool compare_remote_participants(
        const std::vector<GUID_t>& remote_participants1,
        const std::vector<GuidPrefix_t>& remote_participants2)
{
    if (remote_participants1.size() == remote_participants2.size())
    {
        for (auto& participant : remote_participants1)
        {
            if (std::find(remote_participants2.begin(), remote_participants2.end(), participant.guidPrefix) ==
                    remote_participants2.end())
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

void get_participant_from_endpoint(
        const GUID_t& endpoint,
        std::vector<GuidPrefix_t>& participants)
{
    if (std::find(participants.begin(), participants.end(), endpoint.guidPrefix) == participants.end())
    {
        participants.push_back(endpoint.guidPrefix);
    }
}

void get_participants_from_endpoints(
        const std::vector<GUID_t>& endpoints,
        std::vector<GuidPrefix_t>& participants)
{
    participants.clear();

    for (const GUID_t& endpoint : endpoints)
    {
        if (std::find(participants.begin(), participants.end(), endpoint.guidPrefix) == participants.end())
        {
            participants.push_back(endpoint.guidPrefix);
        }
    }
}

const EntityId_t& get_entity_id(
        const std::vector<GUID_t>& endpoints)
{
    if (endpoints.size() == 0)
    {
        return c_EntityId_Unknown;
    }

    const EntityId_t& entityid = endpoints.at(0).entityId;

    for (auto it = endpoints.begin() + 1; it != endpoints.end(); ++it)
    {
        if (entityid != it->entityId)
        {
            return c_EntityId_Unknown;
        }
    }

    return entityid;
}

RTPSMessageGroup::RTPSMessageGroup(
        RTPSParticipantImpl* participant,
        bool internal_buffer)
    : participant_(participant)
    , send_buffer_(!internal_buffer ? participant->get_send_buffer() : nullptr)
    , internal_buffer_(internal_buffer)
{
    // Avoid warning when neither SECURITY nor DEBUG is used
    (void)participant;

    assert(participant);

    if (internal_buffer)
    {
        const GuidPrefix_t& guid_prefix = participant->getGuid().guidPrefix;
        constexpr size_t align_size = sizeof(octet*) - 1;
        uint32_t payload_size = participant->getMaxMessageSize();
        assert(payload_size > 0u);
        payload_size = (payload_size + align_size) & ~align_size;
        send_buffer_.reset(new RTPSMessageGroup_t(
#if HAVE_SECURITY
                    participant->is_secure(),
#endif // if HAVE_SECURITY
                    payload_size, guid_prefix
                    ));
    }

    full_msg_ = &(send_buffer_->rtpsmsg_fullmsg_);
    submessage_msg_ = &(send_buffer_->rtpsmsg_submessage_);

    // Init RTPS message.
    reset_to_header();

    CDRMessage::initCDRMsg(submessage_msg_);

#if HAVE_SECURITY
    if (participant->is_secure())
    {
        encrypt_msg_ = &(send_buffer_->rtpsmsg_encrypt_);
        CDRMessage::initCDRMsg(encrypt_msg_);
    }
#endif // if HAVE_SECURITY
}

RTPSMessageGroup::RTPSMessageGroup(
        RTPSParticipantImpl* participant,
        Endpoint* endpoint,
        const RTPSMessageSenderInterface* msg_sender,
        std::chrono::steady_clock::time_point max_blocking_time_point)
    : RTPSMessageGroup(participant)
{
    assert(endpoint);

    endpoint_ = endpoint;
    sender_ = msg_sender;
    max_blocking_time_point_ = max_blocking_time_point;
    max_blocking_time_is_set_ = true;
}

RTPSMessageGroup::~RTPSMessageGroup() noexcept(false)
{
    try
    {
        send();
    }
    catch (...)
    {
        if (!internal_buffer_)
        {
            participant_->return_send_buffer(std::move(send_buffer_));
        }
        throw;
    }

    if (!internal_buffer_)
    {
        participant_->return_send_buffer(std::move(send_buffer_));
    }
}

void RTPSMessageGroup::reset_to_header()
{
    CDRMessage::initCDRMsg(full_msg_);
    full_msg_->pos = RTPSMESSAGE_HEADER_SIZE;
    full_msg_->length = RTPSMESSAGE_HEADER_SIZE;
}

void RTPSMessageGroup::flush()
{
    send();

    reset_to_header();
}

void RTPSMessageGroup::send()
{
    if (endpoint_ && sender_)
    {

        CDRMessage_t* msgToSend = full_msg_;

        if (full_msg_->length > RTPSMESSAGE_HEADER_SIZE)
        {
            std::unique_lock<RecursiveTimedMutex> lock(endpoint_->getMutex());

#if HAVE_SECURITY
            // TODO(Ricardo) Control message size if it will be encrypted.
            if (participant_->security_attributes().is_rtps_protected && endpoint_->supports_rtps_protection())
            {
                CDRMessage::initCDRMsg(encrypt_msg_);
                full_msg_->pos = RTPSMESSAGE_HEADER_SIZE;
                encrypt_msg_->pos = RTPSMESSAGE_HEADER_SIZE;
                encrypt_msg_->length = RTPSMESSAGE_HEADER_SIZE;
                memcpy(encrypt_msg_->buffer, full_msg_->buffer, RTPSMESSAGE_HEADER_SIZE);

                if (!participant_->security_manager().encode_rtps_message(*full_msg_, *encrypt_msg_,
                        sender_->remote_participants()))
                {
                    logError(RTPS_WRITER, "Error encoding rtps message.");
                    return;
                }

                msgToSend = encrypt_msg_;
            }
#endif // if HAVE_SECURITY

            eprosima::fastdds::statistics::rtps::add_statistics_submessage(msgToSend);

            if (!sender_->send(msgToSend,
                    max_blocking_time_is_set_ ? max_blocking_time_point_ : (std::chrono::steady_clock::now() +
                    std::chrono::hours(24))))
            {
                throw timeout();
            }
            current_sent_bytes_ += msgToSend->length;
        }
    }
}

void RTPSMessageGroup::flush_and_reset()
{
    // Flush
    flush();

    current_dst_ = c_GuidPrefix_Unknown;
}

void RTPSMessageGroup::check_and_maybe_flush(
        const GuidPrefix_t& destination_guid_prefix)
{
    assert(nullptr != sender_);

    CDRMessage::initCDRMsg(submessage_msg_);

    if (sender_->destinations_have_changed())
    {
        flush_and_reset();
    }

    add_info_dst_in_buffer(submessage_msg_, destination_guid_prefix);
}

bool RTPSMessageGroup::insert_submessage(
        const GuidPrefix_t& destination_guid_prefix,
        bool is_big_submessage)
{
    if (!append_message(full_msg_, submessage_msg_))
    {
        // Retry
        flush_and_reset();
        add_info_dst_in_buffer(full_msg_, destination_guid_prefix);

        if (!append_message(full_msg_, submessage_msg_))
        {
            logError(RTPS_WRITER, "Cannot add RTPS submesage to the CDRMessage. Buffer too small");
            return false;
        }
    }

    // Messages with a submessage bigger than 64KB cannot have more submessages and should be flushed
    if (is_big_submessage)
    {
        flush();
    }

    return true;
}

bool RTPSMessageGroup::add_info_dst_in_buffer(
        CDRMessage_t* buffer,
        const GuidPrefix_t& destination_guid_prefix)
{
#if HAVE_SECURITY
    // Add INFO_SRC when we are at the beginning of the message and RTPS protection is enabled
    if ((full_msg_->length == RTPSMESSAGE_HEADER_SIZE) &&
            participant_->security_attributes().is_rtps_protected && endpoint_->supports_rtps_protection())
    {
        RTPSMessageCreator::addSubmessageInfoSRC(buffer, c_ProtocolVersion, c_VendorId_eProsima,
                participant_->getGuid().guidPrefix);
    }
#endif // if HAVE_SECURITY

    if (current_dst_ != destination_guid_prefix)
    {
        current_dst_ = destination_guid_prefix;
        RTPSMessageCreator::addSubmessageInfoDST(buffer, current_dst_);
    }

    return true;
}

bool RTPSMessageGroup::add_info_ts_in_buffer(
        const Time_t& timestamp)
{
    assert(nullptr != sender_);

    logInfo(RTPS_WRITER, "Sending INFO_TS message");

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY

    if (!RTPSMessageCreator::addSubmessageInfoTS(submessage_msg_, timestamp, false))
    {
        logError(RTPS_WRITER, "Cannot add INFO_TS submsg to the CDRMessage. Buffer too small");
        return false;
    }

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_writer_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_WRITER, "Cannot encrypt DATA submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    return true;
}

bool RTPSMessageGroup::add_data(
        const CacheChange_t& change,
        bool expectsInlineQos)
{
    assert(nullptr != sender_);

    logInfo(RTPS_WRITER, "Sending relevant changes as DATA/DATA_FRAG messages");

    // Check limitation
    if (0 < sent_bytes_limitation_ &&
            (change.serializedPayload.length > (sent_bytes_limitation_ - (current_sent_bytes_ + full_msg_->length))))
    {
        flush_and_reset();
        throw limit_exceeded();
    }

    // Check preconditions. If fail flush and reset.
    check_and_maybe_flush();
    add_info_ts_in_buffer(change.sourceTimestamp);

    InlineQosWriter* inlineQos = nullptr;
    if (expectsInlineQos)
    {
        //TODOG INLINEQOS
        //inlineQos = W->getInlineQos();
    }

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY
    const EntityId_t& readerId = get_entity_id(sender_->remote_guids());

    CacheChange_t change_to_add;
    change_to_add.copy_not_memcpy(&change);
    change_to_add.serializedPayload.data = change.serializedPayload.data;
    change_to_add.serializedPayload.length = change.serializedPayload.length;

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_payload_protected)
    {
        SerializedPayload_t encrypt_payload;
        encrypt_payload.data = encrypt_msg_->buffer;
        encrypt_payload.max_size = encrypt_msg_->max_size;

        // If payload protection, encode payload
        if (!participant_->security_manager().encode_serialized_payload(change_to_add.serializedPayload,
                encrypt_payload, endpoint_->getGuid()))
        {
            logError(RTPS_WRITER, "Error encoding change " << change.sequenceNumber);
            change_to_add.serializedPayload.data = nullptr;
            encrypt_payload.data = nullptr;
            return false;
        }

        change_to_add.serializedPayload.data = encrypt_msg_->buffer;
        encrypt_payload.data = nullptr;
        change_to_add.serializedPayload.length = encrypt_payload.length;
    }
#endif // if HAVE_SECURITY

    // TODO (Ricardo). Check to create special wrapper.
    bool is_big_submessage;
    if (!RTPSMessageCreator::addSubmessageData(submessage_msg_, &change_to_add, endpoint_->getAttributes().topicKind,
            readerId, expectsInlineQos, inlineQos, &is_big_submessage))
    {
        logError(RTPS_WRITER, "Cannot add DATA submsg to the CDRMessage. Buffer too small");
        change_to_add.serializedPayload.data = nullptr;
        return false;
    }
    change_to_add.serializedPayload.data = nullptr;

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_writer_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_WRITER, "Cannot encrypt DATA submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    return insert_submessage(is_big_submessage);
}

bool RTPSMessageGroup::add_data_frag(
        const CacheChange_t& change,
        const uint32_t fragment_number,
        bool expectsInlineQos)
{
    assert(nullptr != sender_);

    logInfo(RTPS_WRITER, "Sending relevant changes as DATA/DATA_FRAG messages");

    // Calculate fragment start
    uint32_t fragment_start = change.getFragmentSize() * (fragment_number - 1);
    // Calculate fragment size. If last fragment, size may be smaller
    uint32_t fragment_size = fragment_number < change.getFragmentCount() ? change.getFragmentSize() :
            change.serializedPayload.length - fragment_start;
    // Check limitation
    if (0 < sent_bytes_limitation_ &&
            (fragment_size > (sent_bytes_limitation_ - (current_sent_bytes_ + full_msg_->length))))
    {
        flush_and_reset();
        throw limit_exceeded();
    }

    // Check preconditions. If fail flush and reset.
    check_and_maybe_flush();
    add_info_ts_in_buffer(change.sourceTimestamp);

    InlineQosWriter* inlineQos = nullptr;
    if (expectsInlineQos)
    {
        //TODOG INLINEQOS
        //inlineQos = W->getInlineQos();
    }

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY
    const EntityId_t& readerId = get_entity_id(sender_->remote_guids());

    // TODO (Ricardo). Check to create special wrapper.
    CacheChange_t change_to_add;
    change_to_add.copy_not_memcpy(&change);
    change_to_add.serializedPayload.data = change.serializedPayload.data + fragment_start;
    change_to_add.serializedPayload.length = fragment_size;

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_payload_protected)
    {
        SerializedPayload_t encrypt_payload;
        encrypt_payload.data = encrypt_msg_->buffer;
        encrypt_payload.max_size = encrypt_msg_->max_size;

        // If payload protection, encode payload
        if (!participant_->security_manager().encode_serialized_payload(change_to_add.serializedPayload,
                encrypt_payload, endpoint_->getGuid()))
        {
            logError(RTPS_WRITER, "Error encoding change " << change.sequenceNumber);
            change_to_add.serializedPayload.data = nullptr;
            encrypt_payload.data = nullptr;
            return false;
        }

        change_to_add.serializedPayload.data = encrypt_msg_->buffer;
        encrypt_payload.data = nullptr;
        change_to_add.serializedPayload.length = encrypt_payload.length;
    }
#endif // if HAVE_SECURITY

    if (!RTPSMessageCreator::addSubmessageDataFrag(submessage_msg_, &change, fragment_number,
            change_to_add.serializedPayload, endpoint_->getAttributes().topicKind, readerId,
            expectsInlineQos, inlineQos))
    {
        logError(RTPS_WRITER, "Cannot add DATA_FRAG submsg to the CDRMessage. Buffer too small");
        change_to_add.serializedPayload.data = nullptr;
        return false;
    }
    change_to_add.serializedPayload.data = nullptr;

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_writer_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_WRITER, "Cannot encrypt DATA submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    return insert_submessage(false);
}

bool RTPSMessageGroup::add_heartbeat(
        const SequenceNumber_t& firstSN,
        const SequenceNumber_t& lastSN,
        const Count_t count,
        bool isFinal,
        bool livelinessFlag)
{
    assert(nullptr != sender_);

    check_and_maybe_flush();

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY

    const EntityId_t& readerId = get_entity_id(sender_->remote_guids());

    if (!RTPSMessageCreator::addSubmessageHeartbeat(submessage_msg_, readerId, endpoint_->getGuid().entityId,
            firstSN, lastSN, count, isFinal, livelinessFlag))
    {
        logError(RTPS_WRITER, "Cannot add HEARTBEAT submsg to the CDRMessage. Buffer too small");
        return false;
    }

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_writer_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_WRITER, "Cannot encrypt HEARTBEAT submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    return insert_submessage(false);
}

// TODO (Ricardo) Check with standard 8.3.7.4.5
bool RTPSMessageGroup::add_gap(
        std::set<SequenceNumber_t>& changesSeqNum)
{
    RTPSGapBuilder gap_builder(*this);
    for (const SequenceNumber_t& seq : changesSeqNum)
    {
        if (!gap_builder.add(seq))
        {
            return false;
        }
    }

    return gap_builder.flush();
}

bool RTPSMessageGroup::add_gap(
        const SequenceNumber_t& gap_initial_sequence,
        const SequenceNumberSet_t& gap_bitmap)
{
    assert(nullptr != sender_);

    // Check preconditions. If fail flush and reset.
    check_and_maybe_flush();

    const EntityId_t& readerId = get_entity_id(sender_->remote_guids());

    if (!create_gap_submessage(gap_initial_sequence, gap_bitmap, readerId))
    {
        return false;
    }

    return insert_submessage(false);
}

bool RTPSMessageGroup::add_gap(
        const SequenceNumber_t& gap_initial_sequence,
        const SequenceNumberSet_t& gap_bitmap,
        const GUID_t& reader_guid)
{
    // Check preconditions. If fail flush and reset.
    check_and_maybe_flush(reader_guid.guidPrefix);

    const EntityId_t& readerId = reader_guid.entityId;

    if (!create_gap_submessage(gap_initial_sequence, gap_bitmap, readerId))
    {
        return false;
    }

    return insert_submessage(reader_guid.guidPrefix, false);
}

bool RTPSMessageGroup::create_gap_submessage(
        const SequenceNumber_t& gap_initial_sequence,
        const SequenceNumberSet_t& gap_bitmap,
        const EntityId_t& reader_id)
{
    assert(nullptr != sender_);

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY

    if (!RTPSMessageCreator::addSubmessageGap(submessage_msg_, gap_initial_sequence, gap_bitmap,
            reader_id, endpoint_->getGuid().entityId))
    {
        logError(RTPS_WRITER, "Cannot add GAP submsg to the CDRMessage. Buffer too small");
        return false;
    }

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_writer_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_WRITER, "Cannot encrypt DATA submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    // Notify the statistics module, note that only writers add gaps
    assert(nullptr != dynamic_cast<RTPSWriter*>(endpoint_));
    static_cast<RTPSWriter*>(endpoint_)->on_gap();

    return true;
}

bool RTPSMessageGroup::add_acknack(
        const SequenceNumberSet_t& SNSet,
        int32_t count,
        bool finalFlag)
{
    assert(nullptr != sender_);

    // A vector is used to avoid dynamic allocations, but only first item is used
    size_t n_guids = sender_->remote_guids().size();
    if (n_guids == 0)
    {
        return false;
    }
    assert(n_guids == 1);

    check_and_maybe_flush();

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY

    if (!RTPSMessageCreator::addSubmessageAcknack(submessage_msg_, endpoint_->getGuid().entityId,
            sender_->remote_guids().front().entityId, SNSet, count, finalFlag))
    {
        logError(RTPS_READER, "Cannot add ACKNACK submsg to the CDRMessage. Buffer too small");
        return false;
    }

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_reader_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_READER, "Cannot encrypt ACKNACK submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    // Notify the statistics module, note that only readers add acknacks
    assert(nullptr != dynamic_cast<RTPSReader*>(endpoint_));
    static_cast<fastdds::statistics::StatisticsReaderImpl*>(static_cast<RTPSReader*>(endpoint_))->on_acknack(count);

    return insert_submessage(false);
}

bool RTPSMessageGroup::add_nackfrag(
        const SequenceNumber_t& writerSN,
        FragmentNumberSet_t fnState,
        int32_t count)
{
    assert(nullptr != sender_);

    // A vector is used to avoid dynamic allocations, but only first item is used
    assert(sender_->remote_guids().size() == 1);

    check_and_maybe_flush();

#if HAVE_SECURITY
    uint32_t from_buffer_position = submessage_msg_->pos;
#endif // if HAVE_SECURITY

    if (!RTPSMessageCreator::addSubmessageNackFrag(submessage_msg_, endpoint_->getGuid().entityId,
            sender_->remote_guids().front().entityId, writerSN, fnState, count))
    {
        logError(RTPS_READER, "Cannot add ACKNACK submsg to the CDRMessage. Buffer too small");
        return false;
    }

#if HAVE_SECURITY
    if (endpoint_->getAttributes().security_attributes().is_submessage_protected)
    {
        submessage_msg_->pos = from_buffer_position;
        CDRMessage::initCDRMsg(encrypt_msg_);
        if (!participant_->security_manager().encode_reader_submessage(*submessage_msg_, *encrypt_msg_,
                endpoint_->getGuid(), sender_->remote_guids()))
        {
            logError(RTPS_READER, "Cannot encrypt ACKNACK submessage for writer " << endpoint_->getGuid());
            return false;
        }

        if ((submessage_msg_->max_size - from_buffer_position) >= encrypt_msg_->length)
        {
            memcpy(&submessage_msg_->buffer[from_buffer_position], encrypt_msg_->buffer, encrypt_msg_->length);
            submessage_msg_->length = from_buffer_position + encrypt_msg_->length;
            submessage_msg_->pos = submessage_msg_->length;
        }
        else
        {
            logError(RTPS_OUT, "Not enough memory to copy encrypted data for " << endpoint_->getGuid());
            return false;
        }
    }
#endif // if HAVE_SECURITY

    // Notify the statistics module, note that only readers add NACKFRAGs
    assert(nullptr != dynamic_cast<RTPSReader*>(endpoint_));
    static_cast<RTPSReader*>(endpoint_)->on_nackfrag(count);

    return insert_submessage(false);
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
