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

#ifndef _FASTDDS_SHAREDMEM_SENDERRESOURCE_HPP_
#define _FASTDDS_SHAREDMEM_SENDERRESOURCE_HPP_

#include <fastrtps/rtps/network/SenderResource.h>
#include <rtps/transport/shared_mem/SharedMemTransport.h>

namespace eprosima {
namespace fastdds {
namespace rtps {

class SharedMemSenderResource : public fastrtps::rtps::SenderResource
{
public:

    SharedMemSenderResource(
            SharedMemTransport& transport)
        : fastrtps::rtps::SenderResource(transport.kind())
    {
        // Implementation functions are bound to the right transport parameters
        clean_up = []()
                {
                    // No cleanup is required
                };

        send_buffers_lambda_ = [&transport](
            const NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            fastrtps::rtps::LocatorsIterator* destination_locators_begin,
            fastrtps::rtps::LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point& max_blocking_time_point) -> bool
                {
                    assert(num_buffers <= max_required_buffers);
                    return transport.send(buffers, num_buffers, total_bytes,
                                   destination_locators_begin, destination_locators_end,
                                   max_blocking_time_point);
                };

    }

    virtual ~SharedMemSenderResource()
    {
        if (clean_up)
        {
            clean_up();
        }
    }

    static SharedMemSenderResource* cast(
            TransportInterface& transport,
            SenderResource* sender_resource)
    {
        SharedMemSenderResource* returned_resource = nullptr;

        if (sender_resource->kind() == transport.kind())
        {
            returned_resource = dynamic_cast<SharedMemSenderResource*>(sender_resource);
        }

        return returned_resource;
    }

private:

    SharedMemSenderResource() = delete;

    SharedMemSenderResource(
            const SenderResource&) = delete;

    SharedMemSenderResource& operator =(
            const SenderResource&) = delete;
};

} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_SHAREDMEM_SENDERRESOURCE_HPP_
