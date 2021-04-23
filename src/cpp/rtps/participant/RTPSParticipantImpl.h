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

/**
 * @file RTPSParticipantImpl.h
 */

#ifndef _RTPS_PARTICIPANT_RTPSPARTICIPANTIMPL_H_
#define _RTPS_PARTICIPANT_RTPSPARTICIPANTIMPL_H_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#include <cstdio>
#include <cstdlib>
#include <list>
#include <sys/types.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fastrtps/utils/Semaphore.h>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif // if defined(_WIN32)

#include <fastdds/rtps/attributes/RTPSParticipantAttributes.h>
#include <fastdds/rtps/common/Guid.h>
#include <fastdds/rtps/builtin/discovery/endpoint/EDPSimple.h>
#include <fastdds/rtps/builtin/data/ReaderProxyData.h>
#include <fastdds/rtps/builtin/data/WriterProxyData.h>

#include <fastdds/rtps/network/NetworkFactory.h>
#include <fastdds/rtps/network/ReceiverResource.h>
#include <fastdds/rtps/network/SenderResource.h>
#include <fastdds/rtps/messages/MessageReceiver.h>
#include <fastdds/rtps/resources/ResourceEvent.h>
#include <fastdds/rtps/resources/AsyncWriterThread.h>

#include "../messages/RTPSMessageGroup_t.hpp"
#include "../messages/SendBuffersManager.hpp"

#if HAVE_SECURITY
#include <fastdds/rtps/Endpoint.h>
#include <fastdds/rtps/security/accesscontrol/ParticipantSecurityAttributes.h>
#include <rtps/security/SecurityManager.h>
#endif // if HAVE_SECURITY

namespace eprosima {

namespace fastdds {
namespace dds {
namespace builtin {

class TypeLookupManager;

} // namespace builtin
} // namespace dds
} // namespace fastdds

namespace fastrtps {

class TopicAttributes;
class MessageReceiver;

namespace rtps {

class RTPSParticipant;
class RTPSParticipantListener;
class BuiltinProtocols;
struct CDRMessage_t;
class Endpoint;
class RTPSWriter;
class WriterAttributes;
class WriterHistory;
class WriterListener;
class RTPSReader;
class ReaderAttributes;
class ReaderHistory;
class ReaderListener;
class StatefulReader;
class PDPSimple;
class FlowController;
class IPersistenceService;
class WLP;

/**
 * @brief Class RTPSParticipantImpl, it contains the private implementation of the RTPSParticipant functions and
 * allows the creation and removal of writers and readers. It manages the send and receive threads.
 * @ingroup RTPS_MODULE
 */
class RTPSParticipantImpl
{
    /*
       Receiver Control block is a struct we use to encapsulate the resources that take part in message reception.
       It contains:
       -A ReceiverResource (as produced by the NetworkFactory Element)
       -Its associated MessageReceiver
     */
    typedef struct ReceiverControlBlock
    {
        std::shared_ptr<ReceiverResource> Receiver;
        MessageReceiver* mp_receiver;                  //Associated Readers/Writers inside of MessageReceiver

        ReceiverControlBlock(
                std::shared_ptr<ReceiverResource>& rec)
            : Receiver(rec)
            , mp_receiver(nullptr)
        {
        }

        ReceiverControlBlock(
                ReceiverControlBlock&& origen)
            : Receiver(origen.Receiver)
            , mp_receiver(origen.mp_receiver)
        {
            origen.mp_receiver = nullptr;
            origen.Receiver.reset();
        }

        void disable()
        {
            if (Receiver != nullptr)
            {
                Receiver->disable();
            }
        }

    private:

        ReceiverControlBlock(
                const ReceiverControlBlock&) = delete;
        const ReceiverControlBlock& operator =(
                const ReceiverControlBlock&) = delete;

    } ReceiverControlBlock;

public:

    /**
     * @param param
     * @param guidP
     * @param part
     * @param plisten
     */
    RTPSParticipantImpl(
            uint32_t domain_id,
            const RTPSParticipantAttributes& param,
            const GuidPrefix_t& guidP,
            RTPSParticipant* part,
            RTPSParticipantListener* plisten = nullptr);

    /**
     * @param param
     * @param guidP
     * @param persistence_guid
     * @param part
     * @param plisten
     */
    RTPSParticipantImpl(
            uint32_t domain_id,
            const RTPSParticipantAttributes& param,
            const GuidPrefix_t& guidP,
            const GuidPrefix_t& persistence_guid,
            RTPSParticipant* part,
            RTPSParticipantListener* plisten = nullptr);

    virtual ~RTPSParticipantImpl();

    // Create receiver resources and start builtin protocols
    void enable();

    // Stop builtin protocols and delete receiver resources
    void disable();

    /**
     * Get associated GUID
     * @return Associated GUID
     */
    inline const GUID_t& getGuid() const
    {
        return m_guid;
    }

    void setGuid(
            GUID_t& guid);

    //! Announce RTPSParticipantState (force the sending of a DPD message.)
    void announceRTPSParticipantState();

    //!Stop the RTPSParticipant Announcement (used in tests to avoid multiple packets being send)
    void stopRTPSParticipantAnnouncement();

    //!Reset to timer to make periodic RTPSParticipant Announcements.
    void resetRTPSParticipantAnnouncement();

    void loose_next_change();

    /**
     * Activate a Remote Endpoint defined in the Static Discovery.
     * @param pguid GUID_t of the endpoint.
     * @param userDefinedId userDeinfed Id of the endpoint.
     * @param kind kind of endpoint
     * @return True if correct.
     */
    bool newRemoteEndpointDiscovered(
            const GUID_t& pguid,
            int16_t userDefinedId,
            EndpointKind_t kind);

    /**
     * Assert the liveliness of a remote participant
     * @param remote_guid GuidPrefix_t of the participant.
     */
    void assert_remote_participant_liveliness(
            const GuidPrefix_t& remote_guid);

    /**
     * Get the RTPSParticipant ID
     * @return RTPSParticipant ID
     */
    inline uint32_t getRTPSParticipantID() const
    {
        return (uint32_t)m_att.participantID;
    }

    //!Post to the resource semaphore
    void ResourceSemaphorePost();

    //!Wait for the resource semaphore
    void ResourceSemaphoreWait();

    //!Get Pointer to the Event Resource.
    ResourceEvent& getEventResource()
    {
        return mp_event_thr;
    }

    /**
     * Send a message to several locations
     * @param buffers Array of buffers to gather.
     * @param num_buffers Number of elements on @c buffers.
     * @param total_bytes Total size of the raw data. Should be equal to the sum of the @c length field of all buffers.
     * @param destination_locators_begin Iterator at the first destination locator.
     * @param destination_locators_end Iterator at the end destination locator.
     * @param max_blocking_time_point execution time limit timepoint.
     * @return true if at least one locator has been sent.
     */
    template<class LocatorIteratorT>
    bool sendSync(
            const SenderResource::NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            const LocatorIteratorT& destination_locators_begin,
            const LocatorIteratorT& destination_locators_end,
            std::chrono::steady_clock::time_point& max_blocking_time_point)
    {
        bool ret_code = false;
        std::unique_lock<std::timed_mutex> lock(m_send_resources_mutex_, std::defer_lock);

        if (lock.try_lock_until(max_blocking_time_point))
        {
            ret_code = true;

            for (auto& send_resource : send_resource_list_)
            {
                LocatorIteratorT locators_begin = destination_locators_begin;
                LocatorIteratorT locators_end = destination_locators_end;
                send_resource->send(buffers, num_buffers, total_bytes, &locators_begin, &locators_end,
                        max_blocking_time_point);
            }
        }

        return ret_code;
    }

    //!Get the participant Mutex
    std::recursive_mutex* getParticipantMutex() const
    {
        return mp_mutex;
    }

    /**
     * Get the participant listener
     * @return participant listener
     */
    inline RTPSParticipantListener* getListener()
    {
        return mp_participantListener;
    }

    /**
     * @brief Modifies the participant listener
     * @param listener
     */
    void set_listener(
            RTPSParticipantListener* listener)
    {
        mp_participantListener = listener;
    }

    std::vector<std::string> getParticipantNames() const;

    /**
     * Get the participant
     * @return participant
     */
    inline RTPSParticipant* getUserRTPSParticipant()
    {
        return mp_userParticipant;
    }

    std::vector<std::unique_ptr<FlowController>>& getFlowControllers()
    {
        return m_controllers;
    }

    /*!
     * @remarks Non thread-safe.
     */
    const std::vector<RTPSWriter*>& getAllWriters() const;

    /*!
     * @remarks Non thread-safe.
     */
    const std::vector<RTPSReader*>& getAllReaders() const;

    uint32_t getMaxMessageSize() const;

    uint32_t getMaxDataSize();

    uint32_t calculateMaxDataSize(
            uint32_t length);

#if HAVE_SECURITY
    security::SecurityManager& security_manager()
    {
        return m_security_manager;
    }

    const security::ParticipantSecurityAttributes& security_attributes()
    {
        return security_attributes_;
    }

    bool is_security_initialized() const
    {
        return m_security_manager_initialized;
    }

    bool is_secure() const
    {
        return m_is_security_active;
    }

    bool pairing_remote_reader_with_local_writer_after_security(
            const GUID_t& local_writer,
            const ReaderProxyData& remote_reader_data);

    bool pairing_remote_writer_with_local_reader_after_security(
            const GUID_t& local_reader,
            const WriterProxyData& remote_writer_data);

    /**
     * @brief Checks whether the writer has security attributes enabled
     * @param writer_attributes Attibutes of the writer as given to the create_writer
     */
    bool is_security_enabled_for_writer(
            const WriterAttributes& writer_attributes);

    /**
     * @brief Checks whether the reader has security attributes enabled
     * @param reader_attributes Attibutes of the reader as given to the create_reader
     */
    bool is_security_enabled_for_reader(
            const ReaderAttributes& reader_attributes);

#endif // if HAVE_SECURITY

    PDPSimple* pdpsimple();

    WLP* wlp();

    fastdds::dds::builtin::TypeLookupManager* typelookup_manager() const;

    bool is_intraprocess_only() const
    {
        return is_intraprocess_only_;
    }

    NetworkFactory& network_factory()
    {
        return m_network_Factory;
    }

    inline bool has_shm_transport()
    {
        return has_shm_transport_;
    }

    uint32_t get_min_network_send_buffer_size()
    {
        return m_network_Factory.get_min_send_buffer_size();
    }

    /**
     * Get the list of locators from which this participant may send data.
     *
     * @param [out] locators  LocatorList_t where the list of locators will be stored.
     */
    void get_sending_locators(
            rtps::LocatorList_t& locators) const;

    AsyncWriterThread& async_thread()
    {
        return async_thread_;
    }

    /***
     * @returns A pointer to a local reader given its endpoint guid, or nullptr if not found.
     */
    RTPSReader* find_local_reader(
            const GUID_t& reader_guid);

    /***
     * @returns A pointer to a local writer given its endpoint guid, or nullptr if not found.
     */
    RTPSWriter* find_local_writer(
            const GUID_t& writer_guid);

    /**
     * @brief Fills a new entityId if set to unknown, or checks if a entity already exists with that
     * entityId in other case.
     * @param entityId to check of fill. If filled, EntityKind will be "vendor-specific" (0x01)
     * @return True if filled or the entityId is available.
     */
    bool get_new_entity_id(
            EntityId_t& entityId);

    void set_check_type_function(
            std::function<bool(const std::string&)>&& check_type);

    bool check_type(
            const std::string& type_name)
    {
        if (type_check_fn_ != nullptr)
        {
            return type_check_fn_(type_name);
        }
        return false;
    }

    std::unique_ptr<RTPSMessageGroup_t> get_send_buffer();
    void return_send_buffer(
            std::unique_ptr <RTPSMessageGroup_t>&& buffer);

    uint32_t get_domain_id() const;

    //!Compare metatraffic locators list searching for mutations
    bool did_mutation_took_place_on_meta(
            const LocatorList_t& MulticastLocatorList,
            const LocatorList_t& UnicastLocatorList) const;

private:

    //! DomainId
    uint32_t domain_id_;
    //!Attributes of the RTPSParticipant
    RTPSParticipantAttributes m_att;
    //!Guid of the RTPSParticipant.
    GUID_t m_guid;
    //!Persistence guid of the RTPSParticipant
    GUID_t m_persistence_guid;
    //! Sending resources. - DEPRECATED -Stays commented for reference purposes
    // ResourceSend* mp_send_thr;
    //! Event Resource
    ResourceEvent mp_event_thr;
    //! BuiltinProtocols of this RTPSParticipant
    BuiltinProtocols* mp_builtinProtocols;
    //!Semaphore to wait for the listen thread creation.
    Semaphore* mp_ResourceSemaphore;
    //!Id counter to correctly assign the ids to writers and readers.
    uint32_t IdCounter;
    //!Writer List.
    std::vector<RTPSWriter*> m_allWriterList;
    //!Reader List
    std::vector<RTPSReader*> m_allReaderList;
    //!Listen thread list.
    //!Writer List.
    std::vector<RTPSWriter*> m_userWriterList;
    //!Reader List
    std::vector<RTPSReader*> m_userReaderList;
    //!Network Factory
    NetworkFactory m_network_Factory;
    //!Async writer thread
    AsyncWriterThread async_thread_;
    //! Type cheking function
    std::function<bool(const std::string&)> type_check_fn_;
    //!Pool of send buffers
    std::unique_ptr<SendBuffersManager> send_buffers_;

#if HAVE_SECURITY
    // Security manager
    security::SecurityManager m_security_manager;
    // Security manager initialization result
    bool m_security_manager_initialized = false;
    // Security activation flag
    bool m_is_security_active = false;
#endif // if HAVE_SECURITY

    //! Encapsulates all associated resources on a Receiving element.
    std::list<ReceiverControlBlock> m_receiverResourcelist;
    //! Receiver resource list needs its own mutext to avoid a race condition.
    std::mutex m_receiverResourcelistMutex;

    //!SenderResource List
    std::timed_mutex m_send_resources_mutex_;
    fastdds::rtps::SendResourceList send_resource_list_;

    //!Participant Listener
    RTPSParticipantListener* mp_participantListener;
    //!Pointer to the user participant
    RTPSParticipant* mp_userParticipant;

    RTPSParticipantImpl& operator =(
            const RTPSParticipantImpl&) = delete;

    /**
     * Method to check if a specific entityId already exists in this RTPSParticipant
     * @param ent EnityId to check
     * @param kind Endpoint Kind.
     * @return True if exists.
     */
    bool existsEntityId(
            const EntityId_t& ent,
            EndpointKind_t kind) const;

    /**
     * Assign an endpoint to the ReceiverResources, based on its LocatorLists.
     * @param endp Pointer to the endpoint.
     * @return True if correct.
     */
    bool assignEndpointListenResources(
            Endpoint* endp);

    /** Assign an endpoint to the ReceiverResources as specified specifically on parameter list
     * @param pend Pointer to the endpoint.
     * @param lit Locator list iterator.
     * @param isMulticast Boolean indicating that is multicast.
     * @param isFixed Boolean indicating that is a fixed listenresource.
     * @return True if assigned.
     */
    bool assignEndpoint2LocatorList(
            Endpoint* pend,
            LocatorList_t& list);

    /** Create the new ReceiverResources needed for a new Locator, contains the calls to assignEndpointListenResources
        and consequently assignEndpoint2LocatorList
        @param pend - Pointer to the endpoint which triggered the creation of the Receivers.
        @param unique_flows - Whether unique listening ports should be created for this endpoint.
        @param initial_unique_port - First unique listening port to try.
        @param final_unique_port - Unique listening port that will not be tried.
     */
    bool createAndAssociateReceiverswithEndpoint(
            Endpoint* pend,
            bool unique_flows = false,
            uint16_t initial_unique_port = 0,
            uint16_t final_unique_port = 0);

    /** Create non-existent SendResources based on the Locator list of the entity
        @param pend - Pointer to the endpoint whose SenderResources are to be created
     */
    bool createSendResources(
            Endpoint* pend);

    /** When we want to create a new Resource but the physical channel specified by the Locator
        can not be opened, we want to mutate the Locator to open a more or less equivalent channel.
        @param loc -  Locator we want to change
     */
    Locator_t& applyLocatorAdaptRule(
            Locator_t& loc);

    /**
     * Update port for all endpoint locators when it has a value of 0 and then
     * apply locator normalization.
     *
     * @param [in, out] endpoint_att  EndpointAttributes to be updated
     */
    void normalize_endpoint_locators(
            EndpointAttributes& endpoint_att);

    //!Participant Mutex
    std::recursive_mutex* mp_mutex;

    //!Will this participant use intraprocess only?
    bool is_intraprocess_only_;

    /*
     * Flow controllers for this participant.
     */
    std::vector<std::unique_ptr<FlowController>> m_controllers;

#if HAVE_SECURITY
    security::ParticipantSecurityAttributes security_attributes_;
#endif // if HAVE_SECURITY

    //! Indicates whether the participant has shared-memory transport
    bool has_shm_transport_;

    /**
     * Get persistence service from factory, using endpoint attributes (or participant
     * attributes if endpoint does not define a persistence service config)
     */
    IPersistenceService* get_persistence_service(
            const EndpointAttributes& param);

    /**
     * Returns the Durability kind from which a endpoint is able to use the persistence service.
     */
    DurabilityKind_t get_persistence_durability_red_line(
            bool is_builtin_endpoint);

    /**
     * Check if persistence is required and return persistence service from factory,
     * using endpoint attributes (or participant
     * attributes if endpoint does not define a persistence service config)
     *
     * @param [in]  is_builtin  Whether the enpoint being created is a builtin one.
     * @param [in]  param       Attributes of the endpoint being created.
     * @param [out] service     Pointer to the persistence service.
     *
     * @return false if parameters are not consistent or the service should be created and couldn't
     * @return true if persistence service is not required
     * @return true if persistence service is created
     */
    bool get_persistence_service(
            bool is_builtin,
            const EndpointAttributes& param,
            IPersistenceService*& service);

    template <EndpointKind_t kind, octet no_key, octet with_key>
    bool preprocess_endpoint_attributes(
            const EntityId_t& entity_id,
            EndpointAttributes& att,
            EntityId_t& entId);

    template<typename Functor>
    bool create_writer(
            RTPSWriter** writer_out,
            WriterAttributes& param,
            const EntityId_t& entity_id,
            bool is_builtin,
            const Functor& callback);

    template<typename Functor>
    bool create_reader(
            RTPSReader** reader_out,
            ReaderAttributes& param,
            const EntityId_t& entity_id,
            bool is_builtin,
            bool enable,
            const Functor& callback);

public:

    const RTPSParticipantAttributes& getRTPSParticipantAttributes() const
    {
        return this->m_att;
    }

    /**
     * Create a Writer in this RTPSParticipant.
     * @param Writer Pointer to pointer of the Writer, used as output. Only valid if return==true.
     * @param param WriterAttributes to define the Writer.
     * @param hist Pointer to the WriterHistory.
     * @param listen Pointer to the WriterListener.
     * @param entityId EntityId assigned to the Writer.
     * @param isBuiltin Bool value indicating if the Writer is builtin (Discovery or Liveliness protocol) or is created for the end user.
     * @return True if the Writer was correctly created.
     */
    bool createWriter(
            RTPSWriter** Writer,
            WriterAttributes& param,
            WriterHistory* hist,
            WriterListener* listen,
            const EntityId_t& entityId = c_EntityId_Unknown,
            bool isBuiltin = false);

    /**
     * Create a Writer in this RTPSParticipant with a custom payload pool.
     * @param Writer Pointer to pointer of the Writer, used as output. Only valid if return==true.
     * @param param WriterAttributes to define the Writer.
     * @param payload_pool Shared pointer to the IPayloadPool
     * @param hist Pointer to the WriterHistory.
     * @param listen Pointer to the WriterListener.
     * @param entityId EntityId assigned to the Writer.
     * @param isBuiltin Bool value indicating if the Writer is builtin (Discovery or Liveliness protocol) or is created for the end user.
     * @return True if the Writer was correctly created.
     */
    bool createWriter(
            RTPSWriter** Writer,
            WriterAttributes& param,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            WriterHistory* hist,
            WriterListener* listen,
            const EntityId_t& entityId = c_EntityId_Unknown,
            bool isBuiltin = false);

    /**
     * Create a Reader in this RTPSParticipant.
     * @param Reader Pointer to pointer of the Reader, used as output. Only valid if return==true.
     * @param param ReaderAttributes to define the Reader.
     * @param hist Pointer to the ReaderHistory.
     * @param listen Pointer to the ReaderListener.
     * @param entityId EntityId assigned to the Reader.
     * @param isBuiltin Bool value indicating if the Reader is builtin (Discovery or Liveliness protocol) or is created for the end user.
     * @param enable Whether the reader should be automatically enabled.
     * @return True if the Reader was correctly created.
     */
    bool createReader(
            RTPSReader** Reader,
            ReaderAttributes& param,
            ReaderHistory* hist,
            ReaderListener* listen,
            const EntityId_t& entityId = c_EntityId_Unknown,
            bool isBuiltin = false,
            bool enable = true);

    /**
     * Create a Reader in this RTPSParticipant with a custom payload pool.
     * @param Reader Pointer to pointer of the Reader, used as output. Only valid if return==true.
     * @param param ReaderAttributes to define the Reader.
     * @param payload_pool Shared pointer to the IPayloadPool
     * @param hist Pointer to the ReaderHistory.
     * @param listen Pointer to the ReaderListener.
     * @param entityId EntityId assigned to the Reader.
     * @param isBuiltin Bool value indicating if the Reader is builtin (Discovery or Liveliness protocol) or is created for the end user.
     * @param enable Whether the reader should be automatically enabled.
     * @return True if the Reader was correctly created.
     */
    bool createReader(
            RTPSReader** Reader,
            ReaderAttributes& param,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            ReaderHistory* hist,
            ReaderListener* listen,
            const EntityId_t& entityId = c_EntityId_Unknown,
            bool isBuiltin = false,
            bool enable = true);

    bool enableReader(
            RTPSReader* reader);

    void disableReader(
            RTPSReader* reader);

    /**
     * Register a Writer in the BuiltinProtocols.
     * @param Writer Pointer to the RTPSWriter.
     * @param topicAtt TopicAttributes of the Writer.
     * @param wqos WriterQos.
     * @return True if correctly registered.
     */
    bool registerWriter(
            RTPSWriter* Writer,
            const TopicAttributes& topicAtt,
            const WriterQos& wqos);

    /**
     * Register a Reader in the BuiltinProtocols.
     * @param Reader Pointer to the RTPSReader.
     * @param topicAtt TopicAttributes of the Reader.
     * @param rqos ReaderQos.
     * @return  True if correctly registered.
     */
    bool registerReader(
            RTPSReader* Reader,
            const TopicAttributes& topicAtt,
            const ReaderQos& rqos);

    /**
     * Update local writer QoS
     * @param Writer Writer to update
     * @param wqos New QoS for the writer
     * @return True on success
     */
    bool updateLocalWriter(
            RTPSWriter* Writer,
            const TopicAttributes& topicAtt,
            const WriterQos& wqos);

    /**
     * Update local reader QoS
     * @param Reader Reader to update
     * @param rqos New QoS for the reader
     * @return True on success
     */
    bool updateLocalReader(
            RTPSReader* Reader,
            const TopicAttributes& topicAtt,
            const ReaderQos& rqos);

    /**
     * Get the participant attributes
     * @return Participant attributes
     */
    inline RTPSParticipantAttributes& getAttributes()
    {
        return m_att;
    }

    /**
     * Delete a user endpoint
     * @param Endpoint to delete
     * @return True on success
     */
    bool deleteUserEndpoint(
            Endpoint*);

    /**
     * Get the begin of the user reader list
     * @return Iterator pointing to the begin of the user reader list
     */
    std::vector<RTPSReader*>::iterator userReadersListBegin()
    {
        return m_userReaderList.begin();
    }

    /**
     * Get the end of the user reader list
     * @return Iterator pointing to the end of the user reader list
     */
    std::vector<RTPSReader*>::iterator userReadersListEnd()
    {
        return m_userReaderList.end();
    }

    /**
     * Get the begin of the user writer list
     * @return Iterator pointing to the begin of the user writer list
     */
    std::vector<RTPSWriter*>::iterator userWritersListBegin()
    {
        return m_userWriterList.begin();
    }

    /**
     * Get the end of the user writer list
     * @return Iterator pointing to the end of the user writer list
     */
    std::vector<RTPSWriter*>::iterator userWritersListEnd()
    {
        return m_userWriterList.end();
    }

    /** Helper function that creates ReceiverResources based on a Locator_t List, possibly mutating
     * some and updating the list. DOES NOT associate endpoints with it.
     * @param Locator_list - Locator list to be used to create the ReceiverResources
     * @param ApplyMutation - True if we want to create a Resource with a "similar" locator if the one we provide is unavailable
     * @param RegisterReceiver - True if we want the receiver to be registered. Useful for receivers created after participant is enabled.
     */
    bool createReceiverResources(
            LocatorList_t& Locator_list,
            bool ApplyMutation,
            bool RegisterReceiver);

    void createSenderResources(
            const LocatorList_t& locator_list);

    void createSenderResources(
            const Locator_t& locator);

    bool networkFactoryHasRegisteredTransports() const;

#if HAVE_SECURITY
    void set_endpoint_rtps_protection_supports(
            Endpoint* endpoint,
            bool support)
    {
        endpoint->supports_rtps_protection_ = support;
    }

#endif // if HAVE_SECURITY
};
} // namespace rtps
} /* namespace rtps */
} /* namespace eprosima */
#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif //_RTPS_PARTICIPANT_RTPSPARTICIPANTIMPL_H_
