// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file SubscriberImpl.cpp
 *
 */

#include <fastdds/subscriber/SubscriberImpl.hpp>
#include <fastdds/subscriber/DataReaderImpl.hpp>
#include <fastdds/topic/TopicDescriptionImpl.hpp>
#include <fastdds/domain/DomainParticipantImpl.hpp>

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/SubscriberListener.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <fastdds/rtps/participant/RTPSParticipant.h>
#include <fastdds/dds/log/Log.hpp>

#include <fastrtps/attributes/SubscriberAttributes.h>

#include <fastrtps/xmlparser/XMLProfileManager.h>

namespace eprosima {
namespace fastdds {
namespace dds {

using fastrtps::xmlparser::XMLProfileManager;
using fastrtps::xmlparser::XMLP_ret;
using fastrtps::rtps::InstanceHandle_t;
using fastrtps::Duration_t;
using fastrtps::SubscriberAttributes;

static void set_qos_from_attributes(
        DataReaderQos& qos,
        const SubscriberAttributes& attr)
{
    qos.reader_resource_limits().matched_publisher_allocation = attr.matched_publisher_allocation;
    qos.properties() = attr.properties;
    qos.expects_inline_qos(attr.expectsInlineQos);
    qos.endpoint().unicast_locator_list = attr.unicastLocatorList;
    qos.endpoint().multicast_locator_list = attr.multicastLocatorList;
    qos.endpoint().remote_locator_list = attr.remoteLocatorList;
    qos.endpoint().history_memory_policy = attr.historyMemoryPolicy;
    qos.endpoint().user_defined_id = attr.getUserDefinedID();
    qos.endpoint().entity_id = attr.getEntityID();
    qos.reliable_reader_qos().times = attr.times;
    qos.reliable_reader_qos().disable_positive_ACKs = attr.qos.m_disablePositiveACKs;
    qos.durability() = attr.qos.m_durability;
    qos.durability_service() = attr.qos.m_durabilityService;
    qos.deadline() = attr.qos.m_deadline;
    qos.latency_budget() = attr.qos.m_latencyBudget;
    qos.liveliness() = attr.qos.m_liveliness;
    qos.reliability() = attr.qos.m_reliability;
    qos.lifespan() = attr.qos.m_lifespan;
    qos.user_data().setValue(attr.qos.m_userData);
    qos.ownership() = attr.qos.m_ownership;
    qos.destination_order() = attr.qos.m_destinationOrder;
    qos.type_consistency().type_consistency = attr.qos.type_consistency;
    qos.type_consistency().representation = attr.qos.representation;
    qos.time_based_filter() = attr.qos.m_timeBasedFilter;
    qos.history() = attr.topic.historyQos;
    qos.resource_limits() = attr.topic.resourceLimitsQos;
}

SubscriberImpl::SubscriberImpl(
        DomainParticipantImpl* p,
        const SubscriberQos& qos,
        SubscriberListener* listen)
    : participant_(p)
    , qos_(&qos == &SUBSCRIBER_QOS_DEFAULT ? participant_->get_default_subscriber_qos() : qos)
    , listener_(listen)
    , subscriber_listener_(this)
    , user_subscriber_(nullptr)
    , rtps_participant_(p->rtps_participant())
    , default_datareader_qos_(DATAREADER_QOS_DEFAULT)
{
    SubscriberAttributes sub_attr;
    XMLProfileManager::getDefaultSubscriberAttributes(sub_attr);
    set_qos_from_attributes(default_datareader_qos_, sub_attr);
}

ReturnCode_t SubscriberImpl::enable()
{
    if (qos_.entity_factory().autoenable_created_entities)
    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto topic_readers : readers_)
        {
            for (DataReaderImpl* dr : topic_readers.second)
            {
                dr->user_datareader_->enable();
            }
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

void SubscriberImpl::disable()
{
    set_listener(nullptr);
    user_subscriber_->set_listener(nullptr);
    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto it = readers_.begin(); it != readers_.end(); ++it)
        {
            for (DataReaderImpl* dr : it->second)
            {
                dr->disable();
            }
        }
    }
}

SubscriberImpl::~SubscriberImpl()
{
    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto it = readers_.begin(); it != readers_.end(); ++it)
        {
            for (DataReaderImpl* dr : it->second)
            {
                delete dr;
            }
        }
        readers_.clear();
    }

    delete user_subscriber_;
}

const SubscriberQos& SubscriberImpl::get_qos() const
{
    return qos_;
}

ReturnCode_t SubscriberImpl::set_qos(
        const SubscriberQos& qos)
{
    bool enabled = user_subscriber_->is_enabled();
    const SubscriberQos& qos_to_set = (&qos == &SUBSCRIBER_QOS_DEFAULT) ?
            participant_->get_default_subscriber_qos() : qos;

    if (&qos != &SUBSCRIBER_QOS_DEFAULT)
    {
        ReturnCode_t check_result = check_qos(qos_to_set);
        if (!check_result)
        {
            return check_result;
        }
    }

    if (enabled && !can_qos_be_updated(qos_, qos_to_set))
    {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }
    set_qos(qos_, qos_to_set, !enabled);

    if (enabled)
    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto topic_readers : readers_)
        {
            for (auto reader : topic_readers.second)
            {
                reader->subscriber_qos_updated();
            }
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

const SubscriberListener* SubscriberImpl::get_listener() const
{
    return listener_;
}

ReturnCode_t SubscriberImpl::set_listener(
        SubscriberListener* listener)
{
    listener_ = listener;
    return ReturnCode_t::RETCODE_OK;
}

DataReader* SubscriberImpl::create_datareader(
        TopicDescription* topic,
        const DataReaderQos& qos,
        DataReaderListener* listener,
        const StatusMask& mask)
{
    logInfo(SUBSCRIBER, "CREATING SUBSCRIBER IN TOPIC: " << topic->get_name())
    //Look for the correct type registration
    TypeSupport type_support = participant_->find_type(topic->get_type_name());

    /// Preconditions
    // Check the type was registered.
    if (type_support.empty())
    {
        logError(SUBSCRIBER, "Type : " << topic->get_type_name() << " Not Registered");
        return nullptr;
    }

    if (!DataReaderImpl::check_qos(qos))
    {
        return nullptr;
    }

    topic->get_impl()->reference();

    DataReaderImpl* impl = new DataReaderImpl(
        this,
        type_support,
        topic,
        qos,
        listener);

    DataReader* reader = new DataReader(impl, mask);
    impl->user_datareader_ = reader;

    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        readers_[topic->get_name()].push_back(impl);
    }

    if (user_subscriber_->is_enabled() && qos_.entity_factory().autoenable_created_entities)
    {
        if (ReturnCode_t::RETCODE_OK != reader->enable())
        {
            delete_datareader(reader);
            return nullptr;
        }
    }

    return reader;
}

DataReader* SubscriberImpl::create_datareader_with_profile(
        TopicDescription* topic,
        const std::string& profile_name,
        DataReaderListener* listener,
        const StatusMask& mask)
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    SubscriberAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillSubscriberAttributes(profile_name, attr))
    {
        DataReaderQos qos = default_datareader_qos_;
        set_qos_from_attributes(qos, attr);
        return create_datareader(topic, qos, listener, mask);
    }

    return nullptr;
}

ReturnCode_t SubscriberImpl::delete_datareader(
        DataReader* reader)
{
    if (user_subscriber_ != reader->get_subscriber())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    std::unique_lock<std::mutex> lock(mtx_readers_);
    auto it = readers_.find(reader->impl_->get_topicdescription()->get_name());
    if (it != readers_.end())
    {
        auto dr_it = std::find(it->second.begin(), it->second.end(), reader->impl_);
        if (dr_it != it->second.end())
        {
            //First extract the reader from the maps to free the mutex
            DataReaderImpl* reader_impl = *dr_it;
            reader_impl->set_listener(nullptr);
            it->second.erase(dr_it);
            if (it->second.empty())
            {
                readers_.erase(it);
            }
            lock.unlock();

            //Now we can delete it
            reader_impl->get_topicdescription()->get_impl()->dereference();
            delete (reader_impl);
            return ReturnCode_t::RETCODE_OK;
        }
    }
    return ReturnCode_t::RETCODE_ERROR;
}

DataReader* SubscriberImpl::lookup_datareader(
        const std::string& topic_name) const
{
    std::lock_guard<std::mutex> lock(mtx_readers_);
    auto it = readers_.find(topic_name);
    if (it != readers_.end() && it->second.size() > 0)
    {
        return it->second.front()->user_datareader_;
    }
    return nullptr;
}

ReturnCode_t SubscriberImpl::get_datareaders(
        std::vector<DataReader*>& readers) const
{
    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto it : readers_)
    {
        for (DataReaderImpl* dr : it.second)
        {
            readers.push_back(dr->user_datareader_);
        }
    }
    return ReturnCode_t::RETCODE_OK;
}

bool SubscriberImpl::has_datareaders() const
{
    if (readers_.empty())
    {
        return false;
    }
    return true;
}

/* TODO
   bool SubscriberImpl::begin_access()
   {
    logError(PUBLISHER, "Operation not implemented");
    return false;
   }
 */

/* TODO
   bool SubscriberImpl::end_access()
   {
    logError(PUBLISHER, "Operation not implemented");
    return false;
   }
 */

ReturnCode_t SubscriberImpl::notify_datareaders() const
{
    for (auto it : readers_)
    {
        for (DataReaderImpl* dr : it.second)
        {
            dr->listener_->on_data_available(dr->user_datareader_);
        }
    }
    return ReturnCode_t::RETCODE_OK;
}

/* TODO
   bool SubscriberImpl::delete_contained_entities()
   {
    logError(PUBLISHER, "Operation not implemented");
    return false;
   }
 */

ReturnCode_t SubscriberImpl::set_default_datareader_qos(
        const DataReaderQos& qos)
{
    if (&qos == &DATAREADER_QOS_DEFAULT)
    {
        reset_default_datareader_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t check_result = DataReaderImpl::check_qos(qos);
    if (!check_result)
    {
        return check_result;
    }

    DataReaderImpl::set_qos(default_datareader_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void SubscriberImpl::reset_default_datareader_qos()
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    DataReaderImpl::set_qos(default_datareader_qos_, DATAREADER_QOS_DEFAULT, true);
    SubscriberAttributes attr;
    XMLProfileManager::getDefaultSubscriberAttributes(attr);
    set_qos_from_attributes(default_datareader_qos_, attr);
}

const DataReaderQos& SubscriberImpl::get_default_datareader_qos() const
{
    return default_datareader_qos_;
}

DataReaderQos& SubscriberImpl::get_default_datareader_qos()
{
    return default_datareader_qos_;
}

bool SubscriberImpl::contains_entity(
        const fastrtps::rtps::InstanceHandle_t& handle) const
{
    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto vit : readers_)
    {
        for (DataReaderImpl* dw : vit.second)
        {
            InstanceHandle_t h(dw->guid());
            if (h == handle)
            {
                return true;
            }
        }
    }
    return false;
}

/* TODO
   bool SubscriberImpl::copy_from_topic_qos(
        DataReaderQos&,
        const fastrtps::TopicAttributes&) const
   {
    logError(PUBLISHER, "Operation not implemented");
    return false;
   }
 */

const DomainParticipant* SubscriberImpl::get_participant() const
{
    return participant_->get_participant();
}

void SubscriberImpl::SubscriberReaderListener::on_data_available(
        DataReader* reader)
{
    if (subscriber_->listener_ != nullptr)
    {
        subscriber_->listener_->on_data_available(reader);
        subscriber_->listener_->on_data_on_readers(subscriber_->user_subscriber_);
    }
}

void SubscriberImpl::SubscriberReaderListener::on_subscription_matched(
        DataReader* reader,
        const fastdds::dds::SubscriptionMatchedStatus& info)
{
    if (subscriber_->listener_ != nullptr)
    {
        subscriber_->listener_->on_subscription_matched(reader, info);
    }
}

void SubscriberImpl::SubscriberReaderListener::on_requested_deadline_missed(
        DataReader* reader,
        const fastrtps::RequestedDeadlineMissedStatus& status)
{
    if (subscriber_->listener_ != nullptr)
    {
        subscriber_->listener_->on_requested_deadline_missed(reader, status);
    }
}

void SubscriberImpl::SubscriberReaderListener::on_liveliness_changed(
        DataReader* reader,
        const fastrtps::LivelinessChangedStatus& status)
{
    if (subscriber_->listener_ != nullptr)
    {
        subscriber_->listener_->on_liveliness_changed(reader, status);
    }
}

void SubscriberImpl::SubscriberReaderListener::on_sample_rejected(
        DataReader* /*reader*/,
        const fastrtps::SampleRejectedStatus& /*status*/)
{
    /* TODO
       if (subscriber_->listener_ != nullptr)
       {
        subscriber_->listener_->on_sample_rejected(reader, status);
       }
     */
}

void SubscriberImpl::SubscriberReaderListener::on_requested_incompatible_qos(
        DataReader* /*reader*/,
        const RequestedIncompatibleQosStatus& /*status*/)
{
    /* TODO
       if (subscriber_->listener_ != nullptr)
       {
        subscriber_->listener_->on_requested_incompatible_qos(reader, status);
       }
     */
}

void SubscriberImpl::SubscriberReaderListener::on_sample_lost(
        DataReader* /*reader*/,
        const SampleLostStatus& /*status*/)
{
    /* TODO
       if (subscriber_->listener_ != nullptr)
       {
        subscriber_->listener_->on_sample_rejected(reader, status);
       }
     */
}

const InstanceHandle_t& SubscriberImpl::get_instance_handle() const
{
    return handle_;
}

bool SubscriberImpl::type_in_use(
        const std::string& type_name) const
{
    for (auto it : readers_)
    {
        for (DataReaderImpl* reader : it.second)
        {
            if (reader->topic_attributes().getTopicDataType() == type_name)
            {
                return true; // Is in use
            }
        }
    }
    return false;
}

void SubscriberImpl::set_qos(
        SubscriberQos& to,
        const SubscriberQos& from,
        bool first_time)
{
    if (first_time || !(to.presentation() == from.presentation()))
    {
        to.presentation() = from.presentation();
        to.presentation().hasChanged = true;
    }
    if (from.partition().names().size() > 0)
    {
        to.partition() = from.partition();
        to.partition().hasChanged = true;
    }
    if (to.group_data().getValue() != from.group_data().getValue())
    {
        to.group_data() = from.group_data();
        to.group_data().hasChanged = true;
    }
    if (to.entity_factory().autoenable_created_entities != from.entity_factory().autoenable_created_entities)
    {
        to.entity_factory() = from.entity_factory();
    }
}

ReturnCode_t SubscriberImpl::check_qos(
        const SubscriberQos& qos)
{
    (void) qos;
    return ReturnCode_t::RETCODE_OK;
}

bool SubscriberImpl::can_qos_be_updated(
        const SubscriberQos& to,
        const SubscriberQos& from)
{
    (void) to;
    (void) from;
    return true;
}

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */
