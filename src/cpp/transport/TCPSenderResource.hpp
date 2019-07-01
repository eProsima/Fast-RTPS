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

#include <fastrtps/rtps/network/SenderResource.h>
#include <fastrtps/transport/TCPTransportInterface.h>
#include <fastrtps/transport/TCPChannelResource.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class TCPSenderResource : public SenderResource
{
    public:

        TCPSenderResource(
                TCPTransportInterface& transport,
                std::shared_ptr<TCPChannelResource>& channel)
            : SenderResource(transport.kind())
            , channel_(channel)
        {
            // Implementation functions are bound to the right transport parameters
            clean_up = [this, &transport]()
                {
                    transport.CloseOutputChannel(channel_);
                };

            send_lambda_ = [this, &transport] (
                    const octet* data,
                    uint32_t dataSize,
                    const Locator_t& destination)-> bool
                {
                    return transport.send(data, dataSize, channel_, destination);
                };
        }

        virtual ~TCPSenderResource()
        {
            if (clean_up)
            {
                clean_up();
            }
        }

        std::shared_ptr<TCPChannelResource>& channel() { return channel_; }

        void channel(std::shared_ptr<TCPChannelResource>& channel) { channel_ = channel; }

        static TCPSenderResource* cast(TransportInterface& transport, SenderResource* sender_resource)
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

        TCPSenderResource(const SenderResource&) = delete;

        TCPSenderResource& operator=(const SenderResource&) = delete;

        std::shared_ptr<TCPChannelResource> channel_;
};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // __TRANSPORT_UDPSENDERRESOURCE_HPP__
