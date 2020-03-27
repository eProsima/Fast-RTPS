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
 * @file TypeLookupSubscriber.h
 *
 */

#ifndef HELLOWORLDSUBSCRIBER_H_
#define HELLOWORLDSUBSCRIBER_H_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/topic/DataReader.hpp>
#include <fastdds/dds/topic/DataReaderListener.hpp>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/rtps/common/Types.h>

#include <fastrtps/types/DynamicPubSubType.h>
#include <fastrtps/types/DynamicTypePtr.h>

#include <fastrtps/attributes/SubscriberAttributes.h>

#include <map>

class TypeLookupSubscriber
{
public:
    TypeLookupSubscriber();

    virtual ~TypeLookupSubscriber();

    //!Initialize the subscriber
    bool init();

    //!RUN the subscriber
    void run();

    //!Run the subscriber until number samples have been recevied.
    void run(
            uint32_t number);

    eprosima::fastdds::dds::DomainParticipant* participant();

private:
    eprosima::fastdds::dds::DomainParticipant* mp_participant;

    eprosima::fastdds::dds::Subscriber* mp_subscriber;

    std::map<eprosima::fastdds::dds::DataReader*, eprosima::fastrtps::types::DynamicType_ptr> readers_;

    std::map<eprosima::fastdds::dds::DataReader*, eprosima::fastrtps::types::DynamicData_ptr> datas_;

    eprosima::fastrtps::SubscriberAttributes att_;

    eprosima::fastrtps::ReaderQos qos_;

    eprosima::fastrtps::TopicAttributes topic_;

public:
    class SubListener
        : public eprosima::fastdds::dds::DataReaderListener
        , public eprosima::fastdds::dds::DomainParticipantListener
    {
    public:
        SubListener(TypeLookupSubscriber* sub)
            : n_matched(0)
            , n_samples(0)
            , subscriber_(sub)
        {}

        ~SubListener() override {}

        void on_data_available(
                eprosima::fastdds::dds::DataReader* reader) override;

        void on_subscription_matched(
                eprosima::fastdds::dds::DataReader* reader,
                const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

        void on_type_information_received(
                eprosima::fastdds::dds::DomainParticipant* participant,
                const eprosima::fastrtps::string_255 topic_name,
                const eprosima::fastrtps::string_255 type_name,
                const eprosima::fastrtps::types::TypeInformation& type_information) override;

        eprosima::fastrtps::SampleInfo_t m_info;

        int n_matched;

        uint32_t n_samples;

        TypeLookupSubscriber* subscriber_;

        std::map<std::string, std::string> topic_type_map_;

        eprosima::fastrtps::types::TypeInformation type_info_;
    } m_listener;

private:
    eprosima::fastrtps::types::DynamicPubSubType m_type;
};

#endif /* HELLOWORLDSUBSCRIBER_H_ */
