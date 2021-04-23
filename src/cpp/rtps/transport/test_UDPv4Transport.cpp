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

#include <rtps/transport/test_UDPv4Transport.h>

#include <cstdlib>
#include <functional>

#include <asio.hpp>

using namespace std;

namespace eprosima {
namespace fastdds {
namespace rtps {

using octet = fastrtps::rtps::octet;
using CDRMessage_t = fastrtps::rtps::CDRMessage_t;
using SubmessageHeader_t = fastrtps::rtps::SubmessageHeader_t;
using SequenceNumber_t = fastrtps::rtps::SequenceNumber_t;
using EntityId_t = fastrtps::rtps::EntityId_t;

std::vector<std::vector<octet>> test_UDPv4Transport::test_UDPv4Transport_DropLog;
uint32_t test_UDPv4Transport::test_UDPv4Transport_DropLogLength = 0;
bool test_UDPv4Transport::test_UDPv4Transport_ShutdownAllNetwork = false;
bool test_UDPv4Transport::always_drop_participant_builtin_topic_data = false;
bool test_UDPv4Transport::simulate_no_interfaces = false;

test_UDPv4Transport::test_UDPv4Transport(
        const test_UDPv4TransportDescriptor& descriptor)
    : drop_data_messages_percentage_(descriptor.dropDataMessagesPercentage)
    , drop_data_messages_filter_(descriptor.drop_data_messages_filter_)
    , drop_participant_builtin_topic_data_(descriptor.dropParticipantBuiltinTopicData)
    , drop_publication_builtin_topic_data_(descriptor.dropPublicationBuiltinTopicData)
    , drop_subscription_builtin_topic_data_(descriptor.dropSubscriptionBuiltinTopicData)
    , drop_data_frag_messages_percentage_(descriptor.dropDataFragMessagesPercentage)
    , drop_data_frag_messages_filter_(descriptor.drop_data_frag_messages_filter_)
    , drop_heartbeat_messages_percentage_(descriptor.dropHeartbeatMessagesPercentage)
    , drop_heartbeat_messages_filter_(descriptor.drop_heartbeat_messages_filter_)
    , drop_ack_nack_messages_percentage_(descriptor.dropAckNackMessagesPercentage)
    , drop_ack_nack_messages_filter_(descriptor.drop_ack_nack_messages_filter_)
    , drop_gap_messages_percentage_(descriptor.dropGapMessagesPercentage)
    , drop_gap_messages_filter_(descriptor.drop_gap_messages_filter_)
    , percentage_of_messages_to_drop_(descriptor.percentageOfMessagesToDrop)
    , messages_filter_(descriptor.messages_filter_)
    , sequence_number_data_messages_to_drop_(descriptor.sequenceNumberDataMessagesToDrop)
{
    test_UDPv4Transport_DropLogLength = 0;
    test_UDPv4Transport_ShutdownAllNetwork = false;
    UDPv4Transport::mSendBufferSize = descriptor.sendBufferSize;
    UDPv4Transport::mReceiveBufferSize = descriptor.receiveBufferSize;
    for (auto interf : descriptor.interfaceWhiteList)
    {
        UDPv4Transport::interface_whitelist_.emplace_back(asio::ip::address_v4::from_string(interf));
    }
    test_UDPv4Transport_DropLog.clear();
    test_UDPv4Transport_DropLogLength = descriptor.dropLogLength;
}

test_UDPv4TransportDescriptor::test_UDPv4TransportDescriptor()
    : SocketTransportDescriptor(s_maximumMessageSize, s_maximumInitialPeersRange)
    , dropDataMessagesPercentage(0)
    , drop_data_messages_filter_([](CDRMessage_t&)
            {
                return false;
            }),
    dropParticipantBuiltinTopicData(false),
    dropPublicationBuiltinTopicData(false),
    dropSubscriptionBuiltinTopicData(false),
    dropDataFragMessagesPercentage(0),
    drop_data_frag_messages_filter_([](CDRMessage_t&)
            {
                return false;
            }),
    dropHeartbeatMessagesPercentage(0),
    drop_heartbeat_messages_filter_([](CDRMessage_t&)
            {
                return false;
            }),
    dropAckNackMessagesPercentage(0),
    drop_ack_nack_messages_filter_([](CDRMessage_t&)
            {
                return false;
            }),
    dropGapMessagesPercentage(0),
    drop_gap_messages_filter_([](CDRMessage_t&)
            {
                return false;
            }),
    percentageOfMessagesToDrop(0),
    messages_filter_([](CDRMessage_t&)
            {
                return false;
            }),
    sequenceNumberDataMessagesToDrop(),
    dropLogLength(0)
{
}

TransportInterface* test_UDPv4TransportDescriptor::create_transport() const
{
    return new test_UDPv4Transport(*this);
}

bool test_UDPv4TransportDescriptor::operator ==(
        const test_UDPv4TransportDescriptor& t) const
{
    return (this->dropDataMessagesPercentage == t.dropDataMessagesPercentage &&
           this->dropParticipantBuiltinTopicData == t.dropParticipantBuiltinTopicData &&
           this->dropPublicationBuiltinTopicData == t.dropPublicationBuiltinTopicData &&
           this->dropSubscriptionBuiltinTopicData == t.dropSubscriptionBuiltinTopicData &&
           this->dropDataFragMessagesPercentage == t.dropDataFragMessagesPercentage &&
           this->dropHeartbeatMessagesPercentage == t.dropHeartbeatMessagesPercentage &&
           this->dropAckNackMessagesPercentage == t.dropAckNackMessagesPercentage &&
           this->dropGapMessagesPercentage == t.dropGapMessagesPercentage &&
           this->percentageOfMessagesToDrop == t.percentageOfMessagesToDrop &&
           this->sequenceNumberDataMessagesToDrop == t.sequenceNumberDataMessagesToDrop &&
           this->dropLogLength == t.dropLogLength &&
           SocketTransportDescriptor::operator ==(t));
}

void test_UDPv4Transport::get_ips(
        std::vector<fastrtps::rtps::IPFinder::info_IP>& locNames,
        bool return_loopback)
{

    if (!simulate_no_interfaces)
    {
        UDPv4Transport::get_ips(locNames, return_loopback);
        return;
    }

    if (return_loopback)
    {
        fastrtps::rtps::IPFinder::info_IP local;
        local.type = fastrtps::rtps::IPFinder::IPTYPE::IP4_LOCAL;
        local.dev = "lo";
        local.name = "127.0.0.1";
        local.locator.kind = LOCATOR_KIND_UDPv4;
        fill_local_ip(local.locator);
        locNames.emplace_back(local);
    }
}

bool test_UDPv4Transport::send(
        const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
        uint32_t total_bytes,
        eProsimaUDPSocket& socket,
        fastrtps::rtps::LocatorsIterator* destination_locators_begin,
        fastrtps::rtps::LocatorsIterator* destination_locators_end,
        bool only_multicast_purpose,
        const std::chrono::steady_clock::time_point& max_blocking_time_point)
{
    fastrtps::rtps::LocatorsIterator& it = *destination_locators_begin;

    bool ret = true;

    while (it != *destination_locators_end)
    {
        auto now = std::chrono::steady_clock::now();

        if (now < max_blocking_time_point)
        {
            ret &= send(send_buffers,
                            total_bytes,
                            socket,
                            *it,
                            only_multicast_purpose,
                            std::chrono::duration_cast<std::chrono::microseconds>(now - max_blocking_time_point));

            ++it;
        }
        else // Time is out
        {
            ret = false;
            break;
        }
    }

    return ret;
}

bool test_UDPv4Transport::send(
        const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
        uint32_t total_bytes,
        eProsimaUDPSocket& socket,
        const Locator& remote_locator,
        bool only_multicast_purpose,
        const std::chrono::microseconds& timeout)
{
    if (packet_should_drop(send_buffers, total_bytes))
    {
        log_drop(send_buffers);
        return true;
    }
    else
    {
        return UDPv4Transport::send(send_buffers, total_bytes, socket, remote_locator, only_multicast_purpose,
                       timeout);
    }
}

static bool ReadSubmessageHeader(
        CDRMessage_t& msg,
        SubmessageHeader_t& smh)
{
    if (msg.length - msg.pos < 4)
    {
        return false;
    }

    smh.submessageId = msg.buffer[msg.pos]; msg.pos++;
    smh.flags = msg.buffer[msg.pos]; msg.pos++;
    msg.msg_endian = smh.flags & BIT(0) ? fastrtps::rtps::LITTLEEND : fastrtps::rtps::BIGEND;
    uint16_t length = 0;
    fastrtps::rtps::CDRMessage::readUInt16(&msg, &length);

    if (msg.pos + length > msg.length)
    {
        return false;
    }

    if ((length == 0) && (smh.submessageId != fastrtps::rtps::INFO_TS) && (smh.submessageId != fastrtps::rtps::PAD))
    {
        // THIS IS THE LAST SUBMESSAGE
        smh.submessageLength = msg.length - msg.pos;
        smh.is_last = true;
    }
    else
    {
        smh.submessageLength = length;
        smh.is_last = false;
    }
    return true;
}

bool test_UDPv4Transport::packet_should_drop(
        const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
        uint32_t total_bytes)
{
    if (test_UDPv4Transport_ShutdownAllNetwork)
    {
        return true;
    }

    CDRMessage_t cdrMessage(total_bytes);
    size_t n_bytes = 0;
    for (size_t i = 0; i < max_required_buffers; ++i)
    {
        memcpy(&cdrMessage.buffer[n_bytes], send_buffers[i].data(), send_buffers[i].size());
        n_bytes += send_buffers[i].size();
    }
    assert(total_bytes == n_bytes);
    cdrMessage.length = total_bytes;

    if (cdrMessage.length < RTPSMESSAGE_HEADER_SIZE)
    {
        return false;
    }

    if (cdrMessage.buffer[cdrMessage.pos++] != 'R' ||
            cdrMessage.buffer[cdrMessage.pos++] != 'T' ||
            cdrMessage.buffer[cdrMessage.pos++] != 'P' ||
            cdrMessage.buffer[cdrMessage.pos++] != 'S')
    {
        return false;
    }

    cdrMessage.pos += 4 + 12; // RTPS version + GUID

    SubmessageHeader_t cdrSubMessageHeader;
    while (cdrMessage.pos < cdrMessage.length)
    {
        ReadSubmessageHeader(cdrMessage, cdrSubMessageHeader);
        if (cdrMessage.pos + cdrSubMessageHeader.submessageLength > cdrMessage.length)
        {
            return false;
        }

        SequenceNumber_t sequence_number{SequenceNumber_t::unknown()};
        EntityId_t writer_id;
        auto old_pos = cdrMessage.pos;

        switch (cdrSubMessageHeader.submessageId)
        {
            case fastrtps::rtps::DATA:
                // Get WriterID.
                cdrMessage.pos += 8;
                fastrtps::rtps::CDRMessage::readEntityId(&cdrMessage, &writer_id);
                fastrtps::rtps::CDRMessage::readInt32(&cdrMessage, &sequence_number.high);
                fastrtps::rtps::CDRMessage::readUInt32(&cdrMessage, &sequence_number.low);
                cdrMessage.pos = old_pos;

                if (writer_id == fastrtps::rtps::c_EntityId_SPDPWriter)
                {
                    if (always_drop_participant_builtin_topic_data)
                    {
                        return true;
                    }
                    else if (!drop_participant_builtin_topic_data_)
                    {
                        return false;
                    }
                }
                else if ((!drop_publication_builtin_topic_data_ &&
                        writer_id == fastrtps::rtps::c_EntityId_SEDPPubWriter) ||
                        (!drop_subscription_builtin_topic_data_ &&
                        writer_id == fastrtps::rtps::c_EntityId_SEDPSubWriter))
                {
                    return false;
                }

                if (should_be_dropped(&drop_data_messages_percentage_))
                {
                    return true;
                }
                if (drop_data_messages_filter_(cdrMessage))
                {
                    return true;
                }

                break;

            case fastrtps::rtps::ACKNACK:
                if (should_be_dropped(&drop_ack_nack_messages_percentage_))
                {
                    return true;
                }
                if (drop_ack_nack_messages_filter_(cdrMessage))
                {
                    return true;
                }

                break;

            case fastrtps::rtps::HEARTBEAT:
                cdrMessage.pos += 8;
                fastrtps::rtps::CDRMessage::readInt32(&cdrMessage, &sequence_number.high);
                fastrtps::rtps::CDRMessage::readUInt32(&cdrMessage, &sequence_number.low);
                cdrMessage.pos = old_pos;
                if (should_be_dropped(&drop_heartbeat_messages_percentage_))
                {
                    return true;
                }
                if (drop_heartbeat_messages_filter_(cdrMessage))
                {
                    return true;
                }

                break;

            case fastrtps::rtps::DATA_FRAG:
                if (should_be_dropped(&drop_data_frag_messages_percentage_))
                {
                    return true;
                }
                if (drop_data_frag_messages_filter_(cdrMessage))
                {
                    return true;
                }

                break;

            case fastrtps::rtps::GAP:
                cdrMessage.pos += 8;
                fastrtps::rtps::CDRMessage::readInt32(&cdrMessage, &sequence_number.high);
                fastrtps::rtps::CDRMessage::readUInt32(&cdrMessage, &sequence_number.low);
                cdrMessage.pos = old_pos;
                if (should_be_dropped(&drop_gap_messages_percentage_))
                {
                    return true;
                }
                if (drop_gap_messages_filter_(cdrMessage))
                {
                    return true;
                }

                break;
        }

        if (sequence_number != SequenceNumber_t::unknown() &&
                find(sequence_number_data_messages_to_drop_.begin(),
                sequence_number_data_messages_to_drop_.end(),
                sequence_number) != sequence_number_data_messages_to_drop_.end())
        {
            return true;
        }

        cdrMessage.pos += cdrSubMessageHeader.submessageLength;
    }

    if (should_be_dropped(&percentage_of_messages_to_drop_))
    {
        return true;
    }
    if (messages_filter_(cdrMessage))
    {
        return true;
    }

    return false;
}

bool test_UDPv4Transport::log_drop(
        const std::array<asio::const_buffer, max_required_buffers>& send_buffers)
{
    if (test_UDPv4Transport_DropLog.size() < test_UDPv4Transport_DropLogLength)
    {
        vector<octet> message;
        for (const auto& buf : send_buffers)
        {
            if (buf.size())
            {
                auto byte_data = static_cast<const octet*>(buf.data());
                message.insert(message.end(), byte_data, byte_data + buf.size());
            }
        }
        test_UDPv4Transport_DropLog.push_back(message);
        return true;
    }

    return false;
}

bool test_UDPv4Transport::should_be_dropped(
        PercentageData* percent)
{
    percent->accumulator += percent->percentage;
    if (percent->accumulator >= 100u)
    {
        percent->accumulator -= 100u;
        return true;
    }

    return false;
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
