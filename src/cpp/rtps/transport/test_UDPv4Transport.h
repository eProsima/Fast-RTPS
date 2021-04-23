// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _FASTDDS_TEST_UDPV4_TRANSPORT_H_
#define _FASTDDS_TEST_UDPV4_TRANSPORT_H_

#include <vector>

#include <fastdds/rtps/transport/test_UDPv4TransportDescriptor.h>
#include <fastdds/rtps/messages/RTPS_messages.h>
#include <fastdds/rtps/messages/CDRMessage.h>
#include <fastdds/rtps/common/SequenceNumber.h>
#include <rtps/transport/UDPv4Transport.h>


namespace eprosima {
namespace fastdds {
namespace rtps {

/*
 * This transport acts as a shim over UDPv4, allowing
 * packets to be dropped under certain criteria.
 */

class test_UDPv4Transport : public UDPv4Transport
{
public:

    test_UDPv4Transport(
            const test_UDPv4TransportDescriptor& descriptor);

    virtual bool send(
            const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
            uint32_t total_bytes,
            eProsimaUDPSocket& socket,
            fastrtps::rtps::LocatorsIterator* destination_locators_begin,
            fastrtps::rtps::LocatorsIterator* destination_locators_end,
            bool only_multicast_purpose,
            const std::chrono::steady_clock::time_point& max_blocking_time_point) override;

    RTPS_DllAPI static bool test_UDPv4Transport_ShutdownAllNetwork;
    // Handle to a persistent log of dropped packets. Defaults to length 0 (no logging) to prevent wasted resources.
    RTPS_DllAPI static std::vector<std::vector<fastrtps::rtps::octet>> test_UDPv4Transport_DropLog;
    RTPS_DllAPI static uint32_t test_UDPv4Transport_DropLogLength;
    RTPS_DllAPI static bool always_drop_participant_builtin_topic_data;
    RTPS_DllAPI static bool simulate_no_interfaces;

protected:

    virtual void get_ips(
            std::vector<fastrtps::rtps::IPFinder::info_IP>& locNames,
            bool return_loopback = false) override;

private:

    struct PercentageData
    {
        PercentageData(
                uint8_t percent)
            : percentage(percent)
            , accumulator(0)
        {
        }

        uint8_t percentage;
        uint8_t accumulator;
    };

    typedef std::function<bool (fastrtps::rtps::CDRMessage_t& msg)> filter;

    PercentageData drop_data_messages_percentage_;
    test_UDPv4TransportDescriptor::filter drop_data_messages_filter_;
    bool drop_participant_builtin_topic_data_;
    bool drop_publication_builtin_topic_data_;
    bool drop_subscription_builtin_topic_data_;
    PercentageData drop_data_frag_messages_percentage_;
    test_UDPv4TransportDescriptor::filter drop_data_frag_messages_filter_;
    PercentageData drop_heartbeat_messages_percentage_;
    test_UDPv4TransportDescriptor::filter drop_heartbeat_messages_filter_;
    PercentageData drop_ack_nack_messages_percentage_;
    test_UDPv4TransportDescriptor::filter drop_ack_nack_messages_filter_;
    PercentageData drop_gap_messages_percentage_;
    test_UDPv4TransportDescriptor::filter drop_gap_messages_filter_;
    PercentageData percentage_of_messages_to_drop_;
    test_UDPv4TransportDescriptor::filter messages_filter_;
    std::vector<fastrtps::rtps::SequenceNumber_t> sequence_number_data_messages_to_drop_;


    bool log_drop(
            const std::array<asio::const_buffer, max_required_buffers>& send_buffers);
    bool packet_should_drop(
            const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
            uint32_t total_bytes);
    bool random_chance_drop();
    bool should_be_dropped(
            PercentageData* percentage);

    bool send(
            const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
            uint32_t total_bytes,
            eProsimaUDPSocket& socket,
            const Locator& remote_locator,
            bool only_multicast_purpose,
            const std::chrono::microseconds& timeout);
};

} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_TEST_UDPV4_TRANSPORT_H_
