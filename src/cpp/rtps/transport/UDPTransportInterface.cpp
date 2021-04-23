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

#include <rtps/transport/UDPTransportInterface.h>

#include <utility>
#include <cstring>
#include <algorithm>
#include <chrono>

#include <fastdds/rtps/transport/TransportInterface.h>
#include <fastdds/rtps/messages/CDRMessage.h>
#include <fastdds/dds/log/Log.hpp>
#include <fastrtps/utils/IPLocator.h>
#include <rtps/transport/UDPSenderResource.hpp>

using namespace std;
using namespace asio;

namespace eprosima {
namespace fastdds {
namespace rtps {

using IPLocator = fastrtps::rtps::IPLocator;
using LocatorSelectorEntry = fastrtps::rtps::LocatorSelectorEntry;
using LocatorSelector = fastrtps::rtps::LocatorSelector;
using IPFinder = fastrtps::rtps::IPFinder;
using octet = fastrtps::rtps::octet;
using PortParameters = fastrtps::rtps::PortParameters;
using SenderResource = fastrtps::rtps::SenderResource;
using Log = fastdds::dds::Log;

struct MultiUniLocatorsLinkage
{
    MultiUniLocatorsLinkage(
            LocatorList&& m,
            LocatorList&& u)
        : multicast(std::move(m))
        , unicast(std::move(u))
    {
    }

    LocatorList multicast;
    LocatorList unicast;
};

UDPTransportDescriptor::UDPTransportDescriptor()
    : SocketTransportDescriptor(s_maximumMessageSize, s_maximumInitialPeersRange)
    , m_output_udp_socket(0)
{
}

bool UDPTransportDescriptor::operator ==(
        const UDPTransportDescriptor& t) const
{
    return (this->m_output_udp_socket == t.m_output_udp_socket &&
           this->non_blocking_send == t.non_blocking_send &&
           SocketTransportDescriptor::operator ==(t));
}

UDPTransportInterface::UDPTransportInterface(
        int32_t transport_kind)
    : TransportInterface(transport_kind)
    , mSendBufferSize(0)
    , mReceiveBufferSize(0)
{
}

UDPTransportInterface::~UDPTransportInterface()
{
}

void UDPTransportInterface::clean()
{
    assert(mInputSockets.size() == 0);
}

bool UDPTransportInterface::CloseInputChannel(
        const Locator& locator)
{
    std::vector<UDPChannelResource*> channel_resources;
    {
        std::unique_lock<std::recursive_mutex> scopedLock(mInputMapMutex);
        if (!IsInputChannelOpen(locator))
        {
            return false;
        }

        channel_resources = std::move(mInputSockets.at(IPLocator::getPhysicalPort(locator)));
        mInputSockets.erase(IPLocator::getPhysicalPort(locator));

    }

    // We now disable and release the channels
    for (UDPChannelResource* channel : channel_resources)
    {
        channel->disable();
        channel->release();
        channel->clear();
        delete channel;
    }

    return true;
}

void UDPTransportInterface::CloseOutputChannel(
        eProsimaUDPSocket& socket)
{
    socket.cancel();
    socket.close();
}

bool UDPTransportInterface::DoInputLocatorsMatch(
        const Locator& left,
        const Locator& right) const
{
    return IPLocator::getPhysicalPort(left) == IPLocator::getPhysicalPort(right);
}

bool UDPTransportInterface::init()
{
    if (configuration()->sendBufferSize == 0 || configuration()->receiveBufferSize == 0)
    {
        // Check system buffer sizes.
        ip::udp::socket socket(io_service_);
        socket.open(generate_protocol());

        if (configuration()->sendBufferSize == 0)
        {
            socket_base::send_buffer_size option;
            socket.get_option(option);
            set_send_buffer_size(static_cast<uint32_t>(option.value()));

            if (configuration()->sendBufferSize < s_minimumSocketBuffer)
            {
                set_send_buffer_size(s_minimumSocketBuffer);
                mSendBufferSize = s_minimumSocketBuffer;
            }
        }

        if (configuration()->receiveBufferSize == 0)
        {
            socket_base::receive_buffer_size option;
            socket.get_option(option);
            set_receive_buffer_size(static_cast<uint32_t>(option.value()));

            if (configuration()->receiveBufferSize < s_minimumSocketBuffer)
            {
                set_receive_buffer_size(s_minimumSocketBuffer);
                mReceiveBufferSize = s_minimumSocketBuffer;
            }
        }
    }

    if (configuration()->maxMessageSize > s_maximumMessageSize)
    {
        logError(RTPS_MSG_OUT, "maxMessageSize cannot be greater than 65000");
        return false;
    }

    if (configuration()->maxMessageSize > configuration()->sendBufferSize)
    {
        logError(RTPS_MSG_OUT, "maxMessageSize cannot be greater than send_buffer_size");
        return false;
    }

    if (configuration()->maxMessageSize > configuration()->receiveBufferSize)
    {
        logError(RTPS_MSG_OUT, "maxMessageSize cannot be greater than receive_buffer_size");
        return false;
    }

    // TODO(Ricardo) Create an event that update this list.
    get_ips(currentInterfaces);

    return true;
}

bool UDPTransportInterface::IsInputChannelOpen(
        const Locator& locator) const
{
    std::unique_lock<std::recursive_mutex> scopedLock(mInputMapMutex);
    return IsLocatorSupported(locator) && (mInputSockets.find(IPLocator::getPhysicalPort(
               locator)) != mInputSockets.end());
}

bool UDPTransportInterface::IsLocatorSupported(
        const Locator& locator) const
{
    return locator.kind == transport_kind_;
}

bool UDPTransportInterface::OpenAndBindInputSockets(
        const Locator& locator,
        TransportReceiverInterface* receiver,
        bool is_multicast,
        uint32_t maxMsgSize)
{
    std::unique_lock<std::recursive_mutex> scopedLock(mInputMapMutex);

    try
    {
        std::vector<std::string> vInterfaces = get_binding_interfaces_list();
        for (std::string sInterface : vInterfaces)
        {
            UDPChannelResource* p_channel_resource;
            p_channel_resource = CreateInputChannelResource(sInterface, locator, is_multicast, maxMsgSize, receiver);
            mInputSockets[IPLocator::getPhysicalPort(locator)].push_back(p_channel_resource);
        }
    }
    catch (asio::system_error const& e)
    {
        (void)e;
        logInfo(RTPS_MSG_OUT, "UDPTransport Error binding at port: (" << IPLocator::getPhysicalPort(locator) << ")"
                                                                      << " with msg: " << e.what());
        mInputSockets.erase(IPLocator::getPhysicalPort(locator));
        return false;
    }

    return true;
}

UDPChannelResource* UDPTransportInterface::CreateInputChannelResource(
        const std::string& sInterface,
        const Locator& locator,
        bool is_multicast,
        uint32_t maxMsgSize,
        TransportReceiverInterface* receiver)
{
    eProsimaUDPSocket unicastSocket = OpenAndBindInputSocket(sInterface,
                    IPLocator::getPhysicalPort(locator), is_multicast);
    UDPChannelResource* p_channel_resource = new UDPChannelResource(this, unicastSocket, maxMsgSize, locator,
                    sInterface, receiver);
    return p_channel_resource;
}

eProsimaUDPSocket UDPTransportInterface::OpenAndBindUnicastOutputSocket(
        const ip::udp::endpoint& endpoint,
        uint16_t& port)
{
    eProsimaUDPSocket socket = createUDPSocket(io_service_);
    getSocketPtr(socket)->open(generate_protocol());
    if (mSendBufferSize != 0)
    {
        getSocketPtr(socket)->set_option(socket_base::send_buffer_size(static_cast<int32_t>(mSendBufferSize)));
    }
    getSocketPtr(socket)->set_option(ip::multicast::hops(configuration()->TTL));
    getSocketPtr(socket)->bind(endpoint);
    getSocketPtr(socket)->non_blocking(configuration()->non_blocking_send);

    if (port == 0)
    {
        port = getSocketPtr(socket)->local_endpoint().port();
    }

    return socket;
}

bool UDPTransportInterface::OpenOutputChannel(
        SendResourceList& sender_resource_list,
        const Locator& locator)
{
    if (!IsLocatorSupported(locator))
    {
        return false;
    }

    // We try to find a SenderResource that can be reuse to this locator.
    // Note: This is done in this level because if we do in NetworkFactory level, we have to mantain what transport
    // already reuses a SenderResource.
    for (auto& sender_resource : sender_resource_list)
    {
        UDPSenderResource* udp_sender_resource = UDPSenderResource::cast(*this, sender_resource.get());

        if (udp_sender_resource)
        {
            return true;
        }
    }

    try
    {
        uint16_t port = configuration()->m_output_udp_socket;
        std::vector<IPFinder::info_IP> locNames;
        get_ips(locNames);
        // If there is no whitelist, we can simply open a generic output socket
        // and gain efficiency.
        if (is_interface_whitelist_empty())
        {
            eProsimaUDPSocket unicastSocket = OpenAndBindUnicastOutputSocket(GenerateAnyAddressEndpoint(port), port);
            getSocketPtr(unicastSocket)->set_option(ip::multicast::enable_loopback(true));

            // Outbounding first interface with already created socket.
            if (!locNames.empty())
            {
                SetSocketOutboundInterface(unicastSocket, (*locNames.begin()).name);
            }
            else
            {
                SetSocketOutboundInterface(unicastSocket, localhost_name());
            }

            // If more than one interface, then create sockets for outbounding multicast.
            if (locNames.size() > 1)
            {
                auto locIt = locNames.begin();
                sender_resource_list.emplace_back(
                    static_cast<SenderResource*>(new UDPSenderResource(*this, unicastSocket)));

                // Create other socket for outbounding rest of interfaces.
                for (++locIt; locIt != locNames.end(); ++locIt)
                {
                    uint16_t new_port = 0;
                    try
                    {
                        eProsimaUDPSocket multicastSocket =
                                OpenAndBindUnicastOutputSocket(generate_endpoint((*locIt).name, new_port), new_port);
                        SetSocketOutboundInterface(multicastSocket, (*locIt).name);

                        sender_resource_list.emplace_back(
                            static_cast<SenderResource*>(new UDPSenderResource(*this, multicastSocket, true)));
                    }
                    catch (asio::system_error const& e)
                    {
                        (void)e;
                        logWarning(RTPS_MSG_OUT, "UDPTransport Error binding interface "
                                << (*locIt).name << " (skipping) with msg: " << e.what());
                    }
                }
            }
            else
            {
                // Multicast data will be sent for the only one interface.
                sender_resource_list.emplace_back(
                    static_cast<SenderResource*>(new UDPSenderResource(*this, unicastSocket)));
            }

            if (!locNames.empty())
            {
                // We add localhost output for multicast, so in case the network cable is unplugged, local
                // participants keep receiving DATA(p) announcements
                const std::string& localhost = localhost_name();
                uint16_t new_port = 0;
                try
                {
                    eProsimaUDPSocket multicastSocket =
                            OpenAndBindUnicastOutputSocket(generate_endpoint(localhost, new_port), new_port);
                    SetSocketOutboundInterface(multicastSocket, localhost);

                    sender_resource_list.emplace_back(
                        static_cast<SenderResource*>(new UDPSenderResource(*this, multicastSocket, true)));
                }
                catch (asio::system_error const& e)
                {
                    (void)e;
                    logWarning(RTPS_MSG_OUT, "UDPTransport Error binding interface "
                            << localhost << " (skipping) with msg: " << e.what());
                }
            }
        }
        else
        {
            locNames.clear();
            get_ips(locNames, true);

            bool firstInterface = false;
            for (const auto& infoIP : locNames)
            {
                if (is_interface_allowed(infoIP.name))
                {
                    eProsimaUDPSocket unicastSocket =
                            OpenAndBindUnicastOutputSocket(generate_endpoint(infoIP.name, port), port);
                    SetSocketOutboundInterface(unicastSocket, infoIP.name);
                    if (!firstInterface)
                    {
                        getSocketPtr(unicastSocket)->set_option(ip::multicast::enable_loopback(true));
                        firstInterface = true;
                    }
                    sender_resource_list.emplace_back(
                        static_cast<SenderResource*>(new UDPSenderResource(*this, unicastSocket)));
                }
            }
        }
    }
    catch (asio::system_error const& e)
    {
        (void)e;
        /* TODO Que hacer?
           logError(RTPS_MSG_OUT, "UDPTransport Error binding at port: (" << IPLocator::getPhysicalPort(locator) << ")"
            << " with msg: " << e.what());
           for (auto& socket : mOutputSockets)
           {
            delete socket;
           }
           mOutputSockets.clear();
         */
        return false;
    }

    return true;
}

Locator UDPTransportInterface::RemoteToMainLocal(
        const Locator& remote) const
{
    if (!IsLocatorSupported(remote))
    {
        return false;
    }

    Locator mainLocal(remote);
    //memset(mainLocal.address, 0x00, sizeof(mainLocal.address));
    mainLocal.set_Invalid_Address();
    return mainLocal;
}

bool UDPTransportInterface::transform_remote_locator(
        const Locator& remote_locator,
        Locator& result_locator) const
{
    if (IsLocatorSupported(remote_locator))
    {
        result_locator = remote_locator;
        if (!is_local_locator(result_locator))
        {
            // is_local_locator will return false for multicast addresses as well as
            // remote unicast ones.
            return true;
        }

        // If we get here, the locator is a local unicast address
        if (!is_locator_allowed(result_locator))
        {
            return false;
        }

        // The locator is in the whitelist (or the whitelist is empty)
        Locator loopbackLocator;
        fill_local_ip(loopbackLocator);
        if (is_locator_allowed(loopbackLocator))
        {
            // Loopback locator
            fill_local_ip(result_locator);
        }

        return true;
    }
    return false;
}

bool UDPTransportInterface::send(
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

    auto time_out = std::chrono::duration_cast<std::chrono::microseconds>(
        max_blocking_time_point - std::chrono::steady_clock::now());

    while (it != *destination_locators_end)
    {
        if (IsLocatorSupported(*it))
        {
            ret &= send(send_buffers,
                            total_bytes,
                            socket,
                            *it,
                            only_multicast_purpose,
                            time_out);
        }

        ++it;
    }

    return ret;
}

bool UDPTransportInterface::send(
        const std::array<asio::const_buffer, max_required_buffers>& send_buffers,
        uint32_t total_bytes,
        eProsimaUDPSocket& socket,
        const Locator& remote_locator,
        bool only_multicast_purpose,
        const std::chrono::microseconds& timeout)
{
    if (total_bytes > configuration()->sendBufferSize)
    {
        return false;
    }

    bool success = false;
    bool is_multicast_remote_address = IPLocator::isMulticast(remote_locator);

    if (is_multicast_remote_address || !only_multicast_purpose)
    {
        auto destinationEndpoint = generate_endpoint(remote_locator, IPLocator::getPhysicalPort(remote_locator));

        size_t bytesSent = 0;

        try
        {
            (void)timeout;
#ifndef _WIN32
            struct timeval timeStruct;
            timeStruct.tv_sec = 0;
            timeStruct.tv_usec = timeout.count() > 0 ? timeout.count() : 0;
            setsockopt(getSocketPtr(socket)->native_handle(), SOL_SOCKET, SO_SNDTIMEO,
                    reinterpret_cast<const char*>(&timeStruct), sizeof(timeStruct));
#endif // ifndef _WIN32

            asio::error_code ec;
            bytesSent = getSocketPtr(socket)->send_to(send_buffers, destinationEndpoint, 0, ec);
            if (!!ec)
            {
                if ((ec.value() == asio::error::would_block) ||
                        (ec.value() == asio::error::try_again))
                {
                    logWarning(RTPS_MSG_OUT, "UDP send would have blocked. Packet is dropped.");
                    return true;
                }

                logWarning(RTPS_MSG_OUT, ec.message());
                return false;
            }
        }
        catch (const std::exception& error)
        {
            logWarning(RTPS_MSG_OUT, error.what());
            return false;
        }

        (void)bytesSent;
        logInfo(RTPS_MSG_OUT, "UDPTransport: " << bytesSent << " bytes TO endpoint: " << destinationEndpoint
                                               << " FROM " << getSocketPtr(socket)->local_endpoint());
        success = true;
    }

    return success;
}

/**
 * Invalidate all selector entries containing certain multicast locator.
 *
 * This function will process all entries from 'index' onwards and, if any
 * of them has 'locator' on its multicast list, will invalidate them
 * (i.e. their 'transport_should_process' flag will be changed to false).
 *
 * If this function returns true, the locator received should be selected.
 *
 * @param entries   Selector entries collection to process
 * @param index     Starting index to process
 * @param locator   Locator to be searched
 *
 * @return true when at least one entry was invalidated, false otherwise
 */
static bool check_and_invalidate(
        fastrtps::ResourceLimitedVector<LocatorSelectorEntry*>& entries,
        size_t index,
        const Locator& locator)
{
    bool ret_val = false;
    for (; index < entries.size(); ++index)
    {
        LocatorSelectorEntry* entry = entries[index];
        if (entry->transport_should_process)
        {
            for (const Locator& loc : entry->multicast)
            {
                if (loc == locator)
                {
                    entry->transport_should_process = false;
                    ret_val = true;
                    break;
                }
            }
        }
    }

    return ret_val;
}

void UDPTransportInterface::select_locators(
        LocatorSelector& selector) const
{
    fastrtps::ResourceLimitedVector<LocatorSelectorEntry*>& entries = selector.transport_starts();

    for (size_t i = 0; i < entries.size(); ++i)
    {
        LocatorSelectorEntry* entry = entries[i];
        if (entry->transport_should_process)
        {
            bool selected = false;

            // First try to find a multicast locator which is at least on another list.
            for (size_t j = 0; j < entry->multicast.size() && !selected; ++j)
            {
                if (IsLocatorSupported(entry->multicast[j]))
                {
                    if (check_and_invalidate(entries, i + 1, entry->multicast[j]))
                    {
                        entry->state.multicast.push_back(j);
                        selected = true;
                    }
                    else if (entry->unicast.size() == 0)
                    {
                        entry->state.multicast.push_back(j);
                        selected = true;
                    }
                }
            }

            // If we couldn't find a multicast locator, select all unicast locators
            if (!selected)
            {
                for (size_t j = 0; j < entry->unicast.size(); ++j)
                {
                    if (IsLocatorSupported(entry->unicast[j]) && !selector.is_selected(entry->unicast[j]))
                    {
                        entry->state.unicast.push_back(j);
                        selected = true;
                    }
                }
            }

            // Select this entry if necessary
            if (selected)
            {
                selector.select(i);
            }
        }
    }
}

bool UDPTransportInterface::fillMetatrafficMulticastLocator(
        Locator& locator,
        uint32_t metatraffic_multicast_port) const
{
    if (locator.port == 0)
    {
        locator.port = metatraffic_multicast_port;
    }
    return true;
}

bool UDPTransportInterface::fillMetatrafficUnicastLocator(
        Locator& locator,
        uint32_t metatraffic_unicast_port) const
{
    if (locator.port == 0)
    {
        locator.port = metatraffic_unicast_port;
    }
    return true;
}

bool UDPTransportInterface::configureInitialPeerLocator(
        Locator& locator,
        const PortParameters& port_params,
        uint32_t domainId,
        LocatorList& list) const
{
    if (locator.port == 0)
    {
        for (uint32_t i = 0; i < configuration()->maxInitialPeersRange; ++i)
        {
            Locator auxloc(locator);
            auxloc.port = port_params.getUnicastPort(domainId, i);

            list.push_back(auxloc);
        }
    }
    else
    {
        list.push_back(locator);
    }

    return true;
}

bool UDPTransportInterface::fillUnicastLocator(
        Locator& locator,
        uint32_t well_known_port) const
{
    if (locator.port == 0)
    {
        locator.port = well_known_port;
    }
    return true;
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
