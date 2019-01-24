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


#include <fastrtps/log/Log.h>
#include <fastrtps/rtps/history/WriterHistory.h>
#include <fastrtps/rtps/resources/AsyncWriterThread.h>
#include <fastrtps/rtps/writer/ReaderProxy.h>
#include <fastrtps/rtps/writer/StatefulWriter.h>
#include <fastrtps/rtps/writer/timedevent/NackResponseDelay.h>
#include <fastrtps/rtps/writer/timedevent/NackSupressionDuration.h>
#include <fastrtps/utils/TimeConversion.h>

#include <mutex>

#include <cassert>


namespace eprosima {
namespace fastrtps {
namespace rtps {


ReaderProxy::ReaderProxy(
        const RemoteReaderAttributes& rdata, 
        const WriterTimes& times, 
        StatefulWriter* SW) 
    : m_att(rdata)
    , mp_SFW(SW)
    , mp_nackResponse(nullptr)
    , mp_nackSupression(nullptr)
    , m_lastAcknackCount(0)
    , lastNackfragCount_(0)
{
    if (rdata.endpoint.reliabilityKind == RELIABLE)
    {
        mp_nackResponse = new NackResponseDelay(mp_SFW, rdata.guid, 
            TimeConv::Time_t2MilliSecondsDouble(times.nackResponseDelay));
        mp_nackSupression = new NackSupressionDuration(mp_SFW, rdata.guid, 
            TimeConv::Time_t2MilliSecondsDouble(times.nackSupressionDuration));
    }

    // Use remoteLocatorList as joint unicast + multicast locators
    m_att.endpoint.remoteLocatorList.assign(m_att.endpoint.unicastLocatorList);
    m_att.endpoint.remoteLocatorList.push_back(m_att.endpoint.multicastLocatorList);

    logInfo(RTPS_WRITER, "Reader Proxy created");
}


ReaderProxy::~ReaderProxy()
{
    destroy_timers();
}

void ReaderProxy::destroy_timers()
{
    if (mp_nackResponse != nullptr)
    {
        delete(mp_nackResponse);
        mp_nackResponse = nullptr;
    }

    if (mp_nackSupression != nullptr)
    {
        delete(mp_nackSupression);
        mp_nackSupression = nullptr;
    }
}

void ReaderProxy::addChange(const ChangeForReader_t& change)
{
    assert(change.getSequenceNumber() > changesFromRLowMark_);
    assert(m_changesForReader.rbegin() != m_changesForReader.rend() ?
        change.getSequenceNumber() > m_changesForReader.rbegin()->getSequenceNumber() :
        true);

    // For best effort readers, changes are acked when being sent
    if (m_changesForReader.empty() && change.getStatus() == ACKNOWLEDGED)
    {
        changesFromRLowMark_ = change.getSequenceNumber();
        return;
    }

    m_changesForReader.insert(change);
    //TODO (Ricardo) Remove this functionality from here. It is not his place.
    if (change.getStatus() == UNSENT)
    {
        AsyncWriterThread::wakeUp(mp_SFW);
    }
}

size_t ReaderProxy::countChangesForReader() const
{
    return m_changesForReader.size();
}

bool ReaderProxy::change_is_acked(const SequenceNumber_t& sequence_number) const
{
    if (sequence_number <= changesFromRLowMark_)
    {
        return true;
    }

    auto chit = m_changesForReader.find(ChangeForReader_t(sequence_number));
    assert(chit != m_changesForReader.end());

    return !chit->isRelevant() || chit->getStatus() == ACKNOWLEDGED;
}

void ReaderProxy::acked_changes_set(const SequenceNumber_t& seqNum)
{
    SequenceNumber_t future_low_mark = seqNum;

    if (seqNum > changesFromRLowMark_)
    {
        auto chit = m_changesForReader.find(seqNum);
        m_changesForReader.erase(m_changesForReader.begin(), chit);
    }
    else
    {
        // Special case. Currently only used on Builtin StatefulWriters
        // after losing lease duration.

        SequenceNumber_t current_sequence = seqNum;
        if (seqNum < mp_SFW->get_seq_num_min())
        {
            current_sequence = mp_SFW->get_seq_num_min();
        }
        future_low_mark = current_sequence;

        for (; current_sequence <= changesFromRLowMark_; ++current_sequence)
        {
            CacheChange_t* change = nullptr;

            if (mp_SFW->mp_history->get_change(current_sequence, mp_SFW->getGuid(), &change))
            {
                ChangeForReader_t cr(change);
                cr.setStatus(UNACKNOWLEDGED);
                m_changesForReader.insert(cr);
            }
            else
            {
                ChangeForReader_t cr;
                cr.setStatus(UNACKNOWLEDGED);
                cr.notValid();
                m_changesForReader.insert(cr);
            }
        }
    }

    changesFromRLowMark_ = future_low_mark - 1;
}

bool ReaderProxy::requested_changes_set(const SequenceNumberSet_t& seqNumSet)
{
    bool isSomeoneWasSetRequested = false;

    seqNumSet.for_each([&](SequenceNumber_t sit)
    {
        auto chit = m_changesForReader.find(ChangeForReader_t(sit));

        if (chit != m_changesForReader.end())
        {
            ChangeForReader_t newch(*chit);
            newch.setStatus(REQUESTED);
            newch.markAllFragmentsAsUnsent();

            auto hint = m_changesForReader.erase(chit);

            m_changesForReader.insert(hint, newch);

            isSomeoneWasSetRequested = true;
        }
    });

    if (isSomeoneWasSetRequested)
    {
        logInfo(RTPS_WRITER, "Requested Changes: " << seqNumSet);
    }
    else if (!seqNumSet.empty())
    {
        logWarning(RTPS_WRITER, "Requested Changes: " << seqNumSet
            << " not found (low mark: " << changesFromRLowMark_ << ")");
    }

    return isSomeoneWasSetRequested;
}

void ReaderProxy::set_change_to_status(const SequenceNumber_t& seq_num, ChangeForReaderStatus_t status)
{
    if (seq_num <= changesFromRLowMark_)
    {
        return;
    }

    auto it = m_changesForReader.find(ChangeForReader_t(seq_num));
    bool mustWakeUpAsyncThread = false;

    if (it != m_changesForReader.end())
    {
        if (status == ACKNOWLEDGED && it == m_changesForReader.begin())
        {
            m_changesForReader.erase(it);
            changesFromRLowMark_ = seq_num;
        }
        else
        {
            ChangeForReader_t newch(*it);
            newch.setStatus(status);
            if (status == UNSENT)
            {
                mustWakeUpAsyncThread = true;
            }
            auto hint = m_changesForReader.erase(it);
            m_changesForReader.insert(hint, newch);
        }
    }

    if (mustWakeUpAsyncThread)
    {
        AsyncWriterThread::wakeUp(mp_SFW);
    }
}

bool ReaderProxy::mark_fragment_as_sent_for_change(const CacheChange_t* change, FragmentNumber_t fragment)
{
    if (change->sequenceNumber <= changesFromRLowMark_)
    {
        return false;
    }

    bool allFragmentsSent = false;
    auto it = m_changesForReader.find(ChangeForReader_t(change->sequenceNumber));

    bool mustWakeUpAsyncThread = false;

    if (it != m_changesForReader.end())
    {
        ChangeForReader_t newch(*it);
        newch.markFragmentsAsSent(fragment);
        if (newch.getUnsentFragments().empty())
        {
            allFragmentsSent = true;
        }
        else
        {
            mustWakeUpAsyncThread = true;
        }
        auto hint = m_changesForReader.erase(it);
        m_changesForReader.insert(hint, newch);
    }

    if (mustWakeUpAsyncThread)
    {
        AsyncWriterThread::wakeUp(mp_SFW);
    }

    return allFragmentsSent;
}

void ReaderProxy::convert_status_on_all_changes(ChangeForReaderStatus_t previous, ChangeForReaderStatus_t next)
{
    bool mustWakeUpAsyncThread = false;

    auto it = m_changesForReader.begin();
    while (it != m_changesForReader.end())
    {
        if (it->getStatus() == previous)
        {
            if (next == ACKNOWLEDGED && it == m_changesForReader.begin())
            {
                changesFromRLowMark_ = it->getSequenceNumber();
                it = m_changesForReader.erase(it);
                continue;
            }


                // Note: we can perform this cast as we are not touching the sorting field (seq_num)
            const_cast<ChangeForReader_t&>(*it).setStatus(next);
            if (next == UNSENT && previous != UNSENT)
            {
                mustWakeUpAsyncThread = true;
            }

        }

        ++it;
    }

    if (mustWakeUpAsyncThread)
    {
        AsyncWriterThread::wakeUp(mp_SFW);
    }
}

//TODO(Ricardo)
//void ReaderProxy::setNotValid(const CacheChange_t* change)
void ReaderProxy::setNotValid(CacheChange_t* change)
{
    // Check sequence number is in the container, because it was not clean up.
    if (m_changesForReader.empty() || change->sequenceNumber < m_changesForReader.begin()->getSequenceNumber())
    {
        return;
    }

    auto chit = m_changesForReader.find(ChangeForReader_t(change));

    // Element must be in the container. In other case, bug.
    assert(chit != m_changesForReader.end());

    // Note: we can perform this cast as we are not touching the sorting field (seq_num)
    auto & newch = const_cast<ChangeForReader_t&>(*chit);

    if (chit == m_changesForReader.begin())
    {
        assert(chit->getStatus() != ACKNOWLEDGED);

        // if it is the first element, set state to unacknowledge because from now reader has to confirm
        // it will not be expecting it.
        // Note: we can perform this cast as we are not touching the sorting field (seq_num)
        newch.setStatus(UNACKNOWLEDGED);
    }
    else
    {
        // In case its state is not ACKNOWLEDGED, set it to UNACKNOWLEDGE because from now reader has to confirm
        // it will not be expecting it.
        if (newch.getStatus() != ACKNOWLEDGED)
        {
            newch.setStatus(UNACKNOWLEDGED);
        }
    }
    newch.notValid();
}

bool ReaderProxy::thereIsUnacknowledged() const
{
    for (const ChangeForReader_t& it : m_changesForReader)
    {
        if (it.getStatus() == UNACKNOWLEDGED)
        {
            return true;
        }
    }

    return false;
}

bool change_min(const ChangeForReader_t* ch1, const ChangeForReader_t* ch2)
{
    return ch1->getSequenceNumber() < ch2->getSequenceNumber();
}

bool ReaderProxy::minChange(std::vector<ChangeForReader_t*>* Changes,
    ChangeForReader_t* changeForReader)
{
    *changeForReader = **std::min_element(Changes->begin(), Changes->end(), change_min);
    return true;
}

bool ReaderProxy::requested_fragment_set(
        const SequenceNumber_t& sequence_number, 
        const FragmentNumberSet_t& frag_set)
{
    // Locate the outbound change referenced by the NACK_FRAG
    auto changeIter = std::find_if(m_changesForReader.begin(), m_changesForReader.end(),
        [sequence_number](const ChangeForReader_t& change)
    {
        return change.getSequenceNumber() == sequence_number;
    });
    if (changeIter == m_changesForReader.end())
    {
        return false;
    }

    ChangeForReader_t& newch = const_cast<ChangeForReader_t&>(*changeIter);
    newch.markFragmentsAsUnsent(frag_set);

    // If it was UNSENT, we shouldn't switch back to REQUESTED to prevent stalling.
    if (newch.getStatus() != UNSENT)
    {
        newch.setStatus(REQUESTED);
    }

    return true;
}

bool ReaderProxy::process_nack_frag(
        const GUID_t& reader_guid, 
        uint32_t nack_count,
        const SequenceNumber_t& sequence_number,
        const FragmentNumberSet_t& fragments_state)
{
    if (m_att.guid == reader_guid)
    {
        if (lastNackfragCount_ < nack_count)
        {
            lastNackfragCount_ = nack_count;
            // TODO Not doing Acknowledged.
            if (requested_fragment_set(sequence_number, fragments_state))
            {
                mp_nackResponse->restart_timer();
            }
        }

        return true;
    }

    return false;
}

}   // namespace rtps
}   // namespace fastrtps
}   // namespace eprosima