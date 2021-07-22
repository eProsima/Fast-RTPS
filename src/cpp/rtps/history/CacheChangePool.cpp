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
 * @file CacheChangePool.cpp
 *
 */

#include <fastdds/rtps/common/CacheChange.h>
#include <fastdds/dds/log/Log.hpp>

#include <rtps/history/CacheChangePool.h>

#include <mutex>
#include <cstring>
#include <cassert>
#include <limits>

namespace eprosima {
namespace fastrtps {
namespace rtps {

CacheChangePool::~CacheChangePool()
{
    logInfo(RTPS_UTILS, "ChangePool destructor");

    // Deletion process does not depend on the memory management policy
    for (CacheChange_t* cache : all_caches_)
    {
        delete(cache);
    }
}

CacheChangePool::CacheChangePool(
        const PoolConfig& config)
    : memory_mode_(config.memory_policy)
{
    // Common for all modes: Set the pool size and size limit
    uint32_t pool_size = config.initial_size;
    uint32_t max_pool_size = config.maximum_size;

    logInfo(RTPS_UTILS, "Creating CacheChangePool of size: " << pool_size);

    current_pool_size_ = 0;
    if (max_pool_size > 0)
    {
        if (pool_size > max_pool_size)
        {
            max_pool_size_ = pool_size;
        }
        else
        {
            max_pool_size_ = max_pool_size;
        }
    }
    else
    {
        max_pool_size_ = std::numeric_limits<uint32_t>::max();
    }

    switch (memory_mode_)
    {
        case PREALLOCATED_MEMORY_MODE:
            logInfo(RTPS_UTILS, "Static Mode is active, preallocating memory for pool_size elements");
            allocateGroup(pool_size ? pool_size : 1);
            break;
        case PREALLOCATED_WITH_REALLOC_MEMORY_MODE:
            logInfo(RTPS_UTILS,
                    "Semi-Static Mode is active, preallocating memory for pool_size. Size of the cachechanges can be increased");
            allocateGroup(pool_size ? pool_size : 1);
            break;
        case DYNAMIC_RESERVE_MEMORY_MODE:
            logInfo(RTPS_UTILS, "Dynamic Mode is active, CacheChanges are allocated on request");
            break;
        case DYNAMIC_REUSABLE_MEMORY_MODE:
            logInfo(RTPS_UTILS,
                    "Semi-Dynamic Mode is active, no preallocation but dynamically allocated CacheChanges are reused for future cachechanges");
            break;
    }
}

void CacheChangePool::return_cache_to_pool(
        CacheChange_t* ch)
{
    ch->kind = ALIVE;
    ch->sequenceNumber.high = 0;
    ch->sequenceNumber.low = 0;
    ch->writerGUID = c_Guid_Unknown;
    memset(ch->instanceHandle.value, 0, 16);
    ch->isRead = 0;
    ch->sourceTimestamp.seconds(0);
    ch->sourceTimestamp.fraction(0);
    ch->writer_info.num_sent_submessages = 0;
    ch->setFragmentSize(0);
    free_caches_.push_back(ch);
}

bool CacheChangePool::allocateGroup(
        uint32_t group_size)
{
    // This method should only called from within PREALLOCATED_MEMORY_MODE or PREALLOCATED_WITH_REALLOC_MEMORY_MODE
    assert(memory_mode_ == PREALLOCATED_MEMORY_MODE ||
            memory_mode_ == PREALLOCATED_WITH_REALLOC_MEMORY_MODE);

    logInfo(RTPS_UTILS, "Allocating group of cache changes of size: " << group_size);

    uint32_t desired_size = current_pool_size_ + group_size;
    if (desired_size > max_pool_size_)
    {
        desired_size = max_pool_size_;
        group_size = max_pool_size_ - current_pool_size_;
    }

    if (group_size <= 0)
    {
        logWarning(RTPS_HISTORY, "Maximum number of allowed reserved caches reached");
        return false;
    }

    all_caches_.reserve(desired_size);
    free_caches_.reserve(free_caches_.size() + group_size);

    while (current_pool_size_ < desired_size)
    {
        CacheChange_t* ch = new CacheChange_t();
        all_caches_.push_back(ch);
        free_caches_.push_back(ch);
        ++current_pool_size_;
    }

    return true;
}

CacheChange_t* CacheChangePool::allocateSingle()
{
    /*
     *   In Dynamic Memory Mode CacheChanges are only allocated when they are needed.
     *   This means when the buffer of the message receiver is copied into this struct, the size is allocated.
     *   When the change is released and comes back to the pool, it is deallocated correspondingly.
     *
     *   In Preallocated mode, changes are allocated with a static maximum size and then they are dealt as
     *   they are needed. In Dynamic mode, they are only allocated when they are needed. In Dynamic mode only
     *   the all_caches_ vector is used, in order to keep track of all the changes that are dealt for destruction
     *   purposes.
     *
     */
    bool added = false;
    CacheChange_t* ch = nullptr;

    // This method should only be called from within DYNAMIC_RESERVE_MEMORY_MODE or DYNAMIC_REUSABLE_MEMORY_MODE
    assert(memory_mode_ == DYNAMIC_RESERVE_MEMORY_MODE ||
            memory_mode_ == DYNAMIC_REUSABLE_MEMORY_MODE);

    if (current_pool_size_ < max_pool_size_)
    {
        ++current_pool_size_;
        ch = new CacheChange_t();
        all_caches_.push_back(ch);
        added = true;
    }

    if (!added)
    {
        logWarning(RTPS_HISTORY, "Maximum number of allowed reserved caches reached");
        return nullptr;
    }

    return ch;
}

bool CacheChangePool::reserve_cache(
        CacheChange_t*& cache_change)
{
    cache_change = nullptr;

    if (free_caches_.empty())
    {
        switch (memory_mode_)
        {
            case PREALLOCATED_MEMORY_MODE:
            case PREALLOCATED_WITH_REALLOC_MEMORY_MODE:
                if (!allocateGroup((uint16_t)(ceil((float)current_pool_size_ / 10) + 10)))
                {
                    return false;
                }
                break;

            case DYNAMIC_RESERVE_MEMORY_MODE:
            case DYNAMIC_REUSABLE_MEMORY_MODE:
                cache_change = allocateSingle(); //Allocates a single, empty CacheChange
                return cache_change != nullptr;

            default:
                return false;
        }
    }

    cache_change = free_caches_.back();
    free_caches_.pop_back();
    return true;
}

bool CacheChangePool::release_cache(
        CacheChange_t* cache_change)
{
    switch (memory_mode_)
    {
        case PREALLOCATED_MEMORY_MODE:
        case PREALLOCATED_WITH_REALLOC_MEMORY_MODE:
        case DYNAMIC_REUSABLE_MEMORY_MODE:
            return_cache_to_pool(cache_change);
            break;

        case DYNAMIC_RESERVE_MEMORY_MODE:
            // Find pointer in CacheChange vector, remove element, then delete it
            std::vector<CacheChange_t*>::iterator target =
                    std::find(all_caches_.begin(), all_caches_.end(), cache_change);
            if (target != all_caches_.end())
            {
                // Copy last element into the element being removed
                if (target != --all_caches_.end())
                {
                    *target = std::move(all_caches_.back());
                }

                // Then drop last element
                all_caches_.pop_back();
            }
            else
            {
                logInfo(RTPS_UTILS, "Tried to release a CacheChange that is not logged in the Pool");
                return false;
            }

            delete(cache_change);
            --current_pool_size_;
            break;
    }

    return true;
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
