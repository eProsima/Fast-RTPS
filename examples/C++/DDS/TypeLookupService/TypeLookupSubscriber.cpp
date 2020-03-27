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
 * @file TypeLookupSubscriber.cpp
 *
 */

#include "TypeLookupSubscriber.h"
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include <fastrtps/types/DynamicDataHelper.hpp>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/TypeObjectFactory.h>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

TypeLookupSubscriber::TypeLookupSubscriber()
    : mp_participant(nullptr)
    , mp_subscriber(nullptr)
    , m_listener(this)
{
}

bool TypeLookupSubscriber::init()
{
    ParticipantAttributes PParam;
    PParam.rtps.builtin.discovery_config.discoveryProtocol = SIMPLE;
    PParam.rtps.builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    PParam.rtps.builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    PParam.rtps.builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;
    PParam.rtps.builtin.typelookup_config.use_client = true;
    PParam.rtps.builtin.use_WriterLivelinessProtocol = false;
    PParam.rtps.builtin.domainId = 0;
    PParam.rtps.builtin.discovery_config.leaseDuration = c_TimeInfinite;
    PParam.rtps.setName("Participant_sub");

    mp_participant = DomainParticipantFactory::get_instance()->create_participant(PParam, false, &m_listener);
    if (mp_participant == nullptr)
    {
        return false;
    }
    mp_participant->enable();

    // CREATE THE COMMON READER ATTRIBUTES
    qos_.m_durability.kind = eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS;
    qos_.m_reliability.kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    topic_.topicKind = NO_KEY;
    topic_.topicDataType = "TypeLookup";
    topic_.topicName = "TypeLookupTopic";
    topic_.historyQos.kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    topic_.historyQos.depth = 30;
    topic_.resourceLimitsQos.max_samples = 50;
    topic_.resourceLimitsQos.allocated_samples = 20;

    return true;
}

TypeLookupSubscriber::~TypeLookupSubscriber()
{
    DomainParticipantFactory::get_instance()->delete_participant(mp_participant);
    readers_.clear();
    datas_.clear();
}

void TypeLookupSubscriber::SubListener::on_subscription_matched(
        eprosima::fastdds::dds::DataReader* reader,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        n_matched++;
        std::cout << "Subscriber matched" << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        n_matched--;
        std::cout << "Subscriber unmatched" << std::endl;
        auto itr = subscriber_->readers_.find(reader);
        if (itr != subscriber_->readers_.end())
        {
            subscriber_->readers_.erase(itr);
        }

        auto itd = subscriber_->datas_.find(reader);
        if (itd != subscriber_->datas_.end())
        {
            subscriber_->datas_.erase(itd);
        }

        if (subscriber_->mp_subscriber != nullptr)
        {
            subscriber_->mp_subscriber->delete_datareader(reader);
        }
    }
    else
    {
        std::cout << "Subscriber received an invalid value for SubscriptionMatchedStatus." << std::endl;
    }
}

void TypeLookupSubscriber::SubListener::on_data_available(
        eprosima::fastdds::dds::DataReader* reader)
{
    auto dit = subscriber_->datas_.find(reader);

    if (dit != subscriber_->datas_.end())
    {
        types::DynamicData_ptr data = dit->second;
        if (reader->take_next_sample(data.get(), &m_info) == ReturnCode_t::RETCODE_OK)
        {
            if (m_info.sampleKind == ALIVE)
            {
                types::DynamicType_ptr type = subscriber_->readers_[reader];
                this->n_samples++;
                std::cout << "Received data of type " << type->get_name() << std::endl;
                types::DynamicDataHelper::print(data);
            }
        }
    }
}

void TypeLookupSubscriber::SubListener::on_type_information_received(
        eprosima::fastdds::dds::DomainParticipant*,
        const eprosima::fastrtps::string_255 topic_name,
        const eprosima::fastrtps::string_255 type_name,
        const eprosima::fastrtps::types::TypeInformation& type_information)
{
    std::function<void(const std::string&, const types::DynamicType_ptr)> callback =
        [this, topic_name](const std::string& name, const types::DynamicType_ptr type)
            {
                std::cout << "Discovered type: " << name << " from topic " << topic_name << std::endl;

                if (subscriber_->mp_subscriber == nullptr)
                {
                    SubscriberAttributes Rparam;
                    Rparam = subscriber_->att_;
                    Rparam.topic = subscriber_->topic_;
                    Rparam.topic.topicName = topic_name;
                    Rparam.qos = subscriber_->qos_;
                    subscriber_->mp_subscriber = subscriber_->mp_participant->create_subscriber(
                        SUBSCRIBER_QOS_DEFAULT, Rparam, nullptr);

                    if (subscriber_->mp_subscriber == nullptr)
                    {
                        return;
                    }
                }
                subscriber_->topic_.topicDataType = name;
                DataReader* reader = subscriber_->mp_subscriber->create_datareader(
                    subscriber_->topic_,
                    subscriber_->qos_,
                    &subscriber_->m_listener);

                if (type == nullptr)
                {
                    const types::TypeIdentifier* ident =
                        types::TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(name);

                    if (nullptr != ident)
                    {
                        const types::TypeObject* obj =
                            types::TypeObjectFactory::get_instance()->get_type_object(ident);

                        types::DynamicType_ptr dyn_type =
                            types::TypeObjectFactory::get_instance()->build_dynamic_type(name, ident, obj);

                        if (nullptr != dyn_type)
                        {
                            subscriber_->readers_[reader] = dyn_type;
                            types::DynamicData_ptr data(
                                types::DynamicDataFactory::get_instance()->create_data(dyn_type));
                            subscriber_->datas_[reader] = data;
                        }
                        else
                        {
                            std::cout << "ERROR: DynamicType cannot be created for type: " << name << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "ERROR: TypeIdentifier cannot be retrieved for type: " << name << std::endl;
                    }
                }
                else
                {
                    subscriber_->readers_[reader] = type;
                    types::DynamicData_ptr data(types::DynamicDataFactory::get_instance()->create_data(type));
                    subscriber_->datas_[reader] = data;
                }
            };

    subscriber_->mp_participant->register_remote_type(
        type_information,
        type_name.to_string(),
        callback);
}

void TypeLookupSubscriber::run()
{
    std::cout << "Subscriber running. Please press enter to stop the Subscriber" << std::endl;
    std::cin.ignore();
}

void TypeLookupSubscriber::run(
        uint32_t number)
{
    std::cout << "Subscriber running until " << number << " samples have been received" << std::endl;
    while(number > this->m_listener.n_samples)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
