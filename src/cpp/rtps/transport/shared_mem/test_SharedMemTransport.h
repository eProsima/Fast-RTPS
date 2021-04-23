// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _FASTDDS_TEST_SHAREDMEM_TRANSPORT_H_
#define _FASTDDS_TEST_SHAREDMEM_TRANSPORT_H_

#include <rtps/transport/shared_mem/SharedMemTransport.h>
#include <rtps/transport/shared_mem/test_SharedMemTransportDescriptor.h>

namespace eprosima {
namespace fastdds {
namespace rtps {

class test_SharedMemTransport : public SharedMemTransport
{
public:

    RTPS_DllAPI test_SharedMemTransport(
            const test_SharedMemTransportDescriptor&);

    bool send(
            const NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            fastrtps::rtps::LocatorsIterator* destination_locators_begin,
            fastrtps::rtps::LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point& max_blocking_time_point) override;

    SharedMemChannelResource* CreateInputChannelResource(
            const Locator& locator,
            uint32_t max_msg_size,
            TransportReceiverInterface* receiver) override;

private:

    uint32_t big_buffer_size_;
    uint32_t* big_buffer_size_send_count_;
    uint32_t* big_buffer_size_recv_count_;
};


} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_TEST_SHAREDMEM_TRANSPORT_H_
