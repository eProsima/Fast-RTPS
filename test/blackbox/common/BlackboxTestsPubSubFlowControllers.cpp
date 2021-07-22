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
#include "PubSubWriterReader.hpp"
#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <gtest/gtest.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

class PubSubFlowControllers : public testing::TestWithParam<eprosima::fastdds::rtps::FlowControllerSchedulerPolicy>
{
public:

    void SetUp() override
    {
        scheduler_policy_ = GetParam();
    }

    void TearDown() override
    {
    }

protected:

    eprosima::fastdds::rtps::FlowControllerSchedulerPolicy scheduler_policy_;
};

TEST_P(PubSubFlowControllers, AsyncPubSubAsReliableData64kbWithParticipantFlowControl)
{
    PubSubReader<Data64kbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data64kbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(3).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    uint32_t bytesPerPeriod = 68000;
    uint32_t periodInMs = 500;
    writer.add_throughput_controller_descriptor_to_pparams(scheduler_policy_, bytesPerPeriod, periodInMs);

    writer.history_depth(3).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data64kb_data_generator(3);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(PubSubFlowControllers, AsyncPubSubAsReliableData64kbWithParticipantFlowControlAndUserTransport)
{
    PubSubReader<Data64kbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data64kbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(3).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    uint32_t bytesPerPeriod = 65000;
    uint32_t periodInMs = 500;
    writer.add_throughput_controller_descriptor_to_pparams(scheduler_policy_, bytesPerPeriod, periodInMs);

    auto testTransport = std::make_shared<UDPv4TransportDescriptor>();
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(testTransport);

    writer.history_depth(3).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data64kb_data_generator(3);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(PubSubFlowControllers, AsyncPubSubWithFlowController64kb)
{
    PubSubReader<Data64kbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data64kbType> slowWriter(TEST_TOPIC_NAME);

    reader.history_depth(2).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();
    ASSERT_TRUE(reader.isInitialized());

    uint32_t sizeToClear = 68000; //68kb
    uint32_t periodInMs = 1000; //1sec

    slowWriter.history_depth(2).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(scheduler_policy_, sizeToClear, periodInMs).init();
    ASSERT_TRUE(slowWriter.isInitialized());

    slowWriter.wait_discovery();
    reader.wait_discovery();

    auto data = default_data64kb_data_generator(2);

    reader.startReception(data);
    slowWriter.send(data);
    // In 1 second only one of the messages has time to arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(reader.getReceivedCount(), 1u);
}

TEST_P(PubSubFlowControllers, FlowControllerIfNotAsync)
{
    PubSubWriter<Data64kbType> writer(TEST_TOPIC_NAME);

    uint32_t size = 10000;
    uint32_t periodInMs = 1000;
    writer.add_throughput_controller_descriptor_to_pparams(scheduler_policy_, size, periodInMs).init();
    ASSERT_FALSE(writer.isInitialized());
}

TEST_P(PubSubFlowControllers, AsyncMultipleWritersFlowController64kb)
{
    PubSubWriterReader<Data64kbType> entities(TEST_TOPIC_NAME);

    // Readers configuration
    entities.sub_history_depth(3).
            sub_durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS).
            sub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);

    // Writers configuration.
    uint32_t bytesPerPeriod = 68000;
    uint32_t periodInMs = 500;
    entities.add_throughput_controller_descriptor_to_pparams(scheduler_policy_, bytesPerPeriod, periodInMs).
            pub_history_depth(3).
            pub_durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS).
            pub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE);

    // Creation.
    entities.init();

    ASSERT_TRUE(entities.isInitialized());

    // Create second writer.
    eprosima::fastrtps::rtps::PropertySeq writers2_properties;
    eprosima::fastrtps::rtps::Property priority;
    priority.name("fastdds.sfc.priority");
    priority.value("-1");
    writers2_properties.push_back(priority);
    eprosima::fastrtps::rtps::Property bandwidth_limit;
    bandwidth_limit.name("fastdds.sfc.bandwidth_reservation");
    bandwidth_limit.value("10");
    writers2_properties.push_back(bandwidth_limit);
    ASSERT_TRUE(entities.create_additional_topics(1, "/", writers2_properties));

    eprosima::fastrtps::rtps::PropertySeq writers3_properties;
    priority.name("fastdds.sfc.priority");
    priority.value("1");
    writers3_properties.push_back(priority);
    bandwidth_limit.name("fastdds.sfc.bandwidth_reservation");
    bandwidth_limit.value("15");
    writers3_properties.push_back(bandwidth_limit);
    ASSERT_TRUE(entities.create_additional_topics(1, "/", writers3_properties));

    eprosima::fastrtps::rtps::PropertySeq writers4_properties;
    priority.name("fastdds.sfc.priority");
    priority.value("4");
    writers4_properties.push_back(priority);
    bandwidth_limit.name("fastdds.sfc.bandwidth_reservation");
    bandwidth_limit.value("20");
    writers4_properties.push_back(bandwidth_limit);
    ASSERT_TRUE(entities.create_additional_topics(1, "/", writers4_properties));

    // Because its volatile the durability
    // Wait for discovery.
    entities.wait_discovery();

    auto data = default_data64kb_data_generator(3);

    entities.startReception(data);

    // Send data
    entities.send(data);

    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());

    // Block reader until reception finished or timeout.
    entities.block_for_all();
}

#ifdef INSTANTIATE_TEST_SUITE_P
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_SUITE_P(x, y, z, w)
#else
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_CASE_P(x, y, z, w)
#endif // ifdef INSTANTIATE_TEST_SUITE_P

GTEST_INSTANTIATE_TEST_MACRO(PubSubFlowControllers,
        PubSubFlowControllers,
        testing::Values(
            eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::FIFO,
            eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::ROUND_ROBIN,
            eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::HIGH_PRIORITY,
            eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::PRIORITY_WITH_RESERVATION
            ),
        [](const testing::TestParamInfo<PubSubFlowControllers::ParamType>& info)
        {
            std::string suffix;
            switch (info.param)
            {
                case eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::PRIORITY_WITH_RESERVATION:
                    suffix = "_SCHED_RESERV";
                    break;
                case eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::HIGH_PRIORITY:
                    suffix = "_SCHED_HIGH";
                    break;
                case eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::ROUND_ROBIN:
                    suffix = "_SCHED_ROBIN";
                    break;
                case eprosima::fastdds::rtps::FlowControllerSchedulerPolicy::FIFO:
                default:
                    suffix = "_SCHED_FIFO";
            }

            return "Transport" + suffix;
        });
