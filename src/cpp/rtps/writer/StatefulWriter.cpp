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
 * @file StatefulWriter.cpp
 *
 */

#include <fastdds/rtps/writer/StatefulWriter.h>
#include <fastdds/rtps/writer/WriterListener.h>
#include <fastdds/rtps/writer/IReaderDataFilter.hpp>
#include <fastdds/rtps/writer/ReaderProxy.h>
#include <fastdds/rtps/resources/AsyncWriterThread.h>

#include <rtps/participant/RTPSParticipantImpl.h>
#include <rtps/flowcontrol/FlowController.h>
#include <rtps/history/BasicPayloadPool.hpp>
#include <rtps/DataSharing/DataSharingPayloadPool.hpp>
#include <rtps/DataSharing/DataSharingNotifier.hpp>
#include <rtps/DataSharing/WriterPool.hpp>

#include <fastdds/rtps/messages/RTPSMessageCreator.h>
#include <fastdds/rtps/messages/RTPSMessageGroup.h>

#include <fastdds/rtps/participant/RTPSParticipant.h>
#include <fastdds/rtps/resources/ResourceEvent.h>
#include <fastdds/rtps/resources/TimedEvent.h>

#include <fastdds/rtps/history/WriterHistory.h>

#include <fastdds/dds/log/Log.hpp>
#include <fastrtps/utils/TimeConversion.h>

#include <fastdds/rtps/builtin/BuiltinProtocols.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>

#include <rtps/RTPSDomainImpl.hpp>
#include <rtps/history/CacheChangePool.h>
#include <rtps/messages/RTPSGapBuilder.hpp>
#include <rtps/writer/RTPSWriterCollector.h>

#include "../builtin/discovery/database/DiscoveryDataBase.hpp"

#include <mutex>
#include <vector>
#include <stdexcept>

namespace eprosima {
namespace fastrtps {
namespace rtps {


/**
 * Loops over all the readers in the vector, applying the given routine.
 * The loop continues until the result of the routine is true for any reader
 * or all readers have been processes.
 * The returned value is true if the routine returned true at any point,
 * or false otherwise.
 */
bool for_matched_readers(
        ResourceLimitedVector<ReaderProxy*>& reader_vector_1,
        std::function<bool(ReaderProxy*)> fun)
{
    for (ReaderProxy* remote_reader : reader_vector_1)
    {
        if (fun(remote_reader))
        {
            return true;
        }
    }

    return false;
}

bool for_matched_readers(
        ResourceLimitedVector<ReaderProxy*>& reader_vector_1,
        ResourceLimitedVector<ReaderProxy*>& reader_vector_2,
        std::function<bool(ReaderProxy*)> fun)
{
    if (for_matched_readers(reader_vector_1, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_2, fun);
}

bool for_matched_readers(
        ResourceLimitedVector<ReaderProxy*>& reader_vector_1,
        ResourceLimitedVector<ReaderProxy*>& reader_vector_2,
        ResourceLimitedVector<ReaderProxy*>& reader_vector_3,
        std::function<bool(ReaderProxy*)> fun)
{
    if (for_matched_readers(reader_vector_1, reader_vector_2, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_3, fun);
}

/**
 * Loops over all the readers in the vector, applying the given routine.
 * The loop continues until the result of the routine is true for any reader
 * or all readers have been processes.
 * The returned value is true if the routine returned true at any point,
 * or false otherwise.
 *
 * const version
 */
bool for_matched_readers(
        const ResourceLimitedVector<ReaderProxy*>& reader_vector_1,
        std::function<bool(const ReaderProxy*)> fun)
{
    for (const ReaderProxy* remote_reader : reader_vector_1)
    {
        if (fun(remote_reader))
        {
            return true;
        }
    }

    return false;
}

bool for_matched_readers(
        const ResourceLimitedVector<ReaderProxy*>& reader_vector_1,
        const ResourceLimitedVector<ReaderProxy*>& reader_vector_2,
        std::function<bool(const ReaderProxy*)> fun)
{
    if (for_matched_readers(reader_vector_1, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_2, fun);
}

bool for_matched_readers(
        const ResourceLimitedVector<ReaderProxy*>& reader_vector_1,
        const ResourceLimitedVector<ReaderProxy*>& reader_vector_2,
        const ResourceLimitedVector<ReaderProxy*>& reader_vector_3,
        std::function<bool(const ReaderProxy*)> fun)
{
    if (for_matched_readers(reader_vector_1, reader_vector_2, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_3, fun);
}

using namespace std::chrono;

StatefulWriter::StatefulWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        WriterHistory* history,
        WriterListener* listener)
    : RTPSWriter(pimpl, guid, att, history, listener)
    , periodic_hb_event_(nullptr)
    , nack_response_event_(nullptr)
    , ack_event_(nullptr)
    , m_heartbeatCount(0)
    , m_times(att.times)
    , matched_remote_readers_(att.matched_readers_allocation)
    , matched_readers_pool_(att.matched_readers_allocation)
    , next_all_acked_notify_sequence_(0, 1)
    , all_acked_(false)
    , may_remove_change_cond_()
    , may_remove_change_(0)
    , disable_heartbeat_piggyback_(att.disable_heartbeat_piggyback)
    , disable_positive_acks_(att.disable_positive_acks)
    , keep_duration_us_(att.keep_duration.to_ns() * 1e-3)
    , last_sequence_number_()
    , biggest_removed_sequence_number_()
    , sendBufferSize_(pimpl->get_min_network_send_buffer_size())
    , currentUsageSendBufferSize_(static_cast<int32_t>(pimpl->get_min_network_send_buffer_size()))
    , m_controllers()
    , matched_local_readers_(att.matched_readers_allocation)
    , matched_datasharing_readers_(att.matched_readers_allocation)
{
    init(pimpl, att);
}

StatefulWriter::StatefulWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        WriterHistory* history,
        WriterListener* listener)
    : RTPSWriter(pimpl, guid, att, payload_pool, history, listener)
    , periodic_hb_event_(nullptr)
    , nack_response_event_(nullptr)
    , ack_event_(nullptr)
    , m_heartbeatCount(0)
    , m_times(att.times)
    , matched_remote_readers_(att.matched_readers_allocation)
    , matched_readers_pool_(att.matched_readers_allocation)
    , next_all_acked_notify_sequence_(0, 1)
    , all_acked_(false)
    , may_remove_change_cond_()
    , may_remove_change_(0)
    , disable_heartbeat_piggyback_(att.disable_heartbeat_piggyback)
    , disable_positive_acks_(att.disable_positive_acks)
    , keep_duration_us_(att.keep_duration.to_ns() * 1e-3)
    , last_sequence_number_()
    , biggest_removed_sequence_number_()
    , sendBufferSize_(pimpl->get_min_network_send_buffer_size())
    , currentUsageSendBufferSize_(static_cast<int32_t>(pimpl->get_min_network_send_buffer_size()))
    , m_controllers()
    , matched_local_readers_(att.matched_readers_allocation)
    , matched_datasharing_readers_(att.matched_readers_allocation)
{
    init(pimpl, att);
}

StatefulWriter::StatefulWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        WriterHistory* hist,
        WriterListener* listen)
    : RTPSWriter(pimpl, guid, att, payload_pool, change_pool, hist, listen)
    , periodic_hb_event_(nullptr)
    , nack_response_event_(nullptr)
    , ack_event_(nullptr)
    , m_heartbeatCount(0)
    , m_times(att.times)
    , matched_remote_readers_(att.matched_readers_allocation)
    , matched_readers_pool_(att.matched_readers_allocation)
    , next_all_acked_notify_sequence_(0, 1)
    , all_acked_(false)
    , may_remove_change_cond_()
    , may_remove_change_(0)
    , disable_heartbeat_piggyback_(att.disable_heartbeat_piggyback)
    , disable_positive_acks_(att.disable_positive_acks)
    , keep_duration_us_(att.keep_duration.to_ns() * 1e-3)
    , last_sequence_number_()
    , biggest_removed_sequence_number_()
    , sendBufferSize_(pimpl->get_min_network_send_buffer_size())
    , currentUsageSendBufferSize_(static_cast<int32_t>(pimpl->get_min_network_send_buffer_size()))
    , m_controllers()
    , matched_local_readers_(att.matched_readers_allocation)
    , matched_datasharing_readers_(att.matched_readers_allocation)
{
    init(pimpl, att);
}

void StatefulWriter::init(
        RTPSParticipantImpl* pimpl,
        const WriterAttributes& att)
{
    const RTPSParticipantAttributes& part_att = pimpl->getRTPSParticipantAttributes();

    auto push_mode = PropertyPolicyHelper::find_property(att.endpoint.properties, "fastdds.push_mode");
    m_pushMode = !((nullptr != push_mode) && ("false" == *push_mode));

    periodic_hb_event_ = new TimedEvent(
        pimpl->getEventResource(),
        [&]() -> bool
        {
            return send_periodic_heartbeat();
        },
        TimeConv::Time_t2MilliSecondsDouble(m_times.heartbeatPeriod));

    nack_response_event_ = new TimedEvent(
        pimpl->getEventResource(),
        [&]() -> bool
        {
            perform_nack_response();
            return false;
        },
        TimeConv::Time_t2MilliSecondsDouble(m_times.nackResponseDelay));

    if (disable_positive_acks_)
    {
        ack_event_ = new TimedEvent(
            pimpl->getEventResource(),
            [&]() -> bool
            {
                return ack_timer_expired();
            },
            att.keep_duration.to_ns() * 1e-6);             // in milliseconds
    }

    for (size_t n = 0; n < att.matched_readers_allocation.initial; ++n)
    {
        matched_readers_pool_.push_back(new ReaderProxy(m_times, part_att.allocation.locators, this));
    }
}

StatefulWriter::~StatefulWriter()
{
    logInfo(RTPS_WRITER, "StatefulWriter destructor");

    for (std::unique_ptr<FlowController>& controller : m_controllers)
    {
        controller->disable();
    }

    if (disable_positive_acks_)
    {
        delete(ack_event_);
        ack_event_ = nullptr;
    }

    if (nack_response_event_ != nullptr)
    {
        delete(nack_response_event_);
        nack_response_event_ = nullptr;
    }

    mp_RTPSParticipant->async_thread().unregister_writer(this);

    // After unregistering writer from AsyncWriterThread, delete all flow_controllers because they register the writer in
    // the AsyncWriterThread.
    m_controllers.clear();

    // Stop all active proxies and pass them to the pool
    {
        std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
        while (!matched_remote_readers_.empty())
        {
            ReaderProxy* remote_reader = matched_remote_readers_.back();
            matched_remote_readers_.pop_back();
            remote_reader->stop();
            matched_readers_pool_.push_back(remote_reader);
        }
        while (!matched_local_readers_.empty())
        {
            ReaderProxy* remote_reader = matched_local_readers_.back();
            matched_local_readers_.pop_back();
            remote_reader->stop();
            matched_readers_pool_.push_back(remote_reader);
        }
        while (!matched_datasharing_readers_.empty())
        {
            ReaderProxy* remote_reader = matched_datasharing_readers_.back();
            matched_datasharing_readers_.pop_back();
            remote_reader->stop();
            matched_readers_pool_.push_back(remote_reader);
        }
    }

    // Destroy heartbeat event
    if (periodic_hb_event_ != nullptr)
    {
        delete(periodic_hb_event_);
        periodic_hb_event_ = nullptr;
    }

    // Delete all proxies in the pool
    for (ReaderProxy* remote_reader : matched_readers_pool_)
    {
        delete(remote_reader);
    }

    // TODO [ILG] Should we force this on all cases?
    if (is_datasharing_compatible())
    {
        //Release payloads orderly
        for (std::vector<CacheChange_t*>::iterator chit = mp_history->changesBegin();
                chit != mp_history->changesEnd(); ++chit)
        {
            IPayloadPool* pool = (*chit)->payload_owner();
            if (pool)
            {
                pool->release_payload(**chit);
            }
        }
    }
}

/*
 * CHANGE-RELATED METHODS
 */
void StatefulWriter::prepare_datasharing_delivery(
        CacheChange_t* change)
{
    auto pool = std::dynamic_pointer_cast<WriterPool>(payload_pool_);
    assert (pool != nullptr);

    pool->add_to_shared_history(change);
    logInfo(RTPS_WRITER, "Notifying readers of cache change with SN " << change->sequenceNumber);
}

void StatefulWriter::async_delivery(
        CacheChange_t* change,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    bool should_wake_up = false;

    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [this, &should_wake_up, &change, &max_blocking_time](ReaderProxy* reader)
            {
                ChangeForReader_t changeForReader(change);

                if (m_pushMode || !reader->is_reliable() || reader->is_local_reader())
                {
                    changeForReader.setStatus(UNSENT);
                    should_wake_up = true;
                }
                else
                {
                    changeForReader.setStatus(UNACKNOWLEDGED);
                }
                changeForReader.setRelevance(reader->rtps_is_relevant(change));
                reader->add_change(changeForReader, false, max_blocking_time);

                return false;
            }
            );

    if (should_wake_up)
    {
        mp_RTPSParticipant->async_thread().wake_up(this, max_blocking_time);
    }
    else
    {
        periodic_hb_event_->restart_timer(max_blocking_time);
    }
}

void StatefulWriter::sync_delivery(
        CacheChange_t* change,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    //TODO(Ricardo) Temporal.
    bool expectsInlineQos = false;

    bool should_be_sent = false;
    locator_selector_.reset(false);

    // First step is to add the new CacheChange_t to all reader proxies.
    // It has to be done before sending, because if a timeout is caught, we will not include the
    // CacheChange_t in some reader proxies.
    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [this, &change, &max_blocking_time, &expectsInlineQos, &should_be_sent](ReaderProxy* reader)
            {
                ChangeForReader_t changeForReader(change);
                bool send_to_this_reader = false;

                if (!reader->is_reliable())
                {
                    changeForReader.setStatus(ACKNOWLEDGED);
                    send_to_this_reader = true;
                }
                else
                {
                    if (m_pushMode || reader->is_local_reader())
                    {
                        changeForReader.setStatus(UNDERWAY);
                        send_to_this_reader = true;
                    }
                    else
                    {
                        changeForReader.setStatus(UNACKNOWLEDGED);
                    }
                }

                changeForReader.setRelevance(reader->rtps_is_relevant(change));
                reader->add_change(changeForReader, true, max_blocking_time);
                expectsInlineQos |= reader->expects_inline_qos();

                if (send_to_this_reader)
                {
                    if (reader->is_local_reader())
                    {
                        intraprocess_heartbeat(reader, false);
                        bool delivered = !changeForReader.isRelevant() || intraprocess_delivery(change, reader);
                        reader->set_change_to_status(
                            change->sequenceNumber,
                            delivered ? ACKNOWLEDGED : UNDERWAY,
                            false);
                    }
                    else if (reader->is_datasharing_reader())
                    {
                        reader->datasharing_notify();
                        reader->set_change_to_status(change->sequenceNumber, UNDERWAY, false);
                    }
                    else
                    {
                        should_be_sent = true;
                        locator_selector_.enable(reader->guid());
                    }
                }

                return false;
            }
            );

    //At this point we are sure all information was stored. We now can send data.
    if (!should_be_sent)
    {
        if (getMatchedReadersSize() > 0)
        {
            periodic_hb_event_->restart_timer(max_blocking_time);
        }
    }
    else
    {
        try
        {
            if (!m_separateSendingEnabled)
            {
                size_t num_locators = locator_selector_.selected_size();
                if (num_locators > 0)
                {
                    NetworkFactory& network = mp_RTPSParticipant->network_factory();
                    network.select_locators(locator_selector_);
                    compute_selected_guids();

                    RTPSMessageGroup group(mp_RTPSParticipant, this, *this, max_blocking_time);

                    auto sent_fun = [this, num_locators](
                        CacheChange_t* change,
                        FragmentNumber_t frag)
                            {
                                add_statistics_sent_submessage(change, num_locators);
                                if (frag > 0)
                                {
                                    for (ReaderProxy* it : matched_remote_readers_)
                                    {
                                        bool allFragmentsSent = false;
                                        it->mark_fragment_as_sent_for_change(
                                            change->sequenceNumber,
                                            frag,
                                            allFragmentsSent);
                                    }
                                }
                            };

                    send_data_or_fragments(group, change, expectsInlineQos, sent_fun);
                    send_heartbeat_nts_(all_remote_readers_.size(), group, disable_positive_acks_);
                }
            }
            else
            {
                for (ReaderProxy* it : matched_remote_readers_)
                {
                    RTPSMessageGroup group(mp_RTPSParticipant, this, it->message_sender(),
                            max_blocking_time);

                    if (change->getFragmentCount() > 0)
                    {
                        logError(RTPS_WRITER, "Cannot send large messages on separate sending mode");
                    }
                    else
                    {
                        if (!group.add_data(*change, it->expects_inline_qos()))
                        {
                            logError(RTPS_WRITER, "Error sending change " << change->sequenceNumber);
                        }
                        else
                        {
                            add_statistics_sent_submessage(change, it->locators_size());
                        }
                    }
                    uint32_t last_processed = 0;
                    send_heartbeat_piggyback_nts_(it, group, last_processed);
                }
            }

            on_sample_datas(change->write_params.sample_identity(), change->num_sent_submessages);
            periodic_hb_event_->restart_timer(max_blocking_time);

            if (disable_positive_acks_ && last_sequence_number_ == SequenceNumber_t())
            {
                last_sequence_number_ = change->sequenceNumber;
            }
        }
        catch (const RTPSMessageGroup::timeout&)
        {
            logError(RTPS_WRITER, "Max blocking time reached");
        }
    }

    if (!m_pushMode)
    {
        NetworkFactory& network = mp_RTPSParticipant->network_factory();
        locator_selector_.reset(true);
        network.select_locators(locator_selector_);
        compute_selected_guids();
    }

    check_acked_status();
}

void StatefulWriter::unsent_change_added_to_history(
        CacheChange_t* change,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    bool should_notify_data_sent = false;

    {
        std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

        if (liveliness_lease_duration_ < c_TimeInfinite)
        {
            mp_RTPSParticipant->wlp()->assert_liveliness(
                getGuid(),
                liveliness_kind_,
                liveliness_lease_duration_);
        }

        // Prepare the metadata for datasharing
        if (is_datasharing_compatible())
        {
            prepare_datasharing_delivery(change);
        }

        // Now for the rest of readers
        if (!matched_remote_readers_.empty() || !matched_datasharing_readers_.empty() ||
                !matched_local_readers_.empty())
        {
            if (!isAsync())
            {
                sync_delivery(change, max_blocking_time);
                should_notify_data_sent = true;
            }
            else
            {
                async_delivery(change, max_blocking_time);
            }

            if (disable_positive_acks_)
            {
                auto source_timestamp = system_clock::time_point() + nanoseconds(change->sourceTimestamp.to_ns());
                auto now = system_clock::now();
                auto interval = source_timestamp - now + keep_duration_us_;
                assert(interval.count() >= 0);

                ack_event_->update_interval_millisec((double)duration_cast<milliseconds>(interval).count());
                ack_event_->restart_timer(max_blocking_time);
            }
        }
        else
        {
            logInfo(RTPS_WRITER, "No reader proxy to add change.");
            check_acked_status();
        }
    }

    if (should_notify_data_sent)
    {
        on_data_sent();
    }

    // Throughput should be notified even if no matches are available
    on_publish_throughput(change->serializedPayload.length);
}

bool StatefulWriter::intraprocess_delivery(
        CacheChange_t* change,
        ReaderProxy* reader_proxy)
{
    RTPSReader* reader = reader_proxy->local_reader();
    if (reader)
    {
        if (change->write_params.related_sample_identity() != SampleIdentity::unknown())
        {
            change->write_params.sample_identity(change->write_params.related_sample_identity());
        }
        return reader->processDataMsg(change);
    }
    return false;
}

bool StatefulWriter::intraprocess_gap(
        ReaderProxy* reader_proxy,
        const SequenceNumber_t& seq_num)
{
    RTPSReader* reader = reader_proxy->local_reader();
    if (reader)
    {
        return reader->processGapMsg(m_guid, seq_num, SequenceNumberSet_t(seq_num + 1));
    }

    return false;
}

bool StatefulWriter::intraprocess_heartbeat(
        ReaderProxy* reader_proxy,
        bool liveliness)
{
    bool returned_value = false;

    std::lock_guard<RecursiveTimedMutex> guardW(mp_mutex);
    RTPSReader* reader = reader_proxy->local_reader();

    if (reader)
    {
        SequenceNumber_t first_seq = get_seq_num_min();
        SequenceNumber_t last_seq = get_seq_num_max();

        if (first_seq == c_SequenceNumber_Unknown || last_seq == c_SequenceNumber_Unknown)
        {
            if (liveliness)
            {
                first_seq = next_sequence_number();
                last_seq = first_seq - 1;
            }
        }

        if ((first_seq != c_SequenceNumber_Unknown && last_seq != c_SequenceNumber_Unknown) &&
                (liveliness || reader_proxy->has_changes()))
        {
            incrementHBCount();
            if (true == (returned_value =
                    reader->processHeartbeatMsg(m_guid, m_heartbeatCount, first_seq, last_seq, true, liveliness)))
            {
                if (reader_proxy->durability_kind() < TRANSIENT_LOCAL ||
                        this->getAttributes().durabilityKind < TRANSIENT_LOCAL)
                {
                    SequenceNumber_t first_relevant = reader_proxy->first_relevant_sequence_number();
                    if (first_seq < first_relevant)
                    {
                        reader->processGapMsg(m_guid, first_seq, SequenceNumberSet_t(first_relevant));
                    }
                }
            }
        }
    }

    return returned_value;
}

bool StatefulWriter::change_removed_by_history(
        CacheChange_t* a_change)
{
    SequenceNumber_t sequence_number = a_change->sequenceNumber;

    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    logInfo(RTPS_WRITER, "Change " << sequence_number << " to be removed.");

    // Take note of biggest removed sequence number to improve sending of gaps
    if (sequence_number > biggest_removed_sequence_number_)
    {
        biggest_removed_sequence_number_ = sequence_number;
    }

    // Invalidate CacheChange pointer in ReaderProxies.
    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [sequence_number](ReaderProxy* reader)
            {
                reader->change_has_been_removed(sequence_number);
                return false;
            }
            );

    // remove from datasharing pool history
    if (is_datasharing_compatible())
    {
        auto pool = std::dynamic_pointer_cast<WriterPool>(payload_pool_);
        assert (pool != nullptr);

        pool->remove_from_shared_history(a_change);
        logInfo(RTPS_WRITER, "Removing shared cache change with SN " << a_change->sequenceNumber);
    }

    may_remove_change_ = 2;
    may_remove_change_cond_.notify_one();

    return true;
}

void StatefulWriter::send_any_unsent_changes()
{
    {
        std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

        bool activateHeartbeatPeriod = false;
        SequenceNumber_t max_sequence = mp_history->next_sequence_number();

        if (mp_history->getHistorySize() == 0 || getMatchedReadersSize() == 0)
        {
            send_heartbeat_to_all_readers();
        }
        else if (m_separateSendingEnabled)
        {
            send_changes_separatedly(max_sequence, activateHeartbeatPeriod);
        }
        else
        {
            bool no_flow_controllers = m_controllers.empty() && mp_RTPSParticipant->getFlowControllers().empty();
            if (no_flow_controllers || !there_are_remote_readers_)
            {
                send_all_unsent_changes(max_sequence, activateHeartbeatPeriod);
            }
            else
            {
                send_unsent_changes_with_flow_control(max_sequence, activateHeartbeatPeriod);
            }
        }

        if (activateHeartbeatPeriod)
        {
            periodic_hb_event_->restart_timer();
        }

        // On VOLATILE writers, remove auto-acked (best effort readers) changes
        check_acked_status();
    }

    on_data_sent();

    logInfo(RTPS_WRITER, "Finish sending unsent changes");
}

void StatefulWriter::send_heartbeat_to_all_readers()
{
    // This version is called from send_any_unsent_changes when any of the following conditions is satisfied:
    // a) history is empty
    // b) there are no matched readers

    // It may also be called from send_periodic_heartbeat

    if (m_separateSendingEnabled)
    {
        for (ReaderProxy* reader : matched_remote_readers_)
        {
            send_heartbeat_to_nts(*reader);
        }
    }
    else
    {
        for (ReaderProxy* reader : matched_local_readers_)
        {
            intraprocess_heartbeat(reader);
        }

        for (ReaderProxy* reader : matched_datasharing_readers_)
        {
            reader->datasharing_notify();
        }

        if (there_are_remote_readers_)
        {
            RTPSMessageGroup group(mp_RTPSParticipant, this, *this);
            send_heartbeat_nts_(all_remote_readers_.size(), group, disable_positive_acks_);
        }
    }
}

void StatefulWriter::send_changes_separatedly(
        SequenceNumber_t max_sequence,
        bool& activateHeartbeatPeriod)
{
    // This version is called when all of the following conditions are satisfied:
    // a) history is not empty
    // b) there is at least one matched reader
    // c) separate sending is enabled

    // Process datasharing then
    if (there_are_datasharing_readers_)
    {
        send_all_datasharing_changes(max_sequence);
    }

    for (ReaderProxy* remoteReader : matched_local_readers_)
    {
        // If there are no changes for this reader, simply jump to the next one
        if (!remoteReader->has_changes())
        {
            continue;
        }

        SequenceNumber_t max_ack_seq = SequenceNumber_t::unknown();
        auto unsent_change_process =
                [&](const SequenceNumber_t& seqNum, const ChangeForReader_t* unsentChange)
                {
                    if (unsentChange != nullptr && unsentChange->isRelevant() && unsentChange->isValid())
                    {
                        if (intraprocess_delivery(unsentChange->getChange(), remoteReader))
                        {
                            max_ack_seq = seqNum;
                        }
                        else
                        {
                            remoteReader->set_change_to_status(seqNum, UNDERWAY, false);
                        }
                    }
                    else
                    {
                        if (intraprocess_gap(remoteReader, seqNum))
                        {
                            max_ack_seq = seqNum;
                        }
                        else
                        {
                            remoteReader->set_change_to_status(seqNum, UNDERWAY, true);
                        }
                    }
                };
        remoteReader->for_each_unsent_change(max_sequence, unsent_change_process);
        if (max_ack_seq != SequenceNumber_t::unknown())
        {
            remoteReader->acked_changes_set(max_ack_seq + 1);
        }
    }

    for (ReaderProxy* remoteReader : matched_remote_readers_)
    {
        // If there are no changes for this reader, simply jump to the next one
        if (!remoteReader->has_changes())
        {
            continue;
        }

        // Specific destination message group
        RTPSMessageGroup group(mp_RTPSParticipant, this, remoteReader->message_sender());
        SequenceNumber_t min_history_seq = get_seq_num_min();
        size_t num_locators = remoteReader->locators_size();
        if (remoteReader->is_reliable())
        {
            // Add a HEARTBEAT to the datagram with final flag set to false. This way, the reader must send an
            // ACKNACK message for each DATA that it receives.
            send_heartbeat_nts_(1u, group, false);

            RTPSGapBuilder gaps(group, remoteReader->guid());

            uint32_t lastBytesProcessed = 0;
            auto sent_fun = [this, num_locators, remoteReader, &lastBytesProcessed, &group](
                CacheChange_t* change,
                FragmentNumber_t /*frag*/)
                    {
                        add_statistics_sent_submessage(change, num_locators);
                        // Heartbeat piggyback.
                        send_heartbeat_piggyback_nts_(remoteReader, group, lastBytesProcessed);
                    };

            auto unsent_change_process =
                    [&](const SequenceNumber_t& seqNum, const ChangeForReader_t* unsentChange)
                    {
                        if (unsentChange != nullptr && unsentChange->isRelevant() && unsentChange->isValid())
                        {
                            auto change = unsentChange->getChange();
                            bool sent_ok = send_data_or_fragments(
                                group,
                                change,
                                remoteReader->expects_inline_qos(),
                                sent_fun);
                            on_sample_datas(change->write_params.sample_identity(), change->num_sent_submessages);
                            if (sent_ok)
                            {
                                remoteReader->set_change_to_status(seqNum, UNDERWAY, true);
                                activateHeartbeatPeriod = true;
                            }
                        }
                        else
                        {
                            if (seqNum >= min_history_seq)
                            {
                                gaps.add(seqNum);
                            }
                            remoteReader->set_change_to_status(seqNum, UNDERWAY, true);
                        }
                    };
            remoteReader->for_each_unsent_change(max_sequence, unsent_change_process);
        }
        else
        {
            SequenceNumber_t max_ack_seq = SequenceNumber_t::unknown();
            auto sent_fun = [this, num_locators](
                CacheChange_t* change,
                FragmentNumber_t /*frag*/)
                    {
                        add_statistics_sent_submessage(change, num_locators);
                    };
            auto unsent_change_process =
                    [&](const SequenceNumber_t& seqNum, const ChangeForReader_t* unsentChange)
                    {
                        if (unsentChange != nullptr &&  unsentChange->isRelevant() && unsentChange->isValid())
                        {
                            auto change = unsentChange->getChange();
                            bool sent_ok = send_data_or_fragments(
                                group,
                                change,
                                remoteReader->expects_inline_qos(),
                                sent_fun);
                            on_sample_datas(change->write_params.sample_identity(), change->num_sent_submessages);
                            if (sent_ok)
                            {
                                max_ack_seq = seqNum;
                            }
                        }
                        else
                        {
                            max_ack_seq = seqNum;
                        }
                    };
            remoteReader->for_each_unsent_change(max_sequence, unsent_change_process);
            if (max_ack_seq != SequenceNumber_t::unknown())
            {
                remoteReader->acked_changes_set(max_ack_seq + 1);
            }
        }
    } // Readers loop
}

void StatefulWriter::send_all_intraprocess_changes(
        SequenceNumber_t max_sequence)
{
    for (ReaderProxy* remoteReader : matched_local_readers_)
    {
        intraprocess_heartbeat(remoteReader, false);
        SequenceNumber_t max_ack_seq = SequenceNumber_t::unknown();
        auto unsent_change_process = [&](const SequenceNumber_t& seq_num, const ChangeForReader_t* unsentChange)
                {
                    if (unsentChange != nullptr && unsentChange->isValid())
                    {
                        if (intraprocess_delivery(unsentChange->getChange(), remoteReader))
                        {
                            max_ack_seq = seq_num;
                        }
                        else
                        {
                            remoteReader->set_change_to_status(seq_num, UNDERWAY, false);
                        }
                    }
                    else
                    {
                        if (intraprocess_gap(remoteReader, seq_num))
                        {
                            max_ack_seq = seq_num;
                        }
                        else
                        {
                            remoteReader->set_change_to_status(seq_num, UNDERWAY, true);
                        }
                    }
                };

        remoteReader->for_each_unsent_change(max_sequence, unsent_change_process);
        if (max_ack_seq != SequenceNumber_t::unknown())
        {
            remoteReader->acked_changes_set(max_ack_seq + 1);
        }
    }
}

void StatefulWriter::send_all_datasharing_changes(
        SequenceNumber_t max_sequence)
{
    for (ReaderProxy* remoteReader : matched_datasharing_readers_)
    {
        SequenceNumber_t max_ack_seq = SequenceNumber_t::unknown();
        auto unsent_change_process = [&](const SequenceNumber_t& seq_num, const ChangeForReader_t* /*unsentChange*/)
                {
                    if (!remoteReader->is_reliable())
                    {
                        max_ack_seq = seq_num;
                    }
                    else
                    {
                        remoteReader->set_change_to_status(seq_num, UNACKNOWLEDGED, false);
                    }
                };

        remoteReader->for_each_unsent_change(max_sequence, unsent_change_process);

        if (max_ack_seq != SequenceNumber_t::unknown())
        {
            remoteReader->acked_changes_set(max_ack_seq + 1);
        }

        // Finally notify the reader it has some data to read
        remoteReader->datasharing_notify();
    }
}

void StatefulWriter::send_all_unsent_changes(
        SequenceNumber_t max_sequence,
        bool& activateHeartbeatPeriod)
{
    // This version is called when all of the following conditions are satisfied:
    // a) history is not empty
    // b) there is at least one matched reader
    // c) separate sending is disabled
    // d) either all matched readers are local or no flow controllers are configured

    // Process intraprocess first
    if (there_are_local_readers_)
    {
        send_all_intraprocess_changes(max_sequence);
    }

    // Process datasharing then
    if (there_are_datasharing_readers_)
    {
        send_all_datasharing_changes(max_sequence);
    }

    if (there_are_remote_readers_)
    {
        static constexpr uint32_t implicit_flow_controller_size = RTPSMessageGroup::get_max_fragment_payload_size();

        NetworkFactory& network = mp_RTPSParticipant->network_factory();
        locator_selector_.reset(true);
        network.select_locators(locator_selector_);
        compute_selected_guids();
        size_t num_locators = locator_selector_.selected_size();

        bool acknack_required = next_all_acked_notify_sequence_ < get_seq_num_min();

        RTPSMessageGroup group(mp_RTPSParticipant, this, *this);

        acknack_required |= send_hole_gaps_to_group(group);

        uint32_t lastBytesProcessed = 0;
        auto sent_fun = [this, &num_locators, &lastBytesProcessed, &group](
            CacheChange_t* change,
            FragmentNumber_t /*frag*/)
                {
                    add_statistics_sent_submessage(change, num_locators);
                    // Heartbeat piggyback.
                    send_heartbeat_piggyback_nts_(nullptr, group, lastBytesProcessed);
                };

        RTPSGapBuilder gap_builder(group);
        uint32_t total_sent_size = 0;

        History::iterator cit;
        for (cit = mp_history->changesBegin();
                cit != mp_history->changesEnd() && (total_sent_size < implicit_flow_controller_size);
                cit++)
        {
            SequenceNumber_t seq = (*cit)->sequenceNumber;

            // Deselect all entries on the locator selector (we will only activate the
            // readers for which this sequence number is pending)
            locator_selector_.reset(false);

            bool is_irrelevant = true;   // Will turn to false if change is relevant for at least one reader
            bool should_be_sent = false;
            bool inline_qos = false;
            for (ReaderProxy* remoteReader : matched_remote_readers_)
            {
                if (remoteReader->change_is_unsent(seq, is_irrelevant))
                {
                    should_be_sent = true;
                    locator_selector_.enable(remoteReader->guid());
                    inline_qos |= remoteReader->expects_inline_qos();
                    if (is_irrelevant)
                    {
                        remoteReader->set_change_to_status(seq, UNDERWAY, true);
                    }
                }
            }

            if (locator_selector_.state_has_changed())
            {
                gap_builder.flush();
                group.flush_and_reset();
                network.select_locators(locator_selector_);
                compute_selected_guids();
                num_locators = locator_selector_.selected_size();
            }

            if (should_be_sent)
            {
                if (is_irrelevant)
                {
                    gap_builder.add(seq);
                }
                else
                {
                    auto change = *cit;
                    bool sent_ok = send_data_or_fragments(group, change, inline_qos, sent_fun);
                    on_sample_datas(change->write_params.sample_identity(), change->num_sent_submessages);
                    if (sent_ok)
                    {
                        total_sent_size += (*cit)->serializedPayload.length;
                        bool tmp_bool = false;
                        for (ReaderProxy* remoteReader : matched_remote_readers_)
                        {
                            if (remoteReader->change_is_unsent(seq, tmp_bool))
                            {
                                remoteReader->set_change_to_status(seq, UNDERWAY, true);
                                if (remoteReader->is_reliable())
                                {
                                    activateHeartbeatPeriod = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Heartbeat piggyback.
        if (acknack_required)
        {
            send_heartbeat_nts_(all_remote_readers_.size(), group, disable_positive_acks_);
        }

        group.flush_and_reset();

        locator_selector_.reset(true);
        network.select_locators(locator_selector_);
        compute_selected_guids();
        num_locators = locator_selector_.selected_size();

        if (cit != mp_history->changesEnd())
        {
            mp_RTPSParticipant->async_thread().wake_up(this);
        }
    }
}

void StatefulWriter::send_unsent_changes_with_flow_control(
        SequenceNumber_t max_sequence,
        bool& activateHeartbeatPeriod)
{
    // This version is called when all of the following conditions are satisfied:
    // a) history is not empty
    // b) there is at least one matched reader
    // c) separate sending is disabled
    // d) there is at least one remote matched reader and flow controllers are configured

    // Process intraprocess first
    if (there_are_local_readers_)
    {
        send_all_intraprocess_changes(max_sequence);
    }

    // Process datasharing then
    if (there_are_datasharing_readers_)
    {
        send_all_datasharing_changes(max_sequence);
    }

    // From here onwards, only remote readers should be accessed

    RTPSWriterCollector<ReaderProxy*> relevantChanges;
    bool heartbeat_has_been_sent = false;

    NetworkFactory& network = mp_RTPSParticipant->network_factory();
    locator_selector_.reset(true);
    network.select_locators(locator_selector_);
    compute_selected_guids();

    RTPSMessageGroup group(mp_RTPSParticipant, this, *this);

    // GAP for holes in history sent to the readers that need it
    send_hole_gaps_to_group(group);

    // Reset the state of locator_selector to select all readers
    group.flush_and_reset();
    locator_selector_.reset(true);
    network.select_locators(locator_selector_);
    compute_selected_guids();
    size_t num_locators = locator_selector_.selected_size();

    for (ReaderProxy* remoteReader : matched_remote_readers_)
    {
        if (!heartbeat_has_been_sent && remoteReader->are_there_gaps())
        {
            send_heartbeat_nts_(all_remote_readers_.size(), group, true);
            heartbeat_has_been_sent = true;
        }

        RTPSGapBuilder gaps(group, remoteReader->guid());
        auto unsent_change_process = [&](const SequenceNumber_t& seq_num, const ChangeForReader_t* unsentChange)
                {
                    if (unsentChange != nullptr && unsentChange->isValid())
                    {
                        relevantChanges.add_change(
                            unsentChange->getChange(), remoteReader, unsentChange->getUnsentFragments());
                    }
                    else
                    {
                        // Skip holes in history, as they were added before
                        if (unsentChange != nullptr && remoteReader->is_reliable())
                        {
                            gaps.add(seq_num);
                        }

                        remoteReader->set_change_to_status(seq_num, UNDERWAY, true);
                    }
                };

        remoteReader->for_each_unsent_change(max_sequence, unsent_change_process);
    }

    // Clear all relevant changes through the local controllers first
    for (std::unique_ptr<FlowController>& controller : m_controllers)
    {
        (*controller)(relevantChanges);
    }

    // Clear all relevant changes through the parent controllers
    for (std::unique_ptr<FlowController>& controller : mp_RTPSParticipant->getFlowControllers())
    {
        (*controller)(relevantChanges);
    }

    try
    {
        uint32_t lastBytesProcessed = 0;

        while (!relevantChanges.empty())
        {
            RTPSWriterCollector<ReaderProxy*>::Item changeToSend = relevantChanges.pop();
            bool expectsInlineQos = false;
            locator_selector_.reset(false);

            for (const ReaderProxy* remoteReader : changeToSend.remoteReaders)
            {
                locator_selector_.enable(remoteReader->guid());
                expectsInlineQos |= remoteReader->expects_inline_qos();
            }

            if (locator_selector_.state_has_changed())
            {
                group.flush_and_reset();
                network.select_locators(locator_selector_);
                compute_selected_guids();
                num_locators = locator_selector_.selected_size();
            }

            // TODO(Ricardo) Flowcontroller has to be used in RTPSMessageGroup. Study.
            // And controllers are notified about the changes being sent
            auto change = changeToSend.cacheChange;
            FlowController::NotifyControllersChangeSent(change);

            if (changeToSend.fragmentNumber != 0)
            {
                if (group.add_data_frag(*change, changeToSend.fragmentNumber,
                        expectsInlineQos))
                {
                    add_statistics_sent_submessage(change, num_locators);
                    on_sample_datas(change->write_params.sample_identity(), change->num_sent_submessages);
                    bool must_wake_up_async_thread = false;
                    for (ReaderProxy* remoteReader : changeToSend.remoteReaders)
                    {
                        bool allFragmentsSent = false;
                        if (remoteReader->mark_fragment_as_sent_for_change(
                                    changeToSend.sequenceNumber,
                                    changeToSend.fragmentNumber,
                                    allFragmentsSent))
                        {
                            must_wake_up_async_thread |= !allFragmentsSent;
                            if (remoteReader->is_remote_and_reliable())
                            {
                                activateHeartbeatPeriod = true;
                                if (allFragmentsSent)
                                {
                                    remoteReader->set_change_to_status(changeToSend.sequenceNumber,
                                            UNDERWAY,
                                            true);
                                }
                            }
                            else
                            {
                                if (allFragmentsSent)
                                {
                                    remoteReader->set_change_to_status(changeToSend.sequenceNumber,
                                            ACKNOWLEDGED, false);
                                }
                            }
                        }
                    }

                    if (must_wake_up_async_thread)
                    {
                        mp_RTPSParticipant->async_thread().wake_up(this);
                    }
                }
                else
                {
                    logError(RTPS_WRITER, "Error sending fragment (" << changeToSend.sequenceNumber <<
                            ", " << changeToSend.fragmentNumber << ")");
                }
            }
            else
            {
                if (group.add_data(*change, expectsInlineQos))
                {
                    add_statistics_sent_submessage(change, num_locators);
                    on_sample_datas(change->write_params.sample_identity(), change->num_sent_submessages);
                    for (ReaderProxy* remoteReader : changeToSend.remoteReaders)
                    {
                        remoteReader->set_change_to_status(changeToSend.sequenceNumber, UNDERWAY, true);

                        if (remoteReader->is_remote_and_reliable())
                        {
                            activateHeartbeatPeriod = true;
                        }
                    }
                }
                else
                {
                    logError(RTPS_WRITER, "Error sending change " << changeToSend.sequenceNumber);
                }
            }

            // Heartbeat piggyback.
            send_heartbeat_piggyback_nts_(nullptr, group, lastBytesProcessed);
        }

        group.flush_and_reset();
    }
    catch (const RTPSMessageGroup::timeout&)
    {
        logError(RTPS_WRITER, "Max blocking time reached");
    }

    locator_selector_.reset(true);
    network.select_locators(locator_selector_);
    compute_selected_guids();
}

bool StatefulWriter::send_hole_gaps_to_group(
        RTPSMessageGroup& group)
{
    bool ret_val = false;

    // Add holes in history and send them to all readers in group
    SequenceNumber_t max_removed = biggest_removed_sequence_number_;
    SequenceNumber_t last_sequence = mp_history->next_sequence_number();
    SequenceNumber_t min_history_seq = get_seq_num_min();
    uint32_t history_size = static_cast<uint32_t>(mp_history->getHistorySize());
    if ((min_readers_low_mark_ < max_removed) &&     // some holes pending acknowledgement
            (min_history_seq + history_size != last_sequence)) // There is a hole in the history
    {
        try
        {
            // Only send gaps to readers requiring it
            select_all_readers_with_lowmark_below(max_removed, group);

            send_heartbeat_nts_(all_remote_readers_.size(), group, true);
            ret_val = true;

            // Find holes in history from min_history_seq to last_sequence - 1
            RTPSGapBuilder gap_builder(group);

            // Algorithm starts in min_history_seq
            SequenceNumber_t seq = min_history_seq;

            // Loop all history
            for (auto cit = mp_history->changesBegin(); cit != mp_history->changesEnd(); cit++)
            {
                // Add all sequence numbers until the change's sequence number
                while (seq < (*cit)->sequenceNumber)
                {
                    gap_builder.add(seq);
                    seq++;
                }

                // Skip change's sequence number
                seq++;
            }

            // Add all sequence numbers above last change
            while (seq < last_sequence)
            {
                gap_builder.add(seq);
                seq++;
            }
        }
        catch (const RTPSMessageGroup::timeout&)
        {
            logError(RTPS_WRITER, "Max blocking time reached");
        }
    }

    return ret_val;
}

void StatefulWriter::select_all_readers_with_lowmark_below(
        SequenceNumber_t seq,
        RTPSMessageGroup& group)
{
    // Deselect all entries on the locator selector (we will only activate the
    // readers for which this sequence number is pending)
    locator_selector_.reset(false);

    for_matched_readers(matched_local_readers_, matched_remote_readers_,
            [this, &seq](ReaderProxy* reader)
            {
                if (reader->changes_low_mark() < seq)
                {
                    locator_selector_.enable(reader->guid());
                }

                return false;
            }
            );

    if (locator_selector_.state_has_changed())
    {
        group.flush_and_reset();
        getRTPSParticipant()->network_factory().select_locators(locator_selector_);
        compute_selected_guids();
    }
}

/*
 * MATCHED_READER-RELATED METHODS
 */
void StatefulWriter::update_reader_info(
        bool create_sender_resources)
{
    update_cached_info_nts();
    compute_selected_guids();

    if (create_sender_resources)
    {
        RTPSParticipantImpl* part = getRTPSParticipant();
        locator_selector_.for_each([part](const Locator_t& loc)
                {
                    part->createSenderResources(loc);
                });
    }

    // Check if we have local or remote readers
    there_are_remote_readers_ = !matched_remote_readers_.empty();
    there_are_local_readers_ = !matched_local_readers_.empty();
    there_are_datasharing_readers_ = !matched_datasharing_readers_.empty();
}

bool StatefulWriter::matched_reader_add(
        const ReaderProxyData& rdata)
{
    if (rdata.guid() == c_Guid_Unknown)
    {
        logError(RTPS_WRITER, "Reliable Writer need GUID_t of matched readers");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    // Check if it is already matched.
    if (for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [this, &rdata](ReaderProxy* reader)
            {
                if (reader->guid() == rdata.guid())
                {
                    logInfo(RTPS_WRITER, "Attempting to add existing reader, updating information.");
                    if (reader->update(rdata))
                    {
                        update_reader_info(true);
                    }
                    return true;
                }
                return false;
            }))
    {
        return false;
    }

    // Get a reader proxy from the inactive pool (or create a new one if necessary and allowed)
    ReaderProxy* rp = nullptr;
    if (matched_readers_pool_.empty())
    {
        size_t max_readers = matched_readers_pool_.max_size();
        if (getMatchedReadersSize() + matched_readers_pool_.size() < max_readers)
        {
            const RTPSParticipantAttributes& part_att = mp_RTPSParticipant->getRTPSParticipantAttributes();
            rp = new ReaderProxy(m_times, part_att.allocation.locators, this);
        }
        else
        {
            logWarning(RTPS_WRITER, "Maximum number of reader proxies (" << max_readers <<
                    ") reached for writer " << m_guid);
            return false;
        }
    }
    else
    {
        rp = matched_readers_pool_.back();
        matched_readers_pool_.pop_back();
    }

    // Add info of new datareader.
    rp->start(rdata, is_datasharing_compatible_with(rdata));
    locator_selector_.add_entry(rp->locator_selector_entry());

    if (rp->is_local_reader())
    {
        matched_local_readers_.push_back(rp);
        logInfo(RTPS_WRITER, "Adding reader " << rdata.guid() << " to " << this->m_guid.entityId
                                              << " as local reader");
    }
    else
    {
        if (rp->is_datasharing_reader())
        {
            matched_datasharing_readers_.push_back(rp);
            logInfo(RTPS_WRITER, "Adding reader " << rdata.guid() << " to " << this->m_guid.entityId
                                                  << " as data sharing");
        }
        else
        {
            matched_remote_readers_.push_back(rp);
            logInfo(RTPS_WRITER, "Adding reader " << rdata.guid() << " to " << this->m_guid.entityId
                                                  << " as remote reader");
        }
    }

    update_reader_info(true);

    if (rp->is_datasharing_reader())
    {
        return true;
    }

    RTPSMessageGroup group(mp_RTPSParticipant, this, rp->message_sender());

    // Add initial heartbeat to message group
    if (rp->is_local_reader())
    {
        intraprocess_heartbeat(rp);
    }
    else
    {
        send_heartbeat_nts_(1u, group, disable_positive_acks_);
    }

    SequenceNumber_t current_seq = get_seq_num_min();
    SequenceNumber_t last_seq = get_seq_num_max();

    if (current_seq != SequenceNumber_t::unknown())
    {
        (void)last_seq;
        assert(last_seq != SequenceNumber_t::unknown());
        assert(current_seq <= last_seq);

        RTPSGapBuilder gap_builder(group);
        bool is_reliable = rp->is_reliable();

        for (History::iterator cit = mp_history->changesBegin(); cit != mp_history->changesEnd(); ++cit)
        {
            // This is to cover the case when there are holes in the history
            if (is_reliable)
            {
                while (current_seq != (*cit)->sequenceNumber)
                {
                    if (rp->is_local_reader())
                    {
                        intraprocess_gap(rp, current_seq);
                    }
                    else
                    {
                        try
                        {
                            gap_builder.add(current_seq);
                        }
                        catch (const RTPSMessageGroup::timeout&)
                        {
                            logError(RTPS_WRITER, "Max blocking time reached");
                        }
                    }
                    ++current_seq;
                }
            }
            else
            {
                current_seq = (*cit)->sequenceNumber;
            }

            ChangeForReader_t changeForReader(*cit);
            bool relevance =
                    rp->durability_kind() >= TRANSIENT_LOCAL &&
                    m_att.durabilityKind >= TRANSIENT_LOCAL &&
                    rp->rtps_is_relevant(*cit);
            changeForReader.setRelevance(relevance);
            if (!relevance && is_reliable)
            {
                if (rp->is_local_reader())
                {
                    intraprocess_gap(rp, current_seq);
                }
                else
                {
                    try
                    {
                        gap_builder.add(current_seq);
                    }
                    catch (const RTPSMessageGroup::timeout&)
                    {
                        logError(RTPS_WRITER, "Max blocking time reached");
                    }
                }
            }

            // The ChangeForReader_t status has to be UNACKNOWLEDGED
            if (!rp->is_local_reader() || !changeForReader.isRelevant())
            {
                changeForReader.setStatus(UNACKNOWLEDGED);
            }
            rp->add_change(changeForReader, false);
            ++current_seq;
        }

        // This is to cover the case where the last changes have been removed from the history
        if (is_reliable)
        {
            while (current_seq < next_sequence_number())
            {
                if (rp->is_local_reader())
                {
                    intraprocess_gap(rp, current_seq);
                }
                else
                {
                    try
                    {
                        gap_builder.add(current_seq);
                    }
                    catch (const RTPSMessageGroup::timeout&)
                    {
                        logError(RTPS_WRITER, "Max blocking time reached");
                    }
                }
                ++current_seq;
            }
        }

        try
        {
            if (rp->is_local_reader())
            {
                mp_RTPSParticipant->async_thread().wake_up(this);
            }
            else if (is_reliable)
            {
                // Send Gap
                gap_builder.flush();
            }
        }
        catch (const RTPSMessageGroup::timeout&)
        {
            logError(RTPS_WRITER, "Max blocking time reached");
        }

        // Always activate heartbeat period. We need a confirmation of the reader.
        // The state has to be updated.
        periodic_hb_event_->restart_timer();
    }

    if (!rp->is_local_reader())
    {
        try
        {
            // Send all messages
            group.flush_and_reset();
        }
        catch (const RTPSMessageGroup::timeout&)
        {
            logError(RTPS_WRITER, "Max blocking time reached");
        }
    }

    logInfo(RTPS_WRITER, "Reader Proxy " << rp->guid() << " added to " << this->m_guid.entityId << " with "
                                         << rdata.remote_locators().unicast.size() << "(u)-"
                                         << rdata.remote_locators().multicast.size() <<
            "(m) locators");

    return true;
}

bool StatefulWriter::matched_reader_remove(
        const GUID_t& reader_guid)
{
    ReaderProxy* rproxy = nullptr;
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);

    for (ReaderProxyIterator it = matched_local_readers_.begin();
            it != matched_local_readers_.end(); ++it)
    {
        if ((*it)->guid() == reader_guid)
        {
            logInfo(RTPS_WRITER, "Reader Proxy removed: " << reader_guid);
            rproxy = std::move(*it);
            it = matched_local_readers_.erase(it);
            break;
        }
    }

    if (rproxy == nullptr)
    {
        for (ReaderProxyIterator it = matched_datasharing_readers_.begin();
                it != matched_datasharing_readers_.end(); ++it)
        {
            if ((*it)->guid() == reader_guid)
            {
                logInfo(RTPS_WRITER, "Reader Proxy removed: " << reader_guid);
                rproxy = std::move(*it);
                it = matched_datasharing_readers_.erase(it);
                break;
            }
        }
    }

    if (rproxy == nullptr)
    {
        for (ReaderProxyIterator it = matched_remote_readers_.begin();
                it != matched_remote_readers_.end(); ++it)
        {
            if ((*it)->guid() == reader_guid)
            {
                logInfo(RTPS_WRITER, "Reader Proxy removed: " << reader_guid);
                rproxy = std::move(*it);
                it = matched_remote_readers_.erase(it);
                break;
            }
        }
    }

    locator_selector_.remove_entry(reader_guid);
    update_reader_info(false);

    if (getMatchedReadersSize() == 0)
    {
        periodic_hb_event_->cancel_timer();
    }

    if (rproxy != nullptr)
    {
        rproxy->stop();
        matched_readers_pool_.push_back(rproxy);

        lock.unlock();
        check_acked_status();

        return true;
    }

    logInfo(RTPS_HISTORY, "Reader Proxy doesn't exist in this writer");
    return false;
}

bool StatefulWriter::matched_reader_is_matched(
        const GUID_t& reader_guid)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    return for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                   [&reader_guid](ReaderProxy* reader)
                   {
                       return (reader->guid() == reader_guid);
                   }
                   );
}

bool StatefulWriter::matched_reader_lookup(
        GUID_t& readerGuid,
        ReaderProxy** RP)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    return for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                   [&readerGuid, RP](ReaderProxy* reader)
                   {
                       if (reader->guid() == readerGuid)
                       {
                           *RP = reader;
                           return true;
                       }
                       return false;
                   }
                   );
}

bool StatefulWriter::is_acked_by_all(
        const CacheChange_t* change) const
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    if (change->writerGUID != this->getGuid())
    {
        logWarning(RTPS_WRITER, "The given change is not from this Writer");
        return false;
    }

    return is_acked_by_all(change->sequenceNumber);
}

bool StatefulWriter::is_acked_by_all(
        const SequenceNumber_t seq) const
{
    assert(mp_history->next_sequence_number() > seq);
    return (seq < next_all_acked_notify_sequence_) ||
           !for_matched_readers(matched_local_readers_, matched_remote_readers_,
                   [seq](const ReaderProxy* reader)
                   {
                       return !(reader->change_is_acked(seq));
                   });
}

bool StatefulWriter::all_readers_updated()
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    return !for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                   [](const ReaderProxy* reader)
                   {
                       return (reader->has_changes());
                   }
                   );
}

bool StatefulWriter::wait_for_all_acked(
        const Duration_t& max_wait)
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);
    std::unique_lock<std::mutex> all_acked_lock(all_acked_mutex_);

    all_acked_ = !for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                    [](const ReaderProxy* reader)
                    {
                        return reader->has_changes();
                    }
                    );
    lock.unlock();

    if (!all_acked_)
    {
        std::chrono::microseconds max_w(TimeConv::Duration_t2MicroSecondsInt64(max_wait));
        all_acked_cond_.wait_for(all_acked_lock, max_w, [&]()
                {
                    return all_acked_;
                });
    }

    return all_acked_;
}

void StatefulWriter::rebuild_status_after_load()
{
    SequenceNumber_t min_seq = get_seq_num_min();
    if (min_seq != SequenceNumber_t::unknown())
    {
        biggest_removed_sequence_number_ = min_seq - 1;
        may_remove_change_ = 1;
    }

    SequenceNumber_t next_seq = mp_history->next_sequence_number();
    next_all_acked_notify_sequence_ = next_seq;
    min_readers_low_mark_ = next_seq - 1;
    all_acked_ = true;
}

void StatefulWriter::check_acked_status()
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);

    bool all_acked = true;
    bool has_min_low_mark = false;
    // #8945 If no readers matched, notify all old changes.
    SequenceNumber_t min_low_mark = mp_history->next_sequence_number() - 1;

    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [&all_acked, &has_min_low_mark, &min_low_mark](ReaderProxy* reader)
            {
                SequenceNumber_t reader_low_mark = reader->changes_low_mark();
                if (reader_low_mark < min_low_mark || !has_min_low_mark)
                {
                    has_min_low_mark = true;
                    min_low_mark = reader_low_mark;
                }

                if (reader->has_changes())
                {
                    all_acked = false;
                }

                return false;
            }
            );

    bool something_changed = all_acked;
    SequenceNumber_t min_seq = get_seq_num_min();
    if (min_seq != SequenceNumber_t::unknown())
    {
        // In the case where we haven't received an acknack from a recently matched reader,
        // min_low_mark will be zero, and no change will be notified as received by all
        if (next_all_acked_notify_sequence_ <= min_low_mark)
        {
            if ((mp_listener != nullptr) && (min_low_mark >= get_seq_num_min()))
            {
                // We will inform backwards about the changes received by all readers, starting
                // on min_low_mark down until next_all_acked_notify_sequence_. This way we can
                // safely proceed with the traversal, in case a change is removed from the history
                // inside the callback
                History::iterator history_end = mp_history->changesEnd();
                History::iterator cit =
                        std::lower_bound(mp_history->changesBegin(), history_end, min_low_mark,
                                [](
                                    const CacheChange_t* change,
                                    const SequenceNumber_t& seq)
                                {
                                    return change->sequenceNumber < seq;
                                });
                if (cit != history_end && (*cit)->sequenceNumber == min_low_mark)
                {
                    ++cit;
                }

                SequenceNumber_t seq{};
                SequenceNumber_t end_seq = min_seq > next_all_acked_notify_sequence_ ?
                        min_seq : next_all_acked_notify_sequence_;

                // The iterator starts pointing to the change inmediately after min_low_mark
                --cit;

                do
                {
                    // Avoid notifying changes before next_all_acked_notify_sequence_
                    CacheChange_t* change = *cit;
                    seq = change->sequenceNumber;
                    if (seq < next_all_acked_notify_sequence_)
                    {
                        break;
                    }

                    // Change iterator before it possibly becomes invalidated
                    if (cit != mp_history->changesBegin())
                    {
                        --cit;
                    }

                    // Notify reception of change (may remove that change on VOLATILE writers)
                    mp_listener->onWriterChangeReceivedByAll(this, change);

                    // Stop if we got to either next_all_acked_notify_sequence_ or the first change
                } while (seq > end_seq);
            }

            next_all_acked_notify_sequence_ = min_low_mark + 1;
        }

        if (min_low_mark >= get_seq_num_min())
        {
            may_remove_change_ = 1;
        }

        min_readers_low_mark_ = min_low_mark;
        something_changed = true;
    }

    if (all_acked)
    {
        std::unique_lock<std::mutex> all_acked_lock(all_acked_mutex_);
        SequenceNumber_t next_seq = mp_history->next_sequence_number();
        next_all_acked_notify_sequence_ = next_seq;
        min_readers_low_mark_ = next_seq - 1;
        all_acked_ = true;
        all_acked_cond_.notify_all();
    }

    if (something_changed)
    {
        may_remove_change_cond_.notify_one();
    }
}

bool StatefulWriter::try_remove_change(
        const std::chrono::steady_clock::time_point& max_blocking_time_point,
        std::unique_lock<RecursiveTimedMutex>& lock)
{
    logInfo(RTPS_WRITER, "Starting process try remove change for writer " << getGuid());

    SequenceNumber_t min_low_mark;

    {
        std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
        min_low_mark = next_all_acked_notify_sequence_ - 1;
    }

    SequenceNumber_t calc = min_low_mark < get_seq_num_min() ? SequenceNumber_t() :
            (min_low_mark - get_seq_num_min()) + 1;
    unsigned int may_remove_change = 1;

    if (calc <= SequenceNumber_t())
    {
        may_remove_change_ = 0;
        may_remove_change_cond_.wait_until(lock, max_blocking_time_point,
                [&]()
                {
                    return may_remove_change_ > 0;
                });
        may_remove_change = may_remove_change_;
    }

    // Some changes acked
    if (may_remove_change == 1)
    {
        return mp_history->remove_min_change();
    }
    // Waiting a change was removed.
    else if (may_remove_change == 2)
    {
        return true;
    }

    return false;
}

bool StatefulWriter::wait_for_acknowledgement(
        const SequenceNumber_t& seq,
        const std::chrono::steady_clock::time_point& max_blocking_time_point,
        std::unique_lock<RecursiveTimedMutex>& lock)
{
    return may_remove_change_cond_.wait_until(lock, max_blocking_time_point,
                   [this, &seq]()
                   {
                       return is_acked_by_all(seq);
                   });
}

/*
 * PARAMETER_RELATED METHODS
 */
void StatefulWriter::updateAttributes(
        const WriterAttributes& att)
{
    this->updateTimes(att.times);
}

void StatefulWriter::updateTimes(
        const WriterTimes& times)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    if (m_times.heartbeatPeriod != times.heartbeatPeriod)
    {
        periodic_hb_event_->update_interval(times.heartbeatPeriod);
    }
    if (m_times.nackResponseDelay != times.nackResponseDelay)
    {
        if (nack_response_event_ != nullptr)
        {
            nack_response_event_->update_interval(times.nackResponseDelay);
        }
    }
    if (m_times.nackSupressionDuration != times.nackSupressionDuration)
    {
        for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                [&times](ReaderProxy* reader)
                {
                    reader->update_nack_supression_interval(times.nackSupressionDuration);
                    return false;
                }
                );

        for (ReaderProxy* it : matched_readers_pool_)
        {
            it->update_nack_supression_interval(times.nackSupressionDuration);
        }
    }
    m_times = times;
}

void StatefulWriter::add_flow_controller(
        std::unique_ptr<FlowController> controller)
{
    m_controllers.push_back(std::move(controller));
}

SequenceNumber_t StatefulWriter::next_sequence_number() const
{
    return mp_history->next_sequence_number();
}

bool StatefulWriter::send_periodic_heartbeat(
        bool final,
        bool liveliness)
{
    std::lock_guard<RecursiveTimedMutex> guardW(mp_mutex);

    bool unacked_changes = false;
    if (!liveliness)
    {
        SequenceNumber_t firstSeq, lastSeq;

        firstSeq = get_seq_num_min();
        lastSeq = get_seq_num_max();

        if (firstSeq == c_SequenceNumber_Unknown || lastSeq == c_SequenceNumber_Unknown)
        {
            return false;
        }
        else
        {
            assert(firstSeq <= lastSeq);

            unacked_changes = for_matched_readers(matched_local_readers_, matched_datasharing_readers_,
                            matched_remote_readers_,
                            [](ReaderProxy* reader)
                            {
                                return reader->has_unacknowledged();
                            }
                            );

            if (unacked_changes)
            {
                try
                {
                    send_heartbeat_to_all_readers();
                }
                catch (const RTPSMessageGroup::timeout&)
                {
                    logError(RTPS_WRITER, "Max blocking time reached");
                }
            }
        }
    }
    else if (m_separateSendingEnabled)
    {
        for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                [this, &liveliness, &unacked_changes](ReaderProxy* reader)
                {
                    send_heartbeat_to_nts(*reader, liveliness);
                    unacked_changes = true;
                    return false;
                }
                );
    }
    else
    {
        // This is a liveliness heartbeat, we don't care about checking sequence numbers
        try
        {
            for (ReaderProxy* reader : matched_local_readers_)
            {
                intraprocess_heartbeat(reader, true);
                unacked_changes = true;
            }

            for (ReaderProxy* reader : matched_datasharing_readers_)
            {
                std::shared_ptr<WriterPool> p = std::dynamic_pointer_cast<WriterPool>(payload_pool_);
                assert(p);
                p->assert_liveliness();
                reader->datasharing_notify();
                unacked_changes = true;
            }

            if (there_are_remote_readers_)
            {
                unacked_changes = true;
                RTPSMessageGroup group(mp_RTPSParticipant, this, *this);
                send_heartbeat_nts_(all_remote_readers_.size(), group, final, liveliness);
            }
        }
        catch (const RTPSMessageGroup::timeout&)
        {
            logError(RTPS_WRITER, "Max blocking time reached");
        }
    }

    return unacked_changes;
}

void StatefulWriter::send_heartbeat_to_nts(
        ReaderProxy& remoteReaderProxy,
        bool liveliness,
        bool force /* = false */)
{
    if (remoteReaderProxy.is_reliable() && (force || liveliness || remoteReaderProxy.has_unacknowledged()))
    {
        if (remoteReaderProxy.is_local_reader())
        {
            intraprocess_heartbeat(&remoteReaderProxy, liveliness);
        }
        else if (remoteReaderProxy.is_datasharing_reader())
        {
            remoteReaderProxy.datasharing_notify();
        }
        else
        {
            try
            {
                RTPSMessageGroup group(mp_RTPSParticipant, this, remoteReaderProxy.message_sender());
                send_heartbeat_nts_(1u, group, disable_positive_acks_, liveliness);
                SequenceNumber_t first_seq = get_seq_num_min();
                if (first_seq != c_SequenceNumber_Unknown)
                {
                    SequenceNumber_t first_relevant = remoteReaderProxy.first_relevant_sequence_number();
                    if (remoteReaderProxy.durability_kind() == VOLATILE && first_seq < first_relevant)
                    {
                        group.add_gap(first_seq, SequenceNumberSet_t(first_relevant));
                    }
                    remoteReaderProxy.send_gaps(group, mp_history->next_sequence_number());
                }
            }
            catch (const RTPSMessageGroup::timeout&)
            {
                logError(RTPS_WRITER, "Max blocking time reached");
            }
        }
    }
}

void StatefulWriter::send_heartbeat_nts_(
        size_t number_of_readers,
        RTPSMessageGroup& message_group,
        bool final,
        bool liveliness)
{
    if (!number_of_readers)
    {
        return;
    }

    SequenceNumber_t firstSeq = get_seq_num_min();
    SequenceNumber_t lastSeq = get_seq_num_max();

    if (firstSeq == c_SequenceNumber_Unknown || lastSeq == c_SequenceNumber_Unknown)
    {
        assert(firstSeq == c_SequenceNumber_Unknown && lastSeq == c_SequenceNumber_Unknown);

        if (number_of_readers == 1 || liveliness)
        {
            firstSeq = next_sequence_number();
            lastSeq = firstSeq - 1;
        }
        else
        {
            return;
        }
    }
    else
    {
        assert(firstSeq <= lastSeq);
    }

    incrementHBCount();
    message_group.add_heartbeat(firstSeq, lastSeq, m_heartbeatCount, final, liveliness);
    // Update calculate of heartbeat piggyback.
    currentUsageSendBufferSize_ = static_cast<int32_t>(sendBufferSize_);

    logInfo(RTPS_WRITER, getGuid().entityId << " Sending Heartbeat (" << firstSeq << " - " << lastSeq << ")" );
}

void StatefulWriter::send_heartbeat_piggyback_nts_(
        ReaderProxy* reader,
        RTPSMessageGroup& message_group,
        uint32_t& last_bytes_processed)
{
    if (!disable_heartbeat_piggyback_)
    {
        size_t number_of_readers = reader == nullptr ? all_remote_readers_.size() : 1u;
        if (mp_history->isFull())
        {
            if (reader == nullptr)
            {
                locator_selector_.reset(true);
                if (locator_selector_.state_has_changed())
                {
                    message_group.flush_and_reset();
                    getRTPSParticipant()->network_factory().select_locators(locator_selector_);
                    compute_selected_guids();
                }
            }
            send_heartbeat_nts_(number_of_readers, message_group, disable_positive_acks_);
        }
        else
        {
            uint32_t current_bytes = message_group.get_current_bytes_processed();
            currentUsageSendBufferSize_ -= current_bytes - last_bytes_processed;
            last_bytes_processed = current_bytes;
            if (currentUsageSendBufferSize_ < 0)
            {
                send_heartbeat_nts_(number_of_readers, message_group, disable_positive_acks_);
            }
        }
    }
}

void StatefulWriter::perform_nack_response()
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);
    bool must_wake_up_async_thread = false;
    uint32_t changes_to_resend = 0;

    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [&must_wake_up_async_thread, &changes_to_resend](ReaderProxy* reader)
            {
                uint32_t pending = reader->perform_acknack_response();
                changes_to_resend += pending;

                if ( pending > 0 || reader->are_there_gaps())
                {
                    must_wake_up_async_thread = true;
                    // Do not exit the loop, perform_acknack_response must be executed for all readers
                }

                return false;
            }
            );

    if (must_wake_up_async_thread)
    {
        mp_RTPSParticipant->async_thread().wake_up(this);
    }

    lock.unlock();

    // Notify the statistics module
    on_resent_data(changes_to_resend);
}

void StatefulWriter::perform_nack_supression(
        const GUID_t& reader_guid)
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);

    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [this, &reader_guid](ReaderProxy* reader)
            {
                if (reader->guid() == reader_guid)
                {
                    reader->perform_nack_supression();
                    periodic_hb_event_->restart_timer();
                    return true;
                }
                return false;
            }
            );
}

bool StatefulWriter::process_acknack(
        const GUID_t& writer_guid,
        const GUID_t& reader_guid,
        uint32_t ack_count,
        const SequenceNumberSet_t& sn_set,
        bool final_flag,
        bool& result)
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);
    result = (m_guid == writer_guid);

    if (result)
    {
        SequenceNumber_t received_sequence_number = sn_set.empty() ? sn_set.base() : sn_set.max();
        if (received_sequence_number <= next_sequence_number())
        {
            for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                    [&](ReaderProxy* remote_reader)
                    {
                        if (remote_reader->guid() == reader_guid)
                        {
                            if (remote_reader->check_and_set_acknack_count(ack_count))
                            {
                                // Sequence numbers before Base are set as Acknowledged.
                                remote_reader->acked_changes_set(sn_set.base());
                                if (sn_set.base() > SequenceNumber_t(0, 0))
                                {
                                    if (remote_reader->requested_changes_set(sn_set) || remote_reader->are_there_gaps())
                                    {
                                        nack_response_event_->restart_timer();
                                    }
                                    else if (!final_flag)
                                    {
                                        periodic_hb_event_->restart_timer();
                                    }
                                }
                                else if (sn_set.empty() && !final_flag)
                                {
                                    // This is the preemptive acknack.
                                    if (remote_reader->process_initial_acknack())
                                    {
                                        if (!remote_reader->is_datasharing_reader())
                                        {
                                            if (remote_reader->is_local_reader())
                                            {
                                                mp_RTPSParticipant->async_thread().wake_up(this);
                                            }
                                            else
                                            {
                                                // Send heartbeat if requested
                                                send_heartbeat_to_nts(*remote_reader, false, true);
                                            }
                                        }
                                    }

                                    if (remote_reader->is_local_reader() && !remote_reader->is_datasharing_reader())
                                    {
                                        intraprocess_heartbeat(remote_reader);
                                    }
                                }

                                // Check if all CacheChange are acknowledge, because a user could be waiting
                                // for this, or some CacheChanges could be removed if we are VOLATILE
                                check_acked_status();
                            }
                            return true;
                        }

                        return false;
                    }
                    );
        }
        else
        {
            print_inconsistent_acknack(writer_guid, reader_guid, sn_set.base(), received_sequence_number,
                    next_sequence_number());
        }
    }

    return result;
}

bool StatefulWriter::process_nack_frag(
        const GUID_t& writer_guid,
        const GUID_t& reader_guid,
        uint32_t ack_count,
        const SequenceNumber_t& seq_num,
        const FragmentNumberSet_t fragments_state,
        bool& result)
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);
    result = false;
    if (m_guid == writer_guid)
    {
        result = true;
        for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                [this, &reader_guid, &ack_count, &seq_num, &fragments_state](ReaderProxy* reader)
                {
                    if (reader->guid() == reader_guid)
                    {
                        if (reader->process_nack_frag(reader_guid, ack_count, seq_num, fragments_state))
                        {
                            nack_response_event_->restart_timer();
                        }
                        return true;
                    }
                    return false;
                }
                );
    }

    return result;
}

bool StatefulWriter::ack_timer_expired()
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_mutex);

    // The timer has expired so the earliest non-acked change must be marked as acknowledged
    // This will be done in the first while iteration, as we start with a negative interval

    auto interval = -keep_duration_us_;

    // On the other hand, we've seen in the tests that if samples are sent very quickly with little
    // time between consecutive samples, the timer interval could end up being negative
    // In this case, we keep marking changes as acknowledged until the timer is able to keep up, hence the while
    // loop

    while (interval.count() < 0)
    {
        for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                [this](ReaderProxy* reader)
                {
                    if (reader->disable_positive_acks())
                    {
                        reader->acked_changes_set(last_sequence_number_ + 1);
                    }
                    return false;
                }
                );
        last_sequence_number_++;

        // Get the next cache change from the history
        CacheChange_t* change;

        if (!mp_history->get_change(
                    last_sequence_number_,
                    getGuid(),
                    &change))
        {
            return false;
        }

        auto source_timestamp = system_clock::time_point() + nanoseconds(change->sourceTimestamp.to_ns());
        auto now = system_clock::now();
        interval = source_timestamp - now + keep_duration_us_;
    }
    assert(interval.count() >= 0);

    ack_event_->update_interval_millisec((double)duration_cast<milliseconds>(interval).count());
    return true;
}

void StatefulWriter::print_inconsistent_acknack(
        const GUID_t& writer_guid,
        const GUID_t& reader_guid,
        const SequenceNumber_t& min_requested_sequence_number,
        const SequenceNumber_t& max_requested_sequence_number,
        const SequenceNumber_t& next_sequence_number)
{
    logWarning(RTPS_WRITER, "Inconsistent acknack received. Local Writer "
            << writer_guid << " next SequenceNumber " << next_sequence_number << ". Remote Reader "
            << reader_guid << " requested range is  [" << min_requested_sequence_number
            << ", " << max_requested_sequence_number << "].");
    // This is necessary to avoid Warning of unused variable in case warning log level is disable
    static_cast<void>(writer_guid);
    static_cast<void>(reader_guid);
    static_cast<void>(min_requested_sequence_number);
    static_cast<void>(max_requested_sequence_number);
    static_cast<void>(next_sequence_number);
}

void StatefulWriter::reader_data_filter(
        fastdds::rtps::IReaderDataFilter* reader_data_filter)
{
    reader_data_filter_ = reader_data_filter;
}

const fastdds::rtps::IReaderDataFilter* StatefulWriter::reader_data_filter() const
{
    return reader_data_filter_;
}

}  // namespace rtps
}  // namespace fastrtps
}  // namespace eprosima
