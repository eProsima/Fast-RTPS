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

#ifndef __TRANSPORT_TCPSENDERRESOURCE_HPP__
#define __TRANSPORT_TCPSENDERRESOURCE_HPP__

#include <fastdds/rtps/common/LocatorsIterator.hpp>
#include <fastdds/rtps/network/SenderResource.h>

#include <rtps/transport/TCPTransportInterface.h>
#include <rtps/transport/TCPChannelResource.h>

namespace eprosima {
namespace fastdds {
namespace rtps {

class TCPSenderResource : public fastrtps::rtps::SenderResource
{
public:

    TCPSenderResource(
            TCPTransportInterface& transport,
            std::shared_ptr<TCPChannelResource>& channel)
        : fastrtps::rtps::SenderResource(transport.kind())
        , channel_(channel)
    {
        // Implementation functions are bound to the right transport parameters
        clean_up = [this, &transport]()
                {
                    transport.CloseOutputChannel(channel_);
                };

        send_buffers_lambda_ = [this, &transport](
            const NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            fastrtps::rtps::LocatorsIterator* destination_locators_begin,
            fastrtps::rtps::LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point&) -> bool
                {
                    assert(num_buffers <= max_required_buffers);

                    std::array<asio::const_buffer, max_required_buffers> asio_buffers;
                    uint32_t total_num_bytes = 0;
                    for (size_t i = 0; i < num_buffers; ++i)
                    {
                        asio_buffers[i] = { buffers[i].buffer, buffers[i].length };
                        total_num_bytes += buffers[i].length;
                    }

                    assert(total_num_bytes == total_bytes);
                    return transport.send(asio_buffers, total_bytes, channel_, destination_locators_begin,
                                   destination_locators_end);
                };
    }

    virtual ~TCPSenderResource()
    {
        if (clean_up)
        {
            clean_up();
        }
    }

    std::shared_ptr<TCPChannelResource>& channel()
    {
        return channel_;
    }

    static TCPSenderResource* cast(
            TransportInterface& transport,
            SenderResource* sender_resource)
    {
        TCPSenderResource* returned_resource = nullptr;

        if (sender_resource->kind() == transport.kind())
        {
            returned_resource = dynamic_cast<TCPSenderResource*>(sender_resource);
        }

        return returned_resource;
    }

private:

    TCPSenderResource() = delete;

    TCPSenderResource(
            const SenderResource&) = delete;

    TCPSenderResource& operator =(
            const SenderResource&) = delete;

    std::shared_ptr<TCPChannelResource> channel_;
};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // __TRANSPORT_UDPSENDERRESOURCE_HPP__
