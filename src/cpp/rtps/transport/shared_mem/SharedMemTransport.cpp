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

#include <utility>
#include <cstring>
#include <algorithm>

#include <fastdds/dds/log/Log.hpp>
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/rtps/network/ReceiverResource.h>
#include <fastdds/rtps/network/SenderResource.h>
#include <fastdds/rtps/transport/TransportInterface.h>

#include <fastrtps/rtps/messages/CDRMessage.h>
#include <fastrtps/rtps/messages/MessageReceiver.h>

#include <rtps/transport/shared_mem/SHMLocator.hpp>
#include <rtps/transport/shared_mem/SharedMemTransport.h>
#include <rtps/transport/shared_mem/SharedMemSenderResource.hpp>
#include <rtps/transport/shared_mem/SharedMemChannelResource.hpp>

#include <rtps/transport/shared_mem/SharedMemManager.hpp>

#define SHM_MANAGER_DOMAIN ("fastrtps")

using namespace std;

namespace eprosima {
namespace fastdds {
namespace rtps {

using octet = fastrtps::rtps::octet;
using SenderResource = fastrtps::rtps::SenderResource;
using LocatorSelectorEntry = fastrtps::rtps::LocatorSelectorEntry;
using LocatorSelector = fastrtps::rtps::LocatorSelector;
using PortParameters = fastrtps::rtps::PortParameters;

TransportInterface* SharedMemTransportDescriptor::create_transport() const
{
    return new SharedMemTransport(*this);
}

//*********************************************************
// SharedMemTransport
//*********************************************************

SharedMemTransport::SharedMemTransport(
        const SharedMemTransportDescriptor& descriptor)
    : TransportInterface(LOCATOR_KIND_SHM)
    , configuration_(descriptor)
{

}

SharedMemTransport::SharedMemTransport()
    : TransportInterface(LOCATOR_KIND_SHM)
{
}

SharedMemTransport::~SharedMemTransport()
{
    // Safely clean already opened resources
    clean_up();
}

bool SharedMemTransport::getDefaultMetatrafficMulticastLocators(
        LocatorList& locators,
        uint32_t metatraffic_multicast_port) const
{
    locators.push_back(SHMLocator::create_locator(metatraffic_multicast_port, SHMLocator::Type::MULTICAST));

    return true;
}

bool SharedMemTransport::getDefaultMetatrafficUnicastLocators(
        LocatorList& locators,
        uint32_t metatraffic_unicast_port) const
{
    locators.push_back(SHMLocator::create_locator(metatraffic_unicast_port, SHMLocator::Type::UNICAST));

    return true;
}

bool SharedMemTransport::getDefaultUnicastLocators(
        LocatorList& locators,
        uint32_t unicast_port) const
{
    auto locator = SHMLocator::create_locator(unicast_port, SHMLocator::Type::UNICAST);

    fillUnicastLocator(locator, unicast_port);
    locators.push_back(locator);

    return true;
}

void SharedMemTransport::AddDefaultOutputLocator(
        LocatorList& defaultList)
{
    (void)defaultList;
}

const SharedMemTransportDescriptor* SharedMemTransport::configuration() const
{
    return &configuration_;
}

bool SharedMemTransport::OpenInputChannel(
        const Locator& locator,
        TransportReceiverInterface* receiver,
        uint32_t maxMsgSize)
{
    std::unique_lock<std::recursive_mutex> scopedLock(input_channels_mutex_);

    if (!IsLocatorSupported(locator))
    {
        return false;
    }

    if (!IsInputChannelOpen(locator))
    {
        try
        {
            auto channel_resource = CreateInputChannelResource(locator, maxMsgSize, receiver);
            input_channels_.push_back(channel_resource);
        }
        catch (std::exception& e)
        {
            (void)e;

            logInfo(RTPS_MSG_OUT, std::string("CreateInputChannelResource failed for port ")
                    << locator.port << " msg: " << e.what());
            return false;
        }
    }

    return true;
}

bool SharedMemTransport::is_locator_allowed(
        const Locator& locator) const
{
    return IsLocatorSupported(locator);
}

LocatorList SharedMemTransport::NormalizeLocator(
        const Locator& locator)
{
    LocatorList list;

    list.push_back(locator);

    return list;
}

bool SharedMemTransport::is_local_locator(
        const Locator& locator) const
{
    assert(locator.kind == LOCATOR_KIND_SHM);
    (void)locator;

    return true;
}

void SharedMemTransport::delete_input_channel(
        SharedMemChannelResource* channel)
{
    channel->disable();
    channel->release();
    channel->clear();
    delete channel;
}

bool SharedMemTransport::CloseInputChannel(
        const Locator& locator)
{
    std::lock_guard<std::recursive_mutex> lock(input_channels_mutex_);

    for (auto it = input_channels_.begin(); it != input_channels_.end(); it++)
    {
        if ((*it)->locator() == locator)
        {
            delete_input_channel((*it));
            input_channels_.erase(it);

            return true;
        }
    }

    return false;
}

void SharedMemTransport::clean_up()
{
    try
    {
        // Delete send ports
        opened_ports_.clear();

        // Delete input channels
        {
            std::lock_guard<std::recursive_mutex> lock(input_channels_mutex_);

            for (auto input_channel : input_channels_)
            {
                delete_input_channel(input_channel);
            }

            input_channels_.clear();
        }

        shared_mem_segment_.reset();
    }
    catch (const std::exception& e)
    {
        logWarning(RTPS_MSG_OUT, e.what());
    }
}

bool SharedMemTransport::DoInputLocatorsMatch(
        const Locator& left,
        const Locator& right) const
{
    return left.kind == right.kind && left.port == right.port;
}

bool SharedMemTransport::init()
{
    // TODO(Adolfo): Calculate this value from UDP sockets buffers size.
    static constexpr uint32_t shm_default_segment_size = 512 * 1024;

    if (configuration_.segment_size() == 0)
    {
        configuration_.segment_size(shm_default_segment_size);
    }

    if (configuration_.segment_size() < configuration_.max_message_size())
    {
        logError(RTPS_MSG_OUT, "max_message_size cannot be greater than segment_size");
        return false;
    }

    try
    {
        shared_mem_manager_ = SharedMemManager::create(SHM_MANAGER_DOMAIN);
        shared_mem_segment_ = shared_mem_manager_->create_segment(configuration_.segment_size(),
                        configuration_.port_queue_capacity());

        // Memset the whole segment to zero in order to force physical map of the buffer
        auto buffer = shared_mem_segment_->alloc_buffer(configuration_.segment_size(),
                        (std::chrono::steady_clock::now() + std::chrono::milliseconds(100)));
        memset(buffer->data(), 0, configuration_.segment_size());
        buffer.reset();

        if (!configuration_.rtps_dump_file().empty())
        {
            auto packets_file_consumer = std::unique_ptr<SHMPacketFileConsumer>(
                new SHMPacketFileConsumer(configuration_.rtps_dump_file()));

            packet_logger_ = std::make_shared<PacketsLog<SHMPacketFileConsumer>>();
            packet_logger_->RegisterConsumer(std::move(packets_file_consumer));
        }
    }
    catch (std::exception& e)
    {
        logError(RTPS_MSG_OUT, e.what());
        return false;
    }

    return true;
}

bool SharedMemTransport::IsInputChannelOpen(
        const Locator& locator) const
{
    std::lock_guard<std::recursive_mutex> lock(input_channels_mutex_);

    return IsLocatorSupported(locator) && (std::find_if(
               input_channels_.begin(), input_channels_.end(),
               [&](const SharedMemChannelResource* resource)
               {
                   return locator == resource->locator();
               }) != input_channels_.end());
}

bool SharedMemTransport::IsLocatorSupported(
        const Locator& locator) const
{
    return locator.kind == transport_kind_;
}

SharedMemChannelResource* SharedMemTransport::CreateInputChannelResource(
        const Locator& locator,
        uint32_t maxMsgSize,
        TransportReceiverInterface* receiver)
{
    (void) maxMsgSize;

    // Multicast locators implies ReadShared (Multiple readers) ports.
    auto open_mode = locator.address[0] == 'M' ? SharedMemGlobal::Port::OpenMode::ReadShared :
            SharedMemGlobal::Port::OpenMode::ReadExclusive;

    return new SharedMemChannelResource(
        shared_mem_manager_->open_port(
            locator.port,
            configuration_.port_queue_capacity(),
            configuration_.healthy_check_timeout_ms(),
            open_mode)->create_listener(),
        locator,
        receiver,
        configuration_.rtps_dump_file());
}

bool SharedMemTransport::OpenOutputChannel(
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
        SharedMemSenderResource* sm_sender_resource = SharedMemSenderResource::cast(*this, sender_resource.get());

        if (sm_sender_resource)
        {
            return true;
        }
    }

    try
    {
        sender_resource_list.emplace_back(
            static_cast<SenderResource*>(new SharedMemSenderResource(*this)));
    }
    catch (std::exception& e)
    {
        logError(RTPS_MSG_OUT, "SharedMemTransport error opening port " << std::to_string(locator.port)
                                                                        << " with msg: " << e.what());

        return false;
    }

    return true;
}

Locator SharedMemTransport::RemoteToMainLocal(
        const Locator& remote) const
{
    if (!IsLocatorSupported(remote))
    {
        return false;
    }

    Locator mainLocal(remote);
    mainLocal.set_Invalid_Address();
    return mainLocal;
}

bool SharedMemTransport::transform_remote_locator(
        const Locator& remote_locator,
        Locator& result_locator) const
{
    if (IsLocatorSupported(remote_locator))
    {
        result_locator = remote_locator;

        return true;
    }

    return false;
}

bool SharedMemTransport::send(
        const NetworkBuffer* buffers,
        size_t num_buffers,
        uint32_t total_bytes,
        fastrtps::rtps::LocatorsIterator* destination_locators_begin,
        fastrtps::rtps::LocatorsIterator* destination_locators_end,
        const std::chrono::steady_clock::time_point& max_blocking_time_point)
{
    fastrtps::rtps::LocatorsIterator& it = *destination_locators_begin;

    bool ret = true;

    std::shared_ptr<SharedMemManager::Buffer> shared_buffer;

    try
    {
        while (it != *destination_locators_end)
        {
            if (IsLocatorSupported(*it))
            {
                // Only copy the first time
                if (shared_buffer == nullptr)
                {
                    shared_buffer = shared_mem_segment_->alloc_buffer(total_bytes, max_blocking_time_point);
                    octet* pos = static_cast<octet*>(shared_buffer->data());
                    for (size_t i = 0; i < num_buffers; ++i)
                    {
                        if (buffers[i].length)
                        {
                            memcpy(pos, buffers[i].buffer, buffers[i].length);
                            pos += buffers[i].length;
                        }
                    }
                }

                ret &= send(shared_buffer, *it);

                if (packet_logger_ && ret)
                {
                    packet_logger_->QueueLog({packet_logger_->now(), Locator(), *it, shared_buffer});
                }
            }

            ++it;
        }
    }
    catch (const std::exception& e)
    {
        logInfo(RTPS_TRANSPORT_SHM, e.what());
        (void)e;

        // Segment overflow with discard policy doesn't return error.
        if (!shared_buffer)
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    }

    return ret;

}

std::shared_ptr<SharedMemManager::Port> SharedMemTransport::find_port(
        uint32_t port_id)
{
    auto ports_it = opened_ports_.find(port_id);

    // The port is already opened
    if (ports_it != opened_ports_.end())
    {
        return (*ports_it).second;
    }

    // The port is not opened
    std::shared_ptr<SharedMemManager::Port> port = shared_mem_manager_->
                    open_port(port_id, configuration_.port_queue_capacity(), configuration_.healthy_check_timeout_ms(),
                    SharedMemGlobal::Port::OpenMode::Write);

    opened_ports_[port_id] = port;

    return port;
}

bool SharedMemTransport::push_discard(
        const std::shared_ptr<SharedMemManager::Buffer>& buffer,
        const Locator& remote_locator)
{
    try
    {
        if (!find_port(remote_locator.port)->try_push(buffer))
        {
            logInfo(RTPS_MSG_OUT, "Port " << remote_locator.port << " full. Buffer dropped");
        }
    }
    catch (const std::exception& error)
    {
        logWarning(RTPS_MSG_OUT, error.what());
        return false;
    }

    return true;
}

bool SharedMemTransport::send(
        const std::shared_ptr<SharedMemManager::Buffer>& buffer,
        const Locator& remote_locator)
{
    if (!push_discard(buffer, remote_locator))
    {
        return false;
    }

    logInfo(RTPS_MSG_OUT,
            "(ID:" << std::this_thread::get_id() << ") " << "SharedMemTransport: " << buffer->size() << " bytes to port " <<
            remote_locator.port);

    return true;
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
void SharedMemTransport::select_locators(
        LocatorSelector& selector) const
{
    fastrtps::ResourceLimitedVector<LocatorSelectorEntry*>& entries = selector.transport_starts();

    for (size_t i = 0; i < entries.size(); ++i)
    {
        LocatorSelectorEntry* entry = entries[i];
        if (entry->transport_should_process)
        {
            bool selected = false;

            // With shared-memory transport using multicast vs unicast is not an advantage
            // because no copies are saved. So no multicast locators are selected
            for (size_t j = 0; j < entry->unicast.size(); ++j)
            {
                if (IsLocatorSupported(entry->unicast[j]) && !selector.is_selected(entry->unicast[j]))
                {
                    entry->state.unicast.push_back(j);
                    selected = true;
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

bool SharedMemTransport::fillMetatrafficMulticastLocator(
        Locator& locator,
        uint32_t metatraffic_multicast_port) const
{
    if (locator.port == 0)
    {
        locator.port = metatraffic_multicast_port;
    }

    return true;
}

bool SharedMemTransport::fillMetatrafficUnicastLocator(
        Locator& locator,
        uint32_t metatraffic_unicast_port) const
{
    if (locator.port == 0)
    {
        locator.port = metatraffic_unicast_port;
    }
    return true;
}

bool SharedMemTransport::configureInitialPeerLocator(
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

bool SharedMemTransport::fillUnicastLocator(
        Locator& locator,
        uint32_t well_known_port) const
{
    if (locator.port == 0)
    {
        locator.port = well_known_port;
    }

    return true;
}

}  // namsepace rtps
}  // namespace fastdds
}  // namespace eprosima
