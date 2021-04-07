// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file DomainParticipant.cpp
 */

#include <fastdds/statistics/dds/domain/DomainParticipant.hpp>

#include <string>
#include <sstream>
#include <vector>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/statistics/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/statistics/topic_names.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>
#include <fastrtps/types/TypesBase.h>

#include <fastdds/domain/DomainParticipantImpl.hpp>
#include <fastdds/publisher/DataWriterImpl.hpp>
#include <statistics/types/typesPubSubTypes.h>
#include <utils/SystemInfo.hpp>

namespace eprosima {
namespace fastdds {
namespace statistics {
namespace dds {

constexpr const char* HISTORY_LATENCY_TOPIC_ALIAS = "HISTORY_LATENCY_TOPIC";
constexpr const char* NETWORK_LATENCY_TOPIC_ALIAS = "NETWORK_LATENCY_TOPIC";
constexpr const char* PUBLICATION_THROUGHPUT_TOPIC_ALIAS = "PUBLICATION_THROUGHPUT_TOPIC";
constexpr const char* SUBSCRIPTION_THROUGHPUT_TOPIC_ALIAS = "SUBSCRIPTION_THROUGHPUT_TOPIC";
constexpr const char* RTPS_SENT_TOPIC_ALIAS = "RTPS_SENT_TOPIC";
constexpr const char* RTPS_LOST_TOPIC_ALIAS = "RTPS_LOST_TOPIC";
constexpr const char* RESENT_DATAS_TOPIC_ALIAS = "RESENT_DATAS_TOPIC";
constexpr const char* HEARTBEAT_COUNT_TOPIC_ALIAS = "HEARTBEAT_COUNT_TOPIC";
constexpr const char* ACKNACK_COUNT_TOPIC_ALIAS = "ACKNACK_COUNT_TOPIC";
constexpr const char* NACKFRAG_COUNT_TOPIC_ALIAS = "NACKFRAG_COUNT_TOPIC";
constexpr const char* GAP_COUNT_TOPIC_ALIAS = "GAP_COUNT_TOPIC";
constexpr const char* DATA_COUNT_TOPIC_ALIAS = "DATA_COUNT_TOPIC";
constexpr const char* PDP_PACKETS_TOPIC_ALIAS = "PDP_PACKETS_TOPIC";
constexpr const char* EDP_PACKETS_TOPIC_ALIAS = "EDP_PACKETS_TOPIC";
constexpr const char* DISCOVERY_TOPIC_ALIAS = "DISCOVERY_TOPIC";
constexpr const char* SAMPLE_DATAS_TOPIC_ALIAS = "SAMPLE_DATAS_TOPIC";
constexpr const char* PHYSICAL_DATA_TOPIC_ALIAS = "PHYSICAL_DATA_TOPIC";

ReturnCode_t DomainParticipant::enable_statistics_datawriter(
        const std::string& topic_name,
        const eprosima::fastdds::dds::DataWriterQos& dwqos)
{
#ifndef FASTDDS_STATISTICS
    (void) topic_name;
    (void) dwqos;

    return ReturnCode_t::RETCODE_UNSUPPORTED;
#else
    if(!check_statistics_topic_name(topic_name))
    {
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (!eprosima::fastdds::dds::DataWriterImpl::check_qos(dwqos))
    {
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }

    // Register type and topic
    ReturnCode_t ret = register_statistics_type_and_topic(topic_name);

    return ret; // ReturnCode_t::RETCODE_OK;
#endif // FASTDDS_STATISTICS
}

ReturnCode_t DomainParticipant::disable_statistics_datawriter(
        const std::string& topic_name)
{
#ifndef FASTDDS_STATISTICS
    (void) topic_name;

    return ReturnCode_t::RETCODE_UNSUPPORTED;
#else
    if(!check_statistics_topic_name(topic_name))
    {
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
    return ReturnCode_t::RETCODE_OK;
#endif // FASTDDS_STATISTICS
}

DomainParticipant* DomainParticipant::narrow(
        eprosima::fastdds::dds::DomainParticipant* domain_participant)
{
#ifdef FASTDDS_STATISTICS
    return static_cast<DomainParticipant*>(domain_participant);
#else
    (void)domain_participant;
    return nullptr;
#endif // FASTDDS_STATISTICS
}

const DomainParticipant* DomainParticipant::narrow(
        const eprosima::fastdds::dds::DomainParticipant* domain_participant)
{
#ifdef FASTDDS_STATISTICS
    return static_cast<const DomainParticipant*>(domain_participant);
#else
    (void)domain_participant;
    return nullptr;
#endif // FASTDDS_STATISTICS
}

ReturnCode_t DomainParticipant::enable()
{
    ReturnCode_t ret = eprosima::fastdds::dds::DomainParticipant::enable();

    if (enable_)
    {
        create_statistics_builtin_entities();
    }

    return ret;
}

void DomainParticipant::create_statistics_builtin_entities()
{
    // Builtin publisher
    builtin_publisher_ = impl_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT);

    // Enable statistics datawriters
    // 1. Find fastdds_statistics PropertyPolicyQos
    const std::string* property_topic_list = eprosima::fastrtps::rtps::PropertyPolicyHelper::find_property(
        impl_->get_qos().properties(), "fastdds.statistics");

    if (nullptr != property_topic_list)
    {
        enable_statistics_builtin_datawriters(*property_topic_list);
    }

    // 2. FASTDDS_STATISTICS environment variable
    std::string env_topic_list;
    const char* data;
    if (ReturnCode_t::RETCODE_OK == SystemInfo::get_env(FASTDDS_STATISTICS_ENVIRONMENT_VARIABLE, &data))
    {
        env_topic_list = data;
    }

    if (!env_topic_list.empty())
    {
        enable_statistics_builtin_datawriters(env_topic_list);
    }
}

void DomainParticipant::enable_statistics_builtin_datawriters(
        const std::string& topic_list)
{
    // Parse list and call enable_statistics_datawriter
    std::stringstream topics(topic_list);
    std::string topic;
    while (std::getline(topics, topic, ';'))
    {
        ReturnCode_t ret = enable_statistics_datawriter(topic, STATISTICS_DATAWRITER_QOS);
        // case RETCODE_ERROR is checked and logged in enable_statistics_datawriter.
        // case RETCODE_INCONSISTENT_POLICY cannot happen. STATISTICS_DATAWRITER_QOS is consitent.
        // case RETCODE_UNSUPPORTED cannot happen because this method is only called if FASTDDS_STATISTICS
        // CMake option is enabled
        assert(ret != ReturnCode_t::RETCODE_INCONSISTENT_POLICY);
        assert(ret != ReturnCode_t::RETCODE_UNSUPPORTED);
        if (ret == ReturnCode_t::RETCODE_BAD_PARAMETER)
        {
            logError(STATISTICS_DOMAIN_PARTICIPANT, "Topic " << topic << " is not a valid statistics topic name/alias");
        }
    }
}

void DomainParticipant::delete_statistics_builtin_entities()
{
    std::vector<eprosima::fastdds::dds::DataWriter*> builtin_writers;
    builtin_publisher_->get_datawriters(builtin_writers);
    for (auto writer : builtin_writers)
    {
        std::string topic_name = writer->get_topic()->get_name();
        disable_statistics_datawriter(topic_name);
    }

    // Delete builtin_publisher
    impl_->delete_publisher(builtin_publisher_);
}

bool DomainParticipant::check_statistics_topic_name(
        const std::string& topic)
{
    if (HISTORY_LATENCY_TOPIC != topic &&
            NETWORK_LATENCY_TOPIC != topic &&
            PUBLICATION_THROUGHPUT_TOPIC != topic &&
            SUBSCRIPTION_THROUGHPUT_TOPIC != topic &&
            RTPS_SENT_TOPIC != topic &&
            RTPS_LOST_TOPIC != topic &&
            RESENT_DATAS_TOPIC != topic &&
            HEARTBEAT_COUNT_TOPIC != topic &&
            ACKNACK_COUNT_TOPIC != topic &&
            NACKFRAG_COUNT_TOPIC != topic &&
            GAP_COUNT_TOPIC != topic &&
            DATA_COUNT_TOPIC != topic &&
            PDP_PACKETS_TOPIC != topic &&
            EDP_PACKETS_TOPIC != topic &&
            DISCOVERY_TOPIC != topic &&
            SAMPLE_DATAS_TOPIC != topic &&
            PHYSICAL_DATA_TOPIC != topic &&
            HISTORY_LATENCY_TOPIC_ALIAS != topic &&
            NETWORK_LATENCY_TOPIC_ALIAS != topic &&
            PUBLICATION_THROUGHPUT_TOPIC_ALIAS != topic &&
            SUBSCRIPTION_THROUGHPUT_TOPIC_ALIAS != topic &&
            RTPS_SENT_TOPIC_ALIAS != topic &&
            RTPS_LOST_TOPIC_ALIAS != topic &&
            RESENT_DATAS_TOPIC_ALIAS != topic &&
            HEARTBEAT_COUNT_TOPIC_ALIAS != topic &&
            ACKNACK_COUNT_TOPIC_ALIAS != topic &&
            NACKFRAG_COUNT_TOPIC_ALIAS != topic &&
            GAP_COUNT_TOPIC_ALIAS != topic &&
            DATA_COUNT_TOPIC_ALIAS != topic &&
            PDP_PACKETS_TOPIC_ALIAS != topic &&
            EDP_PACKETS_TOPIC_ALIAS != topic &&
            DISCOVERY_TOPIC_ALIAS != topic &&
            SAMPLE_DATAS_TOPIC_ALIAS != topic &&
            PHYSICAL_DATA_TOPIC_ALIAS != topic)
    {
        return false;
    }
    return true;
}

ReturnCode_t DomainParticipant::register_statistics_type_and_topic(
        const std::string& topic)
{
    ReturnCode_t ret;
    if (HISTORY_LATENCY_TOPIC == topic || HISTORY_LATENCY_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport history_latency_type(new WriterReaderDataPubSubType);
        ret = register_type(history_latency_type);
        // No need to check returned pointer. It fails if the topic already exists, if the QoS is inconsistent and if
        // the type is not registered.
        create_topic(topic, history_latency_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (NETWORK_LATENCY_TOPIC == topic || NETWORK_LATENCY_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport network_latency_type(
            new eprosima::fastdds::statistics::Locator2LocatorDataPubSubType);
        ret = register_type(network_latency_type);
        create_topic(topic, network_latency_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (PUBLICATION_THROUGHPUT_TOPIC == topic || PUBLICATION_THROUGHPUT_TOPIC_ALIAS == topic ||
            SUBSCRIPTION_THROUGHPUT_TOPIC == topic || SUBSCRIPTION_THROUGHPUT_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport throughput_type(new eprosima::fastdds::statistics::EntityDataPubSubType);
        ret = register_type(throughput_type);
        create_topic(topic, throughput_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (RTPS_SENT_TOPIC == topic || RTPS_SENT_TOPIC_ALIAS == topic ||
            RTPS_LOST_TOPIC == topic || RTPS_LOST_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport rtps_traffic_type(
            new eprosima::fastdds::statistics::Entity2LocatorTrafficPubSubType);
        ret = register_type(rtps_traffic_type);
        create_topic(topic, rtps_traffic_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (RESENT_DATAS_TOPIC == topic || RESENT_DATAS_TOPIC_ALIAS == topic ||
            HEARTBEAT_COUNT_TOPIC == topic || HEARTBEAT_COUNT_TOPIC_ALIAS == topic ||
            ACKNACK_COUNT_TOPIC == topic || ACKNACK_COUNT_TOPIC_ALIAS == topic ||
            NACKFRAG_COUNT_TOPIC == topic || NACKFRAG_COUNT_TOPIC_ALIAS == topic ||
            GAP_COUNT_TOPIC == topic || GAP_COUNT_TOPIC_ALIAS == topic ||
            DATA_COUNT_TOPIC == topic || DATA_COUNT_TOPIC_ALIAS == topic ||
            PDP_PACKETS_TOPIC == topic || PDP_PACKETS_TOPIC_ALIAS == topic ||
            EDP_PACKETS_TOPIC == topic || EDP_PACKETS_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport count_type(new eprosima::fastdds::statistics::EntityCountPubSubType);
        ret = register_type(count_type);
        create_topic(topic, count_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (DISCOVERY_TOPIC == topic || DISCOVERY_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport discovery_type(new eprosima::fastdds::statistics::DiscoveryTimePubSubType);
        ret = register_type(discovery_type);
        create_topic(topic, discovery_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (SAMPLE_DATAS_TOPIC == topic || SAMPLE_DATAS_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport sample_identity_count_type(
            new eprosima::fastdds::statistics::SampleIdentityCountPubSubType);
        ret = register_type(sample_identity_count_type);
        create_topic(topic, sample_identity_count_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    else if (PHYSICAL_DATA_TOPIC == topic || PHYSICAL_DATA_TOPIC_ALIAS == topic)
    {
        eprosima::fastdds::dds::TypeSupport physical_data_type(
            new eprosima::fastdds::statistics::PhysicalDataPubSubType);
        ret = register_type(physical_data_type);
        create_topic(topic, physical_data_type->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    }
    return ret;
}

} // dds
} // statistics
} // fastdds
} // eprosima
