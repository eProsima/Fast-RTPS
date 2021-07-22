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
 * @file StatelessWriter.cpp
 *
 */

#include <fastdds/rtps/writer/StatelessWriter.h>
#include <fastdds/rtps/writer/WriterListener.h>
#include <fastdds/rtps/history/WriterHistory.h>
#include <rtps/participant/RTPSParticipantImpl.h>
#include <rtps/history/HistoryAttributesExtension.hpp>
#include <fastdds/rtps/builtin/BuiltinProtocols.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>

#include <algorithm>
#include <mutex>
#include <set>
#include <vector>

#include <fastdds/dds/log/Log.hpp>
#include <rtps/history/BasicPayloadPool.hpp>
#include <rtps/DataSharing/DataSharingPayloadPool.hpp>
#include <rtps/DataSharing/WriterPool.hpp>
#include <rtps/DataSharing/DataSharingNotifier.hpp>
#include <rtps/history/CacheChangePool.h>
#include <rtps/RTPSDomainImpl.hpp>

#include "../flowcontrol/FlowController.hpp"

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
        ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_1,
        std::function<bool(ReaderLocator&)> fun)
{
    for (auto& remote_reader : reader_vector_1)
    {
        if (fun(*remote_reader))
        {
            return true;
        }
    }

    return false;
}

bool for_matched_readers(
        ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_1,
        ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_2,
        std::function<bool(ReaderLocator&)> fun)
{
    if (for_matched_readers(reader_vector_1, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_2, fun);
}

bool for_matched_readers(
        ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_1,
        ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_2,
        ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_3,
        std::function<bool(ReaderLocator&)> fun)
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
        const ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_1,
        std::function<bool(const ReaderLocator)> fun)
{
    for (const auto& remote_reader : reader_vector_1)
    {
        if (fun(*remote_reader))
        {
            return true;
        }
    }

    return false;
}

bool for_matched_readers(
        const ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_1,
        const ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_2,
        std::function<bool(const ReaderLocator)> fun)
{
    if (for_matched_readers(reader_vector_1, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_2, fun);
}

bool for_matched_readers(
        const ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_1,
        const ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_2,
        const ResourceLimitedVector<std::unique_ptr<ReaderLocator>>& reader_vector_3,
        std::function<bool(const ReaderLocator&)> fun)
{
    if (for_matched_readers(reader_vector_1, reader_vector_2, fun))
    {
        return true;
    }
    return for_matched_readers(reader_vector_3, fun);
}

StatelessWriter::StatelessWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& attributes,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* history,
        WriterListener* listener)
    : RTPSWriter(impl, guid, attributes, flow_controller, history, listener)
    , matched_remote_readers_(attributes.matched_readers_allocation)
    , matched_local_readers_(attributes.matched_readers_allocation)
    , matched_datasharing_readers_(attributes.matched_readers_allocation)
    , matched_readers_pool_(attributes.matched_readers_allocation)
    , locator_selector_(*this, attributes.matched_readers_allocation)
{
    init(impl, attributes);
}

StatelessWriter::StatelessWriter(
        RTPSParticipantImpl* impl,
        const GUID_t& guid,
        const WriterAttributes& attributes,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* history,
        WriterListener* listener)
    : RTPSWriter(impl, guid, attributes, payload_pool, flow_controller, history, listener)
    , matched_remote_readers_(attributes.matched_readers_allocation)
    , matched_local_readers_(attributes.matched_readers_allocation)
    , matched_datasharing_readers_(attributes.matched_readers_allocation)
    , matched_readers_pool_(attributes.matched_readers_allocation)
    , locator_selector_(*this, attributes.matched_readers_allocation)
{
    init(impl, attributes);
}

StatelessWriter::StatelessWriter(
        RTPSParticipantImpl* participant,
        const GUID_t& guid,
        const WriterAttributes& attributes,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* history,
        WriterListener* listener)
    : RTPSWriter(participant, guid, attributes, payload_pool, change_pool, flow_controller, history, listener)
    , matched_remote_readers_(attributes.matched_readers_allocation)
    , matched_local_readers_(attributes.matched_readers_allocation)
    , matched_datasharing_readers_(attributes.matched_readers_allocation)
    , matched_readers_pool_(attributes.matched_readers_allocation)
    , locator_selector_(*this, attributes.matched_readers_allocation)
{
    init(participant, attributes);
}

void StatelessWriter::init(
        RTPSParticipantImpl* participant,
        const WriterAttributes& attributes)
{
    get_builtin_guid();

    const RemoteLocatorsAllocationAttributes& loc_alloc =
            participant->getRTPSParticipantAttributes().allocation.locators;

    for (size_t i = 0; i < attributes.matched_readers_allocation.initial; ++i)
    {
        matched_readers_pool_.emplace_back(new ReaderLocator(
                    this,
                    loc_alloc.max_unicast_locators,
                    loc_alloc.max_multicast_locators));
    }
}

StatelessWriter::~StatelessWriter()
{
    logInfo(RTPS_WRITER, "StatelessWriter destructor"; );
    deinit();
}

void StatelessWriter::get_builtin_guid()
{
    if (m_guid.entityId == ENTITYID_SPDP_BUILTIN_RTPSParticipant_WRITER)
    {
        add_guid(locator_selector_, GUID_t { GuidPrefix_t(), c_EntityId_SPDPReader });
    }
#if HAVE_SECURITY
    else if (m_guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER)
    {
        add_guid(locator_selector_, GUID_t { GuidPrefix_t(), participant_stateless_message_reader_entity_id });
    }
#endif // if HAVE_SECURITY
}

bool StatelessWriter::has_builtin_guid()
{
    if (m_guid.entityId == ENTITYID_SPDP_BUILTIN_RTPSParticipant_WRITER)
    {
        return true;
    }
#if HAVE_SECURITY
    if (m_guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER)
    {
        return true;
    }
#endif // if HAVE_SECURITY
    return false;
}

void StatelessWriter::update_reader_info(
        bool create_sender_resources)
{
    bool addGuid = !has_builtin_guid();
    is_inline_qos_expected_ = false;

    for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [this](const ReaderLocator& reader)
            {
                is_inline_qos_expected_ |= reader.expects_inline_qos();
                return false;
            }
            );

    update_cached_info_nts(locator_selector_);
    if (addGuid)
    {
        compute_selected_guids(locator_selector_);
    }

    if (create_sender_resources)
    {
        RTPSParticipantImpl* part = mp_RTPSParticipant;
        locator_selector_.locator_selector.for_each([part](const Locator_t& loc)
                {
                    part->createSenderResources(loc);
                });
    }
}

/*
 *	CHANGE-RELATED METHODS
 */
bool StatelessWriter::datasharing_delivery(
        CacheChange_t* change)
{
    auto pool = std::dynamic_pointer_cast<WriterPool>(payload_pool_);
    assert(pool != nullptr);

    pool->add_to_shared_history(change);
    logInfo(RTPS_WRITER, "Notifying readers of cache change with SN " << change->sequenceNumber);
    for (std::unique_ptr<ReaderLocator>& reader : matched_datasharing_readers_)
    {
        reader->datasharing_notify();
    }
    return true;
}

void StatelessWriter::unsent_change_added_to_history(
        CacheChange_t* change,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    if (liveliness_lease_duration_ < c_TimeInfinite)
    {
        mp_RTPSParticipant->wlp()->assert_liveliness(
            getGuid(),
            liveliness_kind_,
            liveliness_lease_duration_);
    }

    // Notify the datasharing readers
    // This also prepares the metadata for late-joiners
    if (is_datasharing_compatible())
    {
        datasharing_delivery(change);
    }

    // Now for the rest of readers
    if (!fixed_locators_.empty() || getMatchedReadersSize() > 0)
    {
        flow_controller_->add_new_sample(this, change, max_blocking_time);
    }
    else
    {
        logInfo(RTPS_WRITER, "No reader to add change.");
        if (mp_listener != nullptr)
        {
            mp_listener->onWriterChangeReceivedByAll(this, change);
        }
    }

    // Throughput should be notified even if no matches are available
    on_publish_throughput(change->serializedPayload.length);
}

bool StatelessWriter::intraprocess_delivery(
        CacheChange_t* change,
        ReaderLocator& reader_locator)
{
    RTPSReader* reader = reader_locator.local_reader();

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

bool StatelessWriter::change_removed_by_history(
        CacheChange_t* change)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    flow_controller_->remove_change(change);

    // remove from datasharing pool history
    if (is_datasharing_compatible())
    {
        auto pool = std::dynamic_pointer_cast<WriterPool>(payload_pool_);
        assert (pool != nullptr);

        pool->remove_from_shared_history(change);
        logInfo(RTPS_WRITER, "Removing shared cache change with SN " << change->sequenceNumber);
    }

    const uint64_t sequence_number = change->sequenceNumber.to64long();
    if (sequence_number > last_sequence_number_sent_)
    {
        unsent_changes_cond_.notify_all();
    }

    return true;
}

bool StatelessWriter::is_acked_by_all(
        const CacheChange_t* change) const
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    return change->sequenceNumber.to64long() >= last_sequence_number_sent_;
}

bool StatelessWriter::try_remove_change(
        const std::chrono::steady_clock::time_point&,
        std::unique_lock<RecursiveTimedMutex>&)
{
    return mp_history->remove_min_change();
}

bool StatelessWriter::wait_for_acknowledgement(
        const SequenceNumber_t& seq,
        const std::chrono::steady_clock::time_point& max_blocking_time_point,
        std::unique_lock<RecursiveTimedMutex>& lock)
{
    uint64_t sequence_number = seq.to64long();
    auto change_is_acknowledged = [this, sequence_number]()
            {
                return sequence_number >= last_sequence_number_sent_;
            };
    return unsent_changes_cond_.wait_until(lock, max_blocking_time_point, change_is_acknowledged);
}

/*
 *	MATCHED_READER-RELATED METHODS
 */
bool StatelessWriter::matched_reader_add(
        const ReaderProxyData& data)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    assert(data.guid() != c_Guid_Unknown);

    if (for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
            [this, &data](ReaderLocator& reader)
            {
                if (reader.remote_guid() == data.guid())
                {
                    logWarning(RTPS_WRITER, "Attempting to add existing reader, updating information.");
                    if (reader.update(data.remote_locators().unicast,
                    data.remote_locators().multicast,
                    data.m_expectsInlineQos))
                    {
                        update_reader_info(true);
                    }
                    return true;
                }
                return false;
            }
            ))
    {
        return false;
    }

    // Get a locator from the inactive pool (or create a new one if necessary and allowed)
    std::unique_ptr<ReaderLocator> new_reader;
    if (matched_readers_pool_.empty())
    {
        size_t max_readers = matched_readers_pool_.max_size();
        if (getMatchedReadersSize() + matched_readers_pool_.size() < max_readers)
        {
            const RemoteLocatorsAllocationAttributes& loc_alloc =
                    mp_RTPSParticipant->getRTPSParticipantAttributes().allocation.locators;

            new_reader.reset(new ReaderLocator(
                        this,
                        loc_alloc.max_unicast_locators,
                        loc_alloc.max_multicast_locators));
        }
        else
        {
            logWarning(RTPS_WRITER, "Couldn't add matched reader due to resource limits");
            return false;
        }
    }
    else
    {
        new_reader = std::move(matched_readers_pool_.back());
        matched_readers_pool_.pop_back();
    }

    // Add info of new datareader.
    new_reader->start(data.guid(),
            data.remote_locators().unicast,
            data.remote_locators().multicast,
            data.m_expectsInlineQos,
            is_datasharing_compatible_with(data));

    locator_selector_.locator_selector.add_entry(new_reader->locator_selector_entry());

    if (new_reader->is_local_reader())
    {
        matched_local_readers_.push_back(std::move(new_reader));
        logInfo(RTPS_WRITER, "Adding reader " << data.guid() << " to " << this->m_guid.entityId
                                              << " as local reader");
    }
    else if (new_reader->is_datasharing_reader())
    {
        matched_datasharing_readers_.push_back(std::move(new_reader));
        logInfo(RTPS_WRITER, "Adding reader " << data.guid() << " to " << this->m_guid.entityId
                                              << " as data sharing");
    }
    else
    {
        matched_remote_readers_.push_back(std::move(new_reader));
        logInfo(RTPS_WRITER, "Adding reader " << data.guid() << " to " << this->m_guid.entityId
                                              << " as remote reader");
    }

    update_reader_info(true);

    return true;
}

bool StatelessWriter::set_fixed_locators(
        const LocatorList_t& locator_list)
{
#if HAVE_SECURITY
    if (getAttributes().security_attributes().is_submessage_protected ||
            getAttributes().security_attributes().is_payload_protected)
    {
        logError(RTPS_WRITER, "A secure besteffort writer cannot add a lonely locator");
        return false;
    }
#endif // if HAVE_SECURITY

    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    fixed_locators_.push_back(locator_list);
    mp_RTPSParticipant->createSenderResources(fixed_locators_);

    return true;
}

bool StatelessWriter::matched_reader_remove(
        const GUID_t& reader_guid)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);

    if (locator_selector_.locator_selector.remove_entry(reader_guid))
    {
        std::unique_ptr<ReaderLocator> reader;
        for (auto it = matched_local_readers_.begin();
                it != matched_local_readers_.end(); ++it)
        {
            if ((*it)->remote_guid() == reader_guid)
            {
                reader = std::move(*it);
                matched_local_readers_.erase(it);
                break;
            }
        }

        if (reader == nullptr)
        {
            for (auto it = matched_datasharing_readers_.begin();
                    it != matched_datasharing_readers_.end(); ++it)
            {
                if ((*it)->remote_guid() == reader_guid)
                {
                    reader = std::move(*it);
                    matched_datasharing_readers_.erase(it);
                    break;
                }
            }
        }

        if (reader == nullptr)
        {
            for (auto it = matched_remote_readers_.begin();
                    it != matched_remote_readers_.end(); ++it)
            {
                if ((*it)->remote_guid() == reader_guid)
                {
                    reader = std::move(*it);
                    matched_remote_readers_.erase(it);
                    break;
                }
            }
        }

        // guid should be both on locator_selector_ and matched_readers_ or in none
        assert(reader != nullptr);

        reader->stop();
        matched_readers_pool_.push_back(std::move(reader));
        update_reader_info(false);
        logInfo(RTPS_WRITER, "Reader Proxy removed: " << reader_guid);
        return true;
    }

    return false;
}

bool StatelessWriter::matched_reader_is_matched(
        const GUID_t& reader_guid)
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    return for_matched_readers(matched_local_readers_, matched_datasharing_readers_, matched_remote_readers_,
                   [reader_guid](const ReaderLocator& reader)
                   {
                       return reader.remote_guid() == reader_guid;
                   }
                   );
}

void StatelessWriter::unsent_changes_reset()
{
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    std::for_each(mp_history->changesBegin(), mp_history->changesEnd(), [&](CacheChange_t* change)
            {
                flow_controller_->add_new_sample(this, change,
                std::chrono::steady_clock::now() + std::chrono::hours(24));
            });
}

bool StatelessWriter::send_nts(
        CDRMessage_t* message,
        const LocatorSelectorSender& locator_selector,
        std::chrono::steady_clock::time_point& max_blocking_time_point) const
{
    if (!RTPSWriter::send_nts(message, locator_selector, max_blocking_time_point))
    {
        return false;
    }

    return fixed_locators_.empty() ||
           mp_RTPSParticipant->sendSync(message, m_guid,
                   Locators(fixed_locators_.begin()), Locators(fixed_locators_.end()),
                   max_blocking_time_point);
}

DeliveryRetCode StatelessWriter::deliver_sample_nts(
        CacheChange_t* cache_change,
        RTPSMessageGroup& group,
        LocatorSelectorSender& locator_selector,
        const std::chrono::time_point<std::chrono::steady_clock>& /*TODO max_blocking_time*/)
{
    size_t num_locators = locator_selector.locator_selector.selected_size() + fixed_locators_.size();
    uint64_t change_sequence_number = cache_change->sequenceNumber.to64long();
    DeliveryRetCode ret_code = DeliveryRetCode::DELIVERED;

    if (current_sequence_number_sent_ != change_sequence_number)
    {
        current_sequence_number_sent_ = change_sequence_number;
        current_fragment_sent_ = 0;
    }

    // Send to interprocess readers the new sample.
    if (0 == current_fragment_sent_)
    {
        for_matched_readers(matched_local_readers_, [&, cache_change](ReaderLocator& reader)
                {
                    intraprocess_delivery(cache_change, reader);
                    return false;
                });
    }

    try
    {
        uint32_t n_fragments = cache_change->getFragmentCount();

        if (m_separateSendingEnabled)
        {
            std::vector<GUID_t> guids(1);
            if (0 < n_fragments)
            {
                for (FragmentNumber_t frag = current_fragment_sent_ + 1;
                        DeliveryRetCode::DELIVERED == ret_code && frag <= n_fragments; ++frag)
                {
                    for (std::unique_ptr<ReaderLocator>& it : matched_remote_readers_)
                    {
                        group.sender(this, &*it);
                        num_locators = it->locators_size();

                        if (group.add_data_frag(*cache_change, frag, is_inline_qos_expected_))
                        {
                            add_statistics_sent_submessage(cache_change, num_locators);
                        }
                        else
                        {
                            logError(RTPS_WRITER,
                                    "Error sending fragment (" << cache_change->sequenceNumber << ", " << frag <<
                                    ")");
                            ret_code = DeliveryRetCode::NOT_DELIVERED;
                        }
                    }

                    if (DeliveryRetCode::DELIVERED == ret_code)
                    {
                        current_fragment_sent_ = frag;
                    }
                }
            }
            else
            {
                for (std::unique_ptr<ReaderLocator>& it : matched_remote_readers_)
                {
                    group.sender(this, &*it);
                    num_locators = it->locators_size();

                    if (group.add_data(*cache_change, is_inline_qos_expected_))
                    {
                        add_statistics_sent_submessage(cache_change, num_locators);
                    }
                    else
                    {
                        logError(RTPS_WRITER, "Error sending change " << cache_change->sequenceNumber);
                        ret_code = DeliveryRetCode::NOT_DELIVERED;
                    }
                }
            }
        }
        else
        {
            if (0 < num_locators)
            {
                if (0 < n_fragments)
                {
                    for (FragmentNumber_t frag = current_fragment_sent_ + 1;
                            DeliveryRetCode::DELIVERED == ret_code && frag <= n_fragments; ++frag)
                    {
                        if (group.add_data_frag(*cache_change, frag, is_inline_qos_expected_))
                        {
                            current_fragment_sent_ = frag;
                            add_statistics_sent_submessage(cache_change, num_locators);
                        }
                        else
                        {
                            logError(RTPS_WRITER,
                                    "Error sending fragment (" << cache_change->sequenceNumber << ", " << frag << ")");
                            ret_code = DeliveryRetCode::NOT_DELIVERED;
                        }
                    }
                }
                else
                {
                    if (group.add_data(*cache_change, is_inline_qos_expected_))
                    {
                        add_statistics_sent_submessage(cache_change, num_locators);
                    }
                    else
                    {
                        logError(RTPS_WRITER, "Error sending change " << cache_change->sequenceNumber);
                        ret_code = DeliveryRetCode::NOT_DELIVERED;
                    }
                }
            }
        }

        on_sample_datas(cache_change->write_params.sample_identity(),
                cache_change->writer_info.num_sent_submessages);
        on_data_sent();

    }
    catch (const RTPSMessageGroup::timeout&)
    {
        logError(RTPS_WRITER, "Max blocking time reached");
        ret_code = DeliveryRetCode::NOT_DELIVERED;
    }
    catch (const RTPSMessageGroup::limit_exceeded&)
    {
        ret_code = DeliveryRetCode::EXCEEDED_LIMIT;
    }

    group.sender(this, &locator_selector);

    if (DeliveryRetCode::DELIVERED == ret_code &&
            change_sequence_number > last_sequence_number_sent_)
    {
        // This update must be done before calling the callback.
        last_sequence_number_sent_ = change_sequence_number;
        unsent_changes_cond_.notify_all();

        if (nullptr != mp_listener)
        {
            mp_listener->onWriterChangeReceivedByAll(this, cache_change);
        }
    }

    return ret_code;
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
