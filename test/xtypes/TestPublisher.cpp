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
 * @file TestPublisher.cpp
 *
 */

#include "TestPublisher.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <fastrtps/transport/TCPv6TransportDescriptor.h>
#include <fastrtps/transport/UDPv6TransportDescriptor.h>
#include <fastrtps/types/DynamicTypePtr.h>
#include <fastrtps/types/DynamicType.h>
#include <fastrtps/Domain.h>
#include <fastrtps/utils/IPLocator.h>
#include <gtest/gtest.h>
#include <asio.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

TestPublisher::TestPublisher()
    : m_iSamples(-1)
    , m_sentSamples(0)
    , m_iWaitTime(1000)
    , m_Data(nullptr)
    , m_bInitialized(false)
    , using_typelookup_(false)
    , tls_callback_called_(false)
    , mp_participant(nullptr)
    , mp_publisher(nullptr)
    , writer_(nullptr)
    , part_listener_(this)
    , m_pubListener(this)

{
}

bool TestPublisher::init(
        const std::string& topicName,
        int domain,
        eprosima::fastdds::dds::TypeSupport type,
        const eprosima::fastrtps::types::TypeObject* type_object,
        const eprosima::fastrtps::types::TypeIdentifier* type_identifier,
        const eprosima::fastrtps::types::TypeInformation* type_info,
        const std::string& name,
        const eprosima::fastrtps::DataRepresentationQosPolicy* dataRepresentationQos,
        eprosima::fastrtps::rtps::TopicKind_t topic_kind,
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

    if (mp_participant == nullptr)
    {
        return false;
    }
    mp_participant->enable();

    // CREATE THE PUBLISHER
    PublisherAttributes Wparam;
    Wparam.topic.auto_fill_type_object = false;
    Wparam.topic.auto_fill_type_information = false;
    Wparam.topic.topicKind = topic_kind;
    Wparam.topic.topicDataType = m_Type != nullptr ? m_Type->getName() : nullptr;

    //REGISTER THE TYPE
    if (m_Type != nullptr)
    {
        mp_participant->register_type(m_Type);
    }

    std::ostringstream t;
    t << topicName << "_" << asio::ip::host_name() << "_" << domain;
    Wparam.topic.topicName = t.str();
    if (type_object != nullptr)
    {
        Wparam.topic.type = *type_object;
    }
    if (type_identifier != nullptr)
    {
        Wparam.topic.type_id = *type_identifier;
    }
    if (type_info != nullptr)
    {
        Wparam.topic.type_information = *type_info;
    }

    if (dataRepresentationQos != nullptr)
    {
        Wparam.qos.representation = *dataRepresentationQos;
    }
    // Wparam.topic.dataRepresentationQos = XCDR_DATA_REPRESENTATION
    // Wparam.topic.dataRepresentationQos = XML_DATA_REPRESENTATION
    // Wparam.topic.dataRepresentationQos = XCDR2_DATA_REPRESENTATION

    if (m_Type != nullptr)
    {
        mp_publisher = mp_participant->create_publisher(PUBLISHER_QOS_DEFAULT, Wparam, nullptr);
        if (mp_publisher == nullptr)
        {
            return false;
        }

        writer_ = mp_publisher->create_datawriter(Wparam.topic, Wparam.qos, &m_pubListener);

        m_Data = m_Type->createData();
    }

    m_bInitialized = true;

    return true;
}

TestPublisher::~TestPublisher()
{
    if (m_Type)
    {
        m_Type->deleteData(m_Data);
    }
    DomainParticipantFactory::get_instance()->delete_participant(mp_participant);
}

void TestPublisher::waitDiscovery(bool expectMatch, int maxWait)
{
    std::unique_lock<std::mutex> lock(m_mDiscovery);

    if(m_pubListener.n_matched == 0)
        m_cvDiscovery.wait_for(lock, std::chrono::seconds(maxWait));

    if (expectMatch)
    {
        ASSERT_GE(m_pubListener.n_matched, 1);
    }
    else
    {
        ASSERT_EQ(m_pubListener.n_matched, 0);
    }
}

void TestPublisher::waitTypeDiscovery(bool expectMatch, int maxWait)
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

void TestPublisher::matched()
{
    std::unique_lock<std::mutex> lock(m_mDiscovery);
    ++m_pubListener.n_matched;
    if(m_pubListener.n_matched >= 1)
        m_cvDiscovery.notify_one();
}

TestPublisher::PubListener::PubListener(TestPublisher* parent)
    : mParent(parent)
    , n_matched(0)
{
}

void TestPublisher::PubListener::on_publication_matched(
        eprosima::fastdds::dds::DataWriter*,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    if(info.current_count_change > 0)
    {
        std::cout << mParent->m_Name << " matched." << std::endl;
        mParent->matched();
    }
    else
    {
        std::cout << mParent->m_Name << " unmatched."<<std::endl;
    }
}

void TestPublisher::PartListener::on_type_discovery(
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

void TestPublisher::PartListener::on_type_information_received(
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

void TestPublisher::runThread()
{
    int iPrevCount = 0;
    std::cout << m_Name << " running..." << std::endl;
    while (!publish() && iPrevCount < m_iSamples)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(m_iWaitTime));
        ++iPrevCount;
    }
}

void TestPublisher::run()
{
    std::thread thread(&TestPublisher::runThread, this);
    thread.join();
}

bool TestPublisher::publish()
{
    if (m_pubListener.n_matched > 0)
    {
        if (writer_->write(m_Data))
        {
            ++m_sentSamples;
            //std::cout << m_Name << " sent a total of " << m_sentSamples << " samples." << std::endl;
            return true;
        }
        //else
        //{
        //    std::cout << m_Name << " failed to send " << (m_sentSamples + 1) << " sample." << std::endl;
        //}
    }
    return false;
}
