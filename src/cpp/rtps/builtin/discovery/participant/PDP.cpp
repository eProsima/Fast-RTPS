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

/**
 * @file PDP.cpp
 *
 */

#include <fastdds/rtps/builtin/discovery/participant/PDP.h>
#include <fastdds/rtps/builtin/discovery/participant/PDPListener.h>

#include <fastdds/rtps/builtin/BuiltinProtocols.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>

#include <fastdds/rtps/builtin/data/ParticipantProxyData.h>
#include <fastdds/rtps/participant/RTPSParticipantListener.h>
#include <fastdds/rtps/resources/TimedEvent.h>
#include <fastdds/rtps/builtin/data/ReaderProxyData.h>
#include <fastdds/rtps/builtin/data/WriterProxyData.h>

#include <fastdds/rtps/builtin/discovery/endpoint/EDPSimple.h>
#include <fastdds/rtps/builtin/discovery/endpoint/EDPStatic.h>

#include <fastdds/rtps/resources/AsyncWriterThread.h>

#include <rtps/participant/RTPSParticipantImpl.h>

#include <fastdds/rtps/writer/StatelessWriter.h>
#include <fastdds/rtps/reader/StatelessReader.h>
#include <fastdds/rtps/reader/StatefulReader.h>

#include <fastdds/rtps/history/WriterHistory.h>
#include <fastdds/rtps/history/ReaderHistory.h>

#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/DynamicPubSubType.h>

#include <fastrtps/utils/TimeConversion.h>
#include <fastrtps/utils/IPLocator.h>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <fastdds/dds/builtin/typelookup/TypeLookupManager.hpp>
#include <rtps/builtin/data/ProxyHashTables.hpp>

#include <fastdds/dds/log/Log.hpp>

#include <rtps/history/TopicPayloadPoolRegistry.hpp>

#include <mutex>
#include <chrono>

namespace eprosima {
namespace fastdds {
namespace rtps {

// Values for participant type parameter property
const char ParticipantType::SIMPLE[] = "SIMPLE";
const char ParticipantType::SERVER[] = "SERVER";
const char ParticipantType::CLIENT[] = "CLIENT";
const char ParticipantType::BACKUP[] = "BACKUP";

} // namespace rtps
} // namespace fastdds

namespace fastrtps {
namespace rtps {


// Default configuration values for PDP reliable entities.

const Duration_t pdp_heartbeat_period{ 0, 350 * 1000000  }; // 350 milliseconds
const Duration_t pdp_nack_response_delay{ 0, 100 * 1000000  }; // 100 milliseconds
const Duration_t pdp_nack_supression_duration{ 0, 11 * 1000000 }; // 11 milliseconds
const Duration_t pdp_heartbeat_response_delay{ 0, 11 * 1000000 }; // 11 milliseconds

const int32_t pdp_initial_reserved_caches = 20;


PDP::PDP (
        BuiltinProtocols* built,
        const RTPSParticipantAllocationAttributes& allocation)
    : mp_builtin(built)
    , mp_RTPSParticipant(nullptr)
    , mp_PDPWriter(nullptr)
    , mp_PDPReader(nullptr)
    , mp_EDP(nullptr)
    , participant_proxies_number_(allocation.participants.initial)
    , participant_proxies_(allocation.participants)
    , participant_proxies_pool_(allocation.participants)
    , reader_proxies_number_(allocation.total_readers().initial)
    , reader_proxies_pool_(allocation.total_readers())
    , writer_proxies_number_(allocation.total_writers().initial)
    , writer_proxies_pool_(allocation.total_writers())
    , m_hasChangedLocalPDP(true)
    , mp_listener(nullptr)
    , mp_PDPWriterHistory(nullptr)
    , mp_PDPReaderHistory(nullptr)
    , temp_reader_data_(allocation.locators.max_unicast_locators, allocation.locators.max_multicast_locators,
            allocation.data_limits)
    , temp_writer_data_(allocation.locators.max_unicast_locators, allocation.locators.max_multicast_locators,
            allocation.data_limits)
    , mp_mutex(new std::recursive_mutex())
    , resend_participant_info_event_(nullptr)
{
    size_t max_unicast_locators = allocation.locators.max_unicast_locators;
    size_t max_multicast_locators = allocation.locators.max_multicast_locators;

    for (size_t i = 0; i < allocation.participants.initial; ++i)
    {
        participant_proxies_pool_.push_back(new ParticipantProxyData(allocation));
    }

    for (size_t i = 0; i < allocation.total_readers().initial; ++i)
    {
        reader_proxies_pool_.push_back(new ReaderProxyData(max_unicast_locators, max_multicast_locators,
                allocation.data_limits));
    }

    for (size_t i = 0; i < allocation.total_writers().initial; ++i)
    {
        writer_proxies_pool_.push_back(new WriterProxyData(max_unicast_locators, max_multicast_locators,
                allocation.data_limits));
    }
}

PDP::~PDP()
{
    delete resend_participant_info_event_;
    mp_RTPSParticipant->disableReader(mp_PDPReader);
    delete mp_EDP;
    mp_RTPSParticipant->deleteUserEndpoint(mp_PDPWriter);
    mp_RTPSParticipant->deleteUserEndpoint(mp_PDPReader);

    if (mp_PDPWriterHistory)
    {
        PoolConfig cfg = PoolConfig::from_history_attributes(mp_PDPWriterHistory->m_att);
        delete mp_PDPWriterHistory;
        if (writer_payload_pool_)
        {
            writer_payload_pool_->release_history(cfg, false);
            TopicPayloadPoolRegistry::release(writer_payload_pool_);
        }
    }

    if (mp_PDPReaderHistory)
    {
        PoolConfig cfg = PoolConfig::from_history_attributes(mp_PDPReaderHistory->m_att);
        delete mp_PDPReaderHistory;
        if (reader_payload_pool_)
        {
            reader_payload_pool_->release_history(cfg, true);
            TopicPayloadPoolRegistry::release(reader_payload_pool_);
        }
    }

    delete mp_listener;

    for (ParticipantProxyData* it : participant_proxies_)
    {
        delete it;
    }

    for (ParticipantProxyData* it : participant_proxies_pool_)
    {
        delete it;
    }

    for (ReaderProxyData* it : reader_proxies_pool_)
    {
        delete it;
    }

    for (WriterProxyData* it : writer_proxies_pool_)
    {
        delete it;
    }

    delete mp_mutex;
}

ParticipantProxyData* PDP::add_participant_proxy_data(
        const GUID_t& participant_guid,
        bool with_lease_duration)
{
    ParticipantProxyData* ret_val = nullptr;

    // Try to take one entry from the pool
    if (participant_proxies_pool_.empty())
    {
        size_t max_proxies = participant_proxies_.max_size();
        if (participant_proxies_number_ < max_proxies)
        {
            // Pool is empty but limit has not been reached, so we create a new entry.
            ++participant_proxies_number_;
            ret_val = new ParticipantProxyData(mp_RTPSParticipant->getRTPSParticipantAttributes().allocation);
            if (participant_guid != mp_RTPSParticipant->getGuid())
            {
                ret_val->lease_duration_event = new TimedEvent(mp_RTPSParticipant->getEventResource(),
                                [this, ret_val]() -> bool
                                {
                                    check_remote_participant_liveliness(ret_val);
                                    return false;
                                }, 0.0);
            }
        }
        else
        {
            logWarning(RTPS_PDP, "Maximum number of participant proxies (" << max_proxies << \
                    ") reached for participant " << mp_RTPSParticipant->getGuid() << std::endl);
            return nullptr;
        }
    }
    else
    {
        // Pool is not empty, use entry from pool
        ret_val = participant_proxies_pool_.back();
        participant_proxies_pool_.pop_back();
    }

    // Add returned entry to the collection
    ret_val->should_check_lease_duration = with_lease_duration;
    ret_val->m_guid = participant_guid;
    participant_proxies_.push_back(ret_val);

    return ret_val;
}

void PDP::initializeParticipantProxyData(
        ParticipantProxyData* participant_data)
{
    participant_data->m_leaseDuration = mp_RTPSParticipant->getAttributes().builtin.discovery_config.leaseDuration;
    //set_VendorId_eProsima(participant_data->m_VendorId);
    participant_data->m_VendorId = c_VendorId_eProsima;

    participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER;
    participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;

#if HAVE_SECURITY
    participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_SECURE_ANNOUNCER;
    participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_SECURE_DETECTOR;
#endif // if HAVE_SECURITY

    if (mp_RTPSParticipant->getAttributes().builtin.use_WriterLivelinessProtocol)
    {
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;

#if HAVE_SECURITY
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_WRITER;
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_READER;
#endif // if HAVE_SECURITY
    }

    if (mp_RTPSParticipant->getAttributes().builtin.typelookup_config.use_server)
    {
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_TYPELOOKUP_SERVICE_REQUEST_DATA_READER;
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_TYPELOOKUP_SERVICE_REPLY_DATA_WRITER;
    }

    if (mp_RTPSParticipant->getAttributes().builtin.typelookup_config.use_client)
    {
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_TYPELOOKUP_SERVICE_REQUEST_DATA_WRITER;
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_TYPELOOKUP_SERVICE_REPLY_DATA_READER;
    }

#if HAVE_SECURITY
    participant_data->m_availableBuiltinEndpoints |= mp_RTPSParticipant->security_manager().builtin_endpoints();
#endif // if HAVE_SECURITY

    for (const Locator_t& loc : mp_RTPSParticipant->getAttributes().defaultUnicastLocatorList)
    {
        participant_data->default_locators.add_unicast_locator(loc);
    }
    for (const Locator_t& loc : mp_RTPSParticipant->getAttributes().defaultMulticastLocatorList)
    {
        participant_data->default_locators.add_multicast_locator(loc);
    }
    participant_data->m_expectsInlineQos = false;
    participant_data->m_guid = mp_RTPSParticipant->getGuid();
    memcpy( participant_data->m_key.value, participant_data->m_guid.guidPrefix.value, 12);
    memcpy( participant_data->m_key.value + 12, participant_data->m_guid.entityId.value, 4);

    // Keep persistence Guid_Prefix_t in a specific property. This info must be propagated to all builtin endpoints
    {
        GuidPrefix_t persistent = mp_RTPSParticipant->getAttributes().prefix;

        if (persistent != c_GuidPrefix_Unknown)
        {
            participant_data->set_persistence_guid(
                GUID_t(
                    persistent,
                    c_EntityId_RTPSParticipant));
        }
    }

    participant_data->metatraffic_locators.unicast.clear();
    for (const Locator_t& loc : this->mp_builtin->m_metatrafficUnicastLocatorList)
    {
        participant_data->metatraffic_locators.add_unicast_locator(loc);
    }

    participant_data->metatraffic_locators.multicast.clear();
    if (!m_discovery.avoid_builtin_multicast || participant_data->metatraffic_locators.unicast.empty())
    {
        for (const Locator_t& loc: this->mp_builtin->m_metatrafficMulticastLocatorList)
        {
            participant_data->metatraffic_locators.add_multicast_locator(loc);
        }
    }

    participant_data->m_participantName = std::string(mp_RTPSParticipant->getAttributes().getName());

    participant_data->m_userData = mp_RTPSParticipant->getAttributes().userData;

#if HAVE_SECURITY
    IdentityToken* identity_token = nullptr;
    if (mp_RTPSParticipant->security_manager().get_identity_token(&identity_token) && identity_token != nullptr)
    {
        participant_data->identity_token_ = std::move(*identity_token);
        mp_RTPSParticipant->security_manager().return_identity_token(identity_token);
    }

    PermissionsToken* permissions_token = nullptr;
    if (mp_RTPSParticipant->security_manager().get_permissions_token(&permissions_token)
            && permissions_token != nullptr)
    {
        participant_data->permissions_token_ = std::move(*permissions_token);
        mp_RTPSParticipant->security_manager().return_permissions_token(permissions_token);
    }

    if (mp_RTPSParticipant->is_secure())
    {
        const security::ParticipantSecurityAttributes& sec_attrs = mp_RTPSParticipant->security_attributes();
        participant_data->security_attributes_ = sec_attrs.mask();
        participant_data->plugin_security_attributes_ = sec_attrs.plugin_participant_attributes;
    }
    else
    {
        participant_data->security_attributes_ = 0UL;
        participant_data->plugin_security_attributes_ = 0UL;
    }
#endif // if HAVE_SECURITY
}

bool PDP::initPDP(
        RTPSParticipantImpl* part)
{
    logInfo(RTPS_PDP, "Beginning");
    mp_RTPSParticipant = part;
    m_discovery = mp_RTPSParticipant->getAttributes().builtin;
    initial_announcements_ = m_discovery.discovery_config.initial_announcements;

#if HAVE_SECURITY
    if (part->is_secure())
    {
        m_discovery.discovery_config.ignoreParticipantFlags = ParticipantFilteringFlags_t::NO_FILTER;
    }
#endif // if HAVE_SECURITY

    //CREATE ENDPOINTS
    if (!createPDPEndpoints())
    {
        return false;
    }
    //UPDATE METATRAFFIC.
    mp_builtin->updateMetatrafficLocators(this->mp_PDPReader->getAttributes().unicastLocatorList);

    mp_mutex->lock();
    ParticipantProxyData* pdata = add_participant_proxy_data(part->getGuid(), false);
    mp_mutex->unlock();

    if (pdata == nullptr)
    {
        return false;
    }
    initializeParticipantProxyData(pdata);

    // Create lease events on already created proxy data objects
    for (ParticipantProxyData* pool_item : participant_proxies_pool_)
    {
        pool_item->lease_duration_event = new TimedEvent(mp_RTPSParticipant->getEventResource(),
                        [this, pool_item]() -> bool
                        {
                            check_remote_participant_liveliness(pool_item);
                            return false;
                        }, 0.0);
    }

    resend_participant_info_event_ = new TimedEvent(mp_RTPSParticipant->getEventResource(),
                    [&]() -> bool
                    {
                        announceParticipantState(false);
                        set_next_announcement_interval();
                        return true;
                    },
                    0);

    set_initial_announcement_interval();

    return true;
}

bool PDP::enable()
{
    return mp_RTPSParticipant->enableReader(mp_PDPReader);
}

void PDP::announceParticipantState(
        bool new_change,
        bool dispose,
        WriteParams& wparams)
{
    // logInfo(RTPS_PDP, "Announcing RTPSParticipant State (new change: " << new_change << ")");
    CacheChange_t* change = nullptr;

    if (!dispose)
    {
        if (m_hasChangedLocalPDP.exchange(false) || new_change)
        {
            this->mp_mutex->lock();
            ParticipantProxyData* local_participant_data = getLocalParticipantProxyData();
            InstanceHandle_t key = local_participant_data->m_key;
            ParticipantProxyData proxy_data_copy(*local_participant_data);
            this->mp_mutex->unlock();

            if (mp_PDPWriterHistory->getHistorySize() > 0)
            {
                mp_PDPWriterHistory->remove_min_change();
            }
            uint32_t cdr_size = proxy_data_copy.get_serialized_size(true);
            change = mp_PDPWriter->new_change(
                [cdr_size]() -> uint32_t
                {
                    return cdr_size;
                },
                ALIVE, key);

            if (change != nullptr)
            {
                CDRMessage_t aux_msg(change->serializedPayload);

#if __BIG_ENDIAN__
                change->serializedPayload.encapsulation = (uint16_t)PL_CDR_BE;
                aux_msg.msg_endian = BIGEND;
#else
                change->serializedPayload.encapsulation = (uint16_t)PL_CDR_LE;
                aux_msg.msg_endian =  LITTLEEND;
#endif // if __BIG_ENDIAN__

                if (proxy_data_copy.writeToCDRMessage(&aux_msg, true))
                {
                    change->serializedPayload.length = (uint16_t)aux_msg.length;

                    mp_PDPWriterHistory->add_change(change, wparams);
                }
                else
                {
                    logError(RTPS_PDP, "Cannot serialize ParticipantProxyData.");
                }
            }
        }

    }
    else
    {
        this->mp_mutex->lock();
        ParticipantProxyData proxy_data_copy(*getLocalParticipantProxyData());
        this->mp_mutex->unlock();

        if (mp_PDPWriterHistory->getHistorySize() > 0)
        {
            mp_PDPWriterHistory->remove_min_change();
        }
        uint32_t cdr_size = proxy_data_copy.get_serialized_size(true);
        change = mp_PDPWriter->new_change([cdr_size]() -> uint32_t
                        {
                            return cdr_size;
                        },
                        NOT_ALIVE_DISPOSED_UNREGISTERED, getLocalParticipantProxyData()->m_key);

        if (change != nullptr)
        {
            CDRMessage_t aux_msg(change->serializedPayload);

#if __BIG_ENDIAN__
            change->serializedPayload.encapsulation = (uint16_t)PL_CDR_BE;
            aux_msg.msg_endian = BIGEND;
#else
            change->serializedPayload.encapsulation = (uint16_t)PL_CDR_LE;
            aux_msg.msg_endian =  LITTLEEND;
#endif // if __BIG_ENDIAN__

            if (proxy_data_copy.writeToCDRMessage(&aux_msg, true))
            {
                change->serializedPayload.length = (uint16_t)aux_msg.length;

                mp_PDPWriterHistory->add_change(change, wparams);
            }
            else
            {
                logError(RTPS_PDP, "Cannot serialize ParticipantProxyData.");
            }
        }
    }

}

void PDP::stopParticipantAnnouncement()
{
    resend_participant_info_event_->cancel_timer();
}

void PDP::resetParticipantAnnouncement()
{
    resend_participant_info_event_->restart_timer();
}

bool PDP::has_reader_proxy_data(
        const GUID_t& reader)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == reader.guidPrefix)
        {
            ProxyHashTable<ReaderProxyData>& readers = *pit->m_readers;
            return readers.find(reader.entityId) != readers.end();
        }
    }
    return false;
}

bool PDP::lookupReaderProxyData(
        const GUID_t& reader,
        ReaderProxyData& rdata)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == reader.guidPrefix)
        {
            auto rit = pit->m_readers->find(reader.entityId);
            if (rit != pit->m_readers->end())
            {
                rdata.copy(rit->second);
                return true;
            }
        }
    }
    return false;
}

bool PDP::has_writer_proxy_data(
        const GUID_t& writer)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == writer.guidPrefix)
        {
            ProxyHashTable<WriterProxyData>& writers = *pit->m_writers;
            return writers.find(writer.entityId) != writers.end();
        }
    }
    return false;
}

bool PDP::lookupWriterProxyData(
        const GUID_t& writer,
        WriterProxyData& wdata)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == writer.guidPrefix)
        {
            auto wit = pit->m_writers->find(writer.entityId);
            if ( wit != pit->m_writers->end())
            {
                wdata.copy(wit->second);
                return true;
            }
        }
    }
    return false;
}

bool PDP::removeReaderProxyData(
        const GUID_t& reader_guid)
{
    logInfo(RTPS_PDP, "Removing reader proxy data " << reader_guid);
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);

    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == reader_guid.guidPrefix)
        {
            auto rit = pit->m_readers->find(reader_guid.entityId);

            if (rit != pit->m_readers->end())
            {
                ReaderProxyData* pR = rit->second;
                mp_EDP->unpairReaderProxy(pit->m_guid, reader_guid);

                RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();
                if (listener)
                {
                    ReaderDiscoveryInfo info(std::move(*pR));
                    info.status = ReaderDiscoveryInfo::REMOVED_READER;
                    listener->onReaderDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                }

                // Clear reader proxy data and move to pool in order to allow reuse
                pR->clear();
                pit->m_readers->erase(rit);
                reader_proxies_pool_.push_back(pR);
                return true;
            }
        }
    }

    return false;
}

bool PDP::removeWriterProxyData(
        const GUID_t& writer_guid)
{
    logInfo(RTPS_PDP, "Removing writer proxy data " << writer_guid);
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);

    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == writer_guid.guidPrefix)
        {
            auto wit = pit->m_writers->find(writer_guid.entityId);

            if (wit != pit->m_writers->end())
            {
                WriterProxyData* pW = wit->second;
                mp_EDP->unpairWriterProxy(pit->m_guid, writer_guid, false);

                RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();
                if (listener)
                {
                    WriterDiscoveryInfo info(std::move(*pW));
                    info.status = WriterDiscoveryInfo::REMOVED_WRITER;
                    listener->onWriterDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                }

                // Clear writer proxy data and move to pool in order to allow reuse
                pW->clear();
                pit->m_writers->erase(wit);
                writer_proxies_pool_.push_back(pW);

                return true;
            }
        }
    }

    return false;
}

bool PDP::lookup_participant_name(
        const GUID_t& guid,
        string_255& name)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid == guid)
        {
            name = pit->m_participantName;
            return true;
        }
    }
    return false;
}

bool PDP::lookup_participant_key(
        const GUID_t& participant_guid,
        InstanceHandle_t& key)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid == participant_guid)
        {
            key = pit->m_key;
            return true;
        }
    }
    return false;
}

ReaderProxyData* PDP::addReaderProxyData(
        const GUID_t& reader_guid,
        GUID_t& participant_guid,
        std::function<bool(ReaderProxyData*, bool, const ParticipantProxyData&)> initializer_func)
{
    logInfo(RTPS_PDP, "Adding reader proxy data " << reader_guid);
    ReaderProxyData* ret_val = nullptr;

    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);

    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == reader_guid.guidPrefix)
        {
            // Copy participant data to be used outside.
            participant_guid = pit->m_guid;

            // Check that it is not already there:
            auto rpi = pit->m_readers->find(reader_guid.entityId);

            if ( rpi != pit->m_readers->end())
            {
                ret_val = rpi->second;

                if (!initializer_func(ret_val, true, *pit))
                {
                    return nullptr;
                }

                RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();
                if (listener)
                {
                    ReaderDiscoveryInfo info(*ret_val);
                    info.status = ReaderDiscoveryInfo::CHANGED_QOS_READER;
                    listener->onReaderDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                    check_and_notify_type_discovery(listener, *ret_val);
                }

                return ret_val;
            }

            // Try to take one entry from the pool
            if (reader_proxies_pool_.empty())
            {
                size_t max_proxies = reader_proxies_pool_.max_size();
                if (reader_proxies_number_ < max_proxies)
                {
                    // Pool is empty but limit has not been reached, so we create a new entry.
                    ++reader_proxies_number_;
                    ret_val = new ReaderProxyData(
                        mp_RTPSParticipant->getAttributes().allocation.locators.max_unicast_locators,
                        mp_RTPSParticipant->getAttributes().allocation.locators.max_multicast_locators,
                        mp_RTPSParticipant->getAttributes().allocation.data_limits);
                }
                else
                {
                    logWarning(RTPS_PDP, "Maximum number of reader proxies (" << max_proxies <<
                            ") reached for participant " << mp_RTPSParticipant->getGuid() << std::endl);
                    return nullptr;
                }
            }
            else
            {
                // Pool is not empty, use entry from pool
                ret_val = reader_proxies_pool_.back();
                reader_proxies_pool_.pop_back();
            }

            // Add to ParticipantProxyData
            (*pit->m_readers)[reader_guid.entityId] = ret_val;

            if (!initializer_func(ret_val, false, *pit))
            {
                return nullptr;
            }

            RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();
            if (listener)
            {
                ReaderDiscoveryInfo info(*ret_val);
                info.status = ReaderDiscoveryInfo::DISCOVERED_READER;
                listener->onReaderDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                check_and_notify_type_discovery(listener, *ret_val);
            }

            return ret_val;
        }
    }

    return nullptr;
}

WriterProxyData* PDP::addWriterProxyData(
        const GUID_t& writer_guid,
        GUID_t& participant_guid,
        std::function<bool(WriterProxyData*, bool, const ParticipantProxyData&)> initializer_func)
{
    logInfo(RTPS_PDP, "Adding reader proxy data " << writer_guid);
    WriterProxyData* ret_val = nullptr;

    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);

    for (ParticipantProxyData* pit : participant_proxies_)
    {
        if (pit->m_guid.guidPrefix == writer_guid.guidPrefix)
        {
            // Copy participant data to be used outside.
            participant_guid = pit->m_guid;

            // Check that it is not already there:
            auto wpi = pit->m_writers->find(writer_guid.entityId);

            if (wpi != pit->m_writers->end())
            {
                ret_val = wpi->second;

                if (!initializer_func(ret_val, true, *pit))
                {
                    return nullptr;
                }

                RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();
                if (listener)
                {
                    WriterDiscoveryInfo info(*ret_val);
                    info.status = WriterDiscoveryInfo::CHANGED_QOS_WRITER;
                    listener->onWriterDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                    check_and_notify_type_discovery(listener, *ret_val);
                }

                return ret_val;
            }

            // Try to take one entry from the pool
            if (writer_proxies_pool_.empty())
            {
                size_t max_proxies = writer_proxies_pool_.max_size();
                if (writer_proxies_number_ < max_proxies)
                {
                    // Pool is empty but limit has not been reached, so we create a new entry.
                    ++writer_proxies_number_;
                    ret_val = new WriterProxyData(
                        mp_RTPSParticipant->getAttributes().allocation.locators.max_unicast_locators,
                        mp_RTPSParticipant->getAttributes().allocation.locators.max_multicast_locators,
                        mp_RTPSParticipant->getAttributes().allocation.data_limits);
                }
                else
                {
                    logWarning(RTPS_PDP, "Maximum number of writer proxies (" << max_proxies <<
                            ") reached for participant " << mp_RTPSParticipant->getGuid() << std::endl);
                    return nullptr;
                }
            }
            else
            {
                // Pool is not empty, use entry from pool
                ret_val = writer_proxies_pool_.back();
                writer_proxies_pool_.pop_back();
            }

            // Add to ParticipantProxyData
            (*pit->m_writers)[writer_guid.entityId] = ret_val;

            if (!initializer_func(ret_val, false, *pit))
            {
                return nullptr;
            }

            RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();
            if (listener)
            {
                WriterDiscoveryInfo info(*ret_val);
                info.status = WriterDiscoveryInfo::DISCOVERED_WRITER;
                listener->onWriterDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                check_and_notify_type_discovery(listener, *ret_val);
            }

            return ret_val;
        }
    }

    return nullptr;
}

bool PDP::remove_remote_participant(
        const GUID_t& partGUID,
        ParticipantDiscoveryInfo::DISCOVERY_STATUS reason)
{
    if (partGUID == getLocalParticipantProxyData()->m_guid)
    {
        // avoid removing our own data
        return false;
    }

    logInfo(RTPS_PDP, partGUID );
    ParticipantProxyData* pdata = nullptr;

    //Remove it from our vector or RTPSParticipantProxies:
    this->mp_mutex->lock();
    for (ResourceLimitedVector<ParticipantProxyData*>::iterator pit = participant_proxies_.begin();
            pit != participant_proxies_.end(); ++pit)
    {
        if ((*pit)->m_guid == partGUID)
        {
            pdata = *pit;
            participant_proxies_.erase(pit);
            break;
        }
    }
    this->mp_mutex->unlock();

    if (pdata != nullptr)
    {
        if (mp_EDP != nullptr)
        {
            RTPSParticipantListener* listener = mp_RTPSParticipant->getListener();

            for (auto pit : *pdata->m_readers)
            {
                ReaderProxyData* rit = pit.second;
                GUID_t reader_guid(rit->guid());
                if (reader_guid != c_Guid_Unknown)
                {
                    mp_EDP->unpairReaderProxy(partGUID, reader_guid);

                    if (listener)
                    {
                        ReaderDiscoveryInfo info(std::move(*rit));
                        info.status = ReaderDiscoveryInfo::REMOVED_READER;
                        listener->onReaderDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                    }
                }
            }
            for (auto pit : *pdata->m_writers)
            {
                WriterProxyData* wit = pit.second;
                GUID_t writer_guid(wit->guid());
                if (writer_guid != c_Guid_Unknown)
                {
                    mp_EDP->unpairWriterProxy(partGUID, writer_guid,
                            reason == ParticipantDiscoveryInfo::DISCOVERY_STATUS::DROPPED_PARTICIPANT);

                    if (listener)
                    {
                        WriterDiscoveryInfo info(std::move(*wit));
                        info.status = WriterDiscoveryInfo::REMOVED_WRITER;
                        listener->onWriterDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
                    }
                }
            }
        }

        if (mp_builtin->mp_WLP != nullptr)
        {
            this->mp_builtin->mp_WLP->removeRemoteEndpoints(pdata);
        }

        if (mp_builtin->tlm_ != nullptr)
        {
            mp_builtin->tlm_->remove_remote_endpoints(pdata);
        }

        this->mp_EDP->removeRemoteEndpoints(pdata);
        this->removeRemoteEndpoints(pdata);

#if HAVE_SECURITY
        mp_builtin->mp_participantImpl->security_manager().remove_participant(*pdata);
#endif // if HAVE_SECURITY

        this->mp_PDPReaderHistory->getMutex()->lock();
        for (std::vector<CacheChange_t*>::iterator it = this->mp_PDPReaderHistory->changesBegin();
                it != this->mp_PDPReaderHistory->changesEnd(); ++it)
        {
            if ((*it)->instanceHandle == pdata->m_key)
            {
                this->mp_PDPReaderHistory->remove_change(*it);
                break;
            }
        }
        this->mp_PDPReaderHistory->getMutex()->unlock();

        auto listener =  mp_RTPSParticipant->getListener();
        if (listener != nullptr)
        {
            std::lock_guard<std::mutex> lock(callback_mtx_);
            ParticipantDiscoveryInfo info(*pdata);
            info.status = reason;
            listener->onParticipantDiscovery(mp_RTPSParticipant->getUserRTPSParticipant(), std::move(info));
        }

        this->mp_mutex->lock();

        // Return reader proxy objects to pool
        for (auto pit : *pdata->m_readers)
        {
            reader_proxies_pool_.push_back(pit.second);
        }
        pdata->m_readers->clear();

        // Return writer proxy objects to pool
        for (auto pit : *pdata->m_writers)
        {
            writer_proxies_pool_.push_back(pit.second);
        }
        pdata->m_writers->clear();

        // Cancel lease event
        if (pdata->lease_duration_event != nullptr)
        {
            pdata->lease_duration_event->cancel_timer();
        }

        // Return proxy object to pool
        pdata->clear();
        participant_proxies_pool_.push_back(pdata);

        this->mp_mutex->unlock();
        return true;
    }

    return false;
}

const BuiltinAttributes& PDP::builtin_attributes() const
{
    return mp_builtin->m_att;
}

void PDP::assert_remote_participant_liveliness(
        const GuidPrefix_t& remote_guid)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);

    for (ParticipantProxyData* it : this->participant_proxies_)
    {
        if (it->m_guid.guidPrefix == remote_guid)
        {
            // TODO Ricardo: Study if isAlive attribute is necessary.
            it->isAlive = true;
            it->assert_liveliness();
            break;
        }
    }
}

CDRMessage_t PDP::get_participant_proxy_data_serialized(
        Endianness_t endian)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    CDRMessage_t cdr_msg(RTPSMESSAGE_DEFAULT_SIZE);
    cdr_msg.msg_endian = endian;

    if (!getLocalParticipantProxyData()->writeToCDRMessage(&cdr_msg, false))
    {
        cdr_msg.pos = 0;
        cdr_msg.length = 0;
    }

    return cdr_msg;
}

void PDP::check_remote_participant_liveliness(
        ParticipantProxyData* remote_participant)
{
    std::unique_lock<std::recursive_mutex> guard(*this->mp_mutex);

    if (remote_participant->should_check_lease_duration)
    {
        assert(GUID_t::unknown() != remote_participant->m_guid);
        // Check last received message's time_point plus lease duration time doesn't overcome now().
        // If overcame, remove participant.
        auto now = std::chrono::steady_clock::now();
        auto real_lease_tm = remote_participant->last_received_message_tm() +
                std::chrono::microseconds(TimeConv::Duration_t2MicroSecondsInt64(remote_participant->m_leaseDuration));
        if (now > real_lease_tm)
        {
            guard.unlock();
            remove_remote_participant(remote_participant->m_guid, ParticipantDiscoveryInfo::DROPPED_PARTICIPANT);
            return;
        }

        // Calculate next trigger.
        auto next_trigger = real_lease_tm - now;
        remote_participant->lease_duration_event->update_interval_millisec(
            (double)std::chrono::duration_cast<std::chrono::milliseconds>(next_trigger).count());
        remote_participant->lease_duration_event->restart_timer();
    }
}

void PDP::check_and_notify_type_discovery(
        RTPSParticipantListener* listener,
        const WriterProxyData& wdata) const
{
    check_and_notify_type_discovery(
        listener,
        wdata.topicName(),
        wdata.typeName(),
        wdata.has_type_id() ? &wdata.type_id().m_type_identifier : nullptr,
        wdata.has_type() ? &wdata.type().m_type_object : nullptr,
        wdata.has_type_information() ? &wdata.type_information() : nullptr);
}

void PDP::check_and_notify_type_discovery(
        RTPSParticipantListener* listener,
        const ReaderProxyData& rdata) const
{
    check_and_notify_type_discovery(
        listener,
        rdata.topicName(),
        rdata.typeName(),
        rdata.has_type_id() ? &rdata.type_id().m_type_identifier : nullptr,
        rdata.has_type() ? &rdata.type().m_type_object : nullptr,
        rdata.has_type_information() ? &rdata.type_information() : nullptr);
}

void PDP::check_and_notify_type_discovery(
        RTPSParticipantListener* listener,
        const string_255& topic_name,
        const string_255& type_name,
        const types::TypeIdentifier* type_id,
        const types::TypeObject* type_obj,
        const xtypes::TypeInformation* type_info) const
{
    // Notify about type_info
    if (type_info && type_info->assigned())
    {
        listener->on_type_information_received(
            mp_RTPSParticipant->getUserRTPSParticipant(), topic_name, type_name, type_info->type_information);
    }

    // Are we discovering a type?
    types::DynamicType_ptr dyn_type;
    if (type_obj && type_obj->_d() == types::EK_COMPLETE) // Writer shares a Complete TypeObject
    {
        dyn_type = types::TypeObjectFactory::get_instance()->build_dynamic_type(
            type_name.to_string(), type_id, type_obj);
    }
    else if (type_id && type_id->_d() != static_cast<octet>(0x00)
            && type_id->_d() < types::EK_MINIMAL) // Writer shares a TypeIdentifier that doesn't need TypeObject
    {
        dyn_type = types::TypeObjectFactory::get_instance()->build_dynamic_type(
            type_name.to_string(), type_id);
    }

    if (dyn_type != nullptr)
    {
        types::DynamicPubSubType type_support(dyn_type);

        if (!mp_RTPSParticipant->check_type(type_name.to_string()))
        {
            // Discovering a type
            listener->on_type_discovery(
                mp_RTPSParticipant->getUserRTPSParticipant(),
                fastdds::dds::builtin::INVALID_SAMPLE_IDENTITY,
                topic_name,
                type_id,
                type_obj,
                dyn_type);
        }
    }
}

void PDP::set_next_announcement_interval()
{
    if (initial_announcements_.count > 0)
    {
        --initial_announcements_.count;
        resend_participant_info_event_->update_interval(initial_announcements_.period);
    }
    else
    {
        resend_participant_info_event_->update_interval(m_discovery.discovery_config.leaseDuration_announcementperiod);
    }
}

void PDP::set_initial_announcement_interval()
{
    if ((initial_announcements_.count > 0) && (initial_announcements_.period <= c_TimeZero))
    {
        // Force a small interval (1ms) between initial announcements
        logWarning(RTPS_PDP, "Initial announcement period is not strictly positive. Changing to 1ms.");
        initial_announcements_.period = { 0, 1000000 };
    }
    set_next_announcement_interval();
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
