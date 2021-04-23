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

#ifndef _FASTDDS_TCP_CHANNEL_RESOURCE_BASIC_
#define _FASTDDS_TCP_CHANNEL_RESOURCE_BASIC_

#include <asio.hpp>
#include <rtps/transport/TCPChannelResource.h>

namespace eprosima {
namespace fastdds {
namespace rtps {

class TCPChannelResourceBasic : public TCPChannelResource
{
    asio::io_service& service_;
    std::shared_ptr<asio::ip::tcp::socket> socket_;

public:

    // Constructor called when trying to connect to a remote server
    TCPChannelResourceBasic(
            TCPTransportInterface* parent,
            asio::io_service& service,
            const Locator& locator,
            uint32_t maxMsgSize);

    // Constructor called when local server accepted connection
    TCPChannelResourceBasic(
            TCPTransportInterface* parent,
            asio::io_service& service,
            std::shared_ptr<asio::ip::tcp::socket> socket,
            uint32_t maxMsgSize);

    virtual ~TCPChannelResourceBasic();

    void connect(
            const std::shared_ptr<TCPChannelResource>& myself) override;

    void disconnect() override;

    uint32_t read(
            fastrtps::rtps::octet* buffer,
            std::size_t size,
            asio::error_code& ec) override;

    size_t send(
            const fastrtps::rtps::octet* header,
            size_t header_size,
            const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
            asio::error_code& ec) override;

    asio::ip::tcp::endpoint remote_endpoint() const override;
    asio::ip::tcp::endpoint local_endpoint() const override;

    void set_options(
            const TCPTransportDescriptor* options) override;

    void cancel() override;
    void close() override;
    void shutdown(
            asio::socket_base::shutdown_type what) override;

    inline std::shared_ptr<asio::ip::tcp::socket> socket()
    {
        return socket_;
    }

private:

    TCPChannelResourceBasic(
            const TCPChannelResourceBasic&) = delete;
    TCPChannelResourceBasic& operator =(
            const TCPChannelResourceBasic&) = delete;
};


} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_TCP_CHANNEL_RESOURCE_BASIC_
