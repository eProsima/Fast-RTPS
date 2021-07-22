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
 * @file EDPSimple.cpp
 *
 */

#include <fastdds/core/policy/ParameterSerializer.hpp>
#include <fastdds/rtps/builtin/discovery/endpoint/EDPSimple.h>
#include <rtps/builtin/discovery/endpoint/EDPSimpleListeners.h>
#include <fastdds/rtps/builtin/discovery/participant/PDP.h>
#include <fastrtps_deprecated/participant/ParticipantImpl.h>
#include <fastdds/rtps/writer/StatefulWriter.h>
#include <fastdds/rtps/reader/StatefulReader.h>
#include <fastdds/rtps/attributes/HistoryAttributes.h>
#include <fastdds/rtps/attributes/WriterAttributes.h>
#include <fastdds/rtps/attributes/ReaderAttributes.h>
#include <fastdds/rtps/history/ReaderHistory.h>
#include <fastdds/rtps/history/WriterHistory.h>
#include <fastdds/rtps/builtin/data/WriterProxyData.h>
#include <fastdds/rtps/builtin/data/ReaderProxyData.h>
#include <fastdds/rtps/builtin/data/ParticipantProxyData.h>
#include <fastdds/rtps/builtin/BuiltinProtocols.h>

#include <fastdds/dds/log/Log.hpp>

#include <rtps/history/TopicPayloadPoolRegistry.hpp>

#include <rtps/builtin/discovery/endpoint/EDPUtils.hpp>

#include <mutex>
#include <forward_list>
#include <algorithm>

using ParameterList = eprosima::fastdds::dds::ParameterList;

namespace eprosima {
namespace fastrtps {
namespace rtps {

// Default configuration values for EDP entities.
static const Duration_t edp_heartbeat_period{1, 0}; // 1 second
static const Duration_t edp_nack_response_delay{0, 100 * 1000 }; // 100 milliseconds
static const Duration_t edp_nack_supression_duration{0, 10 * 1000}; // 10 milliseconds
static const Duration_t edp_heartbeat_response_delay{0, 10 * 1000}; // 10 milliseconds

static const int32_t edp_reader_initial_reserved_caches = 1;
static const int32_t edp_writer_initial_reserved_caches = 20;

EDPSimple::EDPSimple(
        PDP* p,
        RTPSParticipantImpl* part)
    : EDP(p, part)
    , publications_listener_(nullptr)
    , subscriptions_listener_(nullptr)
    , temp_reader_proxy_data_(
        part->getRTPSParticipantAttributes().allocation.locators.max_unicast_locators,
        part->getRTPSParticipantAttributes().allocation.locators.max_multicast_locators,
        part->getRTPSParticipantAttributes().allocation.data_limits)
    , temp_writer_proxy_data_(
        part->getRTPSParticipantAttributes().allocation.locators.max_unicast_locators,
        part->getRTPSParticipantAttributes().allocation.locators.max_multicast_locators,
        part->getRTPSParticipantAttributes().allocation.data_limits)
{
}

EDPSimple::~EDPSimple()
{
#if HAVE_SECURITY
    if (this->publications_secure_reader_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(publications_secure_reader_.first);
        EDPUtils::release_payload_pool(sec_pub_reader_payload_pool_, publications_secure_writer_.second->m_att, true);
        delete(publications_secure_reader_.second);
    }

    if (this->subscriptions_secure_reader_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(subscriptions_secure_reader_.first);
        EDPUtils::release_payload_pool(sec_sub_reader_payload_pool_, subscriptions_secure_reader_.second->m_att, true);
        delete(subscriptions_secure_reader_.second);
    }

    if (this->publications_secure_writer_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(publications_secure_writer_.first);
        EDPUtils::release_payload_pool(sec_pub_writer_payload_pool_, publications_secure_writer_.second->m_att, false);
        delete(publications_secure_writer_.second);
    }

    if (this->subscriptions_secure_writer_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(subscriptions_secure_writer_.first);
        EDPUtils::release_payload_pool(sec_sub_writer_payload_pool_, subscriptions_secure_writer_.second->m_att, true);
        delete(subscriptions_secure_writer_.second);
    }
#endif // if HAVE_SECURITY

    if (this->publications_reader_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(publications_reader_.first);
        // This payload is created outside the constructor, so it could not be created
        if (nullptr != pub_reader_payload_pool_)
        {
            EDPUtils::release_payload_pool(pub_reader_payload_pool_, publications_reader_.second->m_att, true);
        }
        delete(publications_reader_.second);
    }
    if (this->subscriptions_reader_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(subscriptions_reader_.first);
        // This payload is created outside the constructor, so it could not be created
        if (nullptr != sub_reader_payload_pool_)
        {
            EDPUtils::release_payload_pool(sub_reader_payload_pool_, subscriptions_reader_.second->m_att, true);
        }
        delete(subscriptions_reader_.second);
    }
    if (this->publications_writer_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(publications_writer_.first);
        // This payload is created outside the constructor, so it could not be created
        if (nullptr != pub_writer_payload_pool_)
        {
            EDPUtils::release_payload_pool(pub_writer_payload_pool_, publications_writer_.second->m_att, false);
        }
        delete(publications_writer_.second);
    }
    if (this->subscriptions_writer_.first != nullptr)
    {
        this->mp_RTPSParticipant->deleteUserEndpoint(subscriptions_writer_.first);
        // This payload is created outside the constructor, so it could not be created
        if (nullptr != sub_writer_payload_pool_)
        {
            EDPUtils::release_payload_pool(sub_writer_payload_pool_, subscriptions_writer_.second->m_att, false);
        }
        delete(subscriptions_writer_.second);
    }

    if (nullptr != publications_listener_)
    {
        delete(publications_listener_);
    }

    if (nullptr != subscriptions_listener_)
    {
        delete(subscriptions_listener_);
    }
}

bool EDPSimple::initEDP(
        BuiltinAttributes& attributes)
{
    logInfo(RTPS_EDP, "Beginning Simple Endpoint Discovery Protocol");
    m_discovery = attributes;

    if (!createSEDPEndpoints())
    {
        logError(RTPS_EDP, "Problem creation SimpleEDP endpoints");
        return false;
    }

#if HAVE_SECURITY
    if (mp_RTPSParticipant->is_secure() && !create_sedp_secure_endpoints())
    {
        logError(RTPS_EDP, "Problem creation SimpleEDP endpoints");
        return false;
    }
#endif // if HAVE_SECURITY

    return true;
}

//! Process the info recorded in the persistence database
void EDPSimple::processPersistentData(
        t_p_StatefulReader& reader,
        t_p_StatefulWriter& writer,
        key_list& demises)
{
    std::lock_guard<RecursiveTimedMutex> guardR(reader.first->getMutex());
    std::lock_guard<RecursiveTimedMutex> guardW(writer.first->getMutex());
    std::lock_guard<std::recursive_mutex> guardP(*mp_PDP->getMutex());

    // own server instance
    InstanceHandle_t server_key = mp_PDP->getLocalParticipantProxyData()->m_key;

    // reference own references from writer history
    std::forward_list<CacheChange_t*> removal;

    // List known participants
    key_list known_participants;

    std::for_each(
        mp_PDP->ParticipantProxiesBegin(),
        mp_PDP->ParticipantProxiesEnd(),
        [&known_participants](const ParticipantProxyData* pD)
        {
            known_participants.insert(pD->m_key);
        });

    // We have not processed any PDP message yet but any lease duration callback may have already modified demises

    // aux lambda to retrieve sample identity
    // update format for 2.0.x port
    uint32_t qos_size;
    SampleIdentity si;
    ChangeKind_t kind;

    auto param_process = [&si, &kind](CDRMessage_t* msg, const ParameterId_t& pid, uint16_t plength)
            {
                // we use the PID_PARTICIPANT_GUID to identify a DATA(r|w)
                if (pid == fastdds::dds::PID_PARTICIPANT_GUID )
                {
                    kind = ALIVE;
                    return true;
                }

                if (pid == fastdds::dds::PID_PROPERTY_LIST)
                {
                    ParameterPropertyList_t pl;
                    si = SampleIdentity::unknown();

                    if (!fastdds::dds::ParameterSerializer<ParameterPropertyList_t>::read_from_cdr_message(pl, msg,
                            plength))
                    {
                        return false;
                    }

                    ParameterPropertyList_t::iterator it = pl.begin();
                    it = std::find_if( it, pl.end(),
                                    [](ParameterPropertyList_t::iterator::reference p)
                                    {
                                        return "PID_CLIENT_SERVER_KEY" == p.first();
                                    });

                    if (it != pl.end())
                    {
                        std::istringstream in(it->second());
                        in >> si;
                    }
                }

                return true;
            };


    std::for_each(writer.second->changesBegin(),
            writer.second->changesEnd(),
            [&](CacheChange_t* change)
            {
                // Reset the variables referenced by the lambda
                si = SampleIdentity::unknown();
                kind = NOT_ALIVE_DISPOSED_UNREGISTERED;

                // We must retrieve the identity info from the payload and update the WriteParams
                CDRMessage_t msg(change->serializedPayload);
                ParameterList::readParameterListfromCDRMsg(msg, param_process, true, qos_size);

                // determine kind
                change->kind = kind;

                // recover sample identity
                if (si != SampleIdentity::unknown())
                {
                    change->write_params.sample_identity(si);
                    change->write_params.related_sample_identity(si);
                }

                // Get Participant InstanceHandle
                InstanceHandle_t handle;
                {
                    GUID_t guid = iHandle2GUID(change->instanceHandle);
                    guid.entityId = c_EntityId_RTPSParticipant;
                    handle = guid;
                }

                // mark for removal endpoints from unknown participants
                if ( known_participants.find(handle) == known_participants.end())
                {
                    demises.insert(change->instanceHandle);
                    return;
                }

                // check if its own data: mark for removal and ignore
                if ( handle == server_key)
                {
                    removal.push_front(change);
                    return;
                }

                CacheChange_t* change_to_add = nullptr;

                if (!reader.first->reserveCache(&change_to_add, change->serializedPayload.length)) //Reserve a new cache from the corresponding cache pool
                {
                    logError(RTPS_EDP, "Problem reserving CacheChange in EDPServer reader");
                    return;
                }

                if (!change_to_add->copy(change))
                {
                    logWarning(RTPS_EDP, "Problem copying CacheChange, received data is: "
                        << change->serializedPayload.length << " bytes and max size in EDPServer reader"
                        << " is " << change_to_add->serializedPayload.max_size);

                    reader.first->releaseCache(change_to_add);
                    return;
                }

                if (!reader.first->change_received(change_to_add, nullptr))
                {
                    logInfo(RTPS_EDP, "EDPServer couldn't process database data not add change "
                        << change_to_add->sequenceNumber);
                    reader.first->releaseCache(change_to_add);
                }

                // change_to_add would be released within change_received
            });

    // remove our own old server samples
    for (auto pC : removal)
    {
        writer.second->remove_change(pC);
    }

    // We don't need to awake the server thread because we are in it
}

void EDPSimple::set_builtin_reader_history_attributes(
        HistoryAttributes& attributes)
{
    attributes.initialReservedCaches = edp_reader_initial_reserved_caches;
    attributes.payloadMaxSize = mp_PDP->builtin_attributes().readerPayloadSize;
    attributes.memoryPolicy = mp_PDP->builtin_attributes().readerHistoryMemoryPolicy;
}

void EDPSimple::set_builtin_writer_history_attributes(
        HistoryAttributes& attributes)
{
    attributes.initialReservedCaches = edp_writer_initial_reserved_caches;
    attributes.payloadMaxSize = mp_PDP->builtin_attributes().writerPayloadSize;
    attributes.memoryPolicy = mp_PDP->builtin_attributes().writerHistoryMemoryPolicy;
}

void EDPSimple::set_builtin_reader_attributes(
        ReaderAttributes& attributes)
{
    // Matched writers will depend on total number of participants
    attributes.matched_writers_allocation =
            mp_PDP->getRTPSParticipant()->getRTPSParticipantAttributes().allocation.participants;

    // As participants allocation policy includes the local participant, one has to be substracted
    if (attributes.matched_writers_allocation.initial > 1)
    {
        attributes.matched_writers_allocation.initial--;
    }
    if ((attributes.matched_writers_allocation.maximum > 1) &&
            (attributes.matched_writers_allocation.maximum < std::numeric_limits<size_t>::max()))
    {
        attributes.matched_writers_allocation.maximum--;
    }

    // Locators are copied from the local participant metatraffic locators
    attributes.endpoint.unicastLocatorList.clear();
    for (const Locator_t& loc : this->mp_PDP->getLocalParticipantProxyData()->metatraffic_locators.unicast)
    {
        attributes.endpoint.unicastLocatorList.push_back(loc);
    }
    attributes.endpoint.multicastLocatorList.clear();
    for (const Locator_t& loc : this->mp_PDP->getLocalParticipantProxyData()->metatraffic_locators.multicast)
    {
        attributes.endpoint.multicastLocatorList.push_back(loc);
    }

    // Timings are configured using EDP default values
    attributes.times.heartbeatResponseDelay = edp_heartbeat_response_delay;

    // EDP endpoints are always reliable, transsient local, keyed topics
    attributes.endpoint.reliabilityKind = RELIABLE;
    attributes.endpoint.durabilityKind = TRANSIENT_LOCAL;
    attributes.endpoint.topicKind = WITH_KEY;

    // Built-in EDP readers never expect inline qos
    attributes.expectsInlineQos = false;
}

void EDPSimple::set_builtin_writer_attributes(
        WriterAttributes& attributes)
{
    // Matched readers will depend on total number of participants
    attributes.matched_readers_allocation =
            mp_PDP->getRTPSParticipant()->getRTPSParticipantAttributes().allocation.participants;

    // As participants allocation policy includes the local participant, one has to be substracted
    if (attributes.matched_readers_allocation.initial > 1)
    {
        attributes.matched_readers_allocation.initial--;
    }
    if ((attributes.matched_readers_allocation.maximum > 1) &&
            (attributes.matched_readers_allocation.maximum < std::numeric_limits<size_t>::max()))
    {
        attributes.matched_readers_allocation.maximum--;
    }

    // Locators are copied from the local participant metatraffic locators
    attributes.endpoint.unicastLocatorList.clear();
    for (const Locator_t& loc : this->mp_PDP->getLocalParticipantProxyData()->metatraffic_locators.unicast)
    {
        attributes.endpoint.unicastLocatorList.push_back(loc);
    }
    attributes.endpoint.multicastLocatorList.clear();
    for (const Locator_t& loc : this->mp_PDP->getLocalParticipantProxyData()->metatraffic_locators.multicast)
    {
        attributes.endpoint.multicastLocatorList.push_back(loc);
    }

    // Timings are configured using EDP default values
    attributes.times.heartbeatPeriod = edp_heartbeat_period;
    attributes.times.nackResponseDelay = edp_nack_response_delay;
    attributes.times.nackSupressionDuration = edp_nack_supression_duration;

    // EDP endpoints are always reliable, transsient local, keyed topics
    attributes.endpoint.reliabilityKind = RELIABLE;
    attributes.endpoint.durabilityKind = TRANSIENT_LOCAL;
    attributes.endpoint.topicKind = WITH_KEY;
}

bool EDPSimple::createSEDPEndpoints()
{
    WriterAttributes watt;
    ReaderAttributes ratt;
    HistoryAttributes reader_history_att;
    HistoryAttributes writer_history_att;

    set_builtin_reader_history_attributes(reader_history_att);
    set_builtin_writer_history_attributes(writer_history_att);
    set_builtin_reader_attributes(ratt);
    set_builtin_writer_attributes(watt);

    publications_listener_ = new EDPSimplePUBListener(this);
    subscriptions_listener_ = new EDPSimpleSUBListener(this);

    if (m_discovery.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader)
    {
        if (!EDPUtils::create_edp_writer(mp_RTPSParticipant, "DCPSPublications", c_EntityId_SEDPPubWriter,
                writer_history_att, watt, publications_listener_, pub_writer_payload_pool_, publications_writer_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Publication Writer created");

        if (!EDPUtils::create_edp_reader(mp_RTPSParticipant, "DCPSSubscriptions", c_EntityId_SEDPSubReader,
                reader_history_att, ratt, subscriptions_listener_, sub_reader_payload_pool_, subscriptions_reader_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Subscription Reader created");
    }

    if (m_discovery.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter)
    {
        if (!EDPUtils::create_edp_reader(mp_RTPSParticipant, "DCPSPublications", c_EntityId_SEDPPubReader,
                reader_history_att, ratt, publications_listener_, pub_reader_payload_pool_, publications_reader_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Publication Reader created");

        if (!EDPUtils::create_edp_writer(mp_RTPSParticipant, "DCPSSubscriptions", c_EntityId_SEDPSubWriter,
                writer_history_att, watt, subscriptions_listener_, sub_writer_payload_pool_, subscriptions_writer_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Subscription Writer created");
    }

    logInfo(RTPS_EDP, "Creation finished");
    return true;
}

#if HAVE_SECURITY
bool EDPSimple::create_sedp_secure_endpoints()
{
    WriterAttributes watt;
    ReaderAttributes ratt;
    HistoryAttributes reader_history_att;
    HistoryAttributes writer_history_att;

    set_builtin_reader_history_attributes(reader_history_att);
    set_builtin_writer_history_attributes(writer_history_att);
    set_builtin_reader_attributes(ratt);
    set_builtin_writer_attributes(watt);

    const security::ParticipantSecurityAttributes& part_attr = mp_RTPSParticipant->security_attributes();

    ratt.endpoint.security_attributes().is_submessage_protected = part_attr.is_discovery_protected;
    ratt.endpoint.security_attributes().plugin_endpoint_attributes = PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;

    watt.endpoint.security_attributes().is_submessage_protected = part_attr.is_discovery_protected;
    watt.endpoint.security_attributes().plugin_endpoint_attributes = PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;

    if (part_attr.is_discovery_protected)
    {
        security::PluginParticipantSecurityAttributes plugin_part_attr(part_attr.plugin_participant_attributes);

        if (plugin_part_attr.is_discovery_encrypted)
        {
            ratt.endpoint.security_attributes().plugin_endpoint_attributes |=
                    PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
            watt.endpoint.security_attributes().plugin_endpoint_attributes |=
                    PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
        }
        if (plugin_part_attr.is_discovery_origin_authenticated)
        {
            ratt.endpoint.security_attributes().plugin_endpoint_attributes |=
                    PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
            watt.endpoint.security_attributes().plugin_endpoint_attributes |=
                    PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
        }
    }

    if (m_discovery.discovery_config.m_simpleEDP.enable_builtin_secure_publications_writer_and_subscriptions_reader)
    {
        if (!EDPUtils::create_edp_writer(mp_RTPSParticipant, "DCPSPublicationsSecure",
                sedp_builtin_publications_secure_writer, writer_history_att, watt, publications_listener_,
                sec_pub_writer_payload_pool_, publications_secure_writer_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Publication Writer created");

        if (!EDPUtils::create_edp_reader(mp_RTPSParticipant, "DCPSSubscriptionsSecure",
                sedp_builtin_subscriptions_secure_reader, reader_history_att, ratt, subscriptions_listener_,
                sec_sub_reader_payload_pool_, subscriptions_secure_reader_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Subscription Reader created");
    }

    if (m_discovery.discovery_config.m_simpleEDP.enable_builtin_secure_subscriptions_writer_and_publications_reader)
    {
        if (!EDPUtils::create_edp_reader(mp_RTPSParticipant, "DCPSPublicationsSecure",
                sedp_builtin_publications_secure_reader, reader_history_att, ratt, publications_listener_,
                sec_pub_reader_payload_pool_, publications_secure_reader_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Publication Reader created");

        if (!EDPUtils::create_edp_writer(mp_RTPSParticipant, "DCPSSubscriptionsSecure",
                sedp_builtin_subscriptions_secure_writer, writer_history_att, watt, subscriptions_listener_,
                sec_sub_writer_payload_pool_, subscriptions_secure_writer_))
        {
            return false;
        }

        logInfo(RTPS_EDP, "SEDP Subscription Writer created");
    }

    logInfo(RTPS_EDP, "SEDP Endpoints creation finished");
    return true;
}

#endif // if HAVE_SECURITY

bool EDPSimple::processLocalReaderProxyData(
        RTPSReader* local_reader,
        ReaderProxyData* rdata)
{
    logInfo(RTPS_EDP, rdata->guid().entityId);
    (void)local_reader;

    auto* writer = &subscriptions_writer_;

#if HAVE_SECURITY
    if (local_reader->getAttributes().security_attributes().is_discovery_protected)
    {
        writer = &subscriptions_secure_writer_;
    }
#endif // if HAVE_SECURITY
    CacheChange_t* change = nullptr;
    bool ret_val = serialize_reader_proxy_data(*rdata, *writer, true, &change);
    if (change != nullptr)
    {
        writer->second->add_change(change);
    }
    return ret_val;
}

bool EDPSimple::processLocalWriterProxyData(
        RTPSWriter* local_writer,
        WriterProxyData* wdata)
{
    logInfo(RTPS_EDP, wdata->guid().entityId);
    (void)local_writer;

    auto* writer = &publications_writer_;

#if HAVE_SECURITY
    if (local_writer->getAttributes().security_attributes().is_discovery_protected)
    {
        writer = &publications_secure_writer_;
    }
#endif // if HAVE_SECURITY

    CacheChange_t* change = nullptr;
    bool ret_val = serialize_writer_proxy_data(*wdata, *writer, true, &change);
    if (change != nullptr)
    {
        writer->second->add_change(change);
    }
    return ret_val;
}

bool EDPSimple::serialize_writer_proxy_data(
        const WriterProxyData& data,
        const t_p_StatefulWriter& writer,
        bool remove_same_instance,
        CacheChange_t** created_change)
{
    return serialize_proxy_data(data, writer, remove_same_instance, created_change);
}

bool EDPSimple::serialize_reader_proxy_data(
        const ReaderProxyData& data,
        const t_p_StatefulWriter& writer,
        bool remove_same_instance,
        CacheChange_t** created_change)
{
    return serialize_proxy_data(data, writer, remove_same_instance, created_change);
}

template<typename ProxyData>
bool EDPSimple::serialize_proxy_data(
        const ProxyData& data,
        const t_p_StatefulWriter& writer,
        bool remove_same_instance,
        CacheChange_t** created_change)
{
    assert(created_change != nullptr);
    *created_change = nullptr;

    if (writer.first != nullptr)
    {
        uint32_t cdr_size = data.get_serialized_size(true);
        CacheChange_t* change = writer.first->new_change(
            [cdr_size]() -> uint32_t
            {
                return cdr_size;
            },
            ALIVE, data.key());
        if (change != nullptr)
        {
            CDRMessage_t aux_msg(change->serializedPayload);

#if __BIG_ENDIAN__
            change->serializedPayload.encapsulation = (uint16_t)PL_CDR_BE;
            aux_msg.msg_endian = BIGEND;
#else
            change->serializedPayload.encapsulation = (uint16_t)PL_CDR_LE;
            aux_msg.msg_endian = LITTLEEND;
#endif // if __BIG_ENDIAN__

            data.writeToCDRMessage(&aux_msg, true);
            change->serializedPayload.length = (uint16_t)aux_msg.length;

            if (remove_same_instance)
            {
                std::unique_lock<RecursiveTimedMutex> lock(*writer.second->getMutex());
                for (auto ch = writer.second->changesBegin(); ch != writer.second->changesEnd(); ++ch)
                {
                    if ((*ch)->instanceHandle == change->instanceHandle)
                    {
                        writer.second->remove_change(*ch);
                        break;
                    }
                }
            }
            *created_change = change;
            return true;
        }
        return false;
    }
    return true;
}

bool EDPSimple::removeLocalWriter(
        RTPSWriter* W)
{
    logInfo(RTPS_EDP, W->getGuid().entityId);

    auto* writer = &publications_writer_;

#if HAVE_SECURITY
    if (W->getAttributes().security_attributes().is_discovery_protected)
    {
        writer = &publications_secure_writer_;
    }
#endif // if HAVE_SECURITY

    if (writer->first != nullptr)
    {
        InstanceHandle_t iH;
        iH = W->getGuid();
        CacheChange_t* change = writer->first->new_change(
            [this]() -> uint32_t
            {
                return mp_PDP->builtin_attributes().writerPayloadSize;
            },
            NOT_ALIVE_DISPOSED_UNREGISTERED, iH);
        if (change != nullptr)
        {
            {
                std::lock_guard<RecursiveTimedMutex> guard(*writer->second->getMutex());
                for (auto ch = writer->second->changesBegin(); ch != writer->second->changesEnd(); ++ch)
                {
                    if ((*ch)->instanceHandle == change->instanceHandle)
                    {
                        writer->second->remove_change(*ch);
                        break;
                    }
                }

            }

            writer->second->add_change(change);
        }
    }
    return mp_PDP->removeWriterProxyData(W->getGuid());
}

bool EDPSimple::removeLocalReader(
        RTPSReader* R)
{
    logInfo(RTPS_EDP, R->getGuid().entityId);

    auto* writer = &subscriptions_writer_;

#if HAVE_SECURITY
    if (R->getAttributes().security_attributes().is_discovery_protected)
    {
        writer = &subscriptions_secure_writer_;
    }
#endif // if HAVE_SECURITY

    if (writer->first != nullptr)
    {
        InstanceHandle_t iH;
        iH = (R->getGuid());
        CacheChange_t* change = writer->first->new_change(
            [this]() -> uint32_t
            {
                return mp_PDP->builtin_attributes().writerPayloadSize;
            },
            NOT_ALIVE_DISPOSED_UNREGISTERED, iH);
        if (change != nullptr)
        {
            {
                std::lock_guard<RecursiveTimedMutex> guard(*writer->second->getMutex());
                for (auto ch = writer->second->changesBegin(); ch != writer->second->changesEnd(); ++ch)
                {
                    if ((*ch)->instanceHandle == change->instanceHandle)
                    {
                        writer->second->remove_change(*ch);
                        break;
                    }
                }
            }

            writer->second->add_change(change);
        }
    }
    return mp_PDP->removeReaderProxyData(R->getGuid());
}

void EDPSimple::assignRemoteEndpoints(
        const ParticipantProxyData& pdata)
{
    logInfo(RTPS_EDP, "New DPD received, adding remote endpoints to our SimpleEDP endpoints");
    const NetworkFactory& network = mp_RTPSParticipant->network_factory();
    uint32_t endp = pdata.m_availableBuiltinEndpoints;
    uint32_t auxendp = endp;
    bool use_multicast_locators = !mp_PDP->getRTPSParticipant()->getAttributes().builtin.avoid_builtin_multicast ||
            pdata.metatraffic_locators.unicast.empty();
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;

    std::lock_guard<std::mutex> data_guard(temp_data_lock_);

    temp_reader_proxy_data_.clear();
    temp_reader_proxy_data_.m_expectsInlineQos = false;
    temp_reader_proxy_data_.guid().guidPrefix = pdata.m_guid.guidPrefix;
    temp_reader_proxy_data_.set_remote_locators(pdata.metatraffic_locators, network, use_multicast_locators);
    temp_reader_proxy_data_.m_qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    temp_reader_proxy_data_.m_qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;

    temp_writer_proxy_data_.clear();
    temp_writer_proxy_data_.guid().guidPrefix = pdata.m_guid.guidPrefix;
    temp_writer_proxy_data_.persistence_guid(pdata.get_persistence_guid());
    temp_writer_proxy_data_.set_remote_locators(pdata.metatraffic_locators, network, use_multicast_locators);
    temp_writer_proxy_data_.m_qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    temp_writer_proxy_data_.m_qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;

    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_reader_.first != nullptr) //Exist Pub Writer and i have pub reader
    {
        logInfo(RTPS_EDP, "Adding SEDP Pub Writer to my Pub Reader");
        temp_writer_proxy_data_.guid().entityId = c_EntityId_SEDPPubWriter;
        temp_writer_proxy_data_.set_persistence_entity_id(c_EntityId_SEDPPubWriter);
        publications_reader_.first->matched_writer_add(temp_writer_proxy_data_);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_writer_.first != nullptr) //Exist Pub Detector
    {
        logInfo(RTPS_EDP, "Adding SEDP Pub Reader to my Pub Writer");
        temp_reader_proxy_data_.guid().entityId = c_EntityId_SEDPPubReader;
        publications_writer_.first->matched_reader_add(temp_reader_proxy_data_);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_reader_.first != nullptr) //Exist Pub Announcer
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Writer to my Sub Reader");
        temp_writer_proxy_data_.guid().entityId = c_EntityId_SEDPSubWriter;
        temp_writer_proxy_data_.set_persistence_entity_id(c_EntityId_SEDPSubWriter);
        subscriptions_reader_.first->matched_writer_add(temp_writer_proxy_data_);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_writer_.first != nullptr) //Exist Pub Announcer
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Reader to my Sub Writer");
        temp_reader_proxy_data_.guid().entityId = c_EntityId_SEDPSubReader;
        subscriptions_writer_.first->matched_reader_add(temp_reader_proxy_data_);
    }

#if HAVE_SECURITY
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_secure_reader_.first != nullptr)
    {
        temp_writer_proxy_data_.guid().entityId = sedp_builtin_publications_secure_writer;
        temp_writer_proxy_data_.set_persistence_entity_id(sedp_builtin_publications_secure_writer);

        if (!mp_RTPSParticipant->security_manager().discovered_builtin_writer(
                    publications_secure_reader_.first->getGuid(), pdata.m_guid, temp_writer_proxy_data_,
                    publications_secure_reader_.first->getAttributes().security_attributes()))
        {
            logError(RTPS_EDP, "Security manager returns an error for writer " <<
                    publications_secure_reader_.first->getGuid());
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_secure_writer_.first != nullptr)
    {
        temp_reader_proxy_data_.guid().entityId = sedp_builtin_publications_secure_reader;
        if (!mp_RTPSParticipant->security_manager().discovered_builtin_reader(
                    publications_secure_writer_.first->getGuid(), pdata.m_guid, temp_reader_proxy_data_,
                    publications_secure_writer_.first->getAttributes().security_attributes()))
        {
            logError(RTPS_EDP, "Security manager returns an error for writer " <<
                    publications_secure_writer_.first->getGuid());
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_secure_reader_.first != nullptr)
    {
        temp_writer_proxy_data_.guid().entityId = sedp_builtin_subscriptions_secure_writer;
        temp_writer_proxy_data_.set_persistence_entity_id(sedp_builtin_subscriptions_secure_writer);

        if (!mp_RTPSParticipant->security_manager().discovered_builtin_writer(
                    subscriptions_secure_reader_.first->getGuid(), pdata.m_guid, temp_writer_proxy_data_,
                    subscriptions_secure_reader_.first->getAttributes().security_attributes()))
        {
            logError(RTPS_EDP, "Security manager returns an error for writer " <<
                    subscriptions_secure_reader_.first->getGuid());
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_secure_writer_.first != nullptr)
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Reader to my Sub Writer");
        temp_reader_proxy_data_.guid().entityId = sedp_builtin_subscriptions_secure_reader;
        if (!mp_RTPSParticipant->security_manager().discovered_builtin_reader(
                    subscriptions_secure_writer_.first->getGuid(), pdata.m_guid, temp_reader_proxy_data_,
                    subscriptions_secure_writer_.first->getAttributes().security_attributes()))
        {
            logError(RTPS_EDP, "Security manager returns an error for writer " <<
                    subscriptions_secure_writer_.first->getGuid());
        }
    }
#endif // if HAVE_SECURITY
}

void EDPSimple::removeRemoteEndpoints(
        ParticipantProxyData* pdata)
{
    logInfo(RTPS_EDP, "For RTPSParticipant: " << pdata->m_guid);

    GUID_t tmp_guid;
    tmp_guid.guidPrefix = pdata->m_guid.guidPrefix;

    uint32_t endp = pdata->m_availableBuiltinEndpoints;
    uint32_t auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_reader_.first != nullptr) //Exist Pub Writer and i have pub reader
    {
        tmp_guid.entityId = c_EntityId_SEDPPubWriter;
        publications_reader_.first->matched_writer_remove(tmp_guid);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_writer_.first != nullptr) //Exist Pub Detector
    {
        tmp_guid.entityId = c_EntityId_SEDPPubReader;
        publications_writer_.first->matched_reader_remove(tmp_guid);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_reader_.first != nullptr) //Exist Pub Announcer
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Writer to my Sub Reader");
        tmp_guid.entityId = c_EntityId_SEDPSubWriter;
        subscriptions_reader_.first->matched_writer_remove(tmp_guid);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_writer_.first != nullptr) //Exist Pub Announcer
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Reader to my Sub Writer");
        tmp_guid.entityId = c_EntityId_SEDPSubReader;
        subscriptions_writer_.first->matched_reader_remove(tmp_guid);
    }

#if HAVE_SECURITY
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_secure_reader_.first != nullptr)
    {
        tmp_guid.entityId = sedp_builtin_publications_secure_writer;
        if (publications_secure_reader_.first->matched_writer_remove(tmp_guid))
        {
            mp_RTPSParticipant->security_manager().remove_writer(
                publications_secure_reader_.first->getGuid(), pdata->m_guid, tmp_guid);
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && publications_secure_writer_.first != nullptr)
    {
        tmp_guid.entityId = sedp_builtin_publications_secure_reader;
        if (publications_secure_writer_.first->matched_reader_remove(tmp_guid))
        {
            mp_RTPSParticipant->security_manager().remove_reader(
                publications_secure_writer_.first->getGuid(), pdata->m_guid, tmp_guid);
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_ANNOUNCER;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_secure_reader_.first != nullptr)
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Writer to my Sub Reader");
        tmp_guid.entityId = sedp_builtin_subscriptions_secure_writer;
        if (subscriptions_secure_reader_.first->matched_writer_remove(tmp_guid))
        {
            mp_RTPSParticipant->security_manager().remove_writer(
                subscriptions_secure_reader_.first->getGuid(), pdata->m_guid, tmp_guid);
        }
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_DETECTOR;
    //FIXME: FIX TO NOT FAIL WITH BAD BUILTIN ENDPOINT SET
    //auxendp = 1;
    if (auxendp != 0 && subscriptions_secure_writer_.first != nullptr)
    {
        logInfo(RTPS_EDP, "Adding SEDP Sub Reader to my Sub Writer");
        tmp_guid.entityId = sedp_builtin_subscriptions_secure_reader;
        if (subscriptions_secure_writer_.first->matched_reader_remove(tmp_guid))
        {
            mp_RTPSParticipant->security_manager().remove_reader(
                subscriptions_secure_writer_.first->getGuid(), pdata->m_guid, tmp_guid);
        }
    }
#endif // if HAVE_SECURITY
}

bool EDPSimple::areRemoteEndpointsMatched(
        const ParticipantProxyData* pdata)
{
    uint32_t endp = pdata->m_availableBuiltinEndpoints;

    uint32_t auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
    if (auxendp != 0 && publications_reader_.first != nullptr) //Exist Pub Writer and I have Pub Reader
    {
        GUID_t wguid;
        wguid.guidPrefix = pdata->m_guid.guidPrefix;
        wguid.entityId = c_EntityId_SEDPPubWriter;

        if (!publications_reader_.first->matched_writer_is_matched(wguid))
        {
            return false;
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
    if (auxendp != 0 && publications_writer_.first != nullptr) //Exist Pub Detector
    {
        GUID_t rguid;
        rguid.guidPrefix = pdata->m_guid.guidPrefix;
        rguid.entityId = c_EntityId_SEDPPubReader;

        if (!publications_writer_.first->matched_reader_is_matched(rguid))
        {
            return false;
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
    if (auxendp != 0 && subscriptions_reader_.first != nullptr) //Exist Pub Announcer
    {
        GUID_t wguid;
        wguid.guidPrefix = pdata->m_guid.guidPrefix;
        wguid.entityId = c_EntityId_SEDPSubWriter;

        if (!subscriptions_reader_.first->matched_writer_is_matched(wguid))
        {
            return false;
        }
    }

    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
    if (auxendp != 0 && subscriptions_writer_.first != nullptr) //Exist Pub Announcer
    {
        GUID_t rguid;
        rguid.guidPrefix = pdata->m_guid.guidPrefix;
        rguid.entityId = c_EntityId_SEDPSubReader;

        if (!subscriptions_writer_.first->matched_reader_is_matched(rguid))
        {
            return false;
        }
    }

    return true;
}

#if HAVE_SECURITY
bool EDPSimple::pairing_remote_writer_with_local_builtin_reader_after_security(
        const GUID_t& local_reader,
        const WriterProxyData& remote_writer_data)
{
    bool returned_value = false;

    if (local_reader.entityId == sedp_builtin_publications_secure_reader)
    {
        publications_secure_reader_.first->matched_writer_add(remote_writer_data);
        returned_value = true;
    }
    else if (local_reader.entityId == sedp_builtin_subscriptions_secure_reader)
    {
        subscriptions_secure_reader_.first->matched_writer_add(remote_writer_data);
        returned_value = true;
    }

    return returned_value;
}

bool EDPSimple::pairing_remote_reader_with_local_builtin_writer_after_security(
        const GUID_t& local_writer,
        const ReaderProxyData& remote_reader_data)
{
    bool returned_value = false;

    if (local_writer.entityId == sedp_builtin_publications_secure_writer)
    {
        publications_secure_writer_.first->matched_reader_add(remote_reader_data);
        returned_value = true;
    }
    else if (local_writer.entityId == sedp_builtin_subscriptions_secure_writer)
    {
        subscriptions_secure_writer_.first->matched_reader_add(remote_reader_data);
        returned_value = true;
    }

    return returned_value;
}

#endif // if HAVE_SECURITY

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
