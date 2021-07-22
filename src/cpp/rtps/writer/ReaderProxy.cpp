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
 * @file ReaderProxy.cpp
 *
 */


#include <fastdds/dds/log/Log.hpp>
#include <fastdds/rtps/history/WriterHistory.h>
#include <fastdds/rtps/writer/ReaderProxy.h>
#include <fastdds/rtps/writer/StatefulWriter.h>
#include <fastdds/rtps/resources/TimedEvent.h>
#include <fastrtps/utils/TimeConversion.h>
#include <fastdds/rtps/common/LocatorListComparisons.hpp>

#include <rtps/participant/RTPSParticipantImpl.h>
#include <rtps/history/HistoryAttributesExtension.hpp>

#include "rtps/messages/RTPSGapBuilder.hpp"
#include <rtps/DataSharing/DataSharingNotifier.hpp>

#include <mutex>
#include <cassert>
#include <algorithm>

namespace eprosima {
namespace fastrtps {
namespace rtps {

ReaderProxy::ReaderProxy(
        const WriterTimes& times,
        const RemoteLocatorsAllocationAttributes& loc_alloc,
        StatefulWriter* writer)
    : is_active_(false)
    , locator_info_(
        writer, loc_alloc.max_unicast_locators,
        loc_alloc.max_multicast_locators)
    , durability_kind_(VOLATILE)
    , expects_inline_qos_(false)
    , is_reliable_(false)
    , disable_positive_acks_(false)
    , writer_(writer)
    , changes_for_reader_(resource_limits_from_history(writer->mp_history->m_att, 0))
    , nack_supression_event_(nullptr)
    , initial_heartbeat_event_(nullptr)
    , timers_enabled_(false)
    , last_acknack_count_(0)
    , last_nackfrag_count_(0)
{
    nack_supression_event_ = new TimedEvent(writer_->getRTPSParticipant()->getEventResource(),
                    [&]() -> bool
                    {
                        writer_->perform_nack_supression(guid());
                        return false;
                    },
                    TimeConv::Time_t2MilliSecondsDouble(times.nackSupressionDuration));

    initial_heartbeat_event_ = new TimedEvent(writer_->getRTPSParticipant()->getEventResource(),
                    [&]() -> bool
                    {
                        writer_->intraprocess_heartbeat(this);
                        return false;
                    }, 0);

    stop();
}

bool ReaderProxy::rtps_is_relevant(
        CacheChange_t* change) const
{
    if (nullptr != writer_->reader_data_filter())
    {
        bool ret = writer_->reader_data_filter()->is_relevant(*change, guid());
        logInfo(RTPS_READER_PROXY,
                "Change " << change->instanceHandle << " is relevant for reader " << guid() << "? " << ret);
        return ret;
    }
    return true;
}

ReaderProxy::~ReaderProxy()
{
    if (nack_supression_event_)
    {
        delete(nack_supression_event_);
        nack_supression_event_ = nullptr;
    }

    if (initial_heartbeat_event_)
    {
        delete(initial_heartbeat_event_);
        initial_heartbeat_event_ = nullptr;
    }
}

void ReaderProxy::start(
        const ReaderProxyData& reader_attributes,
        bool is_datasharing)
{
    locator_info_.start(
        reader_attributes.guid(),
        reader_attributes.remote_locators().unicast,
        reader_attributes.remote_locators().multicast,
        reader_attributes.m_expectsInlineQos,
        is_datasharing);

    is_active_ = true;
    durability_kind_ = reader_attributes.m_qos.m_durability.durabilityKind();
    expects_inline_qos_ = reader_attributes.m_expectsInlineQos;
    is_reliable_ = reader_attributes.m_qos.m_reliability.kind != BEST_EFFORT_RELIABILITY_QOS;
    disable_positive_acks_ = reader_attributes.disable_positive_acks();
    if (durability_kind_ == DurabilityKind_t::VOLATILE)
    {
        SequenceNumber_t min_sequence = writer_->get_seq_num_min();
        changes_low_mark_ = (min_sequence == SequenceNumber_t::unknown()) ?
                writer_->next_sequence_number() - 1 : min_sequence - 1;
    }
    else
    {
        acked_changes_set(SequenceNumber_t());  // Simulate initial acknack to set low mark
    }

    timers_enabled_.store(is_remote_and_reliable());
    if (is_local_reader())
    {
        initial_heartbeat_event_->restart_timer();
    }

    logInfo(RTPS_READER_PROXY, "Reader Proxy started");
}

bool ReaderProxy::update(
        const ReaderProxyData& reader_attributes)
{
    durability_kind_ = reader_attributes.m_qos.m_durability.durabilityKind();
    expects_inline_qos_ = reader_attributes.m_expectsInlineQos;
    is_reliable_ = reader_attributes.m_qos.m_reliability.kind != BEST_EFFORT_RELIABILITY_QOS;
    disable_positive_acks_ = reader_attributes.disable_positive_acks();

    locator_info_.update(
        reader_attributes.remote_locators().unicast,
        reader_attributes.remote_locators().multicast,
        reader_attributes.m_expectsInlineQos);

    return true;
}

void ReaderProxy::stop()
{
    locator_info_.stop();
    is_active_ = false;
    disable_timers();

    changes_for_reader_.clear();
    last_acknack_count_ = 0;
    last_nackfrag_count_ = 0;
    changes_low_mark_ = SequenceNumber_t();
}

void ReaderProxy::disable_timers()
{
    if (timers_enabled_.exchange(false))
    {
        nack_supression_event_->cancel_timer();
    }
    initial_heartbeat_event_->cancel_timer();
}

void ReaderProxy::update_nack_supression_interval(
        const Duration_t& interval)
{
    nack_supression_event_->update_interval(interval);
}

void ReaderProxy::add_change(
        const ChangeForReader_t& change,
        bool is_relevant,
        bool restart_nack_supression)
{
    if (restart_nack_supression && timers_enabled_.load())
    {
        nack_supression_event_->restart_timer();
    }

    add_change(change, is_relevant);
}

void ReaderProxy::add_change(
        const ChangeForReader_t& change,
        bool is_relevant,
        bool restart_nack_supression,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    if (restart_nack_supression && timers_enabled_)
    {
        nack_supression_event_->restart_timer(max_blocking_time);
    }

    add_change(change, is_relevant);
}

void ReaderProxy::add_change(
        const ChangeForReader_t& change,
        bool is_relevant)
{
    assert(change.getSequenceNumber() > changes_low_mark_);
    assert(changes_for_reader_.empty() ? true :
            change.getSequenceNumber() > changes_for_reader_.back().getSequenceNumber());

    // Irrelevant changes are not added to the collection
    if (!is_relevant)
    {
        if ( !is_reliable_ &&
                changes_low_mark_ + 1 == change.getSequenceNumber())
        {
            changes_low_mark_ = change.getSequenceNumber();
        }
        return;
    }

    if (changes_for_reader_.push_back(change) == nullptr)
    {
        // This should never happen
        logError(RTPS_READER_PROXY, "Error adding change " << change.getSequenceNumber()
                                                           << " to reader proxy " << guid());
        eprosima::fastdds::dds::Log::Flush();
        assert(false);
    }
}

bool ReaderProxy::has_changes() const
{
    return !changes_for_reader_.empty();
}

bool ReaderProxy::change_is_acked(
        const SequenceNumber_t& seq_num) const
{
    if (seq_num <= changes_low_mark_ || changes_for_reader_.empty())
    {
        return true;
    }

    ChangeConstIterator chit = find_change(seq_num);
    if (chit == changes_for_reader_.end())
    {
        // There is a hole in changes_for_reader_
        // This means a change was removed, or was not relevant.
        return true;
    }

    return chit->getStatus() == ACKNOWLEDGED;
}

bool ReaderProxy::change_is_unsent(
        const SequenceNumber_t& seq_num,
        FragmentNumber_t& next_unsent_frag,
        SequenceNumber_t& gap_seq,
        bool& need_reactivate_periodic_heartbeat) const
{
    if (seq_num <= changes_low_mark_ || changes_for_reader_.empty())
    {
        return false;
    }

    ChangeConstIterator chit = find_change(seq_num);
    if (chit == changes_for_reader_.end())
    {
        // There is a hole in changes_for_reader_
        // This means a change was removed.
        return false;
    }

    bool returned_value = chit->getStatus() == UNSENT;

    if (returned_value)
    {
        next_unsent_frag = chit->get_next_unsent_fragment();
        gap_seq = SequenceNumber_t::unknown();

        if (is_reliable_ && !chit->has_been_delivered())
        {
            need_reactivate_periodic_heartbeat |= true;
            SequenceNumber_t prev =
                    (changes_for_reader_.begin() != chit ?
                    std::prev(chit)->getSequenceNumber() :
                    changes_low_mark_
                    ) + 1;

            if (prev != chit->getSequenceNumber())
            {
                gap_seq = prev;
            }
        }
    }

    return returned_value;
}

void ReaderProxy::acked_changes_set(
        const SequenceNumber_t& seq_num)
{
    SequenceNumber_t future_low_mark = seq_num;

    if (seq_num > changes_low_mark_)
    {
        ChangeIterator chit = find_change(seq_num, false);
        // continue advancing until next change is not acknowledged
        while (chit != changes_for_reader_.end()
                && chit->getSequenceNumber() == future_low_mark
                && chit->getStatus() == ACKNOWLEDGED)
        {
            ++chit;
            ++future_low_mark;
        }
        changes_for_reader_.erase(changes_for_reader_.begin(), chit);
    }
    else
    {
        future_low_mark = changes_low_mark_ + 1;

        if (seq_num == SequenceNumber_t() && durability_kind_ != DurabilityKind_t::VOLATILE)
        {
            // Special case. Currently only used on Builtin StatefulWriters
            // after losing lease duration, and on late joiners to set
            // changes_low_mark_ to match that of the writer.
            SequenceNumber_t min_sequence = writer_->get_seq_num_min();
            if (min_sequence != SequenceNumber_t::unknown())
            {
                SequenceNumber_t current_sequence = seq_num;
                if (seq_num < min_sequence)
                {
                    current_sequence = min_sequence;
                }
                future_low_mark = current_sequence;

                bool should_sort = false;
                for (; current_sequence <= changes_low_mark_; ++current_sequence)
                {
                    // Skip all consecutive changes already in the collection
                    ChangeConstIterator it = find_change(current_sequence);
                    while ( it != changes_for_reader_.end() &&
                            current_sequence <= changes_low_mark_ &&
                            it->getSequenceNumber() == current_sequence)
                    {
                        ++current_sequence;
                        ++it;
                    }

                    if (current_sequence <= changes_low_mark_)
                    {
                        CacheChange_t* change = nullptr;
                        if (writer_->mp_history->get_change(current_sequence, writer_->getGuid(), &change))
                        {
                            should_sort = true;
                            ChangeForReader_t cr(change);
                            cr.setStatus(UNACKNOWLEDGED);
                            changes_for_reader_.push_back(cr);
                        }
                    }
                }
                // Keep changes sorted by sequence number
                if (should_sort)
                {
                    std::sort(changes_for_reader_.begin(), changes_for_reader_.end(), ChangeForReaderCmp());
                }
            }
            else if (!is_local_reader())
            {
                future_low_mark = writer_->next_sequence_number();
            }
        }
    }
    changes_low_mark_ = future_low_mark - 1;
}

bool ReaderProxy::requested_changes_set(
        const SequenceNumberSet_t& seq_num_set,
        RTPSGapBuilder& gap_builder)
{
    bool isSomeoneWasSetRequested = false;

    seq_num_set.for_each([&](SequenceNumber_t sit)
            {
                ChangeIterator chit = find_change(sit, true);
                if (chit != changes_for_reader_.end())
                {
                    if (UNACKNOWLEDGED == chit->getStatus())
                    {
                        chit->setStatus(REQUESTED);
                        chit->markAllFragmentsAsUnsent();
                        isSomeoneWasSetRequested = true;
                    }
                }
                else if (sit > changes_low_mark_)
                {
                    gap_builder.add(sit);
                }
            });

    if (isSomeoneWasSetRequested)
    {
        logInfo(RTPS_READER_PROXY, "Requested Changes: " << seq_num_set);
    }

    return isSomeoneWasSetRequested;
}

bool ReaderProxy::process_initial_acknack(
        const std::function<void(ChangeForReader_t& change)>& func)
{
    if (is_local_reader())
    {
        return 0 != convert_status_on_all_changes(UNACKNOWLEDGED, UNSENT, func);
    }

    return true;
}

void ReaderProxy::from_unsent_to_status(
        const SequenceNumber_t& seq_num,
        ChangeForReaderStatus_t status,
        bool restart_nack_supression,
        bool delivered)
{
    // This function must not be called by a best-effort reader.
    // It will use acked_changes_set().
    assert(is_reliable_);

    if (restart_nack_supression && is_remote_and_reliable())
    {
        assert(timers_enabled_.load());
        nack_supression_event_->restart_timer();
    }

    // Called when delivering an UNSENT sample, the seq_number must exists in the ReaderProxy.
    assert(seq_num > changes_low_mark_);
    ChangeIterator it = find_change(seq_num, true);
    assert(changes_for_reader_.end() != it);
    assert(UNSENT == it->getStatus());
    assert(UNSENT != status);

    if (ACKNOWLEDGED == status && seq_num == changes_low_mark_ + 1)
    {
        assert(changes_for_reader_.begin() == it);
        changes_for_reader_.erase(it);
        changes_low_mark_ = seq_num;
        return;
    }

    it->setStatus(status);

    if (delivered)
    {
        it->set_delivered();
    }
}

bool ReaderProxy::mark_fragment_as_sent_for_change(
        const SequenceNumber_t& seq_num,
        FragmentNumber_t frag_num,
        bool& was_last_fragment)
{
    was_last_fragment = false;

    if (seq_num <= changes_low_mark_)
    {
        return false;
    }

    bool change_found = false;
    ChangeIterator it = find_change(seq_num, true);

    if (it != changes_for_reader_.end())
    {
        change_found = true;
        it->markFragmentsAsSent(frag_num);
        was_last_fragment = it->getUnsentFragments().empty();
    }

    return change_found;
}

bool ReaderProxy::perform_nack_supression()
{
    return 0 != convert_status_on_all_changes(UNDERWAY, UNACKNOWLEDGED);
}

uint32_t ReaderProxy::perform_acknack_response(
        const std::function<void(ChangeForReader_t& change)>& func)
{
    return convert_status_on_all_changes(REQUESTED, UNSENT, func);
}

uint32_t ReaderProxy::convert_status_on_all_changes(
        ChangeForReaderStatus_t previous,
        ChangeForReaderStatus_t next,
        const std::function<void(ChangeForReader_t& change)>& func)
{
    assert(previous > next);

    // NOTE: This is only called for REQUESTED=>UNSENT (acknack response) or
    //       UNDERWAY=>UNACKNOWLEDGED (nack supression)

    uint32_t changed = 0;
    for (ChangeForReader_t& change : changes_for_reader_)
    {
        if (change.getStatus() == previous)
        {
            ++changed;
            change.setStatus(next);

            if (func)
            {
                func(change);
            }
        }
    }

    return changed;
}

void ReaderProxy::change_has_been_removed(
        const SequenceNumber_t& seq_num)
{
    // Check sequence number is in the container, because it was not clean up.
    if (changes_for_reader_.empty() || seq_num < changes_for_reader_.begin()->getSequenceNumber())
    {
        return;
    }

    auto chit = find_change(seq_num);

    if (chit == this->changes_for_reader_.end())
    {
        // No change for this sequence number
        return;
    }

    // In intraprocess, if there is an UNACKNOWLEDGED, a GAP has to be send because there is no reliable mechanism.
    if (is_local_reader() && ACKNOWLEDGED > chit->getStatus())
    {
        writer_->intraprocess_gap(this, seq_num);
    }

    // Element may not be in the container when marked as irrelevant.
    changes_for_reader_.erase(chit);
}

bool ReaderProxy::has_unacknowledged() const
{
    for (const ChangeForReader_t& it : changes_for_reader_)
    {
        if (it.getStatus() == UNACKNOWLEDGED)
        {
            return true;
        }
    }

    return false;
}

bool ReaderProxy::requested_fragment_set(
        const SequenceNumber_t& seq_num,
        const FragmentNumberSet_t& frag_set)
{
    // Locate the outbound change referenced by the NACK_FRAG
    ChangeIterator changeIter = find_change(seq_num, true);
    if (changeIter == changes_for_reader_.end())
    {
        return false;
    }

    changeIter->markFragmentsAsUnsent(frag_set);

    // If it was UNSENT, we shouldn't switch back to REQUESTED to prevent stalling.
    if (changeIter->getStatus() != UNSENT)
    {
        changeIter->setStatus(REQUESTED);
    }

    return true;
}

bool ReaderProxy::process_nack_frag(
        const GUID_t& reader_guid,
        uint32_t nack_count,
        const SequenceNumber_t& seq_num,
        const FragmentNumberSet_t& fragments_state)
{
    if (guid() == reader_guid)
    {
        if (last_nackfrag_count_ < nack_count)
        {
            last_nackfrag_count_ = nack_count;
            if (requested_fragment_set(seq_num, fragments_state))
            {
                return true;
            }
        }
    }

    return false;
}

static bool change_less_than_sequence(
        const ChangeForReader_t& change,
        const SequenceNumber_t& seq_num)
{
    return change.getSequenceNumber() < seq_num;
}

ReaderProxy::ChangeIterator ReaderProxy::find_change(
        const SequenceNumber_t& seq_num,
        bool exact)
{
    ReaderProxy::ChangeIterator it;
    ReaderProxy::ChangeIterator end = changes_for_reader_.end();
    it = std::lower_bound(changes_for_reader_.begin(), end, seq_num, change_less_than_sequence);

    return (!exact)
           ? it
           : it == end
           ? it
           : it->getSequenceNumber() == seq_num ? it : end;
}

ReaderProxy::ChangeConstIterator ReaderProxy::find_change(
        const SequenceNumber_t& seq_num) const
{
    ReaderProxy::ChangeConstIterator it;
    ReaderProxy::ChangeConstIterator end = changes_for_reader_.end();
    it = std::lower_bound(changes_for_reader_.begin(), end, seq_num, change_less_than_sequence);

    return it == end
           ? it
           : it->getSequenceNumber() == seq_num ? it : end;
}

}   // namespace rtps
}   // namespace fastrtps
}   // namespace eprosima
