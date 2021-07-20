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
 * @file SubscriberHistory.cpp
 *
 */

#include <fastrtps/subscriber/SubscriberHistory.h>
#include <fastrtps_deprecated/subscriber/SubscriberImpl.h>

#include <fastdds/rtps/reader/RTPSReader.h>
#include <rtps/reader/WriterProxy.h>

#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastdds/dds/log/Log.hpp>

#include <algorithm>
#include <limits>
#include <mutex>

namespace eprosima {
namespace fastrtps {

using namespace rtps;

using eprosima::fastdds::dds::TopicDataType;

static void get_sample_info(
        SampleInfo_t* info,
        CacheChange_t* change,
        uint32_t ownership_strength)
{
    info->sampleKind = change->kind;
    info->sample_identity.writer_guid(change->writerGUID);
    info->sample_identity.sequence_number(change->sequenceNumber);
    info->sourceTimestamp = change->sourceTimestamp;
    info->receptionTimestamp = change->receptionTimestamp;
    info->ownershipStrength = ownership_strength;
    info->iHandle = change->instanceHandle;
    info->related_sample_identity = change->write_params.sample_identity();
}

static HistoryAttributes to_history_attributes(
        const TopicAttributes& topic_att,
        uint32_t payloadMaxSize,
        MemoryManagementPolicy_t mempolicy)
{
    auto initial_samples = topic_att.resourceLimitsQos.allocated_samples;
    auto max_samples = topic_att.resourceLimitsQos.max_samples;

    if (topic_att.historyQos.kind != KEEP_ALL_HISTORY_QOS)
    {
        max_samples = topic_att.historyQos.depth;
        if (topic_att.getTopicKind() != NO_KEY)
        {
            max_samples *= topic_att.resourceLimitsQos.max_instances;
        }

        initial_samples = std::min(initial_samples, max_samples);
    }

    return HistoryAttributes(mempolicy, payloadMaxSize, initial_samples, max_samples);
}

SubscriberHistory::SubscriberHistory(
        const TopicAttributes& topic_att,
        TopicDataType* type,
        const ReaderQos& qos,
        uint32_t payloadMaxSize,
        MemoryManagementPolicy_t mempolicy)
    : ReaderHistory(to_history_attributes(topic_att, payloadMaxSize, mempolicy))
    , history_qos_(topic_att.historyQos)
    , resource_limited_qos_(topic_att.resourceLimitsQos)
    , topic_att_(topic_att)
    , type_(type)
    , qos_(qos)
    , get_key_object_(nullptr)
{
    if (type_->m_isGetKeyDefined)
    {
        get_key_object_ = type_->createData();
    }

    if (resource_limited_qos_.max_samples == 0)
    {
        resource_limited_qos_.max_samples = std::numeric_limits<int32_t>::max();
    }

    if (resource_limited_qos_.max_instances == 0)
    {
        resource_limited_qos_.max_instances = std::numeric_limits<int32_t>::max();
    }

    if (resource_limited_qos_.max_samples_per_instance == 0)
    {
        resource_limited_qos_.max_samples_per_instance = std::numeric_limits<int32_t>::max();
    }

    using std::placeholders::_1;
    using std::placeholders::_2;

    if (topic_att.getTopicKind() == NO_KEY)
    {
        receive_fn_ = topic_att.historyQos.kind == KEEP_ALL_HISTORY_QOS ?
                std::bind(&SubscriberHistory::received_change_keep_all_no_key, this, _1, _2) :
                std::bind(&SubscriberHistory::received_change_keep_last_no_key, this, _1, _2);
    }
    else
    {
        receive_fn_ = topic_att.historyQos.kind == KEEP_ALL_HISTORY_QOS ?
                std::bind(&SubscriberHistory::received_change_keep_all_with_key, this, _1, _2) :
                std::bind(&SubscriberHistory::received_change_keep_last_with_key, this, _1, _2);
    }
}

SubscriberHistory::~SubscriberHistory()
{
    if (type_->m_isGetKeyDefined)
    {
        type_->deleteData(get_key_object_);
    }
}

bool SubscriberHistory::received_change(
        CacheChange_t* a_change,
        size_t unknown_missing_changes_up_to)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(*mp_mutex);
    return receive_fn_(a_change, unknown_missing_changes_up_to);
}

bool SubscriberHistory::received_change_keep_all_no_key(
        CacheChange_t* a_change,
        size_t unknown_missing_changes_up_to)
{
    // TODO(Ricardo) Check
    if (m_changes.size() + unknown_missing_changes_up_to < static_cast<size_t>(resource_limited_qos_.max_samples))
    {
        return add_received_change(a_change);
    }

    return false;
}

bool SubscriberHistory::received_change_keep_last_no_key(
        CacheChange_t* a_change,
        size_t /* unknown_missing_changes_up_to */ )
{
    bool add = false;
    if (m_changes.size() < static_cast<size_t>(history_qos_.depth))
    {
        add = true;
    }
    else
    {
        // Try to substitute the oldest sample.

        // As the history should be ordered following the presentation QoS, we can always remove the first one.
        add = remove_change_sub(m_changes.at(0));
    }

    if (add)
    {
        return add_received_change(a_change);
    }

    return false;
}

bool SubscriberHistory::received_change_keep_all_with_key(
        CacheChange_t* a_change,
        size_t /* unknown_missing_changes_up_to */ )
{
    // TODO(Miguel C): Should we check unknown_missing_changes_up_to as it is done in received_change_keep_all_no_key?

    t_m_Inst_Caches::iterator vit;
    if (find_key_for_change(a_change, vit))
    {
        std::vector<CacheChange_t*>& instance_changes = vit->second.cache_changes;
        if (instance_changes.size() < static_cast<size_t>(resource_limited_qos_.max_samples_per_instance))
        {
            return add_received_change_with_key(a_change, vit->second.cache_changes);
        }

        logWarning(SUBSCRIBER, "Change not added due to maximum number of samples per instance");
    }

    return false;
}

bool SubscriberHistory::received_change_keep_last_with_key(
        CacheChange_t* a_change,
        size_t /* unknown_missing_changes_up_to */)
{
    t_m_Inst_Caches::iterator vit;
    if (find_key_for_change(a_change, vit))
    {
        bool add = false;
        std::vector<CacheChange_t*>& instance_changes = vit->second.cache_changes;
        if (instance_changes.size() < static_cast<size_t>(history_qos_.depth))
        {
            add = true;
        }
        else
        {
            // Try to substitute the oldest sample.

            // As the instance should be ordered following the presentation QoS, we can always remove the first one.
            add = remove_change_sub(instance_changes.at(0));
        }

        if (add)
        {
            return add_received_change_with_key(a_change, instance_changes);
        }
    }

    return false;
}

bool SubscriberHistory::add_received_change(
        CacheChange_t* a_change)
{
    if (m_isHistoryFull)
    {
        // Discarding the sample.
        logWarning(SUBSCRIBER, "Attempting to add Data to Full ReaderHistory: " << topic_att_.getTopicDataType());
        return false;
    }

    if (add_change(a_change))
    {
        if (m_changes.size() == static_cast<size_t>(m_att.maximumReservedCaches))
        {
            m_isHistoryFull = true;
        }

        logInfo(SUBSCRIBER, topic_att_.getTopicDataType()
                << ": Change " << a_change->sequenceNumber << " added from: "
                << a_change->writerGUID; );

        return true;
    }

    return false;
}

bool SubscriberHistory::add_received_change_with_key(
        CacheChange_t* a_change,
        std::vector<CacheChange_t*>& instance_changes)
{
    if (m_isHistoryFull)
    {
        // Discarting the sample.
        logWarning(SUBSCRIBER, "Attempting to add Data to Full ReaderHistory: " << topic_att_.getTopicDataType());
        return false;
    }

    if (add_change(a_change))
    {
        if (m_changes.size() == static_cast<size_t>(m_att.maximumReservedCaches))
        {
            m_isHistoryFull = true;
        }

        //ADD TO KEY VECTOR

        // As the instance should be ordered following the presentation QoS, and
        // we only support ordering by reception timestamp, we can always add at the end.
        instance_changes.push_back(a_change);

        logInfo(SUBSCRIBER, mp_reader->getGuid().entityId
                << ": Change " << a_change->sequenceNumber << " added from: "
                << a_change->writerGUID << " with KEY: " << a_change->instanceHandle; );

        return true;
    }

    return false;
}

bool SubscriberHistory::find_key_for_change(
        rtps::CacheChange_t* a_change,
        t_m_Inst_Caches::iterator& map_it)
{
    if (!a_change->instanceHandle.isDefined() && type_ != nullptr)
    {
        logInfo(SUBSCRIBER, "Getting Key of change with no Key transmitted");
        type_->deserialize(&a_change->serializedPayload, get_key_object_);
        bool is_key_protected = false;
#if HAVE_SECURITY
        is_key_protected = mp_reader->getAttributes().security_attributes().is_key_protected;
#endif // if HAVE_SECURITY
        if (!type_->getKey(get_key_object_, &a_change->instanceHandle, is_key_protected))
        {
            return false;
        }
    }
    else if (!a_change->instanceHandle.isDefined())
    {
        logWarning(SUBSCRIBER, "NO KEY in topic: " << topic_att_.topicName
                                                   << " and no method to obtain it"; );
        return false;
    }

    return find_key(a_change, &map_it);
}

bool SubscriberHistory::deserialize_change(
        CacheChange_t* change,
        uint32_t ownership_strength,
        void* data,
        SampleInfo_t* info)
{
    if (change->kind == ALIVE)
    {
        if (!type_->deserialize(&change->serializedPayload, data))
        {
            logError(SUBSCRIBER, "Deserialization of data failed");
            return false;
        }
    }

    if (info != nullptr)
    {
        if (topic_att_.topicKind == WITH_KEY &&
                change->instanceHandle == c_InstanceHandle_Unknown &&
                change->kind == ALIVE)
        {
            bool is_key_protected = false;
#if HAVE_SECURITY
            is_key_protected = mp_reader->getAttributes().security_attributes().is_key_protected;
#endif // if HAVE_SECURITY
            type_->getKey(data, &change->instanceHandle, is_key_protected);
        }

        get_sample_info(info, change, ownership_strength);
    }

    return true;
}

bool SubscriberHistory::readNextData(
        void* data,
        SampleInfo_t* info,
        std::chrono::steady_clock::time_point& max_blocking_time)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }

    std::unique_lock<RecursiveTimedMutex> lock(*mp_mutex, std::defer_lock);

    if (lock.try_lock_until(max_blocking_time))
    {
        CacheChange_t* change;
        WriterProxy* wp = nullptr;
        if (mp_reader->nextUnreadCache(&change, &wp))
        {
            logInfo(SUBSCRIBER, mp_reader->getGuid().entityId << ": reading " << change->sequenceNumber);
            uint32_t ownership = wp && qos_.m_ownership.kind == EXCLUSIVE_OWNERSHIP_QOS ?
                    wp->ownership_strength() : 0;
            bool deserialized = deserialize_change(change, ownership, data, info);
            mp_reader->change_read_by_user(change, wp);
            return deserialized;
        }
    }
    return false;
}

bool SubscriberHistory::takeNextData(
        void* data,
        SampleInfo_t* info,
        std::chrono::steady_clock::time_point& max_blocking_time)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }

    std::unique_lock<RecursiveTimedMutex> lock(*mp_mutex, std::defer_lock);

    if (lock.try_lock_until(max_blocking_time))
    {
        CacheChange_t* change = nullptr;
        WriterProxy* wp = nullptr;
        if (mp_reader->nextUntakenCache(&change, &wp))
        {
            logInfo(SUBSCRIBER, mp_reader->getGuid().entityId << ": taking seqNum" << change->sequenceNumber <<
                    " from writer: " << change->writerGUID);
            uint32_t ownership = wp && qos_.m_ownership.kind == EXCLUSIVE_OWNERSHIP_QOS ?
                    wp->ownership_strength() : 0;
            bool deserialized = deserialize_change(change, ownership, data, info);
            mp_reader->change_read_by_user(change, wp);
            bool removed = remove_change_sub(change);
            return (deserialized && removed);
        }
    }

    return false;
}

bool SubscriberHistory::get_first_untaken_info(
        SampleInfo_t* info)
{
    std::lock_guard<RecursiveTimedMutex> lock(*mp_mutex);

    CacheChange_t* change = nullptr;
    WriterProxy* wp = nullptr;
    if (mp_reader->nextUntakenCache(&change, &wp))
    {
        uint32_t ownership = wp && qos_.m_ownership.kind == EXCLUSIVE_OWNERSHIP_QOS ? wp->ownership_strength() : 0;
        get_sample_info(info, change, ownership);
        mp_reader->change_read_by_user(change, wp, false);
        return true;
    }

    return false;
}

bool SubscriberHistory::find_key(
        CacheChange_t* a_change,
        t_m_Inst_Caches::iterator* vit_out)
{
    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(a_change->instanceHandle);
    if (vit != keyed_changes_.end())
    {
        *vit_out = vit;
        return true;
    }

    if (keyed_changes_.size() < static_cast<size_t>(resource_limited_qos_.max_instances))
    {
        *vit_out = keyed_changes_.insert(std::make_pair(a_change->instanceHandle, KeyedChanges())).first;
        return true;
    }
    else
    {
        for (vit = keyed_changes_.begin(); vit != keyed_changes_.end(); ++vit)
        {
            if (vit->second.cache_changes.size() == 0)
            {
                keyed_changes_.erase(vit);
                *vit_out = keyed_changes_.insert(std::make_pair(a_change->instanceHandle, KeyedChanges())).first;
                return true;
            }
        }
        logWarning(SUBSCRIBER, "History has reached the maximum number of instances");
    }
    return false;
}

bool SubscriberHistory::remove_change_sub(
        CacheChange_t* change)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(*mp_mutex);
    if (topic_att_.getTopicKind() == WITH_KEY)
    {
        bool found = false;
        t_m_Inst_Caches::iterator vit;
        if (find_key(change, &vit))
        {
            for (auto chit = vit->second.cache_changes.begin(); chit != vit->second.cache_changes.end(); ++chit)
            {
                if ((*chit)->sequenceNumber == change->sequenceNumber && (*chit)->writerGUID == change->writerGUID)
                {
                    vit->second.cache_changes.erase(chit);
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            logError(SUBSCRIBER, "Change not found on this key, something is wrong");
        }
    }

    if (remove_change(change))
    {
        m_isHistoryFull = false;
        return true;
    }

    return false;
}

bool SubscriberHistory::remove_changes_with_guid(
        const GUID_t& a_guid)
{
    std::lock_guard<RecursiveTimedMutex> guard(*mp_mutex);

    // clean this class related data
    for ( auto mit = keyed_changes_.begin(); mit != keyed_changes_.end(); ++mit)
    {
        auto& caches = mit->second.cache_changes;
        caches.erase(
            std::remove_if(
                caches.begin(),
                caches.end(),
                [&a_guid](CacheChange_t const* const pSample) -> bool
                {
                    return pSample->writerGUID == a_guid;
                }),
            caches.end());
    }

    // clean base class related data
    return ReaderHistory::remove_changes_with_guid(a_guid);
}

bool SubscriberHistory::remove_change_sub(
        CacheChange_t* change,
        iterator& it)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(*mp_mutex);
    if (topic_att_.getTopicKind() == WITH_KEY)
    {
        bool found = false;
        t_m_Inst_Caches::iterator vit;
        if (find_key(change, &vit))
        {
            for (auto chit = vit->second.cache_changes.begin(); chit != vit->second.cache_changes.end(); ++chit)
            {
                if ((*chit)->sequenceNumber == change->sequenceNumber && (*chit)->writerGUID == change->writerGUID)
                {
                    assert(it == chit);
                    it = vit->second.cache_changes.erase(chit);
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            logError(SUBSCRIBER, "Change not found on this key, something is wrong");
        }
    }

    const_iterator chit = find_change_nts(change);
    if (chit == changesEnd())
    {
        logInfo(RTPS_WRITER_HISTORY, "Trying to remove a change not in history");
        return false;
    }

    m_isHistoryFull = false;
    iterator ret_it = remove_change_nts(chit);

    if (topic_att_.getTopicKind() != WITH_KEY)
    {
        it = ret_it;
    }

    return true;
}

bool SubscriberHistory::set_next_deadline(
        const InstanceHandle_t& handle,
        const std::chrono::steady_clock::time_point& next_deadline_us)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(*mp_mutex);

    if (topic_att_.getTopicKind() == NO_KEY)
    {
        next_deadline_us_ = next_deadline_us;
        return true;
    }
    else if (topic_att_.getTopicKind() == WITH_KEY)
    {
        if (keyed_changes_.find(handle) == keyed_changes_.end())
        {
            return false;
        }

        keyed_changes_[handle].next_deadline_us = next_deadline_us;
        return true;
    }

    return false;
}

bool SubscriberHistory::get_next_deadline(
        InstanceHandle_t& handle,
        std::chrono::steady_clock::time_point& next_deadline_us)
{
    if (mp_reader == nullptr || mp_mutex == nullptr)
    {
        logError(SUBSCRIBER, "You need to create a Reader with this History before using it");
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(*mp_mutex);

    if (topic_att_.getTopicKind() == NO_KEY)
    {
        next_deadline_us = next_deadline_us_;
        return true;
    }
    else if (topic_att_.getTopicKind() == WITH_KEY)
    {
        auto min = std::min_element(keyed_changes_.begin(),
                        keyed_changes_.end(),
                        [](
                            const std::pair<InstanceHandle_t, KeyedChanges>& lhs,
                            const std::pair<InstanceHandle_t, KeyedChanges>& rhs)
                        {
                            return lhs.second.next_deadline_us < rhs.second.next_deadline_us;
                        });
        handle = min->first;
        next_deadline_us = min->second.next_deadline_us;
        return true;
    }

    return false;
}

std::pair<bool, SubscriberHistory::instance_info> SubscriberHistory::lookup_instance(
        const InstanceHandle_t& handle,
        bool exact)
{
    if (topic_att_.getTopicKind() == NO_KEY)
    {
        if (handle.isDefined())
        {
            // NO_KEY topics can only return the ficticious instance.
            // Execution can only get here for two reasons:
            // - Looking for a specific instance (exact = true)
            // - Looking for the next instance to the ficticious one (exact = false)
            // In both cases, no instance should be returned
            return { false, {InstanceHandle_t(), nullptr} };
        }
        else
        {
            if (exact)
            {
                // Looking for HANDLE_NIL, nothing to return
                return { false, {InstanceHandle_t(), nullptr} };
            }

            // Looking for the first instance, return the ficticious one containing all changes
            InstanceHandle_t tmp;
            tmp.value[0] = 1;
            return { true, {tmp, &m_changes} };
        }
    }

    t_m_Inst_Caches::iterator it;

    if (exact)
    {
        it = keyed_changes_.find(handle);
    }
    else
    {
        auto comp = [](const InstanceHandle_t& h, const std::pair<InstanceHandle_t, KeyedChanges>& it)
                {
                    return h < it.first;
                };
        it = std::upper_bound(keyed_changes_.begin(), keyed_changes_.end(), handle, comp);
    }

    if (it != keyed_changes_.end())
    {
        return { true, {it->first, &(it->second.cache_changes)} };
    }
    return { false, {InstanceHandle_t(), nullptr} };
}

} // namespace fastrtps
} // namsepace eprosima
