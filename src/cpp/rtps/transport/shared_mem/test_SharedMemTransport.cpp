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

#include <rtps/transport/shared_mem/test_SharedMemTransport.h>
#include <rtps/transport/shared_mem/test_SharedMemChannelResource.hpp>
#include <rtps/transport/shared_mem/SharedMemManager.hpp>
#include <rtps/transport/shared_mem/SHMLocator.hpp>

namespace eprosima {
namespace fastdds {
namespace rtps {

test_SharedMemTransportDescriptor::test_SharedMemTransportDescriptor()
    : SharedMemTransportDescriptor()
{
    big_buffer_size_ = std::numeric_limits<uint32_t>::max();
    big_buffer_size_send_count_ = nullptr;
    big_buffer_size_recv_count_ = nullptr;
}

test_SharedMemTransportDescriptor::test_SharedMemTransportDescriptor(
        const test_SharedMemTransportDescriptor& t)
    : SharedMemTransportDescriptor(t)
{
}

test_SharedMemTransport::test_SharedMemTransport(
        const test_SharedMemTransportDescriptor& t)
    : SharedMemTransport(t)
{
    big_buffer_size_ = t.big_buffer_size_;
    big_buffer_size_send_count_ = t.big_buffer_size_send_count_;
    big_buffer_size_recv_count_ = t.big_buffer_size_recv_count_;
}

TransportInterface* test_SharedMemTransportDescriptor::create_transport() const
{
    return new test_SharedMemTransport(*this);
}

bool test_SharedMemTransport::send(
        const fastrtps::rtps::octet* send_buffer,
        uint32_t send_buffer_size,
        fastrtps::rtps::LocatorsIterator* destination_locators_begin,
        fastrtps::rtps::LocatorsIterator* destination_locators_end,
        const std::chrono::steady_clock::time_point& max_blocking_time_point)
{
    if (send_buffer_size >= big_buffer_size_)
    {
        (*big_buffer_size_send_count_)++;
    }

    return SharedMemTransport::send(send_buffer, send_buffer_size, destination_locators_begin,
                   destination_locators_end, max_blocking_time_point);
}

SharedMemChannelResource* test_SharedMemTransport::CreateInputChannelResource(
        const Locator& locator,
        uint32_t maxMsgSize,
        TransportReceiverInterface* receiver)
{
    (void) maxMsgSize;

    // Multicast locators implies ReadShared (Multiple readers) ports.
    auto open_mode = locator.address[0] == 'M' ? SharedMemGlobal::Port::OpenMode::ReadShared :
            SharedMemGlobal::Port::OpenMode::ReadExclusive;

    return new test_SharedMemChannelResource(
        shared_mem_manager_->open_port(
            SHMLocator::get_pid_from_address(locator),
            locator.port,
            configuration()->port_queue_capacity(),
            configuration()->healthy_check_timeout_ms(),
            open_mode)->create_listener(),
        locator,
        receiver,
        big_buffer_size_,
        big_buffer_size_recv_count_);
}

}  // namespace rtps
}  // namespace fastdds
}  // namespace eprosima
