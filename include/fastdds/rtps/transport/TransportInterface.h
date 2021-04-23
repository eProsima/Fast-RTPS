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

#ifndef _FASTDDS_TRANSPORT_INTERFACE_H
#define _FASTDDS_TRANSPORT_INTERFACE_H

#include <memory>
#include <vector>
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/rtps/common/LocatorSelector.hpp>
#include <fastdds/rtps/common/PortParameters.h>
#include <fastdds/rtps/network/SenderResource.h>
#include <fastdds/rtps/transport/TransportDescriptorInterface.h>
#include <fastdds/rtps/transport/TransportReceiverInterface.h>

namespace eprosima {
namespace fastdds {
namespace rtps {

//! Default maximum message size
constexpr uint32_t s_maximumMessageSize = 65500;
//! Default maximum initial peers range
constexpr uint32_t s_maximumInitialPeersRange = 4;
//! Default minimum socket buffer
constexpr uint32_t s_minimumSocketBuffer = 65536;
//! Default IPv4 address
static const std::string s_IPv4AddressAny = "0.0.0.0";
//! Default IPv6 address
static const std::string s_IPv6AddressAny = "::";

using SendResourceList = std::vector<std::unique_ptr<fastrtps::rtps::SenderResource>>;

/**
 * Interface against which to implement a transport layer, decoupled from FastRTPS internals.
 * TransportInterface expects the user to implement a logical equivalence between Locators and protocol-specific "channels".
 * This equivalence can be narrowing: For instance in UDP/IP, a port would take the role of channel, and several different
 * locators can map to the same port, and hence the same channel.
 * @ingroup TRANSPORT_MODULE
 * */
class TransportInterface
{
public:

    using SenderResource = eprosima::fastrtps::rtps::SenderResource;
    static constexpr size_t max_required_buffers = SenderResource::max_required_buffers;

    /**
     * Aside from the API defined here, an user-defined Transport must define a descriptor data type and a constructor that
     * expects a constant reference to such descriptor. e.g:
     *
     * class MyTransport:
     * public:
     *    typedef struct { ... } MyTransportDescriptor;
     *    MyTransport(const MyTransportDescriptor&);
     *    ...
     */
    virtual ~TransportInterface() = default;

    //! Copy constructor
    TransportInterface(
            const TransportInterface& t) = delete;

    //! Copy assignment
    TransportInterface& operator =(
            const TransportInterface& t) = delete;

    //! Move constructor
    TransportInterface(
            TransportInterface&& t) = delete;

    //! Move assignment
    TransportInterface& operator =(
            TransportInterface&& t) = delete;

    /**
     * Initialize this transport. This method will prepare all the internals of the transport.
     * @return True when the transport was correctly intialized.
     */
    virtual bool init() = 0;

    /**
     * Must report whether the input channel associated to this locator is open. Channels must either be
     * fully closed or fully open, so that "open" and "close" operations are whole and definitive.
     */
    virtual bool IsInputChannelOpen(
            const Locator&) const = 0;

    //! Must report whether the given locator is supported by this transport (typically inspecting its "kind" value).
    virtual bool IsLocatorSupported(
            const Locator&) const = 0;

    //! Must report whether the given locator is allowed by this transport.
    virtual bool is_locator_allowed(
            const Locator&) const = 0;

    //! Returns the locator describing the main (most general) channel that can write to the provided remote locator.
    virtual Locator RemoteToMainLocal(
            const Locator& remote) const = 0;

    /**
     * Transforms a remote locator into a locator optimized for local communications.
     *
     * If the remote locator corresponds to one of the local interfaces, it is converted
     * to the corresponding local address.
     *
     * @param [in]  remote_locator Locator to be converted.
     * @param [out] result_locator Converted locator.
     *
     * @return false if the input locator is not supported/allowed by this transport, true otherwise.
     */
    virtual bool transform_remote_locator(
            const Locator& remote_locator,
            Locator& result_locator) const
    {
        (void)remote_locator;
        (void)result_locator;
        return false;
    }

    //! Must open the channel that maps to/from the given locator. This method must allocate, reserve and mark
    //! any resources that are needed for said channel.
    virtual bool OpenOutputChannel(
            SendResourceList& sender_resource_list,
            const Locator&) = 0;

    /** Opens an input channel to receive incomming connections.
     *   If there is an existing channel it registers the receiver interface.
     */
    virtual bool OpenInputChannel(
            const Locator&,
            TransportReceiverInterface*,
            uint32_t) = 0;

    /**
     * Must close the channel that maps to/from the given locator.
     * IMPORTANT: It MUST be safe to call this method even during a Receive operation on another thread. You must implement
     * any necessary mutual exclusion and timeout mechanisms to make sure the channel can be closed without damage.
     */
    virtual bool CloseInputChannel(
            const Locator&) = 0;

    //! Must report whether two locators map to the same internal channel.
    virtual bool DoInputLocatorsMatch(
            const Locator&,
            const Locator&) const = 0;

    //! Performs locator normalization (assign valid IP if not defined by user)
    virtual LocatorList NormalizeLocator(
            const Locator& locator) = 0;

    /**
     * Performs the locator selection algorithm for this transport.
     *
     * It basically consists of the following steps
     *   - selector.transport_starts is called
     *   - transport handles the selection state of each locator
     *   - if a locator from an entry is selected, selector.select is called for that entry
     *
     * @param [in, out] selector Locator selector.
     */
    virtual void select_locators(
            fastrtps::rtps::LocatorSelector& selector) const = 0;

    //! Must report whether the given locator is from the local host
    virtual bool is_local_locator(
            const Locator& locator) const = 0;

    //! Return the transport configuration (Transport Descriptor)
    virtual TransportDescriptorInterface* get_configuration() = 0;

    //! Add default output locator to the locator list
    virtual void AddDefaultOutputLocator(
            LocatorList& defaultList) = 0;

    //! Add metatraffic multicast locator with the given port
    virtual bool getDefaultMetatrafficMulticastLocators(
            LocatorList& locators,
            uint32_t metatraffic_multicast_port) const = 0;

    //! Add metatraffic unicast locator with the given port
    virtual bool getDefaultMetatrafficUnicastLocators(
            LocatorList& locators,
            uint32_t metatraffic_unicast_port) const = 0;

    //! Add unicast locator with the given port
    virtual bool getDefaultUnicastLocators(
            LocatorList& locators,
            uint32_t unicast_port) const = 0;

    //! Assign port to the given metatraffic multicast locator if not already defined
    virtual bool fillMetatrafficMulticastLocator(
            Locator& locator,
            uint32_t metatraffic_multicast_port) const = 0;

    //! Assign port to the given matatraffic unicast locator if not already defined
    virtual bool fillMetatrafficUnicastLocator(
            Locator& locator,
            uint32_t metatraffic_unicast_port) const = 0;

    //! Configure the initial peer locators list
    virtual bool configureInitialPeerLocator(
            Locator& locator,
            const fastrtps::rtps::PortParameters& port_params,
            uint32_t domainId,
            LocatorList& list) const = 0;

    //! Assign port to the given unicast locator if not already defined
    virtual bool fillUnicastLocator(
            Locator& locator,
            uint32_t well_known_port) const = 0;

    /**
     * @return The maximum datagram size for reception supported by the transport
     */
    virtual uint32_t max_recv_buffer_size() const = 0;

    /**
     * Shutdown method to close the connections of the transports.
     */
    virtual void shutdown()
    {
    }

    //! Return the transport kind
    int32_t kind() const
    {
        return transport_kind_;
    }

protected:

    TransportInterface(
            int32_t transport_kind)
        : transport_kind_(transport_kind)
    {
    }

    int32_t transport_kind_;
};

} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_TRANSPORT_INTERFACE_H
