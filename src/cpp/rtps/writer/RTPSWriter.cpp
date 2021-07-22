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

/*
 * @file RTPSWriter.cpp
 *
 */

#include <mutex>

#include <rtps/history/BasicPayloadPool.hpp>
#include <rtps/history/CacheChangePool.h>

#include <rtps/DataSharing/DataSharingNotifier.hpp>
#include <rtps/DataSharing/WriterPool.hpp>

#include <rtps/participant/RTPSParticipantImpl.h>

#include <fastdds/dds/log/Log.hpp>

#include <fastdds/rtps/writer/RTPSWriter.h>

#include <fastdds/rtps/history/WriterHistory.h>

#include <fastdds/rtps/messages/RTPSMessageCreator.h>

#include <statistics/rtps/StatisticsBase.hpp>
#include <statistics/rtps/messages/RTPSStatisticsMessages.hpp>

#include "../flowcontrol/FlowController.hpp"

namespace eprosima {
namespace fastrtps {
namespace rtps {

RTPSWriter::RTPSWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& att,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen)
    : Endpoint(impl, guid, att.endpoint)
    , flow_controller_(flow_controller)
    , mp_history(hist)
    , mp_listener(listen)
    , is_async_(att.mode == SYNCHRONOUS_WRITER ? false : true)
    , liveliness_kind_(att.liveliness_kind)
    , liveliness_lease_duration_(att.liveliness_lease_duration)
    , liveliness_announcement_period_(att.liveliness_announcement_period)
{
    PoolConfig cfg = PoolConfig::from_history_attributes(hist->m_att);
    std::shared_ptr<IChangePool> change_pool;
    std::shared_ptr<IPayloadPool> payload_pool;
    payload_pool = BasicPayloadPool::get(cfg, change_pool);

    init(payload_pool, change_pool, att);
}

RTPSWriter::RTPSWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen)
    : RTPSWriter(
        impl, guid, att, payload_pool,
        std::make_shared<CacheChangePool>(PoolConfig::from_history_attributes(hist->m_att)),
        flow_controller, hist, listen)
{
}

RTPSWriter::RTPSWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen)
    : Endpoint(impl, guid, att.endpoint)
    , flow_controller_(flow_controller)
    , mp_history(hist)
    , mp_listener(listen)
    , is_async_(att.mode == SYNCHRONOUS_WRITER ? false : true)
    , liveliness_kind_(att.liveliness_kind)
    , liveliness_lease_duration_(att.liveliness_lease_duration)
    , liveliness_announcement_period_(att.liveliness_announcement_period)
{
    init(payload_pool, change_pool, att);
}

void RTPSWriter::init(
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        const WriterAttributes& att)
{
    payload_pool_ = payload_pool;
    change_pool_ = change_pool;
    fixed_payload_size_ = 0;
    if (mp_history->m_att.memoryPolicy == PREALLOCATED_MEMORY_MODE)
    {
        fixed_payload_size_ = mp_history->m_att.payloadMaxSize;
    }

    if (att.endpoint.data_sharing_configuration().kind() != OFF)
    {
        std::shared_ptr<WriterPool> pool = std::dynamic_pointer_cast<WriterPool>(payload_pool);
        if (!pool || !pool->init_shared_memory(this, att.endpoint.data_sharing_configuration().shm_directory()))
        {
            logError(RTPS_WRITER, "Could not initialize DataSharing writer pool");
        }
    }

    mp_history->mp_writer = this;
    mp_history->mp_mutex = &mp_mutex;

    flow_controller_->register_writer(this);

    logInfo(RTPS_WRITER, "RTPSWriter created");
}

RTPSWriter::~RTPSWriter()
{
    logInfo(RTPS_WRITER, "RTPSWriter destructor");

    // Deletion of the events has to be made in child destructor.
    // Also at this point all CacheChange_t must have been released by the child destructor

    mp_history->mp_writer = nullptr;
    mp_history->mp_mutex = nullptr;
}

void RTPSWriter::deinit()
{
    // First, unregister changes from FlowController. This action must be protected.
    {
        std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
        for (auto it = mp_history->changesBegin(); it != mp_history->changesEnd(); ++it)
        {
            flow_controller_->remove_change(*it);
        }
    }
    for (auto it = mp_history->changesBegin(); it != mp_history->changesEnd(); ++it)
    {
        release_change(*it);
    }

    mp_history->m_changes.clear();
    flow_controller_->unregister_writer(this);
}

CacheChange_t* RTPSWriter::new_change(
        const std::function<uint32_t()>& dataCdrSerializedSize,
        ChangeKind_t changeKind,
        InstanceHandle_t handle)
{
    logInfo(RTPS_WRITER, "Creating new change");

    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    CacheChange_t* reserved_change = nullptr;
    if (!change_pool_->reserve_cache(reserved_change))
    {
        logWarning(RTPS_WRITER, "Problem reserving cache from pool");
        return nullptr;
    }

    uint32_t payload_size = fixed_payload_size_ ? fixed_payload_size_ : dataCdrSerializedSize();
    if (!payload_pool_->get_payload(payload_size, *reserved_change))
    {
        change_pool_->release_cache(reserved_change);
        logWarning(RTPS_WRITER, "Problem reserving payload from pool");
        return nullptr;
    }

    reserved_change->kind = changeKind;
    if (m_att.topicKind == WITH_KEY && !handle.isDefined())
    {
        logWarning(RTPS_WRITER, "Changes in KEYED Writers need a valid instanceHandle");
    }
    reserved_change->instanceHandle = handle;
    reserved_change->writerGUID = m_guid;
    reserved_change->writer_info.previous = nullptr;
    reserved_change->writer_info.next = nullptr;
    reserved_change->writer_info.num_sent_submessages = 0;
    return reserved_change;
}

CacheChange_t* RTPSWriter::new_change(
        ChangeKind_t changeKind,
        InstanceHandle_t handle)
{
    logInfo(RTPS_WRITER, "Creating new change");

    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    CacheChange_t* reserved_change = nullptr;
    if (!change_pool_->reserve_cache(reserved_change))
    {
        logWarning(RTPS_WRITER, "Problem reserving cache from pool");
        return nullptr;
    }

    reserved_change->kind = changeKind;
    if (m_att.topicKind == WITH_KEY && !handle.isDefined())
    {
        logWarning(RTPS_WRITER, "Changes in KEYED Writers need a valid instanceHandle");
    }
    reserved_change->instanceHandle = handle;
    reserved_change->writerGUID = m_guid;
    reserved_change->writer_info.previous = nullptr;
    reserved_change->writer_info.next = nullptr;
    reserved_change->writer_info.num_sent_submessages = 0;
    return reserved_change;
}

bool RTPSWriter::release_change(
        CacheChange_t* change)
{
    // Asserting preconditions
    assert(change != nullptr);
    assert(change->writerGUID == m_guid);

    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    IPayloadPool* pool = change->payload_owner();
    if (pool)
    {
        pool->release_payload(*change);
    }
    return change_pool_->release_cache(change);
}

SequenceNumber_t RTPSWriter::get_seq_num_min()
{
    CacheChange_t* change;
    if (mp_history->get_min_change(&change) && change != nullptr)
    {
        return change->sequenceNumber;
    }
    else
    {
        return c_SequenceNumber_Unknown;
    }
}

SequenceNumber_t RTPSWriter::get_seq_num_max()
{
    CacheChange_t* change;
    if (mp_history->get_max_change(&change) && change != nullptr)
    {
        return change->sequenceNumber;
    }
    else
    {
        return c_SequenceNumber_Unknown;
    }
}

uint32_t RTPSWriter::getTypeMaxSerialized()
{
    return mp_history->getTypeMaxSerialized();
}

bool RTPSWriter::remove_older_changes(
        unsigned int max)
{
    logInfo(RTPS_WRITER, "Starting process clean_history for writer " << getGuid());
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    bool limit = (max != 0);

    bool remove_ret = mp_history->remove_min_change();
    bool at_least_one = remove_ret;
    unsigned int count = 1;

    while (remove_ret && (!limit || count < max))
    {
        remove_ret = mp_history->remove_min_change();
        ++count;
    }

    return at_least_one;
}

CONSTEXPR uint32_t info_dst_message_length = 16;
CONSTEXPR uint32_t info_ts_message_length = 12;
CONSTEXPR uint32_t data_frag_submessage_header_length = 36;
CONSTEXPR uint32_t heartbeat_message_length = 32;

uint32_t RTPSWriter::getMaxDataSize()
{
    uint32_t flow_max = flow_controller_->get_max_payload();
    uint32_t part_max = mp_RTPSParticipant->getMaxMessageSize();
    uint32_t max_size = flow_max > part_max ? part_max : flow_max;

    max_size =  calculateMaxDataSize(max_size);
    return max_size &= ~3;
}

uint32_t RTPSWriter::calculateMaxDataSize(
        uint32_t length)
{
    uint32_t maxDataSize = mp_RTPSParticipant->calculateMaxDataSize(length);

    maxDataSize -= info_dst_message_length +
            info_ts_message_length +
            data_frag_submessage_header_length +
            heartbeat_message_length;

    //TODO(Ricardo) inlineqos in future.

#if HAVE_SECURITY
    if (getAttributes().security_attributes().is_submessage_protected)
    {
        maxDataSize -= mp_RTPSParticipant->security_manager().calculate_extra_size_for_rtps_submessage(m_guid);
    }

    if (getAttributes().security_attributes().is_payload_protected)
    {
        maxDataSize -= mp_RTPSParticipant->security_manager().calculate_extra_size_for_encoded_payload(m_guid);
    }
#endif // if HAVE_SECURITY

#ifdef FASTDDS_STATISTICS
    maxDataSize -= eprosima::fastdds::statistics::rtps::statistics_submessage_length;
#endif // FASTDDS_STATISTICS

    return maxDataSize;
}

void RTPSWriter::add_guid(
        LocatorSelectorSender& locator_selector,
        const GUID_t& remote_guid)
{
    const GuidPrefix_t& prefix = remote_guid.guidPrefix;
    locator_selector.all_remote_readers.push_back(remote_guid);
    if (std::find(locator_selector.all_remote_participants.begin(),
            locator_selector.all_remote_participants.end(), prefix) ==
            locator_selector.all_remote_participants.end())
    {
        locator_selector.all_remote_participants.push_back(prefix);
    }
}

void RTPSWriter::compute_selected_guids(
        LocatorSelectorSender& locator_selector)
{
    locator_selector.all_remote_readers.clear();
    locator_selector.all_remote_participants.clear();

    for (LocatorSelectorEntry* entry : locator_selector.locator_selector.transport_starts())
    {
        if (entry->enabled)
        {
            add_guid(locator_selector, entry->remote_guid);
        }
    }
}

void RTPSWriter::update_cached_info_nts(
        LocatorSelectorSender& locator_selector)
{
    locator_selector.locator_selector.reset(true);
    mp_RTPSParticipant->network_factory().select_locators(locator_selector.locator_selector);
}

const LivelinessQosPolicyKind& RTPSWriter::get_liveliness_kind() const
{
    return liveliness_kind_;
}

const Duration_t& RTPSWriter::get_liveliness_lease_duration() const
{
    return liveliness_lease_duration_;
}

const Duration_t& RTPSWriter::get_liveliness_announcement_period() const
{
    return liveliness_announcement_period_;
}

bool RTPSWriter::is_datasharing_compatible() const
{
    return (m_att.data_sharing_configuration().kind() != OFF);
}

bool RTPSWriter::is_datasharing_compatible_with(
        const ReaderProxyData& rdata) const
{
    if (!is_datasharing_compatible() ||
            rdata.m_qos.data_sharing.kind() == fastdds::dds::OFF)
    {
        return false;
    }

    for (auto id : rdata.m_qos.data_sharing.domain_ids())
    {
        if (std::find(m_att.data_sharing_configuration().domain_ids().begin(),
                m_att.data_sharing_configuration().domain_ids().end(), id)
                != m_att.data_sharing_configuration().domain_ids().end())
        {
            return true;
        }
    }
    return false;
}

bool RTPSWriter::is_pool_initialized() const
{
    if (is_datasharing_compatible())
    {
        auto pool = std::dynamic_pointer_cast<WriterPool>(payload_pool_);
        assert (pool != nullptr);
        return pool->is_initialized();
    }
    return true;
}

bool RTPSWriter::send_nts(
        CDRMessage_t* message,
        const LocatorSelectorSender& locator_selector,
        std::chrono::steady_clock::time_point& max_blocking_time_point) const
{
    RTPSParticipantImpl* participant = getRTPSParticipant();

    return locator_selector.locator_selector.selected_size() == 0 ||
           participant->sendSync(message, m_guid, locator_selector.locator_selector.begin(),
                   locator_selector.locator_selector.end(), max_blocking_time_point);
}

#ifdef FASTDDS_STATISTICS

bool RTPSWriter::add_statistics_listener(
        std::shared_ptr<fastdds::statistics::IListener> listener)
{
    return add_statistics_listener_impl(listener);
}

bool RTPSWriter::remove_statistics_listener(
        std::shared_ptr<fastdds::statistics::IListener> listener)
{
    return remove_statistics_listener_impl(listener);
}

#endif // FASTDDS_STATISTICS

void RTPSWriter::add_statistics_sent_submessage(
        CacheChange_t* change,
        size_t num_locators)
{
    static_cast<void>(change);
    static_cast<void>(num_locators);

#ifdef FASTDDS_STATISTICS
    change->writer_info.num_sent_submessages += num_locators;
    on_data_generated(num_locators);
#endif // ifdef FASTDDS_STATISTICS
}

}  // namespace rtps
}  // namespace fastrtps

}  // namespace eprosima
