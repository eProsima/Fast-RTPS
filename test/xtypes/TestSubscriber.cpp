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
 * @file TestSubscriber.cpp
 *
 */

#include "TestSubscriber.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/transport/UDPv6TransportDescriptor.h>
#include <fastrtps/transport/TCPv6TransportDescriptor.h>
#include <fastrtps/types/DynamicTypePtr.h>
#include <fastrtps/types/DynamicType.h>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/DataReader.hpp>
#include <fastrtps/utils/IPLocator.h>
#include <gtest/gtest.h>
#include <asio.hpp>


#include <fastrtps/Domain.h>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

TestSubscriber::TestSubscriber()
    : mp_participant(nullptr)
    , mp_subscriber(nullptr)
    , reader_(nullptr)
    , m_Data(nullptr)
    , m_bInitialized(false)
    , using_typelookup_(false)
    , tls_callback_called_(false)
    , part_listener_(this)
    , m_subListener(this)
{
}

bool TestSubscriber::init(
        const std::string& topicName,
        int domain,
        eprosima::fastrtps::rtps::TopicKind_t topic_kind,
        eprosima::fastdds::dds::TypeSupport type,
        const eprosima::fastrtps::types::TypeObject* type_object,
        const eprosima::fastrtps::types::TypeIdentifier* type_identifier,
        const eprosima::fastrtps::types::TypeInformation* type_info,
        const std::string& name,
        const eprosima::fastrtps::DataRepresentationQosPolicy* dataRepresentationQos,
        const eprosima::fastrtps::TypeConsistencyEnforcementQosPolicy* typeConsistencyQos,
        bool use_typelookup)
{
    m_Name = name;
    m_Type.swap(type);
    using_typelookup_ = use_typelookup;
    ParticipantAttributes PParam;
    PParam.rtps.builtin.domainId = domain;
    PParam.rtps.builtin.discovery_config.leaseDuration = c_TimeInfinite;
    PParam.rtps.builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(1, 0);
    PParam.rtps.builtin.typelookup_config.use_client = using_typelookup_;
    PParam.rtps.builtin.typelookup_config.use_server = using_typelookup_;
    PParam.rtps.setName(m_Name.c_str());

    mp_participant = DomainParticipantFactory::get_instance()->create_participant(PParam, false, &part_listener_);
    if(mp_participant==nullptr)
    {
        std::cout << "ERROR" << std::endl;
        return false;
    }
    mp_participant->enable();

    //CREATE THE SUBSCRIBER
    SubscriberAttributes Rparam;
    Rparam.topic.topicKind = topic_kind;
    Rparam.topic.topicDataType = m_Type != nullptr ? m_Type->getName() : nullptr;
    Rparam.topic.auto_fill_type_object = false;
    Rparam.topic.auto_fill_type_information = false;

    //REGISTER THE TYPE
    if (m_Type != nullptr)
    {
        mp_participant->register_type(m_Type);
    }

    std::ostringstream t;
    t << topicName << "_" << asio::ip::host_name() << "_" << domain;
    Rparam.topic.topicName = t.str();
    if (type_object != nullptr)
    {
        Rparam.topic.type = *type_object;
    }
    if (type_identifier != nullptr)
    {
        Rparam.topic.type_id = *type_identifier;
    }
    if (type_info != nullptr)
    {
        Rparam.topic.type_information = *type_info;
    }

    if (typeConsistencyQos != nullptr)
    {
        Rparam.qos.type_consistency = *typeConsistencyQos;
    }
    if (dataRepresentationQos != nullptr)
    {
        Rparam.qos.representation = *dataRepresentationQos;
    }

    // Create sub and reader if topic was provided
    if (m_Type != nullptr)
    {
        mp_subscriber = mp_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, Rparam, nullptr);

        if (mp_subscriber == nullptr)
        {
            return false;
        }

        reader_ = mp_subscriber->create_datareader(Rparam.topic, Rparam.qos, &m_subListener);
        m_Data = m_Type->createData();
    }

    m_bInitialized = true;
    topic_att = Rparam.topic;
    reader_qos = Rparam.qos;

    return true;
}

TestSubscriber::~TestSubscriber()
{
    if (m_Type != nullptr)
    {
        m_Type->deleteData(m_Data);
    }
    DomainParticipantFactory::get_instance()->delete_participant(mp_participant);
}

TestSubscriber::SubListener::SubListener(TestSubscriber* parent)
    : mParent(parent)
    , n_matched(0)
    , n_samples(0)
{
}

void TestSubscriber::waitDiscovery(bool expectMatch, int maxWait)
{
    std::unique_lock<std::mutex> lock(m_mDiscovery);

    if(m_subListener.n_matched == 0)
        m_cvDiscovery.wait_for(lock, std::chrono::seconds(maxWait));

    if (expectMatch)
    {
        ASSERT_GE(m_subListener.n_matched, 1);
    }
    else
    {
        ASSERT_EQ(m_subListener.n_matched, 0);
    }
}

void TestSubscriber::waitTypeDiscovery(bool expectMatch, int maxWait)
{
    std::unique_lock<std::mutex> lock(mtx_type_discovery_);

    if(!part_listener_.discovered_)
        cv_type_discovery_.wait_for(lock, std::chrono::seconds(maxWait));

    if (expectMatch)
    {
        ASSERT_TRUE(part_listener_.discovered_);
    }
    else
    {
        ASSERT_FALSE(part_listener_.discovered_);
    }
}

void TestSubscriber::matched(bool unmatched)
{
    std::unique_lock<std::mutex> lock(m_mDiscovery);
    if (unmatched)
    {
        --m_subListener.n_matched;
    }
    else
    {
        ++m_subListener.n_matched;
    }
    if(m_subListener.n_matched >= 1)
        m_cvDiscovery.notify_one();
}

void TestSubscriber::SubListener::on_subscription_matched(
        eprosima::fastdds::dds::DataReader*,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info)
{
    if(info.current_count_change > 0)
    {
        mParent->matched();
        std::cout << mParent->m_Name << " matched."<<std::endl;
    }
    else if (info.current_count_change < 0)
    {
        mParent->matched(true);
        std::cout << mParent->m_Name << " unmatched."<<std::endl;
    }
}

void TestSubscriber::SubListener::on_data_available(
        eprosima::fastdds::dds::DataReader* reader)
{
    if (!!reader->take_next_sample(mParent->m_Data, &m_info))
    {
        if (m_info.sampleKind == ALIVE)
        {
            ++n_samples;
            mParent->cv_.notify_one();
            //std::cout << mParent->m_Name << " received a total of " << n_samples << " samples." << std::endl;
        }
    }
}

void TestSubscriber::PartListener::on_type_discovery(
        eprosima::fastdds::dds::DomainParticipant*,
        const rtps::SampleIdentity&,
        const eprosima::fastrtps::string_255& topic,
        const eprosima::fastrtps::types::TypeIdentifier*,
        const eprosima::fastrtps::types::TypeObject*,
        eprosima::fastrtps::types::DynamicType_ptr dyn_type)
{
    if (!parent_->using_typelookup_ || parent_->tls_callback_called_)
    {
        std::cout << "Discovered type: " << dyn_type->get_name() << " on topic: " << topic << std::endl;
        std::lock_guard<std::mutex> lock(parent_->mtx_type_discovery_);
        discovered_ = true;
        parent_->disc_type_ = dyn_type;
        parent_->cv_type_discovery_.notify_one();
    }
}

void TestSubscriber::PartListener::on_type_information_received(
        eprosima::fastdds::dds::DomainParticipant*,
        const eprosima::fastrtps::string_255 topic_name,
        const eprosima::fastrtps::string_255 type_name,
        const eprosima::fastrtps::types::TypeInformation& type_information)
{
    std::function<void(const std::string&, const types::DynamicType_ptr)> callback =
        [this, topic_name](const std::string&, const types::DynamicType_ptr type)
    {
        std::cout << "Callback for type: " << type->get_name() << " on topic: " << topic_name << std::endl;
        parent_->tls_callback_called_ = true;
        on_type_discovery(nullptr, rtps::SampleIdentity(), topic_name, nullptr, nullptr, type);
        parent_->tls_callback_called_ = false;
    };

    std::cout << "Received type information: " << type_name << " on topic: " << topic_name << std::endl;
    parent_->mp_participant->register_remote_type(type_information, type_name.to_string(), callback);
}

DataReader* TestSubscriber::create_datareader()
{
    if (mp_subscriber == nullptr)
    {
        SubscriberAttributes Rparam;
        Rparam.topic = topic_att;
        Rparam.qos = reader_qos;
        mp_subscriber = mp_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, Rparam, nullptr);

        if (mp_subscriber == nullptr)
        {
            return nullptr;
        }
    }
    topic_att.topicDataType = disc_type_->get_name();
    return mp_subscriber->create_datareader(topic_att, reader_qos, &m_subListener);
}

void TestSubscriber::delete_datareader(eprosima::fastdds::dds::DataReader* reader)
{
    mp_subscriber->delete_datareader(reader);
}

bool TestSubscriber::register_discovered_type()
{
    TypeSupport type(disc_type_);
    topic_att.auto_fill_type_object = true;
    topic_att.auto_fill_type_information = true;
    return mp_participant->register_type(type, disc_type_->get_name());
}

void TestSubscriber::run()
{
    std::cout << m_Name << " running..." << std::endl;
    std::cin.ignore();
}
