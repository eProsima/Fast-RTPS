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

#include "BlackboxTests.hpp"

#include "PubSubReader.hpp"
#include "PubSubWriter.hpp"
#include "ReqRepAsReliableHelloWorldRequester.hpp"
#include "ReqRepAsReliableHelloWorldReplier.hpp"

#include <gtest/gtest.h>
#include <fastrtps/xmlparser/XMLProfileManager.h>
#include <fastdds/rtps/common/CDRMessage_t.h>
#include <rtps/transport/test_UDPv4Transport.h>

using namespace eprosima::fastrtps;
using test_UDPv4Transport = eprosima::fastdds::rtps::test_UDPv4Transport;
using test_UDPv4TransportDescriptor = eprosima::fastdds::rtps::test_UDPv4TransportDescriptor;

enum communication_type
{
    TRANSPORT,
    INTRAPROCESS,
    DATASHARING
};

class Volatile : public testing::TestWithParam<communication_type>
{
public:

    void SetUp() override
    {
        LibrarySettingsAttributes library_settings;
        switch (GetParam())
        {
            case INTRAPROCESS:
                library_settings.intraprocess_delivery = IntraprocessDeliveryType::INTRAPROCESS_FULL;
                xmlparser::XMLProfileManager::library_settings(library_settings);
                break;
            case DATASHARING:
                enable_datasharing = true;
                break;
            case TRANSPORT:
            default:
                break;
        }
    }

    void TearDown() override
    {
        LibrarySettingsAttributes library_settings;
        switch (GetParam())
        {
            case INTRAPROCESS:
                library_settings.intraprocess_delivery = IntraprocessDeliveryType::INTRAPROCESS_OFF;
                xmlparser::XMLProfileManager::library_settings(library_settings);
                break;
            case DATASHARING:
                enable_datasharing = false;
                break;
            case TRANSPORT:
            default:
                break;
        }
    }

};

// Test created to check bug #3020 (Github ros2/demos #238)
TEST_P(Volatile, PubSubAsReliableVolatilePubRemoveWithoutSubs)
{
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.history_depth(10).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).init();

    ASSERT_TRUE(writer.isInitialized());

    auto data = default_helloworld_data_generator();

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());

    size_t number_of_changes_removed = 0;
    ASSERT_FALSE(writer.remove_all_changes(&number_of_changes_removed));
}

// Test created to check bug #3087 (Github #230)
TEST_P(Volatile, AsyncPubSubAsNonReliableVolatileHelloworld)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_depth(100).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    reader.startReception(data);
    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

// Test created to check bug #3290 (ROS2 #539)
TEST_P(Volatile, AsyncVolatileKeepAllPubReliableSubNonReliable300Kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(9).
            resource_limits_max_samples(9).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(
        eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::FIFO, bytesPerPeriod, periodInMs).
            init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator(10);

    reader.startReception(data);
    // Send data with some interval, to let async writer thread send samples
    writer.send(data, 300);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

// Test created to check bug #3290 (ROS2 #539)
TEST_P(Volatile, VolatileKeepAllPubReliableSubNonReliableHelloWorld)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(9).
            resource_limits_max_samples(9).
            init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(10);

    reader.startReception(data);
    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

// Test created to check bug #3290 (ROS2 #539)
TEST_P(Volatile, AsyncVolatileKeepAllPubReliableSubNonReliableHelloWorld)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(9).
            resource_limits_max_samples(9).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(10);

    reader.startReception(data);
    // Send data with some interval, to let async writer thread send samples
    writer.send(data, 300);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

// Regression test of Refs #3376, github ros2/rmw_fastrtps #226
TEST_P(Volatile, ReqRepVolatileHelloworldRequesterCheckWriteParams)
{
    ReqRepAsReliableHelloWorldRequester requester;

    requester.durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).init();

    ASSERT_TRUE(requester.isInitialized());

    requester.send(1);
}

// Test created to check bug #5423, github ros2/ros2 #703
TEST_P(Volatile, AsyncVolatileSubBetweenPubs)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(9).
            resource_limits_max_samples(9).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            heartbeat_period_seconds(3600).
            init();

    ASSERT_TRUE(writer.isInitialized());

    HelloWorld hello;
    hello.index(1);
    hello.message("HelloWorld 1");

    writer.send_sample(hello);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(1);
    reader.startReception(data);
    // Send data with some interval, to let async writer thread send samples
    writer.send(data, 300);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Volatile, VolatileSubBetweenPubs)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(9).
            resource_limits_max_samples(9).
            asynchronously(eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE).
            heartbeat_period_seconds(3600).
            init();

    ASSERT_TRUE(writer.isInitialized());

    HelloWorld hello;
    hello.index(1);
    hello.message("HelloWorld 1");

    writer.send_sample(hello);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(1);
    reader.startReception(data);
    // Send all data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Volatile, AsyncVolatileSubBetweenTransientPubs)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer
            .history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS)
            .reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS)
            .durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS)
            .resource_limits_allocated_samples(9)
            .resource_limits_max_samples(9)
            .asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE)
            .heartbeat_period_seconds(3600)
            .init();

    ASSERT_TRUE(writer.isInitialized());

    HelloWorld hello;
    hello.index(1);
    hello.message("HelloWorld 1");

    writer.send_sample(hello);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(1);
    reader.startReception(data);
    // Send data with some interval, to let async writer thread send samples
    writer.send(data, 300);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Volatile, VolatileSubBetweenTransientPubs)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer
            .history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS)
            .reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS)
            .durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS)
            .resource_limits_allocated_samples(9)
            .resource_limits_max_samples(9)
            .asynchronously(eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE)
            .heartbeat_period_seconds(3600)
            .init();

    ASSERT_TRUE(writer.isInitialized());

    HelloWorld hello;
    hello.index(1);
    hello.message("HelloWorld 1");

    writer.send_sample(hello);

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            init();

    ASSERT_TRUE(reader.isInitialized());

    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(1);
    reader.startReception(data);
    // Send all data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Volatile, VolatileLateJoinerSubGapLost)
{
    PubSubReader<HelloWorldType> reader1(TEST_TOPIC_NAME);
    PubSubReader<HelloWorldType> reader2(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);


    reader1.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            //heartbeatResponseDelay(5,0).
            init();

    ASSERT_TRUE(reader1.isInitialized());

    // To simulate lossy conditions
    int gaps_to_drop = 2;
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->drop_gap_messages_filter_ = [&gaps_to_drop](rtps::CDRMessage_t& )
            {
                if (gaps_to_drop > 0)
                {
                    --gaps_to_drop;
                    return true;
                }
                return false;
            };
    testTransport->dropLogLength = 1;

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(9).
            resource_limits_max_samples(9).
            disable_builtin_transport().
            add_user_transport_to_pparams(testTransport).
            asynchronously(eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE).
            init();

    ASSERT_TRUE(writer.isInitialized());


    writer.wait_discovery();
    reader1.wait_discovery();

    auto data = default_helloworld_data_generator(5);

    reader1.startReception(data);

    writer.send_sample(data.front());
    data.pop_front();

    reader2.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            init();

    ASSERT_TRUE(reader2.isInitialized());

    reader2.wait_discovery();
    writer.wait_discovery(2);

    reader2.startReception(data);

    // Send data with some interval, to let async writer thread send samples
    writer.send(data, 300);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader1.block_for_all();
    reader2.block_for_all();
}

// Regression test for redmine bug #11306
TEST_P(Volatile, VolatileWithLostAcks)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(10).
            resource_limits_max_samples(10).
            init();

    ASSERT_TRUE(writer.isInitialized());

    // To simulate lossy conditions
    size_t acks_to_drop = 0;
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->drop_ack_nack_messages_filter_ = [&acks_to_drop](rtps::CDRMessage_t&)
            {
                if (acks_to_drop > 0)
                {
                    --acks_to_drop;
                    return true;
                }
                return false;
            };

    reader.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            resource_limits_allocated_samples(10).
            resource_limits_max_samples(10).
            disable_builtin_transport().
            add_user_transport_to_pparams(testTransport).
            init();

    ASSERT_TRUE(reader.isInitialized());

    // Volatile durability. Endpoints should know each other before communicating.
    reader.wait_discovery();
    writer.wait_discovery();

    // Drop half the acks and perform communication
    acks_to_drop = 5;
    auto data = default_helloworld_data_generator();
    reader.startReception(data);
    writer.send(data);
    ASSERT_TRUE(data.empty());
    reader.block_for_all();

    // Wait for history to be completely acknowledged
    EXPECT_TRUE(writer.waitForAllAcked(std::chrono::minutes(10)));

    // History should be empty, so remove_all_changes should do nothing
    size_t number_of_changes_removed = 0;
    EXPECT_FALSE(writer.remove_all_changes(&number_of_changes_removed));
    EXPECT_EQ(0u, number_of_changes_removed);
}

#ifdef INSTANTIATE_TEST_SUITE_P
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_SUITE_P(x, y, z, w)
#else
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_CASE_P(x, y, z, w)
#endif // ifdef INSTANTIATE_TEST_SUITE_P

GTEST_INSTANTIATE_TEST_MACRO(Volatile,
        Volatile,
        testing::Values(TRANSPORT, INTRAPROCESS, DATASHARING),
        [](const testing::TestParamInfo<Volatile::ParamType>& info)
        {
            switch (info.param)
            {
                case INTRAPROCESS:
                    return "Intraprocess";
                    break;
                case DATASHARING:
                    return "Datasharing";
                    break;
                case TRANSPORT:
                default:
                    return "Transport";
            }

        });
