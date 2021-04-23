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
 * @file RTPSMessageGroup.h
 *
 */

#ifndef _FASTDDS_RTPS_RTPSMESSAGEGROUP_H_
#define _FASTDDS_RTPS_RTPSMESSAGEGROUP_H_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/messages/RTPSMessageSenderInterface.hpp>
#include <fastdds/rtps/messages/RTPSMessageCreator.h>
#include <fastdds/rtps/common/FragmentNumber.h>

#include <vector>
#include <chrono>
#include <cassert>
#include <memory>


namespace eprosima {
namespace fastrtps {
namespace rtps {

class RTPSParticipantImpl;
class Endpoint;
class RTPSMessageGroup_t;

/**
 * RTPSMessageGroup Class used to construct a RTPS message.
 * @ingroup WRITER_MODULE
 */
class RTPSMessageGroup
{
public:

    class timeout : public std::runtime_error
    {
    public:

        timeout()
            : std::runtime_error("timeout")
        {
        }

        virtual ~timeout() = default;
    };

    /**
     * Basic constructor.
     * Constructs a RTPSMessageGroup allowing the destination endpoints to change.
     * @param participant Pointer to the participant sending data.
     * @param endpoint Pointer to the endpoint sending data.
     * @param msg_sender Reference to message sender interface.
     * @param max_blocking_time_point Future time point where blocking send should end.
     */
    RTPSMessageGroup(
            RTPSParticipantImpl* participant,
            Endpoint* endpoint,
            const RTPSMessageSenderInterface& msg_sender,
            std::chrono::steady_clock::time_point max_blocking_time_point =
            std::chrono::steady_clock::now() + std::chrono::hours(24));

    ~RTPSMessageGroup() noexcept(false);

    /**
     * Adds a DATA message to the group.
     * @param change Reference to the cache change to send.
     * @param expects_inline_qos True when one destination is expecting inline QOS.
     * @return True when message was added to the group.
     */
    bool add_data(
            const CacheChange_t& change,
            bool expects_inline_qos);

    /**
     * Adds a DATA_FRAG message to the group.
     * @param change Reference to the cache change to send.
     * @param fragment_number Index (1 based) of the fragment to send.
     * @param expects_inline_qos True when one destination is expecting inline QOS.
     * @return True when message was added to the group.
     */
    bool add_data_frag(
            const CacheChange_t& change,
            const uint32_t fragment_number,
            bool expects_inline_qos);

    /**
     * Adds a HEARTBEAT message to the group.
     * @param first_seq First available sequence number.
     * @param last_seq Last available sequence number.
     * @param count Counting identifier.
     * @param is_final Should final flag be set?
     * @param liveliness_flag Should liveliness flag be set?
     * @return True when message was added to the group.
     */
    bool add_heartbeat(
            const SequenceNumber_t& first_seq,
            const SequenceNumber_t& last_seq,
            Count_t count,
            bool is_final,
            bool liveliness_flag);

    /**
     * Adds one or more GAP messages to the group.
     * @param changes_seq_numbers Set of missed sequence numbers.
     * @return True when messages were added to the group.
     */
    bool add_gap(
            std::set<SequenceNumber_t>& changes_seq_numbers);

    /**
     * Adds one GAP message to the group.
     * @param gap_initial_sequence Start of consecutive sequence numbers.
     * @param gap_bitmap Bitmap of non-consecutive sequence numbers.
     * @return True when message was added to the group.
     */
    bool add_gap(
            const SequenceNumber_t& gap_initial_sequence,
            const SequenceNumberSet_t& gap_bitmap);

    /**
     * Adds one GAP message to the group.
     * @param gap_initial_sequence Start of consecutive sequence numbers.
     * @param gap_bitmap Bitmap of non-consecutive sequence numbers.
     * @param reader_guid GUID of the destination reader.
     * @return True when message was added to the group.
     */
    bool add_gap(
            const SequenceNumber_t& gap_initial_sequence,
            const SequenceNumberSet_t& gap_bitmap,
            const GUID_t& reader_guid);

    /**
     * Adds a ACKNACK message to the group.
     * @param seq_num_set Set of missing sequence numbers.
     * @param count Counting identifier.
     * @param final_flag Should final flag be set?
     * @return True when message was added to the group.
     */
    bool add_acknack(
            const SequenceNumberSet_t& seq_num_set,
            int32_t count,
            bool final_flag);

    /**
     * Adds a NACKFRAG message to the group.
     * @param seq_number Sequence number being nack'ed.
     * @param fn_state Set of missing fragment numbers.
     * @param count Counting identifier.
     * @return True when message was added to the group.
     */
    bool add_nackfrag(
            const SequenceNumber_t& seq_number,
            FragmentNumberSet_t fn_state,
            int32_t count);

    inline uint32_t get_current_bytes_processed() const
    {
        return currentBytesSent_ + full_msg_->length;
    }

    /**
     * To be used whenever destination locators/guids change between two add_xxx calls.
     * Automatically called inside add_xxx calls if destinations_have_changed() method of
     * RTPSMessageSenderInterface returns true.
     * May become private again with a refactor of RTPSMessageSenderInterface, adding a
     * group_has_been_flushed() method.
     */
    void flush_and_reset();

    //! Maximum fragment size minus the headers
    static inline constexpr uint32_t get_max_fragment_payload_size()
    {
        // Max fragment is 64KBytes_max - header - inlineqos - 3(for better alignment)
        return std::numeric_limits<uint16_t>::max() - data_frag_header_size_ - max_inline_qos_size_ - 3;
    }

private:

    static constexpr uint32_t data_frag_header_size_ = 28;
    static constexpr uint32_t max_inline_qos_size_ = 32;

    void append_pending_payload();

    void reset_to_header();

    void flush();

    void send();

    void check_and_maybe_flush()
    {
        check_and_maybe_flush(sender_.destination_guid_prefix());
    }

    void check_and_maybe_flush(
            const GuidPrefix_t& destination_guid_prefix);

    bool insert_submessage(
            bool is_big_submessage)
    {
        return insert_submessage(sender_.destination_guid_prefix(), is_big_submessage);
    }

    bool insert_submessage(
            const GuidPrefix_t& destination_guid_prefix,
            bool is_big_submessage);

    bool add_info_dst_in_buffer(
            CDRMessage_t* buffer,
            const GuidPrefix_t& destination_guid_prefix);

    bool add_info_ts_in_buffer(
            const Time_t& timestamp);

    bool create_gap_submessage(
            const SequenceNumber_t& gap_initial_sequence,
            const SequenceNumberSet_t& gap_bitmap,
            const EntityId_t& reader_id);

    const RTPSMessageSenderInterface& sender_;

    Endpoint* endpoint_;

    CDRMessage_t* full_msg_;

    CDRMessage_t* submessage_msg_;

    uint32_t currentBytesSent_;

    GuidPrefix_t current_dst_;

    RTPSParticipantImpl* participant_;

#if HAVE_SECURITY

    CDRMessage_t* encrypt_msg_;

#endif // if HAVE_SECURITY

    std::chrono::steady_clock::time_point max_blocking_time_point_;

    std::unique_ptr<RTPSMessageGroup_t> send_buffer_;

    octet* pending_data_ = nullptr;
    uint32_t pending_data_size_ = 0;
    uint32_t pending_padding_ = 0;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif /* _FASTDDS_RTPS_RTPSMESSAGEGROUP_H_ */
