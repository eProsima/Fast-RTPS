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
 * @file PublisherHistory.cpp
 *
 */
#include <fastrtps/config.h>

#include <mutex>

#include <fastrtps/publisher/PublisherHistory.h>

#include <fastrtps_deprecated/publisher/PublisherImpl.h>

#include <fastdds/rtps/writer/RTPSWriter.h>

#include <fastdds/dds/log/Log.hpp>

#include <limits>
#include <mutex>

namespace eprosima {
namespace fastrtps {

using namespace rtps;

static HistoryAttributes to_history_attributes(
        const TopicAttributes& topic_att,
        uint32_t payloadMaxSize,
        MemoryManagementPolicy_t mempolicy)
{
    auto initial_samples = topic_att.resourceLimitsQos.allocated_samples;
    auto max_samples = topic_att.resourceLimitsQos.max_samples;
    auto extra_samples = topic_att.resourceLimitsQos.extra_samples;

    if (topic_att.historyQos.kind != KEEP_ALL_HISTORY_QOS)
    {
        max_samples = topic_att.historyQos.depth;
        if (topic_att.getTopicKind() != NO_KEY)
        {
            max_samples *= topic_att.resourceLimitsQos.max_instances;
        }

        initial_samples = std::min(initial_samples, max_samples);
    }

    return HistoryAttributes(mempolicy, payloadMaxSize, initial_samples, max_samples, extra_samples);
}

PublisherHistory::PublisherHistory(
        const TopicAttributes& topic_att,
        uint32_t payloadMaxSize,
        MemoryManagementPolicy_t mempolicy)
    : WriterHistory(to_history_attributes(topic_att, payloadMaxSize, mempolicy))
    , history_qos_(topic_att.historyQos)
    , resource_limited_qos_(topic_att.resourceLimitsQos)
    , topic_att_(topic_att)
{
    if (resource_limited_qos_.max_instances == 0)
    {
        resource_limited_qos_.max_instances = std::numeric_limits<int32_t>::max();
    }

    if (resource_limited_qos_.max_samples_per_instance == 0)
    {
        resource_limited_qos_.max_samples_per_instance = std::numeric_limits<int32_t>::max();
    }
}

PublisherHistory::~PublisherHistory()
{
}

void PublisherHistory::rebuild_instances()
{
    if (topic_att_.getTopicKind() == WITH_KEY)
    {
        for (CacheChange_t* change : m_changes)
        {
            t_m_Inst_Caches::iterator vit;
            if (find_or_add_key(change->instanceHandle, &vit))
            {
                vit->second.cache_changes.push_back(change);
            }
        }
    }
}

bool PublisherHistory::register_instance(
        const InstanceHandle_t& instance_handle,
        std::unique_lock<RecursiveTimedMutex>&,
        const std::chrono::time_point<std::chrono::steady_clock>&)
{
    /// Preconditions
    if (topic_att_.getTopicKind() == NO_KEY)
    {
        return false;
    }

    t_m_Inst_Caches::iterator vit;
    return find_or_add_key(instance_handle, &vit);
}

bool PublisherHistory::add_pub_change(
        CacheChange_t* change,
        WriteParams& wparams,
        std::unique_lock<RecursiveTimedMutex>& lock,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    if (m_isHistoryFull)
    {
        bool ret = false;

        if (history_qos_.kind == KEEP_ALL_HISTORY_QOS)
        {
            ret = this->mp_writer->try_remove_change(max_blocking_time, lock);
        }
        else if (history_qos_.kind == KEEP_LAST_HISTORY_QOS)
        {
            ret = this->remove_min_change();
        }

        if (!ret)
        {
            logWarning(RTPS_HISTORY, "Attempting to add Data to Full WriterCache: " << topic_att_.getTopicDataType());
            return false;
        }
    }

    assert(!m_isHistoryFull);

    bool returnedValue = false;

    // For NO_KEY we can directly add the change
    bool add = (topic_att_.getTopicKind() == NO_KEY);
    if (topic_att_.getTopicKind() == WITH_KEY)
    {
        t_m_Inst_Caches::iterator vit;

        // For WITH_KEY, we take into account the limits on the instance
        // In case we wait for a sequence to be acknowledged, we try several times
        // until we reach the max blocking timepoint
        while (!add)
        {
            // We should have the instance
            if (!find_or_add_key(change->instanceHandle, &vit))
            {
                break;
            }

            if (history_qos_.kind == KEEP_LAST_HISTORY_QOS)
            {
                if (vit->second.cache_changes.size() < static_cast<size_t>(history_qos_.depth))
                {
                    add = true;
                }
                else
                {
                    add = remove_change_pub(vit->second.cache_changes.front());
                }
            }
            else if (history_qos_.kind == KEEP_ALL_HISTORY_QOS)
            {
                if (vit->second.cache_changes.size() <
                        static_cast<size_t>(resource_limited_qos_.max_samples_per_instance))
                {
                    add = true;
                }
                else
                {
                    SequenceNumber_t seq_to_remove = vit->second.cache_changes.front()->sequenceNumber;
                    if (!mp_writer->wait_for_acknowledgement(seq_to_remove, max_blocking_time, lock))
                    {
                        // Timeout waiting. Will not add change to history.
                        break;
                    }

                    // vit may have been invalidated
                    if (!find_or_add_key(change->instanceHandle, &vit))
                    {
                        break;
                    }

                    // If the change we were trying to remove was already removed, try again
                    if (vit->second.cache_changes.empty() ||
                            vit->second.cache_changes.front()->sequenceNumber != seq_to_remove)
                    {
                        continue;
                    }

                    // Remove change if still present
                    add = remove_change_pub(vit->second.cache_changes.front());
                }
            }
        }

        if (add)
        {
            vit->second.cache_changes.push_back(change);
        }
    }

    if (add)
    {
#if HAVE_STRICT_REALTIME
        if (this->add_change_(change, wparams, max_blocking_time))
#else
        if (this->add_change_(change, wparams))
#endif // if HAVE_STRICT_REALTIME
        {
            logInfo(RTPS_HISTORY,
                    topic_att_.getTopicDataType()
                    << " Change " << change->sequenceNumber << " added with key: " << change->instanceHandle
                    << " and " << change->serializedPayload.length << " bytes");
            returnedValue = true;
        }
    }

    return returnedValue;
}

bool PublisherHistory::find_or_add_key(
        const InstanceHandle_t& instance_handle,
        t_m_Inst_Caches::iterator* vit_out)
{
    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(instance_handle);
    if (vit != keyed_changes_.end())
    {
        *vit_out = vit;
        return true;
    }

    if (static_cast<int>(keyed_changes_.size()) < resource_limited_qos_.max_instances)
    {
        *vit_out = keyed_changes_.insert(std::make_pair(instance_handle, KeyedChanges())).first;
        return true;
    }

    return false;
}

bool PublisherHistory::removeAllChange(
        size_t* removed)
{

    size_t rem = 0;
    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);

    while (m_changes.size() > 0)
    {
        if (remove_change_pub(m_changes.front()))
        {
            ++rem;
        }
        else
        {
            break;
        }
    }
    if (removed != nullptr)
    {
        *removed = rem;
    }
    if (rem > 0)
    {
        return true;
    }
    return false;
}

bool PublisherHistory::removeMinChange()
{
    if (mp_writer == nullptr || mp_mutex == nullptr)
    {
        logError(RTPS_HISTORY, "You need to create a Writer with this History before using it");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);
    if (m_changes.size() > 0)
    {
        return remove_change_pub(m_changes.front());
    }
    return false;
}

bool PublisherHistory::remove_change_pub(
        CacheChange_t* change)
{
    if (mp_writer == nullptr || mp_mutex == nullptr)
    {
        logError(RTPS_HISTORY, "You need to create a Writer with this History before using it");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);
    if (topic_att_.getTopicKind() == NO_KEY)
    {
        if (remove_change(change))
        {
            m_isHistoryFull = false;
            return true;
        }

        return false;
    }
    else
    {
        t_m_Inst_Caches::iterator vit;
        if (!this->find_or_add_key(change->instanceHandle, &vit))
        {
            return false;
        }

        for (auto chit = vit->second.cache_changes.begin(); chit != vit->second.cache_changes.end(); ++chit)
        {
            if (((*chit)->sequenceNumber == change->sequenceNumber) && ((*chit)->writerGUID == change->writerGUID))
            {
                if (remove_change(change))
                {
                    vit->second.cache_changes.erase(chit);
                    m_isHistoryFull = false;
                    return true;
                }
            }
        }
        logError(PUBLISHER, "Change not found, something is wrong");
    }
    return false;
}

bool PublisherHistory::remove_change_g(
        CacheChange_t* a_change)
{
    return remove_change_pub(a_change);
}

bool PublisherHistory::remove_instance_changes(
        const rtps::InstanceHandle_t& handle,
        const rtps::SequenceNumber_t& seq_up_to)
{
    if (mp_writer == nullptr || mp_mutex == nullptr)
    {
        logError(RTPS_HISTORY, "You need to create a Writer with this History before using it");
        return false;
    }

    if (topic_att_.getTopicKind() == NO_KEY)
    {
        logError(RTPS_HISTORY, "Cannot be removed instance changes of a NO_KEY DataType");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);
    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(handle);
    if (vit == keyed_changes_.end())
    {
        return false;
    }

    auto chit = vit->second.cache_changes.begin();

    for (; chit != vit->second.cache_changes.end() && (*chit)->sequenceNumber <= seq_up_to; ++chit)
    {
        if (remove_change(*chit))
        {
            m_isHistoryFull = false;
        }
    }

    vit->second.cache_changes.erase(vit->second.cache_changes.begin(), chit);

    if (vit->second.cache_changes.empty())
    {
        keyed_changes_.erase(vit);
    }

    return true;
}

bool PublisherHistory::set_next_deadline(
        const InstanceHandle_t& handle,
        const std::chrono::steady_clock::time_point& next_deadline_us)
{
    if (mp_writer == nullptr || mp_mutex == nullptr)
    {
        logError(RTPS_HISTORY, "You need to create a Writer with this History before using it");
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);

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

bool PublisherHistory::get_next_deadline(
        InstanceHandle_t& handle,
        std::chrono::steady_clock::time_point& next_deadline_us)
{
    if (mp_writer == nullptr || mp_mutex == nullptr)
    {
        logError(RTPS_HISTORY, "You need to create a Writer with this History before using it");
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);

    if (topic_att_.getTopicKind() == WITH_KEY)
    {
        auto min = std::min_element(
            keyed_changes_.begin(),
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
    else if (topic_att_.getTopicKind() == NO_KEY)
    {
        next_deadline_us = next_deadline_us_;
        return true;
    }

    return false;
}

bool PublisherHistory::is_key_registered(
        const InstanceHandle_t& handle)
{
    if (mp_writer == nullptr || mp_mutex == nullptr)
    {
        logError(RTPS_HISTORY, "You need to create a Writer with this History before using it");
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(*this->mp_mutex);
    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(handle);
    return (vit != keyed_changes_.end() &&
           (vit->second.cache_changes.empty() ||
           (NOT_ALIVE_UNREGISTERED != vit->second.cache_changes.back()->kind &&
           NOT_ALIVE_DISPOSED_UNREGISTERED != vit->second.cache_changes.back()->kind
           )
           )
           );
}

}  // namespace fastrtps
}  // namespace eprosima
