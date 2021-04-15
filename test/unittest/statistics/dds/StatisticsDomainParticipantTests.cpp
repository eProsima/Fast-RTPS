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

#include <mutex>
#include <vector>

#include <gtest/gtest.h>

#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/statistics/dds/domain/DomainParticipant.hpp>
#include <fastdds/statistics/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/statistics/topic_names.hpp>
#include <fastrtps/types/TypesBase.h>
#include <statistics/types/typesPubSubTypes.h>

#include "../../logging/mock/MockConsumer.h"

namespace eprosima {
namespace fastdds {
namespace statistics {
namespace dds {

class StatisticsDomainParticipantTests : public ::testing::Test
{
public:

    void helper_block_for_at_least_entries(
            uint32_t amount)
    {
        std::unique_lock<std::mutex> lck(*mutex_);
        mock_consumer_->cv().wait(lck, [this, amount]
                {
                    return mock_consumer_->ConsumedEntriesSize_nts() >= amount;
                });
    }

    eprosima::fastdds::dds::MockConsumer* mock_consumer_;

    mutable std::mutex* mutex_;

protected:

    void SetUp() override
    {
        mutex_ = new std::mutex();
    }

    void TearDown() override
    {
        delete mutex_;
        mutex_ = nullptr;
    }

};

/*
 * This test checks eprosima::fastdds::statistics::dds::DomainParticipant narrow methods.
 * 1. Create a eprosima::fastdds::dds::DomainParticipant
 * 2. Use both narrow methods to obtain the pointer to the children class.
 * If FASTDDS_STATISTICS option is not set, nullptr is expected.
 * Otherwise, a valid pointer is expected.
 * 3. Call both narrow methods with an invalid pointer and check that it returns nullptr
 * 4. Delete DDS entities
 */
TEST_F(StatisticsDomainParticipantTests, NarrowDomainParticipantTest)
{
    // 1. Create DomainParticipant
    eprosima::fastdds::dds::DomainParticipant* participant =
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->
                    create_participant(0, eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);
    ASSERT_NE(participant, nullptr);

    const eprosima::fastdds::dds::DomainParticipant* const_participant = participant;

    // 2. Call to both narrow methods
    eprosima::fastdds::statistics::dds::DomainParticipant* statistics_participant =
            eprosima::fastdds::statistics::dds::DomainParticipant::narrow(participant);
    const eprosima::fastdds::statistics::dds::DomainParticipant* const_statistics_participant =
            eprosima::fastdds::statistics::dds::DomainParticipant::narrow(const_participant);
#ifndef FASTDDS_STATISTICS
    EXPECT_EQ(statistics_participant, nullptr);
    EXPECT_EQ(const_statistics_participant, nullptr);
#else
    EXPECT_NE(statistics_participant, nullptr);
    EXPECT_NE(const_statistics_participant, nullptr);
#endif // FASTDDS_STATISTICS

    // 3. Call narrow methods with invalid parameter
    eprosima::fastdds::dds::DomainParticipant* null_participant = nullptr;
    statistics_participant = eprosima::fastdds::statistics::dds::DomainParticipant::narrow(null_participant);
    EXPECT_EQ(statistics_participant, nullptr);

    const_participant = nullptr;
    const_statistics_participant = eprosima::fastdds::statistics::dds::DomainParticipant::narrow(const_participant);
    EXPECT_EQ(const_statistics_participant, nullptr);

    // 4. Delete DDS entities
    ASSERT_EQ(eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant),
            eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
}

/*
 * This test checks both eprosima::fastdds::statistics::dds::DomainParticipant enable_statistics_datawriter() and
 * disable_statistics_datawriter() methods.
 * 1. If the FASTDDS_STATISTICS compilation flag is not set, check that the methods return RETCODE_UNSUPPORTED.
 * Check that this error takes precedence over other possible errors.
 * 2. Narrow DomainParticipant to the children class.
 * 3. Create TypeSupports.
 * 4. Check that the types are not registered yet.
 * 5. Check that the topics do not exist yet.
 * 6. Enable each statistics DataWriter checking that topics are created and types are registered.
 * 7. Enable an already enabled statistics DataWriter and check that it returns RETCODE_OK.
 * 8. Call enable_statistics_datawriter method with an invalid topic name and check that returns RETCODE_BAD_PARAMETER.
 * 9. Disable one statistics DataWriter and check that it is successful.
 * 10. Enable the previous statistics DataWriter with an inconsistent QoS and check that it returns
 * RETCODE_INCONSISTENT_POLICY.
 * 11. Check error code precedence: RETCODE_BAD_PARAMETER takes precedence over RETCODE_INCONSISTENT_POLICY.
 * The case where the create_datawriter fails returning RETCODE_ERROR is not checked because it only passes the error
 * upstream.
 * 12. Try to disable an already disabled statistics DataWriter and check that returns RETCODE_ERROR.
 * 13. Check that if an invalid topic name is provided to the disable_statistics_datawriter method, it returns
 * RETCODE_BAD_PARAMETER.
 * The case where the delete_datawriter fails returning RETCODE_ERROR is not checked because it only passes the error
 * upstream.
 * 14. Delete DDS entities.
 */
TEST_F(StatisticsDomainParticipantTests, EnableDisableStatisticsDataWriterTest)
{
    eprosima::fastdds::dds::DomainParticipant* participant =
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->
                    create_participant(0, eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);
    ASSERT_NE(participant, nullptr);

    eprosima::fastdds::dds::DataWriterQos inconsistent_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
    inconsistent_qos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
    inconsistent_qos.ownership().kind = eprosima::fastdds::dds::EXCLUSIVE_OWNERSHIP_QOS;

#ifndef FASTDDS_STATISTICS
    // 1. Compilation flag not set
    eprosima::fastdds::statistics::dds::DomainParticipant* statistics_participant =
            static_cast<eprosima::fastdds::statistics::dds::DomainParticipant*>(participant);
    ASSERT_NE(statistics_participant, nullptr);

    eprosima::fastrtps::types::ReturnCode_t ret = statistics_participant->enable_statistics_datawriter(
        HISTORY_LATENCY_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_UNSUPPORTED);

    ret = statistics_participant->enable_statistics_datawriter("INVALID_TOPIC", inconsistent_qos);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_UNSUPPORTED);

    ret = statistics_participant->disable_statistics_datawriter(HISTORY_LATENCY_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_UNSUPPORTED);

    ret = statistics_participant->disable_statistics_datawriter("INVALID_TOPIC");
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_UNSUPPORTED);
#else
    // 2. Narrow DomainParticipant to eprosima::fastdds::statistics::dds::DomainParticipant
    eprosima::fastdds::statistics::dds::DomainParticipant* statistics_participant =
            eprosima::fastdds::statistics::dds::DomainParticipant::narrow(participant);
    ASSERT_NE(statistics_participant, nullptr);

    // 3. Create TypeSupports for the different DataTypes
    eprosima::fastdds::dds::TypeSupport history_latency_type(
        new eprosima::fastdds::statistics::WriterReaderDataPubSubType);
    eprosima::fastdds::dds::TypeSupport network_latency_type(
        new eprosima::fastdds::statistics::Locator2LocatorDataPubSubType);
    eprosima::fastdds::dds::TypeSupport throughput_type(
        new eprosima::fastdds::statistics::EntityDataPubSubType);
    eprosima::fastdds::dds::TypeSupport rtps_traffic_type(
        new eprosima::fastdds::statistics::Entity2LocatorTrafficPubSubType);
    eprosima::fastdds::dds::TypeSupport count_type(
        new eprosima::fastdds::statistics::EntityCountPubSubType);
    eprosima::fastdds::dds::TypeSupport discovery_type(
        new eprosima::fastdds::statistics::DiscoveryTimePubSubType);
    eprosima::fastdds::dds::TypeSupport sample_identity_count_type(
        new eprosima::fastdds::statistics::SampleIdentityCountPubSubType);
    eprosima::fastdds::dds::TypeSupport physical_data_type(
        new eprosima::fastdds::statistics::PhysicalDataPubSubType);
    eprosima::fastdds::dds::TypeSupport null_type(nullptr);

    // 4. Check that the types are not registered yet
    EXPECT_EQ(null_type, statistics_participant->find_type(history_latency_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(network_latency_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(throughput_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(rtps_traffic_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(count_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(discovery_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(sample_identity_count_type.get_type_name()));
    EXPECT_EQ(null_type, statistics_participant->find_type(physical_data_type.get_type_name()));

    // 5. Check that the topics do not exist
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(HISTORY_LATENCY_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(NETWORK_LATENCY_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(PUBLICATION_THROUGHPUT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(SUBSCRIPTION_THROUGHPUT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(RTPS_SENT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(RTPS_LOST_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(RESENT_DATAS_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(HEARTBEAT_COUNT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(ACKNACK_COUNT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(NACKFRAG_COUNT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(GAP_COUNT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(DATA_COUNT_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(PDP_PACKETS_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(EDP_PACKETS_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(DISCOVERY_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(SAMPLE_DATAS_TOPIC));
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(PHYSICAL_DATA_TOPIC));

    // 6. Enable each statistics DataWriter checking that topics are created and types are registered.
    eprosima::fastrtps::types::ReturnCode_t ret = statistics_participant->enable_statistics_datawriter(
        HISTORY_LATENCY_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(HISTORY_LATENCY_TOPIC));
    EXPECT_TRUE(history_latency_type == statistics_participant->find_type(history_latency_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(NETWORK_LATENCY_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(NETWORK_LATENCY_TOPIC));
    EXPECT_TRUE(network_latency_type == statistics_participant->find_type(network_latency_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(PUBLICATION_THROUGHPUT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(PUBLICATION_THROUGHPUT_TOPIC));
    EXPECT_TRUE(throughput_type == statistics_participant->find_type(throughput_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(
        SUBSCRIPTION_THROUGHPUT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(SUBSCRIPTION_THROUGHPUT_TOPIC));
    EXPECT_TRUE(throughput_type == statistics_participant->find_type(throughput_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(RTPS_SENT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(RTPS_SENT_TOPIC));
    EXPECT_TRUE(rtps_traffic_type == statistics_participant->find_type(rtps_traffic_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(RTPS_LOST_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(RTPS_LOST_TOPIC));
    EXPECT_TRUE(rtps_traffic_type == statistics_participant->find_type(rtps_traffic_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(RESENT_DATAS_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(RESENT_DATAS_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(HEARTBEAT_COUNT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(HEARTBEAT_COUNT_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(ACKNACK_COUNT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(ACKNACK_COUNT_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(NACKFRAG_COUNT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(NACKFRAG_COUNT_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(GAP_COUNT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(GAP_COUNT_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(DATA_COUNT_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(DATA_COUNT_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(PDP_PACKETS_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(PDP_PACKETS_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(EDP_PACKETS_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(EDP_PACKETS_TOPIC));
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(DISCOVERY_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(DISCOVERY_TOPIC));
    EXPECT_TRUE(discovery_type == statistics_participant->find_type(discovery_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(SAMPLE_DATAS_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(SAMPLE_DATAS_TOPIC));
    EXPECT_TRUE(sample_identity_count_type == statistics_participant->find_type(
                sample_identity_count_type.get_type_name()));

    ret = statistics_participant->enable_statistics_datawriter(PHYSICAL_DATA_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(PHYSICAL_DATA_TOPIC));
    EXPECT_TRUE(physical_data_type == statistics_participant->find_type(physical_data_type.get_type_name()));

    // 7. Enable an already enabled statistics DataWriter
    ret = statistics_participant->enable_statistics_datawriter(SAMPLE_DATAS_TOPIC, STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);

    // 8. Invalid topic name
    ret = statistics_participant->enable_statistics_datawriter("INVALID_TOPIC", STATISTICS_DATAWRITER_QOS);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_BAD_PARAMETER);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription("INVALID_TOPIC"));

    // 9. Disable statistics DataWriter
    ret = statistics_participant->disable_statistics_datawriter(HISTORY_LATENCY_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(HISTORY_LATENCY_TOPIC));
    EXPECT_NE(nullptr, statistics_participant->lookup_topicdescription(PDP_PACKETS_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(history_latency_type.get_type_name()));

    // 10. Enable previous statistics DataWriter with an inconsistent QoS
    ret = statistics_participant->enable_statistics_datawriter(HISTORY_LATENCY_TOPIC, inconsistent_qos);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_INCONSISTENT_POLICY);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(HISTORY_LATENCY_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(history_latency_type.get_type_name()));

    // 11. RETCODE_BAD_PARAMETER error has precedence over RETCODE_INCONSISTENT_POLICY
    ret = statistics_participant->enable_statistics_datawriter("INVALID_TOPIC", inconsistent_qos);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_BAD_PARAMETER);

    // 12. Disable already disabled DataWriter
    ret = statistics_participant->disable_statistics_datawriter(HISTORY_LATENCY_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);

    // 13. Disable invalid topic name
    ret = statistics_participant->disable_statistics_datawriter("INVALID_TOPIC");
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_BAD_PARAMETER);

    // 14. Remove DDS entities
    ret = statistics_participant->disable_statistics_datawriter(NETWORK_LATENCY_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(NETWORK_LATENCY_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(network_latency_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(PUBLICATION_THROUGHPUT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(PUBLICATION_THROUGHPUT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(throughput_type == statistics_participant->find_type(throughput_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(SUBSCRIPTION_THROUGHPUT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(SUBSCRIPTION_THROUGHPUT_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(throughput_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(RTPS_SENT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(RTPS_SENT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(rtps_traffic_type == statistics_participant->find_type(rtps_traffic_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(RTPS_LOST_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(RTPS_LOST_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(rtps_traffic_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(RESENT_DATAS_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(RESENT_DATAS_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(HEARTBEAT_COUNT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(HEARTBEAT_COUNT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(ACKNACK_COUNT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(ACKNACK_COUNT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(NACKFRAG_COUNT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(NACKFRAG_COUNT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(GAP_COUNT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(GAP_COUNT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(DATA_COUNT_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(DATA_COUNT_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(PDP_PACKETS_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(PDP_PACKETS_TOPIC));
    // The type is being used by another topic yet
    EXPECT_TRUE(count_type == statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(EDP_PACKETS_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(EDP_PACKETS_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(DISCOVERY_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(DISCOVERY_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(discovery_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(SAMPLE_DATAS_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(SAMPLE_DATAS_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(sample_identity_count_type.get_type_name()));

    ret = statistics_participant->disable_statistics_datawriter(PHYSICAL_DATA_TOPIC);
    EXPECT_EQ(ret, eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
    EXPECT_EQ(nullptr, statistics_participant->lookup_topicdescription(PHYSICAL_DATA_TOPIC));
    EXPECT_EQ(null_type, statistics_participant->find_type(physical_data_type.get_type_name()));
#endif // FASTDDS_STATISTICS

    EXPECT_EQ(eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->
                    delete_participant(statistics_participant),
            eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK);
}

/**
 * This test checks that when the topic name provided is not valid, a log error is printed.
 * 1. Create a participant with the property fastdds.statistics set to an invalid topic name
 * 2. Check that there is no topic/type registered in the participant
 * 3. Wait for the logError entry to be consumed
 */
TEST_F(StatisticsDomainParticipantTests, CreateParticipantWithInvalidTopicName)
{
#ifdef FASTDDS_STATISTICS
    mock_consumer_ = new eprosima::fastdds::dds::MockConsumer();

    eprosima::fastdds::dds::Log::RegisterConsumer(std::unique_ptr<eprosima::fastdds::dds::LogConsumer>(mock_consumer_));
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Error);
    eprosima::fastdds::dds::Log::SetCategoryFilter(std::regex("(STATISTICS_DOMAIN_PARTICIPANT)"));
    eprosima::fastdds::dds::Log::SetErrorStringFilter(std::regex("(not a valid statistics topic name/alias)"));

    // 1. Create DomainParticipant
    eprosima::fastdds::dds::DomainParticipantQos pqos;
    pqos.properties().properties().emplace_back("fastdds.statistics",
            "INVALID_TOPIC_NAME1; INVALID_TOPIC_NAME2");

    eprosima::fastdds::dds::DomainParticipant* participant =
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->
                    create_participant(0, pqos);
    ASSERT_NE(participant, nullptr);

    // 2. Check topics/types
    // Create TypeSupports
    eprosima::fastdds::dds::TypeSupport history_latency_type(
        new eprosima::fastdds::statistics::WriterReaderDataPubSubType);
    eprosima::fastdds::dds::TypeSupport network_latency_type(
        new eprosima::fastdds::statistics::Locator2LocatorDataPubSubType);
    eprosima::fastdds::dds::TypeSupport throughput_type(
        new eprosima::fastdds::statistics::EntityDataPubSubType);
    eprosima::fastdds::dds::TypeSupport rtps_traffic_type(
        new eprosima::fastdds::statistics::Entity2LocatorTrafficPubSubType);
    eprosima::fastdds::dds::TypeSupport count_type(
        new eprosima::fastdds::statistics::EntityCountPubSubType);
    eprosima::fastdds::dds::TypeSupport discovery_type(
        new eprosima::fastdds::statistics::DiscoveryTimePubSubType);
    eprosima::fastdds::dds::TypeSupport sample_identity_count_type(
        new eprosima::fastdds::statistics::SampleIdentityCountPubSubType);
    eprosima::fastdds::dds::TypeSupport physical_data_type(
        new eprosima::fastdds::statistics::PhysicalDataPubSubType);
    eprosima::fastdds::dds::TypeSupport null_type(nullptr);

    EXPECT_EQ(null_type, participant->find_type(history_latency_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(network_latency_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(throughput_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(rtps_traffic_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(count_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(discovery_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(sample_identity_count_type.get_type_name()));
    EXPECT_EQ(null_type, participant->find_type(physical_data_type.get_type_name()));

    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::HISTORY_LATENCY_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::NETWORK_LATENCY_TOPIC));
    EXPECT_EQ(nullptr,
            participant->lookup_topicdescription(eprosima::fastdds::statistics::PUBLICATION_THROUGHPUT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(
                eprosima::fastdds::statistics::SUBSCRIPTION_THROUGHPUT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::RTPS_SENT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::RTPS_LOST_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::RESENT_DATAS_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::HEARTBEAT_COUNT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::ACKNACK_COUNT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::NACKFRAG_COUNT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::GAP_COUNT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::DATA_COUNT_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::PDP_PACKETS_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::EDP_PACKETS_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::DISCOVERY_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::SAMPLE_DATAS_TOPIC));
    EXPECT_EQ(nullptr, participant->lookup_topicdescription(eprosima::fastdds::statistics::PHYSICAL_DATA_TOPIC));

    // 3. Wait until logError entries are captured
    helper_block_for_at_least_entries(2);
    auto consumed_entries = mock_consumer_->ConsumedEntries();
    EXPECT_EQ(consumed_entries.size(), 2u);
#endif // FASTDDS_STATISTICS
}

} // namespace dds
} // namespace statistics
} // namespace fastdds
} // namespace eprosima

int main(
        int argc,
        char** argv)
{
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Error);

    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();

    eprosima::fastdds::dds::Log::KillThread();
    return ret;
}
