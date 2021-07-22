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
 * @file PDPSimple.cpp
 *
 */

#include <fastdds/rtps/builtin/discovery/participant/PDPSimple.h>
#include <fastdds/rtps/builtin/discovery/participant/PDPListener.h>
#include <fastdds/rtps/builtin/discovery/endpoint/EDPSimple.h>
#include <fastdds/rtps/builtin/discovery/endpoint/EDPStatic.h>
#include <fastdds/rtps/resources/TimedEvent.h>
#include <fastdds/rtps/builtin/BuiltinProtocols.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>
#include <fastdds/rtps/builtin/data/ParticipantProxyData.h>
#include <fastdds/rtps/builtin/data/ReaderProxyData.h>
#include <fastdds/rtps/builtin/data/WriterProxyData.h>

#include <fastdds/rtps/participant/RTPSParticipantListener.h>
#include <fastdds/rtps/writer/StatelessWriter.h>

#include <fastdds/rtps/reader/StatelessReader.h>
#include <fastdds/rtps/reader/StatefulReader.h>

#include <fastdds/rtps/history/WriterHistory.h>
#include <fastdds/rtps/history/ReaderHistory.h>

#include <fastdds/dds/builtin/typelookup/TypeLookupManager.hpp>

#include <fastrtps/utils/TimeConversion.h>
#include <fastrtps/utils/IPLocator.h>

#include <rtps/history/TopicPayloadPoolRegistry.hpp>
#include <rtps/participant/RTPSParticipantImpl.h>

#include <fastdds/dds/log/Log.hpp>

#include <mutex>

using namespace eprosima::fastrtps;

namespace eprosima {
namespace fastrtps {
namespace rtps {


PDPSimple::PDPSimple (
        BuiltinProtocols* built,
        const RTPSParticipantAllocationAttributes& allocation)
    : PDP(built, allocation)
{
}

PDPSimple::~PDPSimple()
{
}

void PDPSimple::initializeParticipantProxyData(
        ParticipantProxyData* participant_data)
{
    PDP::initializeParticipantProxyData(participant_data);

    if (getRTPSParticipant()->getAttributes().builtin.discovery_config.
                    use_SIMPLE_EndpointDiscoveryProtocol)
    {
        if (getRTPSParticipant()->getAttributes().builtin.discovery_config.m_simpleEDP.
                        use_PublicationWriterANDSubscriptionReader)
        {
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
        }

        if (getRTPSParticipant()->getAttributes().builtin.discovery_config.m_simpleEDP.
                        use_PublicationReaderANDSubscriptionWriter)
        {
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
        }

#if HAVE_SECURITY
        if (getRTPSParticipant()->getAttributes().builtin.discovery_config.m_simpleEDP.
                        enable_builtin_secure_publications_writer_and_subscriptions_reader)
        {
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_ANNOUNCER;
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_DETECTOR;
        }

        if (getRTPSParticipant()->getAttributes().builtin.discovery_config.m_simpleEDP.
                        enable_builtin_secure_subscriptions_writer_and_publications_reader)
        {
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_ANNOUNCER;
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_DETECTOR;
        }
#endif // if HAVE_SECURITY
    }
    else if (!getRTPSParticipant()->getAttributes().builtin.discovery_config.
                    use_STATIC_EndpointDiscoveryProtocol)
    {
        logError(RTPS_PDP, "Neither EDP simple nor EDP static enabled. Endpoints will not be discovered.");
    }
}

bool PDPSimple::init(
        RTPSParticipantImpl* part)
{
    // The DATA(p) must be processed after EDP endpoint creation
    if (!PDP::initPDP(part))
    {
        return false;
    }

    //INIT EDP
    if (m_discovery.discovery_config.use_STATIC_EndpointDiscoveryProtocol)
    {
        mp_EDP = new EDPStatic(this, mp_RTPSParticipant);
        if (!mp_EDP->initEDP(m_discovery))
        {
            logError(RTPS_PDP, "Endpoint discovery configuration failed");
            return false;
        }
    }
    else if (m_discovery.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol)
    {
        mp_EDP = new EDPSimple(this, mp_RTPSParticipant);
        if (!mp_EDP->initEDP(m_discovery))
        {
            logError(RTPS_PDP, "Endpoint discovery configuration failed");
            return false;
        }
    }
    else
    {
        logWarning(RTPS_PDP, "No EndpointDiscoveryProtocol defined");
        return false;
    }

    return true;
}

ParticipantProxyData* PDPSimple::createParticipantProxyData(
        const ParticipantProxyData& participant_data,
        const GUID_t&)
{
    std::unique_lock<std::recursive_mutex> lock(*getMutex());

    // decide if we dismiss the participant using the ParticipantFilteringFlags
    const ParticipantFilteringFlags_t& flags = m_discovery.discovery_config.ignoreParticipantFlags;

    if (flags != ParticipantFilteringFlags_t::NO_FILTER)
    {
        const GUID_t& remote = participant_data.m_guid;
        const GUID_t& local = getLocalParticipantProxyData()->m_guid;

        if (!local.is_on_same_host_as(remote))
        {
            if (flags & ParticipantFilteringFlags::FILTER_DIFFERENT_HOST)
            {
                return nullptr;
            }
        }
        else
        {
            bool filter_same = (flags& ParticipantFilteringFlags::FILTER_SAME_PROCESS) != 0;
            bool filter_different = (flags& ParticipantFilteringFlags::FILTER_DIFFERENT_PROCESS) != 0;

            if (filter_same && filter_different)
            {
                return nullptr;
            }

            bool is_same = local.is_on_same_process_as(remote);

            if ((filter_same && is_same) || (filter_different && !is_same))
            {
                return nullptr;
            }
        }
    }

    ParticipantProxyData* pdata = add_participant_proxy_data(participant_data.m_guid, true);
    if (pdata != nullptr)
    {
        pdata->copy(participant_data);
        pdata->isAlive = true;
        pdata->lease_duration_event->update_interval(pdata->m_leaseDuration);
        pdata->lease_duration_event->restart_timer();
    }

    return pdata;
}

// EDPStatic requires matching on ParticipantProxyData property updates
bool PDPSimple::updateInfoMatchesEDP()
{
    return dynamic_cast<EDPStatic*>(mp_EDP) != nullptr;
}

void PDPSimple::announceParticipantState(
        bool new_change,
        bool dispose,
        WriteParams& wp)
{
    PDP::announceParticipantState(new_change, dispose, wp);

    if (!(dispose || new_change))
    {
        StatelessWriter* pW = dynamic_cast<StatelessWriter*>(mp_PDPWriter);

        if (pW != nullptr)
        {
            pW->unsent_changes_reset();
        }
        else
        {
            logError(RTPS_PDP, "Using PDPSimple protocol with a reliable writer");
        }
    }
}

bool PDPSimple::createPDPEndpoints()
{
    logInfo(RTPS_PDP, "Beginning");

    const RTPSParticipantAllocationAttributes& allocation =
            mp_RTPSParticipant->getRTPSParticipantAttributes().allocation;

    //SPDP BUILTIN RTPSParticipant READER
    HistoryAttributes hatt;
    hatt.payloadMaxSize = mp_builtin->m_att.readerPayloadSize;
    hatt.memoryPolicy = mp_builtin->m_att.readerHistoryMemoryPolicy;
    hatt.initialReservedCaches = 25;
    if (allocation.participants.initial > 0)
    {
        hatt.initialReservedCaches = (int32_t)allocation.participants.initial;
    }
    if (allocation.participants.maximum < std::numeric_limits<size_t>::max())
    {
        hatt.maximumReservedCaches = (int32_t)allocation.participants.maximum;
    }

    PoolConfig reader_pool_cfg = PoolConfig::from_history_attributes(hatt);
    reader_payload_pool_ = TopicPayloadPoolRegistry::get("DCPSParticipant", reader_pool_cfg);
    reader_payload_pool_->reserve_history(reader_pool_cfg, true);

    mp_PDPReaderHistory = new ReaderHistory(hatt);
    ReaderAttributes ratt;
    ratt.endpoint.multicastLocatorList = mp_builtin->m_metatrafficMulticastLocatorList;
    ratt.endpoint.unicastLocatorList = mp_builtin->m_metatrafficUnicastLocatorList;
    ratt.endpoint.topicKind = WITH_KEY;
    ratt.endpoint.durabilityKind = TRANSIENT_LOCAL;
    ratt.endpoint.reliabilityKind = BEST_EFFORT;
    ratt.matched_writers_allocation = allocation.participants;
    mp_listener = new PDPListener(this);
    if (mp_RTPSParticipant->createReader(&mp_PDPReader, ratt, reader_payload_pool_, mp_PDPReaderHistory, mp_listener,
            c_EntityId_SPDPReader, true, false))
    {
#if HAVE_SECURITY
        mp_RTPSParticipant->set_endpoint_rtps_protection_supports(mp_PDPReader, false);
#endif // if HAVE_SECURITY
    }
    else
    {
        logError(RTPS_PDP, "SimplePDP Reader creation failed");
        delete mp_PDPReaderHistory;
        mp_PDPReaderHistory = nullptr;
        delete mp_listener;
        mp_listener = nullptr;
        reader_payload_pool_->release_history(reader_pool_cfg, true);
        reader_payload_pool_.reset();
        return false;
    }

    //SPDP BUILTIN RTPSParticipant WRITER
    hatt.payloadMaxSize = mp_builtin->m_att.writerPayloadSize;
    hatt.initialReservedCaches = 1;
    hatt.maximumReservedCaches = 1;
    hatt.memoryPolicy = mp_builtin->m_att.writerHistoryMemoryPolicy;

    PoolConfig writer_pool_cfg = PoolConfig::from_history_attributes(hatt);
    writer_payload_pool_ = TopicPayloadPoolRegistry::get("DCPSParticipant", writer_pool_cfg);
    writer_payload_pool_->reserve_history(writer_pool_cfg, false);

    mp_PDPWriterHistory = new WriterHistory(hatt);
    WriterAttributes watt;
    watt.endpoint.endpointKind = WRITER;
    watt.endpoint.durabilityKind = TRANSIENT_LOCAL;
    watt.endpoint.reliabilityKind = BEST_EFFORT;
    watt.endpoint.topicKind = WITH_KEY;
    watt.endpoint.remoteLocatorList = m_discovery.initialPeersList;
    watt.matched_readers_allocation = allocation.participants;

    if (mp_RTPSParticipant->getRTPSParticipantAttributes().throughputController.bytesPerPeriod != UINT32_MAX &&
            mp_RTPSParticipant->getRTPSParticipantAttributes().throughputController.periodMillisecs != 0)
    {
        watt.mode = ASYNCHRONOUS_WRITER;
    }

    RTPSWriter* wout;
    if (mp_RTPSParticipant->createWriter(&wout, watt, writer_payload_pool_, mp_PDPWriterHistory, nullptr,
            c_EntityId_SPDPWriter, true))
    {
#if HAVE_SECURITY
        mp_RTPSParticipant->set_endpoint_rtps_protection_supports(wout, false);
#endif // if HAVE_SECURITY
        mp_PDPWriter = wout;
        if (mp_PDPWriter != nullptr)
        {
            const NetworkFactory& network = mp_RTPSParticipant->network_factory();
            LocatorList_t fixed_locators;
            Locator_t local_locator;
            for (const Locator_t& loc : mp_builtin->m_initialPeersList)
            {
                if (network.transform_remote_locator(loc, local_locator))
                {
                    fixed_locators.push_back(local_locator);
                }
            }
            dynamic_cast<StatelessWriter*>(wout)->set_fixed_locators(fixed_locators);
        }
    }
    else
    {
        logError(RTPS_PDP, "SimplePDP Writer creation failed");
        delete mp_PDPWriterHistory;
        mp_PDPWriterHistory = nullptr;
        writer_payload_pool_->release_history(writer_pool_cfg, false);
        writer_payload_pool_.reset();
        return false;
    }
    logInfo(RTPS_PDP, "SPDP Endpoints creation finished");
    return true;
}

void PDPSimple::assignRemoteEndpoints(
        ParticipantProxyData* pdata)
{
    logInfo(RTPS_PDP, "For RTPSParticipant: " << pdata->m_guid.guidPrefix);

    const NetworkFactory& network = mp_RTPSParticipant->network_factory();
    uint32_t endp = pdata->m_availableBuiltinEndpoints;
    uint32_t auxendp = endp;
    bool use_multicast_locators = !mp_RTPSParticipant->getAttributes().builtin.avoid_builtin_multicast ||
            pdata->metatraffic_locators.unicast.empty();
    auxendp &= DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER;
    if (auxendp != 0)
    {
        std::lock_guard<std::mutex> data_guard(temp_data_lock_);
        temp_writer_data_.clear();
        temp_writer_data_.guid().guidPrefix = pdata->m_guid.guidPrefix;
        temp_writer_data_.guid().entityId = c_EntityId_SPDPWriter;
        temp_writer_data_.persistence_guid(pdata->get_persistence_guid());
        temp_writer_data_.set_persistence_entity_id(c_EntityId_SPDPWriter);
        temp_writer_data_.set_remote_locators(pdata->metatraffic_locators, network, use_multicast_locators);
        temp_writer_data_.m_qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
        temp_writer_data_.m_qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        mp_PDPReader->matched_writer_add(temp_writer_data_);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;
    if (auxendp != 0)
    {
        std::lock_guard<std::mutex> data_guard(temp_data_lock_);
        temp_reader_data_.clear();
        temp_reader_data_.m_expectsInlineQos = false;
        temp_reader_data_.guid().guidPrefix = pdata->m_guid.guidPrefix;
        temp_reader_data_.guid().entityId = c_EntityId_SPDPReader;
        temp_reader_data_.set_remote_locators(pdata->metatraffic_locators, network, use_multicast_locators);
        temp_reader_data_.m_qos.m_reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        temp_reader_data_.m_qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        mp_PDPWriter->matched_reader_add(temp_reader_data_);

        StatelessWriter* pW = dynamic_cast<StatelessWriter*>(mp_PDPWriter);

        if (pW != nullptr)
        {
            pW->unsent_changes_reset();
        }
        else
        {
            logError(RTPS_PDP, "Using PDPSimple protocol with a reliable writer");
        }
    }

#if HAVE_SECURITY
    // Validate remote participant
    mp_RTPSParticipant->security_manager().discovered_participant(*pdata);
#else
    //Inform EDP of new RTPSParticipant data:
    notifyAboveRemoteEndpoints(*pdata);
#endif // if HAVE_SECURITY
}

void PDPSimple::removeRemoteEndpoints(
        ParticipantProxyData* pdata)
{
    logInfo(RTPS_PDP, "For RTPSParticipant: " << pdata->m_guid);
    uint32_t endp = pdata->m_availableBuiltinEndpoints;
    uint32_t auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER;
    if (auxendp != 0)
    {
        GUID_t writer_guid(pdata->m_guid.guidPrefix, c_EntityId_SPDPWriter);
        mp_PDPReader->matched_writer_remove(writer_guid);
    }
    auxendp = endp;
    auxendp &= DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;
    if (auxendp != 0)
    {
        GUID_t reader_guid(pdata->m_guid.guidPrefix, c_EntityId_SPDPReader);
        mp_PDPWriter->matched_reader_remove(reader_guid);
    }
}

void PDPSimple::notifyAboveRemoteEndpoints(
        const ParticipantProxyData& pdata)
{
    //Inform EDP of new RTPSParticipant data:
    if (mp_EDP != nullptr)
    {
        mp_EDP->assignRemoteEndpoints(pdata);
    }

    if (mp_builtin->mp_WLP != nullptr)
    {
        mp_builtin->mp_WLP->assignRemoteEndpoints(pdata);
    }

    if (mp_builtin->tlm_ != nullptr)
    {
        mp_builtin->tlm_->assign_remote_endpoints(pdata);
    }
}

bool PDPSimple::newRemoteEndpointStaticallyDiscovered(
        const GUID_t& pguid,
        int16_t userDefinedId,
        EndpointKind_t kind)
{
    string_255 pname;
    if (lookup_participant_name(pguid, pname))
    {
        if (kind == WRITER)
        {
            dynamic_cast<EDPStatic*>(mp_EDP)->newRemoteWriter(pguid, pname, userDefinedId);
        }
        else
        {
            dynamic_cast<EDPStatic*>(mp_EDP)->newRemoteReader(pguid, pname, userDefinedId);
        }
    }
    return false;
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
