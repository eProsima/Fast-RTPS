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
 * Publisher.cpp
 *
 */
#include <fastrtps/config.h>

#include <fastrtps_deprecated/publisher/PublisherImpl.h>
#include <fastrtps_deprecated/participant/ParticipantImpl.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastrtps/publisher/PublisherListener.h>

#include <fastdds/rtps/writer/RTPSWriter.h>
#include <fastdds/rtps/writer/StatefulWriter.h>
#include <fastdds/rtps/writer/StatelessWriter.h>

#include <fastdds/rtps/participant/RTPSParticipant.h>
#include <fastdds/rtps/RTPSDomain.h>

#include <fastdds/dds/log/Log.hpp>
#include <fastrtps/utils/TimeConversion.h>
#include <fastdds/rtps/resources/ResourceEvent.h>
#include <fastdds/rtps/resources/TimedEvent.h>
#include <fastdds/rtps/builtin/BuiltinProtocols.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>

#include <rtps/history/TopicPayloadPoolRegistry.hpp>
#include <rtps/participant/RTPSParticipantImpl.h>

using namespace eprosima::fastrtps;
using namespace ::rtps;
using namespace std::chrono;

using eprosima::fastdds::dds::TopicDataType;

PublisherImpl::PublisherImpl(
        ParticipantImpl* p,
        TopicDataType* pdatatype,
        const PublisherAttributes& att,
        PublisherListener* listen )
    : mp_participant(p)
    , mp_writer(nullptr)
    , mp_type(pdatatype)
    , m_att(att)
#pragma warning (disable : 4355 )
    , m_history(att.topic,
            pdatatype->m_typeSize
#if HAVE_SECURITY
            // In future v2 changepool is in writer, and writer set this value to cachechagepool.
            + 20 /*SecureDataHeader*/ + 4 + ((2 * 16) /*EVP_MAX_IV_LENGTH max block size*/ - 1 ) /* SecureDataBodey*/
            + 16 + 4 /*SecureDataTag*/
#endif // if HAVE_SECURITY
            , att.historyMemoryPolicy)
    , mp_listener(listen)
#pragma warning (disable : 4355 )
    , m_writerListener(this)
    , mp_userPublisher(nullptr)
    , mp_rtpsParticipant(nullptr)
    , deadline_duration_us_(m_att.qos.m_deadline.period.to_ns() * 1e-3)
    , timer_owner_()
    , deadline_missed_status_()
    , lifespan_duration_us_(m_att.qos.m_lifespan.duration.to_ns() * 1e-3)
{
    std::string topic_name = m_att.topic.getTopicName().to_string();
    PoolConfig pool_cfg = PoolConfig::from_history_attributes(m_history.m_att);
    payload_pool_ = TopicPayloadPoolRegistry::get(topic_name, pool_cfg);
    payload_pool_->reserve_history(pool_cfg, false);

    deadline_timer_ = new TimedEvent(mp_participant->get_resource_event(),
                    [&]() -> bool
                    {
                        return deadline_missed();
                    },
                    att.qos.m_deadline.period.to_ns() * 1e-6);

    lifespan_timer_ = new TimedEvent(mp_participant->get_resource_event(),
                    [&]() -> bool
                    {
                        return lifespan_expired();
                    },
                    m_att.qos.m_lifespan.duration.to_ns() * 1e-6);
}

PublisherImpl::~PublisherImpl()
{
    delete(lifespan_timer_);
    delete(deadline_timer_);

    if (mp_writer != nullptr)
    {
        logInfo(PUBLISHER, this->getGuid().entityId << " in topic: " << this->m_att.topic.topicName);
    }

    RTPSDomain::removeRTPSWriter(mp_writer);
    delete(this->mp_userPublisher);

    std::string topic_name = m_att.topic.getTopicName().to_string();
    PoolConfig pool_cfg = PoolConfig::from_history_attributes(m_history.m_att);
    payload_pool_->release_history(pool_cfg, false);
}

bool PublisherImpl::create_new_change(
        ChangeKind_t changeKind,
        void* data)
{
    WriteParams wparams;
    return create_new_change_with_params(changeKind, data, wparams);
}

bool PublisherImpl::create_new_change_with_params(
        ChangeKind_t changeKind,
        void* data,
        WriteParams& wparams)
{
    InstanceHandle_t handle;
    if (m_att.topic.topicKind == WITH_KEY)
    {
        bool is_key_protected = false;
#if HAVE_SECURITY
        is_key_protected = mp_writer->getAttributes().security_attributes().is_key_protected;
#endif // if HAVE_SECURITY
        mp_type->getKey(data, &handle, is_key_protected);
    }

    return create_new_change_with_params(changeKind, data, wparams, handle);
}

bool PublisherImpl::create_new_change_with_params(
        ChangeKind_t changeKind,
        void* data,
        WriteParams& wparams,
        const InstanceHandle_t& handle)
{
    /// Preconditions
    if (data == nullptr)
    {
        logError(PUBLISHER, "Data pointer not valid");
        return false;
    }

    if (changeKind == NOT_ALIVE_UNREGISTERED || changeKind == NOT_ALIVE_DISPOSED ||
            changeKind == NOT_ALIVE_DISPOSED_UNREGISTERED)
    {
        if (m_att.topic.topicKind == NO_KEY)
        {
            logError(PUBLISHER, "Topic is NO_KEY, operation not permitted");
            return false;
        }
    }

    // Block lowlevel writer
    auto max_blocking_time = steady_clock::now() +
            microseconds(::TimeConv::Time_t2MicroSecondsInt64(m_att.qos.m_reliability.max_blocking_time));

#if HAVE_STRICT_REALTIME
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex(), std::defer_lock);
    if (lock.try_lock_until(max_blocking_time))
#else
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());
#endif // if HAVE_STRICT_REALTIME
    {
        CacheChange_t* ch = mp_writer->new_change(mp_type->getSerializedSizeProvider(data), changeKind, handle);
        if (ch != nullptr)
        {
            if (changeKind == ALIVE)
            {
                //If these two checks are correct, we asume the cachechange is valid and thwn we can write to it.
                if (!mp_type->serialize(data, &ch->serializedPayload))
                {
                    logWarning(RTPS_WRITER, "RTPSWriter:Serialization returns false"; );
                    mp_writer->release_change(ch);
                    return false;
                }
            }

            InstanceHandle_t change_handle = ch->instanceHandle;
            if (!this->m_history.add_pub_change(ch, wparams, lock, max_blocking_time))
            {
                mp_writer->release_change(ch);
                return false;
            }

            if (m_att.qos.m_deadline.period != c_TimeInfinite)
            {
                if (!m_history.set_next_deadline(
                            change_handle,
                            steady_clock::now() + duration_cast<system_clock::duration>(deadline_duration_us_)))
                {
                    logError(PUBLISHER, "Could not set the next deadline in the history");
                }
                else
                {
                    if (timer_owner_ == handle || timer_owner_ == InstanceHandle_t())
                    {
                        if (deadline_timer_reschedule())
                        {
                            deadline_timer_->cancel_timer();
                            deadline_timer_->restart_timer();
                        }
                    }
                }
            }

            if (m_att.qos.m_lifespan.duration != c_TimeInfinite)
            {
                lifespan_duration_us_ = duration<double, std::ratio<1, 1000000>>(
                    m_att.qos.m_lifespan.duration.to_ns() * 1e-3);
                lifespan_timer_->update_interval_millisec(m_att.qos.m_lifespan.duration.to_ns() * 1e-6);
                lifespan_timer_->restart_timer();
            }

            return true;
        }
    }

    return false;
}

InstanceHandle_t PublisherImpl::register_instance(
        void* instance)
{
    /// Preconditions
    if (instance == nullptr)
    {
        logError(PUBLISHER, "Data pointer not valid");
        return c_InstanceHandle_Unknown;
    }

    if (m_att.topic.topicKind == NO_KEY)
    {
        logError(PUBLISHER, "Topic is NO_KEY, operation not permitted");
        return c_InstanceHandle_Unknown;
    }

    InstanceHandle_t instance_handle = c_InstanceHandle_Unknown;
    bool is_key_protected = false;
#if HAVE_SECURITY
    is_key_protected = mp_writer->getAttributes().security_attributes().is_key_protected;
#endif // if HAVE_SECURITY
    mp_type->getKey(instance, &instance_handle, is_key_protected);

    // Block lowlevel writer
    auto max_blocking_time = std::chrono::steady_clock::now() +
            std::chrono::microseconds(::TimeConv::Time_t2MicroSecondsInt64(m_att.qos.m_reliability.max_blocking_time));

#if HAVE_STRICT_REALTIME
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex(), std::defer_lock);
    if (lock.try_lock_until(max_blocking_time))
#else
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());
#endif // if HAVE_STRICT_REALTIME
    {
        if (m_history.register_instance(instance_handle, lock, max_blocking_time))
        {
            return instance_handle;
        }
    }

    return c_InstanceHandle_Unknown;
}

bool PublisherImpl::unregister_instance(
        void* instance,
        const InstanceHandle_t& handle,
        bool dispose)
{
    /// Preconditions
    if (instance == nullptr)
    {
        logError(PUBLISHER, "Data pointer not valid");
        return false;
    }

    if (m_att.topic.topicKind == NO_KEY)
    {
        logError(PUBLISHER, "Topic is NO_KEY, operation not permitted");
        return false;
    }

    bool returned_value = false;
    InstanceHandle_t ih = handle;

#if !defined(NDEBUG)
    if (c_InstanceHandle_Unknown == ih)
#endif // if !defined(NDEBUG)
    {
        bool is_key_protected = false;
#if HAVE_SECURITY
        is_key_protected = mp_writer->getAttributes().security_attributes().is_key_protected;
#endif // if HAVE_SECURITY
        mp_type->getKey(instance, &ih, is_key_protected);
    }

#if !defined(NDEBUG)
    if (c_InstanceHandle_Unknown != handle && ih != handle)
    {
        logError(PUBLISHER, "handle differs from data's key.");
        return false;
    }
#endif // if !defined(NDEBUG)

    if (m_history.is_key_registered(ih))
    {
        WriteParams wparams;
        ChangeKind_t change_kind = dispose ? NOT_ALIVE_DISPOSED : NOT_ALIVE_UNREGISTERED;
        returned_value = create_new_change_with_params(change_kind, instance, wparams, ih);
    }

    return returned_value;
}

bool PublisherImpl::removeMinSeqChange()
{
    return m_history.removeMinChange();
}

bool PublisherImpl::removeAllChange(
        size_t* removed)
{
    return m_history.removeAllChange(removed);
}

const GUID_t& PublisherImpl::getGuid()
{
    return mp_writer->getGuid();
}

//
bool PublisherImpl::updateAttributes(
        const PublisherAttributes& att)
{
    bool updated = true;
    bool missing = false;
    if (this->m_att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS)
    {
        if (att.unicastLocatorList.size() != this->m_att.unicastLocatorList.size() ||
                att.multicastLocatorList.size() != this->m_att.multicastLocatorList.size())
        {
            logWarning(PUBLISHER, "Locator Lists cannot be changed or updated in this version");
            updated &= false;
        }
        else
        {
            for (LocatorListConstIterator lit1 = this->m_att.unicastLocatorList.begin();
                    lit1 != this->m_att.unicastLocatorList.end(); ++lit1)
            {
                missing = true;
                for (LocatorListConstIterator lit2 = att.unicastLocatorList.begin();
                        lit2 != att.unicastLocatorList.end(); ++lit2)
                {
                    if (*lit1 == *lit2)
                    {
                        missing = false;
                        break;
                    }
                }
                if (missing)
                {
                    logWarning(PUBLISHER, "Locator: " << *lit1 << " not present in new list");
                    logWarning(PUBLISHER, "Locator Lists cannot be changed or updated in this version");
                }
            }
            for (LocatorListConstIterator lit1 = this->m_att.multicastLocatorList.begin();
                    lit1 != this->m_att.multicastLocatorList.end(); ++lit1)
            {
                missing = true;
                for (LocatorListConstIterator lit2 = att.multicastLocatorList.begin();
                        lit2 != att.multicastLocatorList.end(); ++lit2)
                {
                    if (*lit1 == *lit2)
                    {
                        missing = false;
                        break;
                    }
                }
                if (missing)
                {
                    logWarning(PUBLISHER, "Locator: " << *lit1 << " not present in new list");
                    logWarning(PUBLISHER, "Locator Lists cannot be changed or updated in this version");
                }
            }
        }
    }

    //TOPIC ATTRIBUTES
    if (this->m_att.topic != att.topic)
    {
        logWarning(PUBLISHER, "Topic Attributes cannot be updated");
        updated &= false;
    }
    //QOS:
    //CHECK IF THE QOS CAN BE SET
    if (!this->m_att.qos.canQosBeUpdated(att.qos))
    {
        updated &= false;
    }
    if (updated)
    {
        if (this->m_att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS)
        {
            //UPDATE TIMES:
            StatefulWriter* sfw = (StatefulWriter*)mp_writer;
            sfw->updateTimes(att.times);
        }

        this->m_att.qos.setQos(att.qos, false);
        this->m_att = att;
        //Notify the participant that a Writer has changed its QOS
        mp_rtpsParticipant->updateWriter(this->mp_writer, m_att.topic, m_att.qos);

        // Deadline

        if (m_att.qos.m_deadline.period != c_TimeInfinite)
        {
            deadline_duration_us_ =
                    duration<double, std::ratio<1, 1000000>>(m_att.qos.m_deadline.period.to_ns() * 1e-3);
            deadline_timer_->update_interval_millisec(m_att.qos.m_deadline.period.to_ns() * 1e-6);
        }
        else
        {
            deadline_timer_->cancel_timer();
        }

        // Lifespan

        if (m_att.qos.m_lifespan.duration != c_TimeInfinite)
        {
            lifespan_duration_us_ =
                    duration<double, std::ratio<1, 1000000>>(m_att.qos.m_lifespan.duration.to_ns() * 1e-3);
            lifespan_timer_->update_interval_millisec(m_att.qos.m_lifespan.duration.to_ns() * 1e-6);
        }
        else
        {
            lifespan_timer_->cancel_timer();
        }
    }

    return updated;
}

void PublisherImpl::PublisherWriterListener::onWriterMatched(
        RTPSWriter* /*writer*/,
        MatchingInfo& info)
{
    if ( mp_publisherImpl->mp_listener != nullptr )
    {
        mp_publisherImpl->mp_listener->onPublicationMatched(mp_publisherImpl->mp_userPublisher, info);
    }
}

void PublisherImpl::PublisherWriterListener::onWriterChangeReceivedByAll(
        RTPSWriter* /*writer*/,
        CacheChange_t* ch)
{
    if (mp_publisherImpl->m_att.topic.topicKind == WITH_KEY &&
            (NOT_ALIVE_UNREGISTERED == ch->kind ||
            NOT_ALIVE_DISPOSED_UNREGISTERED == ch->kind))
    {
        mp_publisherImpl->m_history.remove_instance_changes(ch->instanceHandle, ch->sequenceNumber);
    }
    else if (mp_publisherImpl->m_att.qos.m_durability.kind == VOLATILE_DURABILITY_QOS)
    {
        mp_publisherImpl->m_history.remove_change_g(ch);
    }
}

void PublisherImpl::PublisherWriterListener::on_liveliness_lost(
        RTPSWriter* writer,
        const LivelinessLostStatus& status)
{
    (void)writer;

    if (mp_publisherImpl->mp_listener != nullptr)
    {
        mp_publisherImpl->mp_listener->on_liveliness_lost(
            mp_publisherImpl->mp_userPublisher,
            status);
    }
}

bool PublisherImpl::wait_for_all_acked(
        const eprosima::fastrtps::Duration_t& max_wait)
{
    return mp_writer->wait_for_all_acked(max_wait);
}

bool PublisherImpl::deadline_timer_reschedule()
{
    assert(m_att.qos.m_deadline.period != c_TimeInfinite);

    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());

    steady_clock::time_point next_deadline_us;
    if (!m_history.get_next_deadline(timer_owner_, next_deadline_us))
    {
        logError(PUBLISHER, "Could not get the next deadline from the history");
        return false;
    }

    auto interval_ms = duration_cast<milliseconds>(next_deadline_us - steady_clock::now());
    deadline_timer_->update_interval_millisec((double)interval_ms.count());
    return true;

}

bool PublisherImpl::deadline_missed()
{
    assert(m_att.qos.m_deadline.period != c_TimeInfinite);

    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());

    deadline_missed_status_.total_count++;
    deadline_missed_status_.total_count_change++;
    deadline_missed_status_.last_instance_handle = timer_owner_;
    mp_listener->on_offered_deadline_missed(mp_userPublisher, deadline_missed_status_);
    deadline_missed_status_.total_count_change = 0;

    if (!m_history.set_next_deadline(
                timer_owner_,
                steady_clock::now() + duration_cast<system_clock::duration>(deadline_duration_us_)))
    {
        logError(PUBLISHER, "Could not set the next deadline in the history");
        return false;
    }

    return deadline_timer_reschedule();
}

void PublisherImpl::get_offered_deadline_missed_status(
        OfferedDeadlineMissedStatus& status)
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());

    status = deadline_missed_status_;
    deadline_missed_status_.total_count_change = 0;
}

bool PublisherImpl::lifespan_expired()
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());

    CacheChange_t* earliest_change;
    while (m_history.get_earliest_change(&earliest_change))
    {
        auto source_timestamp = system_clock::time_point() + nanoseconds(earliest_change->sourceTimestamp.to_ns());
        auto now = system_clock::now();

        // Check that the earliest change has expired (the change which started the timer could have been removed from the history)
        if (now - source_timestamp < lifespan_duration_us_)
        {
            auto interval = source_timestamp - now + lifespan_duration_us_;
            lifespan_timer_->update_interval_millisec((double)duration_cast<milliseconds>(interval).count());
            return true;
        }

        // The earliest change has expired
        m_history.remove_change_pub(earliest_change);

        // Set the timer for the next change if there is one
        if (!m_history.get_earliest_change(&earliest_change))
        {
            return false;
        }

        // Calculate when the next change is due to expire and restart
        source_timestamp = system_clock::time_point() + nanoseconds(earliest_change->sourceTimestamp.to_ns());
        now = system_clock::now();
        auto interval = source_timestamp - now + lifespan_duration_us_;

        if (interval.count() > 0)
        {
            lifespan_timer_->update_interval_millisec((double)duration_cast<milliseconds>(interval).count());
            return true;
        }
    }

    return false;
}

void PublisherImpl::get_liveliness_lost_status(
        LivelinessLostStatus& status)
{
    std::unique_lock<RecursiveTimedMutex> lock(mp_writer->getMutex());

    status = mp_writer->liveliness_lost_status_;

    mp_writer->liveliness_lost_status_.total_count_change = 0u;
}

void PublisherImpl::assert_liveliness()
{
    if (!mp_rtpsParticipant->wlp()->assert_liveliness(
                mp_writer->getGuid(),
                mp_writer->get_liveliness_kind(),
                mp_writer->get_liveliness_lease_duration()))
    {
        logError(PUBLISHER, "Could not assert liveliness of writer " << mp_writer->getGuid());
    }

    if (m_att.qos.m_liveliness.kind == MANUAL_BY_TOPIC_LIVELINESS_QOS)
    {
        // As described in the RTPS specification, if liveliness kind is manual a heartbeat must be sent
        // This only applies to stateful writers, as stateless writers do not send heartbeats

        StatefulWriter* stateful_writer = dynamic_cast<StatefulWriter*>(mp_writer);

        if (stateful_writer != nullptr)
        {
            stateful_writer->send_periodic_heartbeat(true, true);
        }
    }
}

void PublisherImpl::get_sending_locators(
        rtps::LocatorList_t& locators) const
{
    mp_writer->getRTPSParticipant()->get_sending_locators(locators);
}

std::shared_ptr<rtps::IPayloadPool> PublisherImpl::payload_pool()
{
    return payload_pool_;
}
