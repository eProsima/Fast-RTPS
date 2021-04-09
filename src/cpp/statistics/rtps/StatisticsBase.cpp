// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file StatisticsBase.cpp
 */

#include "StatisticsBase.hpp"
#include <rtps/participant/RTPSParticipantImpl.h>
#include <fastdds/dds/log/Log.hpp>

#include <cmath>

using namespace eprosima::fastdds::statistics;
using eprosima::fastrtps::rtps::RTPSParticipantImpl;

namespace eprosima {
namespace fastdds {
namespace statistics {

detail::Locator_s to_statistics_type(eprosima::fastrtps::rtps::Locator_t l)
{
    return *reinterpret_cast<detail::Locator_s*>(&l);
}

detail::GUID_s to_statistics_type(eprosima::fastrtps::rtps::GUID_t g)
{
    return *reinterpret_cast<detail::GUID_s*>(&g);
}

} // statistics
} // fastdds
} // eprosima

StatisticsAncillary* StatisticsListenersImpl::get_aux_members() const
{
    return members_.get();
}

bool StatisticsListenersImpl::add_statistics_listener_impl(
        std::shared_ptr<fastdds::statistics::IListener> listener)
{
    if(!listener)
    {
        // avoid nullptr
        return false;
    }

    std::lock_guard<fastrtps::RecursiveTimedMutex> lock(get_statistics_mutex());

    // add the new listener
    return members_->listeners.insert(listener).second;
}

bool StatisticsListenersImpl::remove_statistics_listener_impl(
        std::shared_ptr<fastdds::statistics::IListener> listener)
{
    std::lock_guard<fastrtps::RecursiveTimedMutex> lock(get_statistics_mutex());

    if(!listener)
    {
        // avoid nullptr
        return false;
    }

    return 1 == members_->listeners.erase(listener);
}

std::recursive_mutex& StatisticsParticipantImpl::get_statistics_mutex()
{
    static_assert(
            std::is_base_of<StatisticsParticipantImpl, RTPSParticipantImpl>::value,
            "This must be called from RTPSParticipantImpl");

    return *static_cast<RTPSParticipantImpl*>(this)->getParticipantMutex();
}

void StatisticsParticipantImpl::ListenerProxy::on_statistics_data(const Data& data)
{
    // only delegate if the mask matches
    if ( mask_ & data._d() )
    {
        external_->on_statistics_data(data);
    }
}

bool StatisticsParticipantImpl::ListenerProxy::operator<(const ListenerProxy& right) const
{
    return external_ < right.external_;
}

uint32_t StatisticsParticipantImpl::ListenerProxy::mask() const
{
    return mask_;
}

void StatisticsParticipantImpl::ListenerProxy::mask(uint32_t update) const
{
    mask_ = update;
}

bool StatisticsParticipantImpl::are_datawriters_involved(const uint32_t mask) const
{
    using namespace fastdds::statistics;

    constexpr uint32_t writers_maks = HISTORY2HISTORY_LATENCY \
        | PUBLICATION_THROUGHPUT \
        | RESENT_DATAS \
        | HEARTBEAT_COUNT \
        | DATA_COUNT;

    return writers_maks & mask;
}

bool StatisticsParticipantImpl::are_datareaders_involved(const uint32_t mask) const
{
    using namespace fastdds::statistics;

    constexpr uint32_t readers_maks = HISTORY2HISTORY_LATENCY \
        | SUBSCRIPTION_THROUGHPUT \
        | ACKNACK_COUNT \
        | NACKFRAG_COUNT \
        | GAP_COUNT;

    return readers_maks & mask;
}

bool StatisticsParticipantImpl::add_statistics_listener(
        std::shared_ptr<fastdds::statistics::IListener> listener,
        fastdds::statistics::EventKind kind)
{
    std::lock_guard<std::recursive_mutex> lock(get_statistics_mutex());

    uint32_t mask = kind, new_mask, old_mask;

    if(!listener || 0 == mask)
    {
        // avoid nullptr
        return false;
    }

    // add the new listener, and identify selection changes
    auto res = listeners_.emplace(std::make_shared<ListenerProxy>(listener, mask));
    const ListenerProxy& proxy = **res.first;

    if (res.second)
    {
        new_mask = mask;
        old_mask = 0;
    }
    else
    {
        old_mask = proxy.mask();
        new_mask = old_mask | mask;

        if( old_mask == new_mask )
        {
            // nop
            return false;
        }

        proxy.mask(new_mask);
    }

    // Check if the listener should be register in the writers
    bool writers_res = true;
    if (are_datawriters_involved(new_mask)
            && !are_datawriters_involved(old_mask))
    {
        writers_res = register_in_datawriter(proxy.get_shared_ptr());
    }

    // Check if the listener should be register in the writers
    bool readers_res = true;
    if (are_datareaders_involved(new_mask)
            && !are_datareaders_involved(old_mask))
    {
        readers_res = register_in_datareader(proxy.get_shared_ptr());
    }

    return writers_res && readers_res;
}

bool StatisticsParticipantImpl::remove_statistics_listener(
        std::shared_ptr<fastdds::statistics::IListener> listener,
        fastdds::statistics::EventKind kind)
{
    using namespace std;

    std::lock_guard<std::recursive_mutex> lock(get_statistics_mutex());

    uint32_t mask = kind, new_mask, old_mask;

    if(!listener || 0 == mask)
    {
        // avoid nullptr
        return false;
    }

    ProxyCollection::iterator it;
    auto proxy = make_shared<ListenerProxy>(listener, mask);
    it = listeners_.find(proxy);

    if ( listeners_.end() == it )
    {
        // not registered
        return false;
    }

    // Check where we must unregister
    proxy = *it;
    old_mask = proxy->mask();
    new_mask = old_mask & ~mask;

    if (old_mask == new_mask )
    {
        // nop
        return false;
    }

    if ( new_mask )
    {
        // update
        proxy->mask(new_mask);
    }
    else
    {
        // remove
        listeners_.erase(it);
    }

    bool writers_res = true;
    if (!are_datawriters_involved(new_mask)
            && are_datawriters_involved(old_mask))
    {
        writers_res = unregister_in_datawriter(proxy->get_shared_ptr());
    }

    bool readers_res = true;
    if (!are_datareaders_involved(new_mask)
            && are_datareaders_involved(old_mask))
    {
        readers_res = unregister_in_datareader(proxy->get_shared_ptr());
    }

    return writers_res && readers_res
        && ((old_mask & mask) == mask); // return false if there were unregistered entities
}

void StatisticsParticipantImpl::on_rtps_sent(
        const fastrtps::rtps::Locator_t & loc,
        unsigned long payload_size)
{
    using namespace std;
    using eprosima::fastrtps::rtps::RTPSParticipantImpl;

    static_assert(
            std::is_base_of<StatisticsParticipantImpl, RTPSParticipantImpl>::value,
            "This must be called from RTPSParticipantImpl");

    // Compose callback and update the inner state
    Entity2LocatorTraffic notification;
    notification.src_guid(to_statistics_type(static_cast<RTPSParticipantImpl*>(this)->getGuid()));
    notification.dst_locator(to_statistics_type(loc));

    {
        std::lock_guard<std::recursive_mutex> lock(get_statistics_mutex());

        auto & val = traffic[loc];
        notification.packet_count(++val.packet_count);
        notification.byte_count(val.byte_count += payload_size);
        notification.byte_magnitude_order((int16_t)floor(log10(float(val.byte_count))));
    }

    // Callback
    Data d;
    // note that the setter sets RTPS_SENT by default
    d.entity2locator_traffic(notification);

    for_each_listener([&d](const Key& l)
            {
                l->on_statistics_data(d);
            });
}
