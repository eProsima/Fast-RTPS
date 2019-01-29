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

#include <fastrtps/rtps/writer/StatefulWriter.h>
#include <fastrtps/rtps/writer/WriterListener.h>
#include <fastrtps/rtps/writer/ReaderProxy.h>
#include <fastrtps/rtps/resources/AsyncWriterThread.h>

#include "../participant/RTPSParticipantImpl.h"
#include "../flowcontrol/FlowController.h"

#include <fastrtps/rtps/messages/RTPSMessageCreator.h>
#include <fastrtps/rtps/messages/RTPSMessageGroup.h>

#include <fastrtps/rtps/writer/timedevent/PeriodicHeartbeat.h>
#include <fastrtps/rtps/writer/timedevent/NackSupressionDuration.h>
#include <fastrtps/rtps/writer/timedevent/NackResponseDelay.h>

#include <fastrtps/rtps/history/WriterHistory.h>

#include <fastrtps/log/Log.h>
#include <fastrtps/utils/TimeConversion.h>

#include "RTPSWriterCollector.h"
#include "StatefulWriterOrganizer.h"

#include <mutex>
#include <vector>

using namespace eprosima::fastrtps::rtps;


StatefulWriter::StatefulWriter(RTPSParticipantImpl* pimpl,GUID_t& guid,
        WriterAttributes& att,WriterHistory* hist,WriterListener* listen):
    RTPSWriter(pimpl, guid, att, hist, listen),
    mp_periodicHB(nullptr), m_times(att.times),
    all_acked_(false), may_remove_change_(0),
    disableHeartbeatPiggyback_(att.disableHeartbeatPiggyback),
    sendBufferSize_(pimpl->get_min_network_send_buffer_size()),
    currentUsageSendBufferSize_(static_cast<int32_t>(pimpl->get_min_network_send_buffer_size()))
{
    m_heartbeatCount = 0;
    if(guid.entityId == c_EntityId_SEDPPubWriter)
        m_HBReaderEntityId = c_EntityId_SEDPPubReader;
    else if(guid.entityId == c_EntityId_SEDPSubWriter)
        m_HBReaderEntityId = c_EntityId_SEDPSubReader;
    else if(guid.entityId == c_EntityId_WriterLiveliness)
        m_HBReaderEntityId= c_EntityId_ReaderLiveliness;
    else
        m_HBReaderEntityId = c_EntityId_Unknown;
    mp_periodicHB = new PeriodicHeartbeat(this,TimeConv::Time_t2MilliSecondsDouble(m_times.heartbeatPeriod));
}


StatefulWriter::~StatefulWriter()
{
    AsyncWriterThread::removeWriter(*this);

    logInfo(RTPS_WRITER,"StatefulWriter destructor");

    for (ReaderProxy* remote_reader : matched_readers)
    {
        remote_reader->destroy_timers();
    }

    if (mp_periodicHB != nullptr)
    {
        delete(mp_periodicHB);
        mp_periodicHB = nullptr;
    }

    for (ReaderProxy* remote_reader : matched_readers)
    {
        delete(remote_reader);
    }
}

/*
 *	CHANGE-RELATED METHODS
 */

void StatefulWriter::unsent_change_added_to_history(CacheChange_t* change)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);

#if HAVE_SECURITY
    encrypt_cachechange(change);
#endif

    //TODO Think about when set liveliness assertion when writer is asynchronous.
    this->setLivelinessAsserted(true);

    if(!matched_readers.empty())
    {
        if(!isAsync())
        {
            //TODO(Ricardo) Temporal.
            bool expectsInlineQos = false;
            std::vector<GUID_t> guids(1);

            for(ReaderProxy* it : matched_readers)
            {
                ChangeForReader_t changeForReader(change);

                // TODO(Ricardo) Study next case: Not push mode, writer reliable and reader besteffort.
                if(m_pushMode)
                {
                    if(it->m_att.endpoint.reliabilityKind == RELIABLE)
                    {
                        changeForReader.setStatus(UNDERWAY);
                    }
                    else
                    {
                        changeForReader.setStatus(ACKNOWLEDGED);
                    }
                }
                else
                {
                    changeForReader.setStatus(UNACKNOWLEDGED);
                }

                changeForReader.setRelevance(it->rtps_is_relevant(change));
                it->add_change(changeForReader, true);
                expectsInlineQos |= it->m_att.expectsInlineQos;

                if (m_separateSendingEnabled)
                {
                    guids.at(0) = it->m_att.guid;
                    RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages, 
                        it->m_att.endpoint.remoteLocatorList, guids);
                    if (!group.add_data(*change, guids, it->m_att.endpoint.remoteLocatorList, it->m_att.expectsInlineQos))
                    {
                        logError(RTPS_WRITER, "Error sending change " << change->sequenceNumber);
                    }
                    uint32_t last_processed = 0;
                    send_heartbeat_piggyback_nts_(guids, it->m_att.endpoint.remoteLocatorList, group, last_processed);
                }
            }

            if (!m_separateSendingEnabled)
            {
                RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages);
                if (!group.add_data(*change, all_remote_readers_, mAllShrinkedLocatorList, expectsInlineQos))
                {
                    logError(RTPS_WRITER, "Error sending change " << change->sequenceNumber);
                }

                // Heartbeat piggyback.
                uint32_t last_processed = 0;
                send_heartbeat_piggyback_nts_(group, last_processed);
            }

            this->mp_periodicHB->restart_timer();
            if ( (mp_listener != nullptr) && this->is_acked_by_all(change) )
            {
                mp_listener->onWriterChangeReceivedByAll(this, change);
            }
        }
        else
        {
            for(ReaderProxy* it : matched_readers)
            {
                ChangeForReader_t changeForReader(change);

                if(m_pushMode)
                {
                    changeForReader.setStatus(UNSENT);
                }
                else
                {
                    changeForReader.setStatus(UNACKNOWLEDGED);
                }

                changeForReader.setRelevance(it->rtps_is_relevant(change));
                it->add_change(changeForReader, false);
            }

            if (m_pushMode)
            {
                AsyncWriterThread::wakeUp(this);
            }
        }
    }
    else
    {
        logInfo(RTPS_WRITER,"No reader proxy to add change.");
        if (mp_listener != nullptr)
        {
            mp_listener->onWriterChangeReceivedByAll(this, change);
        }
    }
}


bool StatefulWriter::change_removed_by_history(CacheChange_t* a_change)
{
    SequenceNumber_t sequence_number = a_change->sequenceNumber;

    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    logInfo(RTPS_WRITER,"Change "<< sequence_number << " to be removed.");

    // Invalidate CacheChange pointer in ReaderProxies.
    for(ReaderProxy* it : matched_readers)
    {
        it->change_has_been_removed(sequence_number);
    }

    std::unique_lock<std::mutex> may_lock(may_remove_change_mutex_);
    may_remove_change_ = 2;
    may_remove_change_cond_.notify_one();

    return true;
}

void StatefulWriter::send_any_unsent_changes()
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);

    bool activateHeartbeatPeriod = false;

    // Separate sending for asynchronous writers
    if (m_pushMode && m_separateSendingEnabled && isAsync())
    {
        std::vector<GUID_t> guids(1);

        for (auto remoteReader : matched_readers)
        {
            // For possible GAP
            std::set<SequenceNumber_t> irrelevant;

            // Specific destination message group
            guids.at(0) = remoteReader->m_att.guid;
            const LocatorList_t& locators = remoteReader->m_att.endpoint.remoteLocatorList;
            RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages, locators, guids);

            // Loop all changes
            bool is_reliable = (remoteReader->m_att.endpoint.reliabilityKind == RELIABLE);
            auto unsent_change_process = [&](const ChangeForReader_t* unsentChange)
            {
                SequenceNumber_t seqNum = unsentChange->getSequenceNumber();

                if (unsentChange->isRelevant() && unsentChange->isValid())
                {
                    // As we checked we are not async, we know we cannot have fragments
                    if (group.add_data(*(unsentChange->getChange()), guids, locators, remoteReader->m_att.expectsInlineQos))
                    {
                        if (is_reliable)
                        {
                            remoteReader->set_change_to_status(seqNum, UNDERWAY, true);
                            activateHeartbeatPeriod = true;
                        }
                        else
                        {
                            remoteReader->set_change_to_status(seqNum, ACKNOWLEDGED, false);
                        }
                    }
                    else
                    {
                        logError(RTPS_WRITER, "Error sending change " << seqNum);
                    }
                }
                else
                {
                    if (is_reliable)
                    {
                        irrelevant.emplace(seqNum);
                    }
                    remoteReader->set_change_to_status(seqNum, UNDERWAY, false); //TODO(Ricardo) Review
                } // Relevance
            };
            remoteReader->for_each_unsent_change(unsent_change_process);

            if (!irrelevant.empty())
            {
                group.add_gap(irrelevant, guids, locators);
            }
        } // Readers loop
    }
    else
    {

        RTPSWriterCollector<ReaderProxy*> relevantChanges;
        StatefulWriterOrganizer notRelevantChanges;

        for (auto remoteReader : matched_readers)
        {
            auto unsent_change_process = [&](const ChangeForReader_t* unsentChange)
            {
                if (unsentChange->isRelevant() && unsentChange->isValid())
                {
                    if (m_pushMode)
                    {
                        relevantChanges.add_change(unsentChange->getChange(), remoteReader, unsentChange->getUnsentFragments());
                    }
                    else // Change status to UNACKNOWLEDGED
                    {
                        remoteReader->set_change_to_status(unsentChange->getSequenceNumber(), UNACKNOWLEDGED, false);
                    }
                }
                else
                {
                    notRelevantChanges.add_sequence_number(unsentChange->getSequenceNumber(), remoteReader);
                    remoteReader->set_change_to_status(unsentChange->getSequenceNumber(), UNDERWAY, false); //TODO(Ricardo) Review
                }
            };

            remoteReader->for_each_unsent_change(unsent_change_process);
        }

        if (m_pushMode)
        {
            // Clear all relevant changes through the local controllers first
            for (auto& controller : m_controllers)
                (*controller)(relevantChanges);

            // Clear all relevant changes through the parent controllers
            for (auto& controller : mp_RTPSParticipant->getFlowControllers())
                (*controller)(relevantChanges);

            RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages);
            uint32_t lastBytesProcessed = 0;

            while (!relevantChanges.empty())
            {
                RTPSWriterCollector<ReaderProxy*>::Item changeToSend = relevantChanges.pop();
                std::vector<GUID_t> remote_readers;
                std::vector<LocatorList_t> locatorLists;
                bool expectsInlineQos = false;

                for (const ReaderProxy* remoteReader : changeToSend.remoteReaders)
                {
                    remote_readers.push_back(remoteReader->m_att.guid);
                    locatorLists.push_back(remoteReader->m_att.endpoint.remoteLocatorList);
                    expectsInlineQos |= remoteReader->m_att.expectsInlineQos;
                }

                // TODO(Ricardo) Flowcontroller has to be used in RTPSMessageGroup. Study.
                // And controllers are notified about the changes being sent
                FlowController::NotifyControllersChangeSent(changeToSend.cacheChange);

                if (changeToSend.fragmentNumber != 0)
                {
                    if (group.add_data_frag(*changeToSend.cacheChange, changeToSend.fragmentNumber, remote_readers,
                        mp_RTPSParticipant->network_factory().ShrinkLocatorLists(locatorLists),
                        expectsInlineQos))
                    {
                        bool must_wake_up_async_thread = false;
                        for (auto remoteReader : changeToSend.remoteReaders)
                        {
                            bool allFragmentsSent = false;
                            if (remoteReader->mark_fragment_as_sent_for_change(
                                changeToSend.sequenceNumber,
                                changeToSend.fragmentNumber,
                                allFragmentsSent))
                            {
                                must_wake_up_async_thread |= !allFragmentsSent;
                                if (remoteReader->m_att.endpoint.reliabilityKind == RELIABLE)
                                {
                                    activateHeartbeatPeriod = true;
                                    assert(remoteReader->nack_supression_event_ != nullptr);
                                    if (allFragmentsSent)
                                    {
                                        remoteReader->set_change_to_status(changeToSend.sequenceNumber, UNDERWAY, true);
                                    }
                                }
                                else
                                {
                                    if (allFragmentsSent)
                                    {
                                        remoteReader->set_change_to_status(changeToSend.sequenceNumber, ACKNOWLEDGED, false);
                                    }
                                }
                            }
                        }

                        if (must_wake_up_async_thread)
                        {
                            AsyncWriterThread::wakeUp(this);
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
                    if (group.add_data(*changeToSend.cacheChange, remote_readers,
                        mp_RTPSParticipant->network_factory().ShrinkLocatorLists(locatorLists),
                        expectsInlineQos))
                    {
                        for (auto remoteReader : changeToSend.remoteReaders)
                        {
                            if (remoteReader->m_att.endpoint.reliabilityKind == RELIABLE)
                            {
                                remoteReader->set_change_to_status(changeToSend.sequenceNumber, UNDERWAY, true);
                                activateHeartbeatPeriod = true;
                            }
                            else
                            {
                                remoteReader->set_change_to_status(changeToSend.sequenceNumber, ACKNOWLEDGED, false);
                            }
                        }
                    }
                    else
                    {
                        logError(RTPS_WRITER, "Error sending change " << changeToSend.sequenceNumber);
                    }
                }

                // Heartbeat piggyback.
                send_heartbeat_piggyback_nts_(group, lastBytesProcessed);
            }

            for (auto pair : notRelevantChanges.elements())
            {
                std::vector<GUID_t> remote_readers;
                std::vector<LocatorList_t> locatorLists;

                for (auto remoteReader : pair.first)
                {
                    remote_readers.push_back(remoteReader->m_att.guid);
                    locatorLists.push_back(remoteReader->m_att.endpoint.remoteLocatorList);
                }
                group.add_gap(pair.second, remote_readers,
                    mp_RTPSParticipant->network_factory().ShrinkLocatorLists(locatorLists));
            }
        }
        else
        {
            RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages);
            send_heartbeat_nts_(all_remote_readers_, mAllShrinkedLocatorList, group, true);
        }
    }

    if (activateHeartbeatPeriod)
        this->mp_periodicHB->restart_timer();

    // On VOLATILE writers, remove auto-acked (best effort readers) changes
    check_acked_status();

    logInfo(RTPS_WRITER, "Finish sending unsent changes");
}


/*
 *	MATCHED_READER-RELATED METHODS
 */
bool StatefulWriter::matched_reader_add(RemoteReaderAttributes& rdata)
{
    if (rdata.guid == c_Guid_Unknown)
    {
        logError(RTPS_WRITER, "Reliable Writer need GUID_t of matched readers");
        return false;
    }

    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);

    std::vector<LocatorList_t> allLocatorLists;

    // Check if it is already matched.
    for(ReaderProxy* it : matched_readers)
    {
        if(it->m_att.guid == rdata.guid)
        {
            logInfo(RTPS_WRITER, "Attempting to add existing reader" << endl);
            return false;
        }

        allLocatorLists.push_back(it->m_att.endpoint.remoteLocatorList);
    }

    // Add info of new datareader.
    all_remote_readers_.push_back(rdata.guid);
    LocatorList_t locators(rdata.endpoint.unicastLocatorList);
    locators.push_back(rdata.endpoint.multicastLocatorList);
    allLocatorLists.push_back(locators);

    update_cached_info_nts(allLocatorLists);

    getRTPSParticipant()->createSenderResources(mAllShrinkedLocatorList, false);

    rdata.endpoint.unicastLocatorList =
        mp_RTPSParticipant->network_factory().ShrinkLocatorLists({rdata.endpoint.unicastLocatorList});

    ReaderProxy* rp = new ReaderProxy(rdata, m_times, this);
    std::set<SequenceNumber_t> not_relevant_changes;

    SequenceNumber_t current_seq = get_seq_num_min();
    SequenceNumber_t last_seq = get_seq_num_max();

    if(current_seq != SequenceNumber_t::unknown())
    {
        (void)last_seq;
        assert(last_seq != SequenceNumber_t::unknown());
        assert(current_seq <= last_seq);

        for(std::vector<CacheChange_t*>::iterator cit = mp_history->changesBegin();
                cit != mp_history->changesEnd(); ++cit)
        {
            while((*cit)->sequenceNumber != current_seq)
            {
                ChangeForReader_t changeForReader(current_seq);
                changeForReader.setRelevance(false);
                not_relevant_changes.insert(current_seq);
                changeForReader.setStatus(UNACKNOWLEDGED);
                rp->add_change(changeForReader, false);
                ++current_seq;
            }

            ChangeForReader_t changeForReader(*cit);

            if(rp->m_att.endpoint.durabilityKind >= TRANSIENT_LOCAL && this->getAttributes().durabilityKind >= TRANSIENT_LOCAL)
            {
                changeForReader.setRelevance(rp->rtps_is_relevant(*cit));
                if(!rp->rtps_is_relevant(*cit))
                    not_relevant_changes.insert(changeForReader.getSequenceNumber());
            }
            else
            {
                changeForReader.setRelevance(false);
                not_relevant_changes.insert(changeForReader.getSequenceNumber());
            }

            changeForReader.setStatus(UNACKNOWLEDGED);
            rp->add_change(changeForReader, false);
            ++current_seq;
        }

        assert(last_seq + 1 == current_seq);

        std::vector<GUID_t> guids(1, rp->m_att.guid);
        const LocatorList_t& locatorsList = rp->m_att.endpoint.remoteLocatorList;
        RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages, locatorsList, guids);

        // Send initial heartbeat
        send_heartbeat_nts_(guids, locatorsList, group, false);

        // Send Gap
        if(!not_relevant_changes.empty())
        {
            group.add_gap(not_relevant_changes, guids, locatorsList);
        }

        // Always activate heartbeat period. We need a confirmation of the reader.
        // The state has to be updated.
        this->mp_periodicHB->restart_timer();
    }
    else
    {
        send_heartbeat_to_nts(*rp, false);
    }

    matched_readers.push_back(rp);

    logInfo(RTPS_WRITER, "Reader Proxy "<< rp->m_att.guid<< " added to " << this->m_guid.entityId << " with "
            <<rp->m_att.endpoint.unicastLocatorList.size()<<"(u)-"
            <<rp->m_att.endpoint.multicastLocatorList.size()<<"(m) locators");

    return true;
}

bool StatefulWriter::matched_reader_remove(const RemoteReaderAttributes& rdata)
{
    ReaderProxy *rproxy = nullptr;
    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    std::vector<LocatorList_t> allLocatorLists;

    auto it = matched_readers.begin();
    while(it != matched_readers.end())
    {
        if((*it)->m_att.guid == rdata.guid)
        {
            logInfo(RTPS_WRITER, "Reader Proxy removed: " << (*it)->m_att.guid);
            rproxy = std::move(*it);
            it = matched_readers.erase(it);

            continue;
        }

        allLocatorLists.push_back((*it)->m_att.endpoint.remoteLocatorList);
        ++it;
    }

    all_remote_readers_.remove(rdata.guid);
    update_cached_info_nts(allLocatorLists);

    if(matched_readers.size()==0)
        this->mp_periodicHB->cancel_timer();

    lock.unlock();

    if(rproxy != nullptr)
    {
        delete rproxy;

        check_acked_status();

        return true;
    }

    logInfo(RTPS_HISTORY,"Reader Proxy doesn't exist in this writer");
    return false;
}

bool StatefulWriter::matched_reader_is_matched(const RemoteReaderAttributes& rdata)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    for(ReaderProxy* it : matched_readers)
    {
        if(it->m_att.guid == rdata.guid)
        {
            return true;
        }
    }
    return false;
}

bool StatefulWriter::matched_reader_lookup(GUID_t& readerGuid,ReaderProxy** RP)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    for(ReaderProxy* it : matched_readers)
    {
        if(it->m_att.guid == readerGuid)
        {
            *RP = it;
            return true;
        }
    }
    return false;
}

bool StatefulWriter::is_acked_by_all(const CacheChange_t* change) const
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);

    if(change->writerGUID != this->getGuid())
    {
        logWarning(RTPS_WRITER,"The given change is not from this Writer");
        return false;
    }

    return std::all_of(matched_readers.begin(), matched_readers.end(),
        [this, change](const ReaderProxy* reader)
        {
            return reader->change_is_acked(change->sequenceNumber);
        });
}

bool StatefulWriter::wait_for_all_acked(const Duration_t& max_wait)
{
    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);
    std::unique_lock<std::mutex> all_acked_lock(all_acked_mutex_);

    all_acked_ = std::none_of(matched_readers.begin(), matched_readers.end(),
        [](const ReaderProxy* reader)
        {
            return reader->has_changes();
        });
    lock.unlock();

    if(!all_acked_)
    {
        std::chrono::microseconds max_w(::TimeConv::Time_t2MicroSecondsInt64(max_wait));
        all_acked_cond_.wait_for(all_acked_lock, max_w, [&]() { return all_acked_; });
    }

    return all_acked_;
}

void StatefulWriter::check_acked_status()
{
    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    bool all_acked = true;
    SequenceNumber_t min_low_mark;

    for(ReaderProxy* it : matched_readers)
    {
        SequenceNumber_t reader_low_mark = it->get_low_mark();
        if(min_low_mark == SequenceNumber_t() || reader_low_mark < min_low_mark)
        {
            min_low_mark = reader_low_mark;
        }

        if(it->has_changes())
        {
            all_acked = false;
        }
    }

    if(get_seq_num_min() != SequenceNumber_t::unknown())
    {
        // Inform of samples acked.
        if(mp_listener != nullptr)
        {
            std::vector<CacheChange_t*> all_acked_changes;
            for(SequenceNumber_t current_seq = get_seq_num_min(); current_seq <= min_low_mark; ++current_seq)
            {
                for(std::vector<CacheChange_t*>::iterator cit = mp_history->changesBegin();
                        cit != mp_history->changesEnd(); ++cit)
                {
                    if((*cit)->sequenceNumber == current_seq)
                    {
                        all_acked_changes.push_back(*cit);
                    }
                }
            }
            for(auto cit = all_acked_changes.begin(); cit != all_acked_changes.end(); ++cit)
            {
                mp_listener->onWriterChangeReceivedByAll(this, *cit);
            }
        }

        SequenceNumber_t calc = min_low_mark < get_seq_num_min() ? SequenceNumber_t() :
            (min_low_mark - get_seq_num_min()) + 1;
        if (calc > SequenceNumber_t())
        {
            std::unique_lock<std::mutex> may_lock(may_remove_change_mutex_);
            may_remove_change_ = 1;
            may_remove_change_cond_.notify_one();
        }
    }

    if(all_acked)
    {
        std::unique_lock<std::mutex> all_acked_lock(all_acked_mutex_);
        all_acked_ = true;
        all_acked_cond_.notify_all();
    }
}

bool StatefulWriter::try_remove_change(std::chrono::microseconds& microseconds,
        std::unique_lock<std::recursive_mutex>& lock)
{
    logInfo(RTPS_WRITER, "Starting process try remove change for writer " << getGuid());

    SequenceNumber_t min_low_mark;

    for(ReaderProxy* it : matched_readers)
    {
        SequenceNumber_t reader_low_mark = it->get_low_mark();
        if (min_low_mark == SequenceNumber_t() || reader_low_mark < min_low_mark)
        {
            min_low_mark = reader_low_mark;
        }
    }

    SequenceNumber_t calc = min_low_mark < get_seq_num_min() ? SequenceNumber_t() :
        (min_low_mark - get_seq_num_min()) + 1;
    unsigned int may_remove_change = 1;

    if(calc <= SequenceNumber_t())
    {
        lock.unlock();
        std::unique_lock<std::mutex> may_lock(may_remove_change_mutex_);
        may_remove_change_ = 0;
        may_remove_change_cond_.wait_for(may_lock, microseconds,
                [&]() { return may_remove_change_ > 0; });
        may_remove_change = may_remove_change_;
        may_lock.unlock();
        lock.lock();
    }

    // Some changes acked
    if(may_remove_change == 1)
    {
        return mp_history->remove_min_change();
    }
    // Waiting a change was removed.
    else if(may_remove_change == 2)
    {
        return true;
    }

    return false;
}

/*
 * PARAMETER_RELATED METHODS
 */
void StatefulWriter::updateAttributes(const WriterAttributes& att)
{
    this->updateTimes(att.times);
}

void StatefulWriter::updateTimes(const WriterTimes& times)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    if(m_times.heartbeatPeriod != times.heartbeatPeriod)
    {
        this->mp_periodicHB->update_interval(times.heartbeatPeriod);
    }
    if(m_times.nackResponseDelay != times.nackResponseDelay)
    {
        for(ReaderProxy* it : matched_readers)
        {
            it->update_nack_response_interval(times.nackResponseDelay);
        }
    }
    if(m_times.nackSupressionDuration != times.nackSupressionDuration)
    {
        for (ReaderProxy* it : matched_readers)
        {
            it->update_nack_supression_interval(times.nackSupressionDuration);
        }
    }
    m_times = times;
}

void StatefulWriter::add_flow_controller(std::unique_ptr<FlowController> controller)
{
    m_controllers.push_back(std::move(controller));
}

SequenceNumber_t StatefulWriter::next_sequence_number() const
{
    return mp_history->next_sequence_number();
}

bool StatefulWriter::send_periodic_heartbeat()
{
    std::lock_guard<std::recursive_mutex> guardW(*mp_mutex);

    bool unacked_changes = false;
    if (m_separateSendingEnabled)
    {

        for (ReaderProxy* it : matched_readers)
        {
            if (it->has_unacknowledged())
            {
                // FinalFlag is always false because this class is used only by StatefulWriter in Reliable.
                send_heartbeat_to_nts(*it, false);
                unacked_changes = true;
            }
        }
    }
    else
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

            unacked_changes = std::any_of(matched_readers.begin(), matched_readers.end(),
                [](const ReaderProxy* reader)
                {
                    return reader->has_unacknowledged();
                });

            if (unacked_changes)
            {
                RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages,
                    mAllShrinkedLocatorList, all_remote_readers_);
                send_heartbeat_nts_(all_remote_readers_, mAllShrinkedLocatorList, group, false);
            }
        }
    }

    return unacked_changes;
}

void StatefulWriter::send_heartbeat_to_nts(
    ReaderProxy& remoteReaderProxy, 
    bool final)
{
    std::vector<GUID_t> tmp_guids(1, remoteReaderProxy.m_att.guid);
    const LocatorList_t& locators = remoteReaderProxy.m_att.endpoint.remoteLocatorList;
    RTPSMessageGroup group(mp_RTPSParticipant, this, RTPSMessageGroup::WRITER, m_cdrmessages,
        locators, tmp_guids);

    send_heartbeat_nts_(tmp_guids, locators, group, final);
}

void StatefulWriter::send_heartbeat_nts_(
    const std::vector<GUID_t>& remote_readers, 
    const LocatorList_t& locators,
    RTPSMessageGroup& message_group, 
    bool final)
{
    SequenceNumber_t firstSeq = get_seq_num_min();
    SequenceNumber_t lastSeq = get_seq_num_max();

    if (firstSeq == c_SequenceNumber_Unknown || lastSeq == c_SequenceNumber_Unknown)
    {
        assert(firstSeq == c_SequenceNumber_Unknown && lastSeq == c_SequenceNumber_Unknown);

        if(remote_readers.size() == 1)
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

    // FinalFlag is always false because this class is used only by StatefulWriter in Reliable.
    message_group.add_heartbeat(remote_readers,
            firstSeq, lastSeq, m_heartbeatCount, final, false, locators);
    // Update calculate of heartbeat piggyback.
    currentUsageSendBufferSize_ = static_cast<int32_t>(sendBufferSize_);

    logInfo(RTPS_WRITER, getGuid().entityId << " Sending Heartbeat (" << firstSeq << " - " << lastSeq <<")" );
}

void StatefulWriter::send_heartbeat_piggyback_nts_(
    const std::vector<GUID_t>& remote_readers, 
    const LocatorList_t& locators,
    RTPSMessageGroup& message_group,
    uint32_t& last_bytes_processed)
{
    if (!disableHeartbeatPiggyback_)
    {
        if (mp_history->isFull())
        {
            send_heartbeat_nts_(remote_readers, locators, message_group, false);
        }
        else
        {
            uint32_t current_bytes = message_group.get_current_bytes_processed();
            currentUsageSendBufferSize_ -= current_bytes - last_bytes_processed;
            last_bytes_processed = current_bytes;
            if (currentUsageSendBufferSize_ < 0)
            {
                send_heartbeat_nts_(remote_readers, locators, message_group, false);
            }
        }
    }
}

void StatefulWriter::send_heartbeat_piggyback_nts_(
    RTPSMessageGroup& message_group,
    uint32_t& last_bytes_processed)
{
    send_heartbeat_piggyback_nts_(all_remote_readers_, mAllShrinkedLocatorList, message_group, last_bytes_processed);
}

void StatefulWriter::perform_nack_response(const GUID_t& reader_guid)
{
    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    for (auto remote_reader : matched_readers)
    {
        if (remote_reader->m_att.guid == reader_guid)
        {
            if (remote_reader->convert_status_on_all_changes(REQUESTED, UNSENT))
            {
                AsyncWriterThread::wakeUp(this);
            }
            return;
        }
    }
}

void StatefulWriter::perform_nack_supression(const GUID_t& reader_guid)
{
    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    for (auto remote_reader : matched_readers)
    {
        if (remote_reader->m_att.guid == reader_guid)
        {
            if (remote_reader->m_att.endpoint.reliabilityKind == RELIABLE)
            {
                remote_reader->convert_status_on_all_changes(UNDERWAY, UNACKNOWLEDGED);
                mp_periodicHB->restart_timer();
            }
            return;
        }
    }
}

void StatefulWriter::process_acknack(
        const GUID_t reader_guid, 
        uint32_t ack_count,
        const SequenceNumberSet_t& sn_set, 
        bool final_flag)
{
    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    for(auto remote_reader : matched_readers)
    {
        if(remote_reader->m_att.guid == reader_guid)
        {
            if(remote_reader->check_and_set_acknack_count(ack_count))
            {
                if(sn_set.base() != SequenceNumber_t(0, 0))
                {
                    // Sequence numbers before Base are set as Acknowledged.
                    remote_reader->acked_changes_set(sn_set.base());
                    if (remote_reader->requested_changes_set(sn_set))
                    {
                    }
                    else if(!final_flag)
                    {
                        mp_periodicHB->restart_timer();
                    }
                }
                else if(sn_set.empty() && !final_flag)
                {
                    send_heartbeat_to_nts(*remote_reader, true);
                }

                // Check if all CacheChange are acknowledge, because a user could be waiting
                // for this, of if VOLATILE should be removed CacheChanges
                check_acked_status();
            }
            break;
        }
    }
}
