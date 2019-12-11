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

#include <fastdds/rtps/transport/TransportInterface.h>
#include <fastrtps/rtps/messages/CDRMessage.h>
#include <fastrtps/log/Log.h>
#include <fastrtps/utils/Semaphore.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastdds/rtps/network/ReceiverResource.h>
#include <fastdds/rtps/network/SenderResource.h>
#include <fastrtps/rtps/messages/MessageReceiver.h>

#include <rtps/transport/shared_mem/SharedMemTransport.h>
#include <rtps/transport/shared_mem/SharedMemSenderResource.hpp>
#include <rtps/transport/shared_mem/SharedMemChannelResource.hpp>

#include <rtps/transport/shared_mem/SharedMemManager.hpp>

#define SHMEM_MANAGER_DOMAIN ("fastrtps")

using namespace std;

using namespace eprosima;
using namespace eprosima::fastdds;
using namespace eprosima::fastdds::rtps;

using Locator_t = fastrtps::rtps::Locator_t;
using LocatorList_t = fastrtps::rtps::LocatorList_t;
using Log = dds::Log;
using octet = fastrtps::rtps::octet;
using SenderResource = fastrtps::rtps::SenderResource;
using LocatorSelectorEntry = fastrtps::rtps::LocatorSelectorEntry;
using LocatorSelector = fastrtps::rtps::LocatorSelector;
using PortParameters = fastrtps::rtps::PortParameters;

//*********************************************************
// SharedMemTransportDescriptor
//*********************************************************

SharedMemTransportDescriptor::SharedMemTransportDescriptor()
    : TransportDescriptorInterface(SharedMemTransport::maximum_message_size, s_maximumInitialPeersRange)
	, segment_size(SharedMemTransport::default_segment_size)
	, port_queue_capacity(SharedMemTransport::default_port_queue_capacity)
{
}

SharedMemTransportDescriptor::SharedMemTransportDescriptor(
        const SharedMemTransportDescriptor& t)
    : TransportDescriptorInterface(t)
	, segment_size(t.segment_size)
	, port_queue_capacity(SharedMemTransport::default_port_queue_capacity)
{
}

TransportInterface* SharedMemTransportDescriptor::create_transport() const
{
    return new SharedMemTransport(*this);
}

//*********************************************************
// SharedMemTransport
//*********************************************************

SharedMemTransport::SharedMemTransport(
        const SharedMemTransportDescriptor& descriptor)
    : TransportInterface(LOCATOR_KIND_SHMEM)
    , configuration_(descriptor)
{
    
}

SharedMemTransport::SharedMemTransport()
    : TransportInterface(LOCATOR_KIND_SHMEM)
{
}

SharedMemTransport::~SharedMemTransport()
{
    clean();
}

bool SharedMemTransport::getDefaultMetatrafficMulticastLocators(
        LocatorList_t &locators,
        uint32_t metatraffic_multicast_port) const
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_SHMEM;
    locator.port = static_cast<uint16_t>(metatraffic_multicast_port);
    locator.set_Invalid_Address();
    locators.push_back(locator);
    return true;
}

bool SharedMemTransport::getDefaultMetatrafficUnicastLocators(
        LocatorList_t &locators,
        uint32_t metatraffic_unicast_port) const
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_SHMEM;
    locator.port = static_cast<uint16_t>(metatraffic_unicast_port);
    locator.set_Invalid_Address();
    locators.push_back(locator);

    return true;
}

bool SharedMemTransport::getDefaultUnicastLocators(
        LocatorList_t &locators,
        uint32_t unicast_port) const
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_SHMEM;
    locator.set_Invalid_Address();
    fillUnicastLocator(locator, unicast_port);
    locators.push_back(locator);

    return true;
}

void SharedMemTransport::AddDefaultOutputLocator(
        LocatorList_t &defaultList)
{
	(void)defaultList;
}

const SharedMemTransportDescriptor* SharedMemTransport::configuration() const
{
    return &configuration_;
}

bool SharedMemTransport::OpenInputChannel(
        const Locator_t& locator,
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
			logInfo(RTPS_MSG_OUT, std::string("CreateInputChannelResource failed for port ") 
				<< locator.port << " msg: " << e.what());
			return false;
		}
	}

	return true;
}

bool SharedMemTransport::is_locator_allowed(
        const Locator_t& locator) const
{
    return IsLocatorSupported(locator);
}

LocatorList_t SharedMemTransport::NormalizeLocator(
        const Locator_t& locator)
{
    LocatorList_t list;

    list.push_back(locator);

    return list;
}

bool SharedMemTransport::is_local_locator(
        const Locator_t& locator) const
{
    assert(locator.kind == LOCATOR_KIND_SHMEM);

    return true;
}

void SharedMemTransport::clean()
{
	assert(input_channels_.size() == 0);
}

bool SharedMemTransport::CloseInputChannel(
		const Locator_t& locator)
{
	std::vector<SharedMemChannelResource*> channel_resources;

	{
		std::lock_guard<std::recursive_mutex> lock(input_channels_mutex_);

		if (!IsInputChannelOpen(locator))
			return false;

		channel_resources = std::move(input_channels_);
	}

	// We now disable and release the channels
	for (SharedMemChannelResource* channel : channel_resources)
	{
		channel->disable();
		channel->release();
		channel->clear();
		delete channel;
	}

	return true;
}

bool SharedMemTransport::DoInputLocatorsMatch(
		const Locator_t& left, 
		const Locator_t& right) const
{
	return left.kind == right.kind && left.port == right.port;
}

bool SharedMemTransport::init()
{
	try
	{
		shared_mem_manager_ = std::make_shared<SharedMemManager>(SHMEM_MANAGER_DOMAIN);
		shared_mem_segment_ = shared_mem_manager_->create_segment(configuration_.segment_size);
	}
	catch (std::exception& e)
	{
		logError(RTPS_MSG_OUT, e.what());
		return false;
	}

	return true;
}

bool SharedMemTransport::IsInputChannelOpen(
		const Locator_t& locator) const
{
	std::lock_guard<std::recursive_mutex> lock(input_channels_mutex_);

	return IsLocatorSupported(locator) && (std::find_if(
		input_channels_.begin(), input_channels_.end(),
		[&](const SharedMemChannelResource* resource) { return locator == resource->locator(); }) != input_channels_.end());
}

bool SharedMemTransport::IsLocatorSupported(
		const Locator_t& locator) const
{
	return locator.kind == transport_kind_;
}

SharedMemChannelResource* SharedMemTransport::CreateInputChannelResource(
		const Locator_t& locator,
		uint32_t maxMsgSize,
		TransportReceiverInterface* receiver)
{
	return new SharedMemChannelResource(this, shared_mem_manager_->open_port(locator.port,1)->create_listener(), maxMsgSize, locator, receiver);
}

bool SharedMemTransport::OpenOutputChannel(
		SendResourceList& sender_resource_list,
		const Locator_t& locator)
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

Locator_t SharedMemTransport::RemoteToMainLocal(
		const Locator_t& remote) const
{
	if (!IsLocatorSupported(remote))
	{
		return false;
	}

	Locator_t mainLocal(remote);
	mainLocal.set_Invalid_Address();
	return mainLocal;
}

bool SharedMemTransport::transform_remote_locator(
		const Locator_t& remote_locator,
		Locator_t& result_locator) const
{
	if (IsLocatorSupported(remote_locator))
	{
		result_locator = remote_locator;

		return true;
	}

	return false;
}

std::shared_ptr<SharedMemManager::Buffer> SharedMemTransport::copy_to_shared_buffer(
        const octet* send_buffer,
        uint32_t send_buffer_size)
{
	std::shared_ptr<SharedMemManager::Buffer> shared_buffer = shared_mem_segment_->alloc_buffer(send_buffer_size);	

	memcpy(shared_buffer->data(), send_buffer, send_buffer_size);

	return shared_buffer;
}

bool SharedMemTransport::send(
		const octet* send_buffer,
		uint32_t send_buffer_size,
		fastrtps::rtps::LocatorsIterator* destination_locators_begin,
        fastrtps::rtps::LocatorsIterator* destination_locators_end,
		const std::chrono::steady_clock::time_point& max_blocking_time_point)
{
	fastrtps::rtps::LocatorsIterator& it = *destination_locators_begin;

    bool ret = true;

	try
	{
		std::shared_ptr<SharedMemManager::Buffer> shared_buffer;
		
		shared_buffer = copy_to_shared_buffer(send_buffer, send_buffer_size);

		while (it != *destination_locators_end)
		{
			auto now = std::chrono::steady_clock::now();

			if (now < max_blocking_time_point)
			{
				ret &=	send(shared_buffer, 
							*it,
							std::chrono::duration_cast<std::chrono::microseconds>(max_blocking_time_point - now));

				++it;
			}
			else // Time is out
			{
				ret = false;
				break;
			}
		}

	}
	catch(const std::exception& e)
	{
		logWarning(RTPS_MSG_OUT, e.what());
		ret = false;
	}
	
    return ret;
}

std::shared_ptr<SharedMemManager::Port> SharedMemTransport::find_port(uint32_t port_id)
{
	try
	{
		return opened_ports_.at(port_id);
	}
	catch(const std::out_of_range& e)
	{
		// The port is not opened
		std::shared_ptr<SharedMemManager::Port> port = shared_mem_manager_->open_port(port_id, configuration_.port_queue_capacity);
		opened_ports_[port_id] = port;

		return port;
	}
}

bool SharedMemTransport::send(
		const std::shared_ptr<SharedMemManager::Buffer>& buffer,
		const Locator_t& remote_locator,
		const std::chrono::microseconds& timeout)
{
	if (!IsLocatorSupported(remote_locator) /*|| buffer->size > configuration()->sendBufferSize*/)
	{
		return false;
	}
	
	try
	{
		find_port(remote_locator.port)->push(buffer);
	}
	catch (const std::exception& error)
	{
		logWarning(RTPS_MSG_OUT, error.what());
		return false;
	}

	logInfo(RTPS_MSG_OUT, "SharedMemTransport: " << buffer->size() << " bytes to port " << remote_locator.port);

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
static bool check_and_invalidate(
		fastrtps::ResourceLimitedVector<LocatorSelectorEntry*>& entries,
		size_t index,
		const Locator_t& locator)
{
	bool ret_val = false;
	for (; index < entries.size(); ++index)
	{
		LocatorSelectorEntry* entry = entries[index];
		if (entry->transport_should_process)
		{
			for (const Locator_t& loc : entry->multicast)
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
					if (IsLocatorSupported(entry->unicast[j].kind) && !selector.is_selected(entry->unicast[j]))
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

bool SharedMemTransport::fillMetatrafficMulticastLocator(
		Locator_t& locator,
		uint32_t metatraffic_multicast_port) const
{
	if (locator.port == 0)
	{
		locator.port = metatraffic_multicast_port;
	}

	return true;
}

bool SharedMemTransport::fillMetatrafficUnicastLocator(
		Locator_t& locator,
		uint32_t metatraffic_unicast_port) const
{
	if (locator.port == 0)
	{
		locator.port = metatraffic_unicast_port;
	}
	return true;
}

bool SharedMemTransport::configureInitialPeerLocator(
		Locator_t& locator, 
		const PortParameters& port_params,
		uint32_t domainId, 
		LocatorList_t& list) const
{
	if (locator.port == 0)
	{
		for (uint32_t i = 0; i < configuration()->maxInitialPeersRange; ++i)
		{
			Locator_t auxloc(locator);
			auxloc.port = port_params.getUnicastPort(domainId, i);

			list.push_back(auxloc);
		}
	}
	else
		list.push_back(locator);

	return true;
}

bool SharedMemTransport::fillUnicastLocator(
		Locator_t& locator, 
		uint32_t well_known_port) const
{
	if (locator.port == 0)
	{
		locator.port = well_known_port;
	}

	return true;
}
