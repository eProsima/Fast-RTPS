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

#include <fastdds/rtps/writer/RTPSWriter.h>

#include <fastdds/dds/log/Log.hpp>

#include <fastdds/rtps/history/WriterHistory.h>
#include <fastdds/rtps/messages/RTPSMessageCreator.h>

#include <rtps/history/BasicPayloadPool.hpp>
#include <rtps/history/CacheChangePool.h>
#include <rtps/DataSharing/DataSharingNotifier.hpp>
#include <rtps/DataSharing/WriterPool.hpp>
#include <rtps/flowcontrol/FlowController.h>
#include <rtps/participant/RTPSParticipantImpl.h>

#include <mutex>

namespace eprosima {
namespace fastrtps {
namespace rtps {

RTPSWriter::RTPSWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& att,
        WriterHistory* hist,
        WriterListener* listen)
    : Endpoint(impl, guid, att.endpoint)
    , mp_history(hist)
    , mp_listener(listen)
    , is_async_(att.mode == SYNCHRONOUS_WRITER ? false : true)
    , locator_selector_(att.matched_readers_allocation)
    , all_remote_readers_(att.matched_readers_allocation)
    , all_remote_participants_(att.matched_readers_allocation)
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
        WriterHistory* hist,
        WriterListener* listen)
    : RTPSWriter(
        impl, guid, att, payload_pool,
        std::make_shared<CacheChangePool>(PoolConfig::from_history_attributes(hist->m_att)),
        hist, listen)
{
}

RTPSWriter::RTPSWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        WriterHistory* hist,
        WriterListener* listen)
    : Endpoint(impl, guid, att.endpoint)
    , mp_history(hist)
    , mp_listener(listen)
    , is_async_(att.mode == SYNCHRONOUS_WRITER ? false : true)
    , locator_selector_(att.matched_readers_allocation)
    , all_remote_readers_(att.matched_readers_allocation)
    , all_remote_participants_(att.matched_readers_allocation)
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

    logInfo(RTPS_WRITER, "RTPSWriter created");
}

RTPSWriter::~RTPSWriter()
{
    logInfo(RTPS_WRITER, "RTPSWriter destructor");

    // Deletion of the events has to be made in child destructor.

    for (auto it = mp_history->changesBegin(); it != mp_history->changesEnd(); ++it)
    {
        release_change(*it);
    }

    mp_history->mp_writer = nullptr;
    mp_history->mp_mutex = nullptr;
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

uint32_t RTPSWriter::getMaxDataSize()
{
    return calculateMaxDataSize(mp_RTPSParticipant->getMaxMessageSize());
}

uint32_t RTPSWriter::calculateMaxDataSize(
        uint32_t length)
{
    uint32_t maxDataSize = mp_RTPSParticipant->calculateMaxDataSize(length);

    maxDataSize -= info_dst_message_length +
            info_ts_message_length +
            data_frag_submessage_header_length;

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

    return maxDataSize;
}

void RTPSWriter::add_guid(
        const GUID_t& remote_guid)
{
    const GuidPrefix_t& prefix = remote_guid.guidPrefix;
    all_remote_readers_.push_back(remote_guid);
    if (std::find(all_remote_participants_.begin(), all_remote_participants_.end(), prefix) ==
            all_remote_participants_.end())
    {
        all_remote_participants_.push_back(prefix);
    }
}

void RTPSWriter::compute_selected_guids()
{
    all_remote_readers_.clear();
    all_remote_participants_.clear();

    for (LocatorSelectorEntry* entry : locator_selector_.transport_starts())
    {
        if (entry->enabled)
        {
            add_guid(entry->remote_guid);
        }
    }
}

void RTPSWriter::update_cached_info_nts()
{
    locator_selector_.reset(true);
    mp_RTPSParticipant->network_factory().select_locators(locator_selector_);
}

bool RTPSWriter::destinations_have_changed() const
{
    return false;
}

GuidPrefix_t RTPSWriter::destination_guid_prefix() const
{
    return all_remote_participants_.size() == 1 ? all_remote_participants_.at(0) : c_GuidPrefix_Unknown;
}

const std::vector<GuidPrefix_t>& RTPSWriter::remote_participants() const
{
    return all_remote_participants_;
}

const std::vector<GUID_t>& RTPSWriter::remote_guids() const
{
    return all_remote_readers_;
}

bool RTPSWriter::send(
        const RTPSMessageSenderInterface::NetworkBuffer* buffers,
        size_t num_buffers,
        uint32_t total_bytes,
        std::chrono::steady_clock::time_point& max_blocking_time_point) const
{
    RTPSParticipantImpl* participant = getRTPSParticipant();

    return locator_selector_.selected_size() == 0 ||
           participant->sendSync(buffers, num_buffers, total_bytes,
                   locator_selector_.begin(), locator_selector_.end(), max_blocking_time_point);
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

#ifdef FASTDDS_STATISTICS

bool RTPSWriter::add_statistics_listener(
        std::shared_ptr<fastdds::statistics::IListener> listener)
{
    (void)listener;
    return false;
}

bool RTPSWriter::remove_statistics_listener(
        std::shared_ptr<fastdds::statistics::IListener> listener)
{
    (void)listener;
    return false;
}

#endif // FASTDDS_STATISTICS

}  // namespace rtps
}  // namespace fastrtps
}  // namespace eprosima
