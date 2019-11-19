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
 * @file ThreadEvent.cpp
 *
 */

#include <fastrtps/rtps/resources/ResourceEvent.h>
#include <fastrtps/log/Log.h>

#include "TimedEventImpl.h"

#include <cassert>
#include <thread>

namespace eprosima {
namespace fastrtps {
namespace rtps {

static bool event_compare(
        TimedEventImpl* lhs,
        TimedEventImpl* rhs)
{
    return lhs->next_trigger_time() < rhs->next_trigger_time();
}

ResourceEvent::ResourceEvent()
    : stop_(false)
{
}

ResourceEvent::~ResourceEvent()
{
    // All timer should be unregistered before destroying this object.
    assert(pending_timers_.empty());

    logInfo(RTPS_PARTICIPANT, "Removing event thread");
    stop_.store(true);

    if (thread_.joinable())
    {
        thread_.join();
    }
}

bool ResourceEvent::register_timer_nts(
        TimedEventImpl* event)
{
    if (std::find(pending_timers_.begin(), pending_timers_.end(), event) == pending_timers_.end())
    {
        pending_timers_.push_back(event);
        return true;
    }

    return false;
}

void ResourceEvent::activate_timer(
        TimedEventImpl* event)
{
    std::vector<TimedEventImpl*>::iterator low_bound;
    std::vector<TimedEventImpl*>::iterator end_it = active_timers_.end();
    
    // Find insertion position
    low_bound = std::lower_bound(active_timers_.begin(), end_it, event, event_compare);

    // If event is not found from there onwards ...
    if (std::find(low_bound, end_it, event) == end_it)
    {
        // ... add it on its place
        active_timers_.emplace(low_bound, event);
    }
}

void ResourceEvent::unregister_timer(
        TimedEventImpl* event)
{
    assert(!stop_.load());

    std::unique_lock<TimedMutex> lock(mutex_);

    cv_.wait(lock, [&]()
    {
        return allow_to_delete_;
    });

    bool should_notify = false;
    std::vector<TimedEventImpl*>::iterator it;

    // Remove from pending
    it = std::find(pending_timers_.begin(), pending_timers_.end(), event);
    if (it != pending_timers_.end())
    {
        pending_timers_.erase(it);
        should_notify = true;
    }

    std::vector<TimedEventImpl*>::iterator end_it = active_timers_.end();

    // Find with binary search
    it = std::lower_bound(active_timers_.begin(), end_it, event, event_compare);

    // Find the event on the list
    for(; it != end_it; ++it)
    {
        if (*it == event)
        {
            // Remove from list
            active_timers_.erase(it);
            should_notify = true;
            break;
        }
    }

    if (should_notify)
    {
        // Notify the execution thread that something changed
        cv_.notify_one();
    }
}

void ResourceEvent::notify(
        TimedEventImpl* event)
{
    std::unique_lock<TimedMutex> lock(mutex_);

    if (register_timer_nts(event))
    {
        // Notify the execution thread that something changed
        cv_.notify_one();
    }
}

void ResourceEvent::notify(
        TimedEventImpl* event,
        const std::chrono::steady_clock::time_point& timeout)
{
    std::unique_lock<TimedMutex> lock(mutex_, std::defer_lock);

    if (lock.try_lock_until(timeout))
    {
        if (register_timer_nts(event))
        {
            // Notify the execution thread that something changed
            cv_.notify_one();
        }
    }
}

void ResourceEvent::run_io_service()
{
    while (!stop_.load())
    {
        update_current_time();
        do_timer_actions();

        std::unique_lock<TimedMutex> lock(mutex_);

        allow_to_delete_ = true;
        cv_.notify_one();

        std::chrono::steady_clock::time_point next_trigger =
            active_timers_.empty() ?
                current_time_ + std::chrono::seconds(1) :
                active_timers_[0]->next_trigger_time();

        cv_.wait_until(lock, next_trigger);

        allow_to_delete_ = false;
    }
}

void ResourceEvent::sort_timers()
{
    std::sort(active_timers_.begin(), active_timers_.end(), event_compare);
}

void ResourceEvent::update_current_time()
{
    current_time_ = std::chrono::steady_clock::now();
}

void ResourceEvent::do_timer_actions()
{
    std::chrono::steady_clock::time_point cancel_time =
        current_time_ + std::chrono::hours(24);

    bool did_something = false;

    // Process pending orders
    {
        std::unique_lock<TimedMutex> lock(mutex_);
        for (TimedEventImpl* tp : pending_timers_)
        {
            did_something = true;
            tp->update(current_time_, cancel_time);
            activate_timer(tp);
        }
        pending_timers_.clear();
    }

    // Trigger active timers
    for (TimedEventImpl* tp : active_timers_)
    {
        if (tp->next_trigger_time() <= current_time_)
        {
            did_something = true;
            tp->trigger(current_time_, cancel_time);
        }
        else
        {
            break;
        }
    }

    // If an action was made, keep active_timers_ sorted
    if (did_something)
    {
        sort_timers();
        active_timers_.erase(
            std::lower_bound(active_timers_.begin(), active_timers_.end(), nullptr,
                [cancel_time](
                        TimedEventImpl* a,
                        TimedEventImpl* b)
                {
                    (void)b;
                    return a->next_trigger_time() < cancel_time;
                }),
            active_timers_.end()
            );
    }
}

void ResourceEvent::init_thread()
{
    thread_ = std::thread(&ResourceEvent::run_io_service, this);
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
