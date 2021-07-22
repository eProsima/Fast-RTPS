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
 * @file DomainParticipantImpl.cpp
 *
 */

#include <fastdds/domain/DomainParticipantImpl.hpp>

#include <fastdds/dds/builtin/typelookup/TypeLookupManager.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>

#include <fastdds/rtps/RTPSDomain.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.h>
#include <fastdds/rtps/participant/RTPSParticipant.h>
#include <fastdds/rtps/reader/ReaderDiscoveryInfo.h>
#include <fastdds/rtps/writer/WriterDiscoveryInfo.h>

#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>

#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/DynamicTypeBuilderFactory.h>
#include <fastrtps/types/DynamicPubSubType.h>
#include <fastrtps/types/DynamicType.h>
#include <fastrtps/types/DynamicTypeMember.h>

#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <fastdds/publisher/PublisherImpl.hpp>
#include <fastdds/subscriber/SubscriberImpl.hpp>
#include <fastdds/topic/TopicImpl.hpp>

#include <rtps/RTPSDomainImpl.hpp>

#include <chrono>

namespace eprosima {
namespace fastdds {
namespace dds {

using fastrtps::xmlparser::XMLProfileManager;
using fastrtps::xmlparser::XMLP_ret;
using fastrtps::ParticipantAttributes;
using fastrtps::TopicAttributes;
using fastrtps::SubscriberAttributes;
using fastrtps::PublisherAttributes;
using fastrtps::rtps::RTPSDomain;
using fastrtps::rtps::RTPSParticipant;
using fastrtps::rtps::ParticipantDiscoveryInfo;
#if HAVE_SECURITY
using fastrtps::rtps::ParticipantAuthenticationInfo;
#endif // if HAVE_SECURITY
using fastrtps::rtps::ReaderDiscoveryInfo;
using fastrtps::rtps::ReaderProxyData;
using fastrtps::rtps::WriterDiscoveryInfo;
using fastrtps::rtps::WriterProxyData;
using fastrtps::rtps::GUID_t;
using fastrtps::rtps::EndpointKind_t;
using fastrtps::rtps::ResourceEvent;
using eprosima::fastdds::dds::Log;

static void set_attributes_from_qos(
        fastrtps::rtps::RTPSParticipantAttributes& attr,
        const DomainParticipantQos& qos)
{
    attr.allocation = qos.allocation();
    attr.properties = qos.properties();
    attr.setName(qos.name());
    attr.prefix = qos.wire_protocol().prefix;
    attr.participantID = qos.wire_protocol().participant_id;
    attr.builtin = qos.wire_protocol().builtin;
    attr.port = qos.wire_protocol().port;
    attr.throughputController = qos.wire_protocol().throughput_controller;
    attr.defaultUnicastLocatorList = qos.wire_protocol().default_unicast_locator_list;
    attr.defaultMulticastLocatorList = qos.wire_protocol().default_multicast_locator_list;
    attr.userTransports = qos.transport().user_transports;
    attr.useBuiltinTransports = qos.transport().use_builtin_transports;
    attr.sendSocketBufferSize = qos.transport().send_socket_buffer_size;
    attr.listenSocketBufferSize = qos.transport().listen_socket_buffer_size;
    attr.userData = qos.user_data().data_vec();
    attr.flow_controllers = qos.flow_controllers();
}

static void set_qos_from_attributes(
        TopicQos& qos,
        const TopicAttributes& attr)
{
    qos.history() = attr.historyQos;
    qos.resource_limits() = attr.resourceLimitsQos;
}

static void set_qos_from_attributes(
        SubscriberQos& qos,
        const SubscriberAttributes& attr)
{
    qos.group_data().setValue(attr.qos.m_groupData);
    qos.partition() = attr.qos.m_partition;
    qos.presentation() = attr.qos.m_presentation;
}

static void set_qos_from_attributes(
        PublisherQos& qos,
        const PublisherAttributes& attr)
{
    qos.group_data().setValue(attr.qos.m_groupData);
    qos.partition() = attr.qos.m_partition;
    qos.presentation() = attr.qos.m_presentation;
}

DomainParticipantImpl::DomainParticipantImpl(
        DomainParticipant* dp,
        DomainId_t did,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listen)
    : domain_id_(did)
    , next_instance_id_(0)
    , qos_(qos)
    , rtps_participant_(nullptr)
    , participant_(dp)
    , listener_(listen)
    , default_pub_qos_(PUBLISHER_QOS_DEFAULT)
    , default_sub_qos_(SUBSCRIBER_QOS_DEFAULT)
    , default_topic_qos_(TOPIC_QOS_DEFAULT)
#pragma warning (disable : 4355 )
    , rtps_listener_(this)
{
    participant_->impl_ = this;

    PublisherAttributes pub_attr;
    XMLProfileManager::getDefaultPublisherAttributes(pub_attr);
    set_qos_from_attributes(default_pub_qos_, pub_attr);

    SubscriberAttributes sub_attr;
    XMLProfileManager::getDefaultSubscriberAttributes(sub_attr);
    set_qos_from_attributes(default_sub_qos_, sub_attr);

    TopicAttributes top_attr;
    XMLProfileManager::getDefaultTopicAttributes(top_attr);
    set_qos_from_attributes(default_topic_qos_, top_attr);

    // Pre calculate participant id and generated guid
    participant_id_ = qos_.wire_protocol().participant_id;
    eprosima::fastrtps::rtps::RTPSDomainImpl::create_participant_guid(participant_id_, guid_);
}

void DomainParticipantImpl::disable()
{
    if (participant_)
    {
        participant_->set_listener(nullptr);
    }
    rtps_listener_.participant_ = nullptr;

    // The function to disable the DomainParticipantImpl is called from
    // DomainParticipantFactory::delete_participant() and DomainParticipantFactory destructor.
    if (rtps_participant_ != nullptr)
    {
        rtps_participant_->set_listener(nullptr);

        {
            std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto pub_it = publishers_.begin(); pub_it != publishers_.end(); ++pub_it)
            {
                pub_it->second->disable();
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx_subs_);
            for (auto sub_it = subscribers_.begin(); sub_it != subscribers_.end(); ++sub_it)
            {
                sub_it->second->disable();
            }
        }
    }
}

DomainParticipantImpl::~DomainParticipantImpl()
{
    {
        std::lock_guard<std::mutex> lock(mtx_pubs_);
        for (auto pub_it = publishers_.begin(); pub_it != publishers_.end(); ++pub_it)
        {
            delete pub_it->second;
        }
        publishers_.clear();
        publishers_by_handle_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(mtx_subs_);

        for (auto sub_it = subscribers_.begin(); sub_it != subscribers_.end(); ++sub_it)
        {
            delete sub_it->second;
        }
        subscribers_.clear();
        subscribers_by_handle_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(mtx_topics_);

        for (auto topic_it = topics_.begin(); topic_it != topics_.end(); ++topic_it)
        {
            delete topic_it->second;
        }
        topics_.clear();
        topics_by_handle_.clear();
    }

    if (rtps_participant_ != nullptr)
    {
        RTPSDomain::removeRTPSParticipant(rtps_participant_);
    }

    {
        std::lock_guard<std::mutex> lock(mtx_types_);
        types_.clear();
    }

    if (participant_)
    {
        participant_->impl_ = nullptr;
        delete participant_;
        participant_ = nullptr;
    }
}

ReturnCode_t DomainParticipantImpl::enable()
{
    // Should not have been previously enabled
    assert(rtps_participant_ == nullptr);

    fastrtps::rtps::RTPSParticipantAttributes rtps_attr;
    set_attributes_from_qos(rtps_attr, qos_);
    rtps_attr.participantID = participant_id_;

    // If DEFAULT_ROS2_MASTER_URI is specified then try to create default client if
    // that already exists.
    RTPSParticipant* part = RTPSDomain::clientServerEnvironmentCreationOverride(
        domain_id_,
        false,
        rtps_attr,
        &rtps_listener_);

    if (part == nullptr)
    {
        part = RTPSDomain::createParticipant(domain_id_, false, rtps_attr, &rtps_listener_);

        if (part == nullptr)
        {
            logError(DOMAIN_PARTICIPANT, "Problem creating RTPSParticipant");
            return ReturnCode_t::RETCODE_ERROR;
        }
    }

    guid_ = part->getGuid();
    rtps_participant_ = part;
    rtps_participant_->enable();

    rtps_participant_->set_check_type_function(
        [this](const std::string& type_name) -> bool
        {
            return find_type(type_name).get() != nullptr;
        });

    if (qos_.entity_factory().autoenable_created_entities)
    {
        // Enable topics first
        {
            std::lock_guard<std::mutex> lock(mtx_topics_);

            for (auto topic : topics_)
            {
                topic.second->user_topic_->enable();
            }
        }

        // Enable publishers
        {
            std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto pub : publishers_)
            {
                pub.second->rtps_participant_ = rtps_participant_;
                pub.second->user_publisher_->enable();
            }
        }

        // Enable subscribers
        {
            std::lock_guard<std::mutex> lock(mtx_subs_);

            for (auto sub : subscribers_)
            {
                sub.second->rtps_participant_ = rtps_participant_;
                sub.second->user_subscriber_->enable();
            }
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::set_qos(
        const DomainParticipantQos& qos)
{
    bool enabled = (rtps_participant_ != nullptr);
    const DomainParticipantQos& qos_to_set = (&qos == &PARTICIPANT_QOS_DEFAULT) ?
            DomainParticipantFactory::get_instance()->get_default_participant_qos() : qos;

    if (&qos != &PARTICIPANT_QOS_DEFAULT)
    {
        ReturnCode_t ret_val = check_qos(qos_to_set);
        if (!ret_val)
        {
            return ret_val;
        }
    }

    if (enabled && !can_qos_be_updated(qos_, qos_to_set))
    {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }
    set_qos(qos_, qos_to_set, !enabled);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_qos(
        DomainParticipantQos& qos) const
{
    qos = qos_;
    return ReturnCode_t::RETCODE_OK;
}

const DomainParticipantQos& DomainParticipantImpl::get_qos() const
{
    return qos_;
}

ReturnCode_t DomainParticipantImpl::delete_publisher(
        const Publisher* pub)
{
    if (participant_ != pub->get_participant())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    std::lock_guard<std::mutex> lock(mtx_pubs_);
    auto pit = publishers_.find(const_cast<Publisher*>(pub));

    if (pit != publishers_.end())
    {
        assert(pub->get_instance_handle() == pit->second->get_instance_handle()
                && "The publisher instance handle does not match the publisher implementation instance handle");
        if (pub->has_datawriters())
        {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        pit->second->set_listener(nullptr);
        publishers_by_handle_.erase(publishers_by_handle_.find(pit->second->get_instance_handle()));
        delete pit->second;
        publishers_.erase(pit);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DomainParticipantImpl::delete_subscriber(
        const Subscriber* sub)
{
    if (participant_ != sub->get_participant())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    std::lock_guard<std::mutex> lock(mtx_subs_);
    auto sit = subscribers_.find(const_cast<Subscriber*>(sub));

    if (sit != subscribers_.end())
    {
        assert(sub->get_instance_handle() == sit->second->get_instance_handle()
                && "The subscriber instance handle does not match the subscriber implementation instance handle");
        if (sub->has_datareaders())
        {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        sit->second->set_listener(nullptr);
        subscribers_by_handle_.erase(subscribers_by_handle_.find(sit->second->get_instance_handle()));
        delete sit->second;
        subscribers_.erase(sit);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DomainParticipantImpl::delete_topic(
        const Topic* topic)
{
    if (topic == nullptr)
    {
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (participant_ != topic->get_participant())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    std::lock_guard<std::mutex> lock(mtx_topics_);
    auto it = topics_.find(topic->get_name());

    if (it != topics_.end())
    {
        assert(topic->get_instance_handle() == it->second->get_topic()->get_instance_handle()
                && "The topic instance handle does not match the topic implementation instance handle");
        if (it->second->is_referenced())
        {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        it->second->set_listener(nullptr);
        topics_by_handle_.erase(topic->get_instance_handle());
        delete it->second;
        topics_.erase(it);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_ERROR;
}

const InstanceHandle_t& DomainParticipantImpl::get_instance_handle() const
{
    return static_cast<const InstanceHandle_t&>(guid_);
}

const GUID_t& DomainParticipantImpl::guid() const
{
    return guid_;
}

Publisher* DomainParticipantImpl::create_publisher(
        const PublisherQos& qos,
        PublisherListener* listener,
        const StatusMask& mask)
{
    if (!PublisherImpl::check_qos(qos))
    {
        // The PublisherImpl::check_qos() function is not yet implemented and always returns ReturnCode_t::RETCODE_OK.
        // It will be implemented in future releases of Fast DDS.
        // logError(PARTICIPANT, "PublisherQos inconsistent or not supported");
        // return nullptr;
    }

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    PublisherImpl* pubimpl = create_publisher_impl(qos, listener);
    Publisher* pub = new Publisher(pubimpl, mask);
    pubimpl->user_publisher_ = pub;
    pubimpl->rtps_participant_ = rtps_participant_;

    bool enabled = rtps_participant_ != nullptr;

    // Create InstanceHandle for the new publisher
    InstanceHandle_t pub_handle;
    create_instance_handle(pub_handle);
    pubimpl->handle_ = pub_handle;

    //SAVE THE PUBLISHER INTO MAPS
    std::lock_guard<std::mutex> lock(mtx_pubs_);
    publishers_by_handle_[pub_handle] = pub;
    publishers_[pub] = pubimpl;

    // Enable publisher if appropriate
    if (enabled && qos_.entity_factory().autoenable_created_entities)
    {
        ReturnCode_t ret_publisher_enable = pub->enable();
        assert(ReturnCode_t::RETCODE_OK == ret_publisher_enable);
        (void)ret_publisher_enable;
    }

    return pub;
}

Publisher* DomainParticipantImpl::create_publisher_with_profile(
        const std::string& profile_name,
        PublisherListener* listener,
        const StatusMask& mask)
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    PublisherAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile_name, attr))
    {
        PublisherQos qos = default_pub_qos_;
        set_qos_from_attributes(qos, attr);
        return create_publisher(qos, listener, mask);
    }

    return nullptr;
}

PublisherImpl* DomainParticipantImpl::create_publisher_impl(
        const PublisherQos& qos,
        PublisherListener* listener)
{
    return new PublisherImpl(this, qos, listener);
}

/* TODO
   Subscriber* DomainParticipantImpl::get_builtin_subscriber()
   {
    logError(PARTICIPANT, "Not implemented.");
    return nullptr;
   }
 */

/* TODO
   bool DomainParticipantImpl::ignore_participant(
        const InstanceHandle_t& handle)
   {
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

/* TODO
   bool DomainParticipantImpl::ignore_topic(
        const InstanceHandle_t& handle)
   {
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

/* TODO
   bool DomainParticipantImpl::ignore_publication(
        const InstanceHandle_t& handle)
   {
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

/* TODO
   bool DomainParticipantImpl::ignore_subscription(
        const InstanceHandle_t& handle)
   {
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

DomainId_t DomainParticipantImpl::get_domain_id() const
{
    return domain_id_;
}

/* TODO
   bool DomainParticipantImpl::delete_contained_entities()
   {
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

ReturnCode_t DomainParticipantImpl::assert_liveliness()
{
    if (rtps_participant_ == nullptr)
    {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    if (rtps_participant_->wlp() != nullptr)
    {
        if (rtps_participant_->wlp()->assert_liveliness_manual_by_participant())
        {
            return ReturnCode_t::RETCODE_OK;
        }
    }
    else
    {
        logError(PARTICIPANT, "Invalid WLP, cannot assert liveliness of participant");
    }
    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DomainParticipantImpl::set_default_publisher_qos(
        const PublisherQos& qos)
{
    if (&qos == &PUBLISHER_QOS_DEFAULT)
    {
        reset_default_publisher_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t ret_val = PublisherImpl::check_qos(qos);
    if (!ret_val)
    {
        // The PublisherImpl::check_qos() function is not yet implemented and always returns ReturnCode_t::RETCODE_OK.
        // It will be implemented in future releases of Fast DDS.
        // return ret_val;
    }
    PublisherImpl::set_qos(default_pub_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::reset_default_publisher_qos()
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    PublisherImpl::set_qos(default_pub_qos_, PUBLISHER_QOS_DEFAULT, true);
    PublisherAttributes attr;
    XMLProfileManager::getDefaultPublisherAttributes(attr);
    set_qos_from_attributes(default_pub_qos_, attr);
}

const PublisherQos& DomainParticipantImpl::get_default_publisher_qos() const
{
    return default_pub_qos_;
}

const ReturnCode_t DomainParticipantImpl::get_publisher_qos_from_profile(
        const std::string& profile_name,
        PublisherQos& qos) const
{
    PublisherAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile_name, attr))
    {
        qos = default_pub_qos_;
        set_qos_from_attributes(qos, attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DomainParticipantImpl::set_default_subscriber_qos(
        const SubscriberQos& qos)
{
    if (&qos == &SUBSCRIBER_QOS_DEFAULT)
    {
        reset_default_subscriber_qos();
        return ReturnCode_t::RETCODE_OK;
    }
    ReturnCode_t check_result = SubscriberImpl::check_qos(qos);
    if (!check_result)
    {
        // The SubscriberImpl::check_qos() function is not yet implemented and always returns ReturnCode_t::RETCODE_OK.
        // It will be implemented in future releases of Fast DDS.
        // return check_result;
    }
    SubscriberImpl::set_qos(default_sub_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::reset_default_subscriber_qos()
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    SubscriberImpl::set_qos(default_sub_qos_, SUBSCRIBER_QOS_DEFAULT, true);
    SubscriberAttributes attr;
    XMLProfileManager::getDefaultSubscriberAttributes(attr);
    set_qos_from_attributes(default_sub_qos_, attr);
}

const SubscriberQos& DomainParticipantImpl::get_default_subscriber_qos() const
{
    return default_sub_qos_;
}

const ReturnCode_t DomainParticipantImpl::get_subscriber_qos_from_profile(
        const std::string& profile_name,
        SubscriberQos& qos) const
{
    SubscriberAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillSubscriberAttributes(profile_name, attr))
    {
        qos = default_sub_qos_;
        set_qos_from_attributes(qos, attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DomainParticipantImpl::set_default_topic_qos(
        const TopicQos& qos)
{
    if (&qos == &TOPIC_QOS_DEFAULT)
    {
        reset_default_topic_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t ret_val = TopicImpl::check_qos(qos);
    if (!ret_val)
    {
        return ret_val;
    }

    TopicImpl::set_qos(default_topic_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::reset_default_topic_qos()
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    TopicImpl::set_qos(default_topic_qos_, TOPIC_QOS_DEFAULT, true);
    TopicAttributes attr;
    XMLProfileManager::getDefaultTopicAttributes(attr);
    set_qos_from_attributes(default_topic_qos_, attr);
}

const TopicQos& DomainParticipantImpl::get_default_topic_qos() const
{
    return default_topic_qos_;
}

const ReturnCode_t DomainParticipantImpl::get_topic_qos_from_profile(
        const std::string& profile_name,
        TopicQos& qos) const
{
    TopicAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillTopicAttributes(profile_name, attr))
    {
        qos = default_topic_qos_;
        set_qos_from_attributes(qos, attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

/* TODO
   bool DomainParticipantImpl::get_discovered_participants(
        std::vector<InstanceHandle_t>& participant_handles) const
   {
    (void)participant_handles;
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

/* TODO
   bool DomainParticipantImpl::get_discovered_topics(
        std::vector<InstanceHandle_t>& topic_handles) const
   {
    (void)topic_handles;
    logError(PARTICIPANT, "Not implemented.");
    return false;
   }
 */

bool DomainParticipantImpl::contains_entity(
        const InstanceHandle_t& handle,
        bool recursive) const
{
    // Look for publishers
    {
        std::lock_guard<std::mutex> lock(mtx_pubs_);
        if (publishers_by_handle_.find(handle) != publishers_by_handle_.end())
        {
            return true;
        }
    }

    // Look for subscribers
    {
        std::lock_guard<std::mutex> lock(mtx_subs_);
        if (subscribers_by_handle_.find(handle) != subscribers_by_handle_.end())
        {
            return true;
        }
    }

    // Look for topics
    {
        std::lock_guard<std::mutex> lock(mtx_topics_);
        if (topics_by_handle_.find(handle) != topics_by_handle_.end())
        {
            return true;
        }
    }

    if (recursive)
    {
        // Look into publishers
        {
            std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto pit : publishers_)
            {
                if (pit.second->contains_entity(handle))
                {
                    return true;
                }
            }
        }

        // Look into subscribers
        {
            std::lock_guard<std::mutex> lock(mtx_subs_);
            for (auto sit : subscribers_)
            {
                if (sit.second->contains_entity(handle))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

ReturnCode_t DomainParticipantImpl::get_current_time(
        fastrtps::Time_t& current_time) const
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= seconds;
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

    current_time.seconds = static_cast<int32_t>(seconds.count());
    current_time.nanosec = static_cast<uint32_t>(nanos.count());

    return ReturnCode_t::RETCODE_OK;
}

const DomainParticipant* DomainParticipantImpl::get_participant() const
{
    return participant_;
}

DomainParticipant* DomainParticipantImpl::get_participant()
{
    return participant_;
}

std::vector<std::string> DomainParticipantImpl::get_participant_names() const
{
    return rtps_participant_ == nullptr ?
           std::vector<std::string> {}
           :
           rtps_participant_->getParticipantNames();
}

Subscriber* DomainParticipantImpl::create_subscriber(
        const SubscriberQos& qos,
        SubscriberListener* listener,
        const StatusMask& mask)
{
    if (!SubscriberImpl::check_qos(qos))
    {
        // The SubscriberImpl::check_qos() function is not yet implemented and always returns ReturnCode_t::RETCODE_OK.
        // It will be implemented in future releases of Fast DDS.
        // logError(PARTICIPANT, "SubscriberQos inconsistent or not supported");
        // return nullptr;
    }

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    SubscriberImpl* subimpl = create_subscriber_impl(qos, listener);
    Subscriber* sub = new Subscriber(subimpl, mask);
    subimpl->user_subscriber_ = sub;
    subimpl->rtps_participant_ = this->rtps_participant_;

    // Create InstanceHandle for the new subscriber
    InstanceHandle_t sub_handle;
    bool enabled = rtps_participant_ != nullptr;

    // Create InstanceHandle for the new subscriber
    create_instance_handle(sub_handle);
    subimpl->handle_ = sub_handle;

    //SAVE THE PUBLISHER INTO MAPS
    std::lock_guard<std::mutex> lock(mtx_subs_);
    subscribers_by_handle_[sub_handle] = sub;
    subscribers_[sub] = subimpl;

    // Enable subscriber if appropriate
    if (enabled && qos_.entity_factory().autoenable_created_entities)
    {
        ReturnCode_t ret_subscriber_enable = sub->enable();
        assert(ReturnCode_t::RETCODE_OK == ret_subscriber_enable);
        (void)ret_subscriber_enable;
    }

    return sub;
}

Subscriber* DomainParticipantImpl::create_subscriber_with_profile(
        const std::string& profile_name,
        SubscriberListener* listener,
        const StatusMask& mask)
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    SubscriberAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillSubscriberAttributes(profile_name, attr))
    {
        SubscriberQos qos = default_sub_qos_;
        set_qos_from_attributes(qos, attr);
        return create_subscriber(qos, listener, mask);
    }

    return nullptr;
}

SubscriberImpl* DomainParticipantImpl::create_subscriber_impl(
        const SubscriberQos& qos,
        SubscriberListener* listener)
{
    return new SubscriberImpl(this, qos, listener);
}

Topic* DomainParticipantImpl::create_topic(
        const std::string& topic_name,
        const std::string& type_name,
        const TopicQos& qos,
        TopicListener* listener,
        const StatusMask& mask)
{
    //Look for the correct type registration
    TypeSupport type_support = find_type(type_name);
    if (type_support.empty())
    {
        logError(PARTICIPANT, "Type : " << type_name << " Not Registered");
        return nullptr;
    }

    if (!TopicImpl::check_qos(qos))
    {
        logError(PARTICIPANT, "TopicQos inconsistent or not supported");
        return nullptr;
    }

    bool enabled = rtps_participant_ != nullptr;

    std::lock_guard<std::mutex> lock(mtx_topics_);

    //Check there is no Topic with the same name
    if (topics_.find(topic_name) != topics_.end())
    {
        logError(PARTICIPANT, "Topic with name : " << topic_name << " already exists");
        return nullptr;
    }

    InstanceHandle_t topic_handle;
    create_instance_handle(topic_handle);

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    TopicImpl* topic_impl = new TopicImpl(this, type_support, qos, listener);
    Topic* topic = new Topic(topic_name, type_name, topic_impl, mask);
    topic_impl->user_topic_ = topic;
    topic->set_instance_handle(topic_handle);

    //SAVE THE TOPIC INTO MAPS
    topics_by_handle_[topic_handle] = topic;
    topics_[topic_name] = topic_impl;

    // Enable topic if appropriate
    if (enabled && qos_.entity_factory().autoenable_created_entities)
    {
        ReturnCode_t ret_topic_enable = topic->enable();
        assert(ReturnCode_t::RETCODE_OK == ret_topic_enable);
        (void)ret_topic_enable;
    }

    return topic;
}

Topic* DomainParticipantImpl::create_topic_with_profile(
        const std::string& topic_name,
        const std::string& type_name,
        const std::string& profile_name,
        TopicListener* listener,
        const StatusMask& mask)
{
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    TopicAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillTopicAttributes(profile_name, attr))
    {
        TopicQos qos = default_topic_qos_;
        set_qos_from_attributes(qos, attr);
        return create_topic(topic_name, type_name, qos, listener, mask);
    }

    return nullptr;
}

TopicDescription* DomainParticipantImpl::lookup_topicdescription(
        const std::string& topic_name) const
{
    auto it = topics_.find(topic_name);

    if (it != topics_.end())
    {
        return it->second->user_topic_;
    }

    return nullptr;
}

const TypeSupport DomainParticipantImpl::find_type(
        const std::string& type_name) const
{
    std::lock_guard<std::mutex> lock(mtx_types_);

    auto type_it = types_.find(type_name);

    if (type_it != types_.end())
    {
        return type_it->second;
    }

    return TypeSupport(nullptr);
}

ReturnCode_t DomainParticipantImpl::register_type(
        const TypeSupport type,
        const std::string& type_name)
{
    if (type_name.size() <= 0)
    {
        logError(PARTICIPANT, "Registered Type must have a name");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    TypeSupport t = find_type(type_name);

    if (!t.empty())
    {
        if (t == type)
        {
            return ReturnCode_t::RETCODE_OK;
        }

        logError(PARTICIPANT, "Another type with the same name '" << type_name << "' is already registered.");
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    logInfo(PARTICIPANT, "Type " << type_name << " registered.");
    std::lock_guard<std::mutex> lock(mtx_types_);
    types_.insert(std::make_pair(type_name, type));

    if (type->auto_fill_type_object() || type->auto_fill_type_information())
    {
        register_dynamic_type_to_factories(type);
    }

    return ReturnCode_t::RETCODE_OK;
}

bool DomainParticipantImpl::register_dynamic_type_to_factories(
        const TypeSupport& type) const
{
    using namespace  eprosima::fastrtps::types;
    DynamicPubSubType* dpst = dynamic_cast<DynamicPubSubType*>(type.get());
    if (dpst != nullptr) // Registering a dynamic type.
    {
        TypeObjectFactory* objectFactory = TypeObjectFactory::get_instance();
        DynamicTypeBuilderFactory* dynFactory = DynamicTypeBuilderFactory::get_instance();
        const TypeIdentifier* id = objectFactory->get_type_identifier_trying_complete(dpst->getName());
        if (id == nullptr)
        {
            std::map<MemberId, DynamicTypeMember*> membersMap;
            dpst->GetDynamicType()->get_all_members(membersMap);
            std::vector<const MemberDescriptor*> members;
            for (auto it : membersMap)
            {
                members.push_back(it.second->get_descriptor());
            }
            TypeObject typeObj;
            dynFactory->build_type_object(dpst->GetDynamicType()->get_type_descriptor(), typeObj, &members);
            // Minimal too
            dynFactory->build_type_object(dpst->GetDynamicType()->get_type_descriptor(), typeObj, &members, false);
            const TypeIdentifier* type_id2 = objectFactory->get_type_identifier(dpst->getName());
            const TypeObject* type_obj = objectFactory->get_type_object(dpst->getName());
            if (type_id2 == nullptr)
            {
                logError(DOMAIN_PARTICIPANT, "Cannot register dynamic type " << dpst->getName());
            }
            else
            {
                objectFactory->add_type_object(dpst->getName(), type_id2, type_obj);

                // Complete, just to make sure it is generated
                const TypeIdentifier* type_id_complete = objectFactory->get_type_identifier(dpst->getName(), true);
                const TypeObject* type_obj_complete = objectFactory->get_type_object(dpst->getName(), true);
                objectFactory->add_type_object(dpst->getName(), type_id_complete, type_obj_complete); // Add complete
                return true;
            }
        }
    }

    return false; // Isn't a registered dynamic type.
}

ReturnCode_t DomainParticipantImpl::unregister_type(
        const std::string& type_name)
{
    if (type_name.size() <= 0)
    {
        logError(PARTICIPANT, "Registered Type must have a name");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    TypeSupport t = find_type(type_name);

    if (t.empty())
    {
        return ReturnCode_t::RETCODE_OK; // Not registered, so unregistering complete.
    }

    {
        // Check is any subscriber is using the type
        std::lock_guard<std::mutex> lock(mtx_subs_);

        for (auto sit : subscribers_)
        {
            if (sit.second->type_in_use(type_name))
            {
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET; // Is in use
            }
        }
    }

    {
        // Check is any publisher is using the type
        std::lock_guard<std::mutex> lock(mtx_pubs_);

        for (auto pit : publishers_)
        {
            if (pit.second->type_in_use(type_name))
            {
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET; // Is in use
            }
        }
    }

    std::lock_guard<std::mutex> lock(mtx_types_);
    types_.erase(type_name);

    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::MyRTPSParticipantListener::onParticipantDiscovery(
        RTPSParticipant*,
        ParticipantDiscoveryInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_participant_discovery(participant_->participant_, std::move(info));
    }
}

#if HAVE_SECURITY
void DomainParticipantImpl::MyRTPSParticipantListener::onParticipantAuthentication(
        RTPSParticipant*,
        ParticipantAuthenticationInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->onParticipantAuthentication(participant_->participant_, std::move(info));
    }
}

#endif // if HAVE_SECURITY

void DomainParticipantImpl::MyRTPSParticipantListener::onReaderDiscovery(
        RTPSParticipant*,
        ReaderDiscoveryInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_subscriber_discovery(participant_->participant_, std::move(info));
    }
}

void DomainParticipantImpl::MyRTPSParticipantListener::onWriterDiscovery(
        RTPSParticipant*,
        WriterDiscoveryInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_publisher_discovery(participant_->participant_, std::move(info));
    }
}

void DomainParticipantImpl::MyRTPSParticipantListener::on_type_discovery(
        RTPSParticipant*,
        const fastrtps::rtps::SampleIdentity& request_sample_id,
        const fastrtps::string_255& topic,
        const fastrtps::types::TypeIdentifier* identifier,
        const fastrtps::types::TypeObject* object,
        fastrtps::types::DynamicType_ptr dyn_type)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_type_discovery(
            participant_->participant_,
            request_sample_id,
            topic,
            identifier,
            object,
            dyn_type);
    }

    participant_->check_get_type_request(request_sample_id, identifier, object, dyn_type);
}

void DomainParticipantImpl::MyRTPSParticipantListener::on_type_dependencies_reply(
        RTPSParticipant*,
        const fastrtps::rtps::SampleIdentity& request_sample_id,
        const fastrtps::types::TypeIdentifierWithSizeSeq& dependencies)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_type_dependencies_reply(
            participant_->participant_, request_sample_id, dependencies);
    }

    participant_->check_get_dependencies_request(request_sample_id, dependencies);
}

void DomainParticipantImpl::MyRTPSParticipantListener::on_type_information_received(
        RTPSParticipant*,
        const fastrtps::string_255& topic_name,
        const fastrtps::string_255& type_name,
        const fastrtps::types::TypeInformation& type_information)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        if (type_information.complete().typeid_with_size().type_id()._d() > 0
                || type_information.minimal().typeid_with_size().type_id()._d() > 0)
        {
            participant_->listener_->on_type_information_received(
                participant_->participant_, topic_name, type_name, type_information);
        }
    }
}

bool DomainParticipantImpl::new_remote_endpoint_discovered(
        const GUID_t& partguid,
        uint16_t endpointId,
        EndpointKind_t kind)
{
    if (rtps_participant_ != nullptr)
    {
        if (kind == fastrtps::rtps::WRITER)
        {
            return rtps_participant_->newRemoteWriterDiscovered(partguid, static_cast<int16_t>(endpointId));
        }
        else
        {
            return rtps_participant_->newRemoteReaderDiscovered(partguid, static_cast<int16_t>(endpointId));
        }
    }

    return false;
}

ResourceEvent& DomainParticipantImpl::get_resource_event() const
{
    return rtps_participant_->get_resource_event();
}

fastrtps::rtps::SampleIdentity DomainParticipantImpl::get_type_dependencies(
        const fastrtps::types::TypeIdentifierSeq& in) const
{
    return rtps_participant_->typelookup_manager()->get_type_dependencies(in);
}

fastrtps::rtps::SampleIdentity DomainParticipantImpl::get_types(
        const fastrtps::types::TypeIdentifierSeq& in) const
{
    return rtps_participant_->typelookup_manager()->get_types(in);
}

ReturnCode_t DomainParticipantImpl::register_remote_type(
        const fastrtps::types::TypeInformation& type_information,
        const std::string& type_name,
        std::function<void(const std::string& name, const fastrtps::types::DynamicType_ptr type)>& callback)
{
    using namespace fastrtps::types;

    if (rtps_participant_ == nullptr)
    {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    TypeObjectFactory* factory = TypeObjectFactory::get_instance();
    // Check if plain
    if (type_information.complete().typeid_with_size().type_id()._d() < EK_MINIMAL)
    {
        DynamicType_ptr dyn = factory->build_dynamic_type(
            type_name,
            &type_information.minimal().typeid_with_size().type_id());

        if (nullptr != dyn)
        {
            //callback(type_name, dyn); // For plain types, don't call the callback
            return register_dynamic_type(dyn);
        }
        // If cannot create the dynamic type, probably is because it depend on unknown types.
        // We must continue.
    }

    // Check if already available
    TypeObject obj;
    factory->typelookup_get_type(
        type_information.complete().typeid_with_size().type_id(),
        obj);

    if (obj._d() != 0)
    {
        DynamicType_ptr dyn = factory->build_dynamic_type(
            type_name,
            &type_information.complete().typeid_with_size().type_id(),
            &obj);

        if (nullptr != dyn)
        {
            //callback(type_name, dyn); // If the type is already registered, don't call the callback.
            return register_dynamic_type(dyn);
        }
    }
    else if (rtps_participant_->typelookup_manager() != nullptr)
    {
        TypeIdentifierSeq dependencies;
        TypeIdentifierSeq retrieve_objects;

        fill_pending_dependencies(type_information.complete().dependent_typeids(), dependencies, retrieve_objects);

        fastrtps::rtps::SampleIdentity request_dependencies;
        fastrtps::rtps::SampleIdentity request_objects;

        // Lock now, we don't want to process the reply before we add the requests' ID to the maps.
        std::lock_guard<std::mutex> lock(mtx_request_cb_);

        // If any pending dependency exists, retrieve it.
        if (!dependencies.empty())
        {
            request_dependencies = get_type_dependencies(dependencies);
        }

        // If any pending TypeObject exists, retrieve it
        if (!retrieve_objects.empty())
        {
            request_objects = get_types(retrieve_objects);
        }

        // If no more dependencies but failed to create, probably we only need the TypeObject
        dependencies.clear(); // Reuse the same vector.
        dependencies.push_back(type_information.complete().typeid_with_size().type_id());
        fastrtps::rtps::SampleIdentity requestId = get_types(dependencies);

        // Add everything to maps
        register_callbacks_.emplace(std::make_pair(requestId, std::make_pair(type_name, callback)));
        std::vector<fastrtps::rtps::SampleIdentity> vector;
        vector.push_back(requestId); // Add itself

        if (builtin::INVALID_SAMPLE_IDENTITY != request_dependencies)
        {
            vector.push_back(request_dependencies);
            child_requests_.emplace(std::make_pair(request_dependencies, requestId));
        }

        if (builtin::INVALID_SAMPLE_IDENTITY != request_objects)
        {
            vector.push_back(request_objects);
            child_requests_.emplace(std::make_pair(request_objects, requestId));
        }

        // Move the filled vector to the map
        parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));

        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
}

bool DomainParticipantImpl::check_get_type_request(
        const fastrtps::rtps::SampleIdentity& requestId,
        const fastrtps::types::TypeIdentifier* identifier,
        const fastrtps::types::TypeObject* object,
        fastrtps::types::DynamicType_ptr dyn_type)
{
    // Maybe we have a pending request?
    if (builtin::INVALID_SAMPLE_IDENTITY != requestId)
    {
        // First level request?
        std::lock_guard<std::mutex> lock(mtx_request_cb_);

        auto cb_it = register_callbacks_.find(requestId);

        if (cb_it != register_callbacks_.end())
        {
            const std::string& name = cb_it->second.first;
            const auto& callback = cb_it->second.second;

            if (nullptr != dyn_type)
            {
                dyn_type->set_name(name);
                if (register_dynamic_type(dyn_type) == ReturnCode_t::RETCODE_OK)
                {
                    callback(name, dyn_type);
                    remove_parent_request(requestId);
                    return true;
                }
            }

            // Exists the request, but the provided dyn_type isn't valid.
            // Register the received TypeObject into factory and recreate the DynamicType.
            fastrtps::types::TypeObjectFactory::get_instance()->add_type_object(name, identifier, object);

            auto pending = parent_requests_.find(requestId);
            if (pending != parent_requests_.end() && pending->second.size() < 2) // Exists and everything is solved.
            {
                fastrtps::types::DynamicType_ptr dynamic =
                        fastrtps::types::TypeObjectFactory::get_instance()->build_dynamic_type(name, identifier,
                                object);

                if (nullptr != dynamic)
                {
                    if (register_dynamic_type(dynamic) == ReturnCode_t::RETCODE_OK)
                    {
                        callback(name, dynamic);
                        remove_parent_request(requestId);
                        return true;
                    }
                }
            }
            // Failed, cannot register the type yet, probably child request still pending.
            return false;
        }

        // Child request?
        auto child_it = child_requests_.find(requestId);

        if (child_it != child_requests_.end())
        {
            // Register received TypeObject into factory, remove the iterator from the map and check our parent.
            fastrtps::types::TypeObjectFactory::get_instance()->add_type_object(
                get_inner_type_name(requestId), identifier, object);
            remove_child_request(requestId);
        }
    }
    return false;
}

void DomainParticipantImpl::fill_pending_dependencies(
        const fastrtps::types::TypeIdentifierWithSizeSeq& dependencies,
        fastrtps::types::TypeIdentifierSeq& pending_identifiers,
        fastrtps::types::TypeIdentifierSeq& pending_objects) const
{
    using namespace fastrtps::types;
    for (const TypeIdentifierWithSize& tiws : dependencies)
    {
        // Check that we don't know that dependency
        if (!TypeObjectFactory::get_instance()->typelookup_check_type_identifier(tiws.type_id()))
        {
            pending_identifiers.push_back(tiws.type_id());
        }
        // Check if we need to retrieve the TypeObject
        if (tiws.type_id()._d() >= EK_MINIMAL)
        {
            TypeObject obj;
            TypeObjectFactory::get_instance()->typelookup_get_type(tiws.type_id(), obj);
            if (obj._d() == 0)
            {
                // Failed, so we must retrieve it.
                pending_objects.push_back(tiws.type_id());
            }
        }
    }
}

bool DomainParticipantImpl::check_get_dependencies_request(
        const fastrtps::rtps::SampleIdentity& requestId,
        const fastrtps::types::TypeIdentifierWithSizeSeq& dependencies)
{
    using namespace fastrtps::types;

    // Maybe we have a pending request?
    if (builtin::INVALID_SAMPLE_IDENTITY != requestId)
    {
        TypeIdentifierSeq next_dependencies;
        TypeIdentifierSeq retrieve_objects;

        // First level request?
        std::lock_guard<std::mutex> lock(mtx_request_cb_);

        auto cb_it = register_callbacks_.find(requestId);

        if (cb_it != register_callbacks_.end())
        {
            fill_pending_dependencies(dependencies, next_dependencies, retrieve_objects);

            // If any pending dependency exists, retrieve it
            if (!next_dependencies.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_type_dependencies(next_dependencies);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            // Add received dependencies to the factory
            for (const TypeIdentifierWithSize& tiws : dependencies)
            {
                if (tiws.type_id()._d() >= EK_MINIMAL)
                {
                    // This dependency needs a TypeObject
                    retrieve_objects.push_back(tiws.type_id());
                }
                else
                {
                    TypeObjectFactory::get_instance()->add_type_identifier(
                        get_inner_type_name(requestId), &tiws.type_id());
                }
            }

            // If any pending TypeObject exists, retrieve it
            if (!retrieve_objects.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_types(retrieve_objects);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            if (next_dependencies.empty() && retrieve_objects.empty())
            {
                // Finished?
                on_child_requests_finished(requestId);
                return true;
            }

            return false;
        }

        // Child request?
        auto child_it = child_requests_.find(requestId);

        if (child_it != child_requests_.end())
        {
            fill_pending_dependencies(dependencies, next_dependencies, retrieve_objects);

            // If any pending dependency exists, retrieve it
            if (!next_dependencies.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_type_dependencies(next_dependencies);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            // Add received dependencies to the factory
            for (const TypeIdentifierWithSize& tiws : dependencies)
            {
                if (tiws.type_id()._d() >= EK_MINIMAL)
                {
                    // This dependency needs a TypeObject
                    retrieve_objects.push_back(tiws.type_id());
                }
                else
                {
                    TypeObjectFactory::get_instance()->add_type_identifier(
                        get_inner_type_name(requestId), &tiws.type_id());
                }
            }

            // If any pending TypeObject exists, retrieve it
            if (!retrieve_objects.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_types(retrieve_objects);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            if (next_dependencies.empty() && retrieve_objects.empty())
            {
                remove_child_request(requestId);
                return true;
            }

            return false;
        }
    }
    return false;
}

ReturnCode_t DomainParticipantImpl::register_dynamic_type(
        fastrtps::types::DynamicType_ptr dyn_type)
{
    TypeSupport type(new fastrtps::types::DynamicPubSubType(dyn_type));
    return participant_->register_type(type);
}

void DomainParticipantImpl::remove_parent_request(
        const fastrtps::rtps::SampleIdentity& request)
{
    // If a parent request if going to be deleted, delete all its children too.
    auto cb_it = register_callbacks_.find(request);
    auto parent_it = parent_requests_.find(request);

    if (parent_requests_.end() != parent_it)
    {
        for (const fastrtps::rtps::SampleIdentity& child_id : parent_it->second)
        {
            auto child_it = child_requests_.find(child_id);
            if (child_requests_.end() != child_it)
            {
                child_requests_.erase(child_it);
            }
        }
        parent_requests_.erase(parent_it);
    }

    if (register_callbacks_.end() != cb_it)
    {
        register_callbacks_.erase(cb_it);
    }
}

void DomainParticipantImpl::remove_child_request(
        const fastrtps::rtps::SampleIdentity& request)
{
    auto child_it = child_requests_.find(request);
    if (child_requests_.end() != child_it)
    {
        fastrtps::rtps::SampleIdentity parent_request = child_it->second;
        child_requests_.erase(child_it);

        auto parent_it = parent_requests_.find(parent_request);
        if (parent_requests_.end() != parent_it)
        {
            std::vector<fastrtps::rtps::SampleIdentity>& pending = parent_it->second;
            pending.erase(std::find(pending.begin(), pending.end(), request));
            if (pending.empty())
            {
                parent_requests_.erase(parent_it);
            }
        }

        on_child_requests_finished(parent_request);
    }
}

void DomainParticipantImpl::on_child_requests_finished(
        const fastrtps::rtps::SampleIdentity& parent)
{
    auto pending_requests_it = parent_requests_.find(parent);
    // Do I have no more pending childs?
    if (parent_requests_.end() == pending_requests_it || pending_requests_it->second.empty())
    {
        // Am I a children?
        auto child_it = child_requests_.find(parent);
        if (child_requests_.end() != child_it)
        {
            remove_child_request(parent);
        }
        else
        {
            // Or a top-level request?
            auto cb_it = register_callbacks_.find(parent);
            if (pending_requests_it->second.size() < 2)
            {
                parent_requests_.erase(pending_requests_it);
            }
            cb_it->second.second(cb_it->second.first, fastrtps::types::DynamicType_ptr(nullptr)); // Everything should be already registered
            register_callbacks_.erase(cb_it);
        }
    }
}

std::string DomainParticipantImpl::get_inner_type_name(
        const fastrtps::rtps::SampleIdentity& id) const
{
    std::stringstream ss;
    ss << "type_" << id.writer_guid() << "_" << id.sequence_number();
    std::string str = ss.str();
    std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
    str.erase(std::remove(str.begin(), str.end(), '.'), str.end());
    std::replace(str.begin(), str.end(), '|', '_');
    return str;
}

bool DomainParticipantImpl::has_active_entities()
{
    if (!publishers_.empty())
    {
        return true;
    }
    if (!subscribers_.empty())
    {
        return true;
    }
    if (!topics_.empty())
    {
        return true;
    }
    return false;
}

void DomainParticipantImpl::set_qos(
        DomainParticipantQos& to,
        const DomainParticipantQos& from,
        bool first_time)
{
    if (!(to.entity_factory() == from.entity_factory()))
    {
        to.entity_factory() = from.entity_factory();
    }
    if (!(to.user_data() == from.user_data()))
    {
        to.user_data() = from.user_data();
        to.user_data().hasChanged = true;
    }
    if (first_time && !(to.allocation() == from.allocation()))
    {
        to.allocation() = from.allocation();
    }
    if (first_time && !(to.properties() == from.properties()))
    {
        to.properties() = from.properties();
    }
    if (first_time && !(to.wire_protocol() == from.wire_protocol()))
    {
        to.wire_protocol() = from.wire_protocol();
    }
    if (first_time && !(to.transport() == from.transport()))
    {
        to.transport() = from.transport();
    }
    if (first_time && to.name() != from.name())
    {
        to.name() = from.name();
    }
}

fastrtps::types::ReturnCode_t DomainParticipantImpl::check_qos(
        const DomainParticipantQos& qos)
{
    if (qos.allocation().data_limits.max_user_data == 0 ||
            qos.allocation().data_limits.max_user_data > qos.user_data().getValue().size())
    {
        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
}

bool DomainParticipantImpl::can_qos_be_updated(
        const DomainParticipantQos& to,
        const DomainParticipantQos& from)
{
    bool updatable = true;
    if (!(to.allocation() == from.allocation()))
    {
        updatable = false;
        logWarning(RTPS_QOS_CHECK, "ParticipantResourceLimitsQos cannot be changed after the participant is enabled");
    }
    if (!(to.properties() == from.properties()))
    {
        updatable = false;
        logWarning(RTPS_QOS_CHECK, "PropertyPolilyQos cannot be changed after the participant is enabled");
    }
    if (!(to.wire_protocol() == from.wire_protocol()))
    {
        updatable = false;
        logWarning(RTPS_QOS_CHECK, "WireProtocolConfigQos cannot be changed after the participant is enabled");
    }
    if (!(to.transport() == from.transport()))
    {
        updatable = false;
        logWarning(RTPS_QOS_CHECK, "TransportConfigQos cannot be changed after the participant is enabled");
    }
    if (!(to.name() == from.name()))
    {
        updatable = false;
        logWarning(RTPS_QOS_CHECK, "Participant name cannot be changed after the participant is enabled");
    }
    return updatable;
}

void DomainParticipantImpl::create_instance_handle(
        InstanceHandle_t& handle)
{
    using eprosima::fastrtps::rtps::octet;

    ++next_instance_id_;
    handle = guid_;
    handle.value[15] = 0x01; // Vendor specific;
    handle.value[14] = static_cast<octet>(next_instance_id_ & 0xFF);
    handle.value[13] = static_cast<octet>((next_instance_id_ >> 8) & 0xFF);
    handle.value[12] = static_cast<octet>((next_instance_id_ >> 16) & 0xFF);
}

DomainParticipantListener* DomainParticipantImpl::get_listener_for(
        const StatusMask& status)
{
    if (participant_->get_status_mask().is_active(status))
    {
        return listener_;
    }
    return nullptr;
}

}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima
