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

#include <fastdds/dds/log/Log.hpp>
#include <fastrtps/xmlparser/XMLProfileManager.h>
#include <rtps/transport/test_UDPv4Transport.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;
using test_UDPv4Transport = eprosima::fastdds::rtps::test_UDPv4Transport;
using test_UDPv4TransportDescriptor = eprosima::fastdds::rtps::test_UDPv4TransportDescriptor;

enum communication_type
{
    TRANSPORT,
    INTRAPROCESS,
    DATASHARING
};

class PubSubFragments : public testing::TestWithParam<communication_type>
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

protected:

    void do_fragment_test(
            const std::string& topic_name,
            std::list<Data1mb>& data,
            bool asynchronous,
            bool reliable,
            bool volatile_reader,
            bool volatile_writer,
            bool small_fragments)
    {
        PubSubReader<Data1mbType> reader(topic_name);
        PubSubWriter<Data1mbType> writer(topic_name);

        reader
                .socket_buffer_size(1048576) // accomodate large and fast fragments
                .history_depth(static_cast<int32_t>(data.size()))
                .reliability(reliable ?
                eprosima::fastrtps::RELIABLE_RELIABILITY_QOS :
                eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS)
                .durability_kind(volatile_reader ?
                eprosima::fastrtps::VOLATILE_DURABILITY_QOS :
                eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS)
                .init();

        ASSERT_TRUE(reader.isInitialized());

        if (small_fragments)
        {
            auto testTransport = std::make_shared<UDPv4TransportDescriptor>();
            testTransport->sendBufferSize = 1024;
            testTransport->maxMessageSize = 1024;
            testTransport->receiveBufferSize = 65536;
            writer.disable_builtin_transport();
            writer.add_user_transport_to_pparams(testTransport);
        }

        if (asynchronous)
        {
            writer.asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE);
        }

        writer
                .history_depth(static_cast<int32_t>(data.size()))
                .reliability(reliable ?
                eprosima::fastrtps::RELIABLE_RELIABILITY_QOS :
                eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS)
                .durability_kind(volatile_writer ?
                eprosima::fastrtps::VOLATILE_DURABILITY_QOS :
                eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS)
                .init();

        ASSERT_TRUE(writer.isInitialized());

        // Because its volatile the durability
        // Wait for discovery.
        writer.wait_discovery();
        reader.wait_discovery();

        reader.startReception(data);
        // Send data
        writer.send(data, reliable ? 0u : 30u);
        ASSERT_TRUE(data.empty());

        // Block reader until reception finished or timeout.
        if (reliable)
        {
            reader.block_for_all();
        }
        else
        {
            reader.block_for_at_least(2);
        }
    }

};

TEST_P(PubSubFragments, PubSubAsNonReliableData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, false, true, false, false);
}

TEST_P(PubSubFragments, PubSubAsNonReliableVolatileData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, false, true, true, false);
}

TEST_P(PubSubFragments, PubSubAsNonReliableTransientLocalData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, false, false, false, false);
}

TEST_P(PubSubFragments, PubSubAsNonReliableData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, false, true, false, true);
}

TEST_P(PubSubFragments, PubSubAsNonReliableVolatileData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, false, true, true, true);
}

TEST_P(PubSubFragments, PubSubAsNonReliableTransientLocalData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, false, false, false, true);
}

TEST_P(PubSubFragments, PubSubAsReliableData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, true, true, false, false);
}

TEST_P(PubSubFragments, PubSubAsReliableVolatileData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, true, true, true, false);
}

TEST_P(PubSubFragments, PubSubAsReliableTransientLocalData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, true, false, false, false);
}

TEST_P(PubSubFragments, PubSubAsReliableData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, true, true, false, true);
}

TEST_P(PubSubFragments, PubSubAsReliableVolatileData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, true, true, true, true);
}

TEST_P(PubSubFragments, PubSubAsReliableTransientLocalData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, false, true, false, false, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, false, true, false, false);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableVolatileData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, false, true, true, false);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableTransientLocalData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, false, false, false, false);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, false, true, false, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableVolatileData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, false, true, true, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableTransientLocalData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, false, false, false, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, true, true, false, false);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableVolatileData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, true, true, true, false);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableTransientLocalData300kb)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, true, false, false, false);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, true, true, false, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableVolatileData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, true, true, true, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableTransientLocalData300kbSmallFragments)
{
    auto data = default_data300kb_data_generator();
    do_fragment_test(TEST_TOPIC_NAME, data, true, true, false, false, true);
}

TEST_P(PubSubFragments, AsyncPubSubAsNonReliableData300kbWithFlowControl)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator();

    reader.startReception(data);
    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

TEST_P(PubSubFragments, AsyncPubSubAsReliableData300kbWithFlowControl)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator(5);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST(PubSubFragments, AsyncPubSubAsReliableData300kbInLossyConditions)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 300000;
    uint32_t periodInMs = 200;
    writer.add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs);

    // To simulate lossy conditions, we are going to remove the default
    // bultin transport, and instead use a lossy shim layer variant.
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->sendBufferSize = 65536;
    testTransport->receiveBufferSize = 65536;
    // We drop 20% of all data frags
    testTransport->dropDataFragMessagesPercentage = 20;
    testTransport->dropLogLength = 1;
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(testTransport);

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator(5);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();

    // Sanity check. Make sure we have dropped a few packets
    ASSERT_EQ(
        test_UDPv4Transport::test_UDPv4Transport_DropLog.size(),
        testTransport->dropLogLength);
}

TEST(PubSubFragments, AsyncPubSubAsReliableVolatileData300kbInLossyConditions)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 300000;
    uint32_t periodInMs = 200;
    writer.add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs);

    // To simulate lossy conditions, we are going to remove the default
    // bultin transport, and instead use a lossy shim layer variant.
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->sendBufferSize = 65536;
    testTransport->receiveBufferSize = 65536;
    // We drop 20% of all data frags
    testTransport->dropDataFragMessagesPercentage = 20;
    testTransport->dropLogLength = 1;
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(testTransport);

    writer.history_depth(5).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator(5);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();

    // Sanity check. Make sure we have dropped a few packets
    ASSERT_EQ(
        test_UDPv4Transport::test_UDPv4Transport_DropLog.size(),
        testTransport->dropLogLength);
}

TEST(PubSubFragments, AsyncPubSubAsReliableData300kbInLossyConditionsSmallFragments)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 300000;
    uint32_t periodInMs = 200;
    writer.add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs);

    // To simulate lossy conditions, we are going to remove the default
    // bultin transport, and instead use a lossy shim layer variant.
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->sendBufferSize = 1024;
    testTransport->maxMessageSize = 1024;
    testTransport->receiveBufferSize = 65536;
    // We are sending around 300 fragments per sample.
    // We drop 1% of all data frags
    testTransport->dropDataFragMessagesPercentage = 1;
    testTransport->dropLogLength = 1;
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(testTransport);

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator(5);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();

    // Sanity check. Make sure we have dropped a few packets
    ASSERT_EQ(
        test_UDPv4Transport::test_UDPv4Transport_DropLog.size(),
        testTransport->dropLogLength);
}

TEST(PubSubFragments, AsyncPubSubAsReliableVolatileData300kbInLossyConditionsSmallFragments)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 300000;
    uint32_t periodInMs = 200;
    writer.add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs);

    // To simulate lossy conditions, we are going to remove the default
    // bultin transport, and instead use a lossy shim layer variant.
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->sendBufferSize = 1024;
    testTransport->maxMessageSize = 1024;
    testTransport->receiveBufferSize = 65536;
    // We are sending around 300 fragments per sample.
    // We drop 1% of all data frags
    testTransport->dropDataFragMessagesPercentage = 1;
    testTransport->dropLogLength = 1;
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(testTransport);

    writer.history_depth(5).
            durability_kind(eprosima::fastrtps::VOLATILE_DURABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_data_generator(5);

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();

    // Sanity check. Make sure we have dropped a few packets
    ASSERT_EQ(
        test_UDPv4Transport::test_UDPv4Transport_DropLog.size(),
        testTransport->dropLogLength);
}

TEST(PubSubFragments, AsyncFragmentSizeTest)
{
    // ThroghputController size large than maxMessageSize.
    {
        PubSubReader<Data64kbType> reader(TEST_TOPIC_NAME);
        PubSubWriter<Data64kbType> writer(TEST_TOPIC_NAME);

        reader.history_depth(10).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

        ASSERT_TRUE(reader.isInitialized());

        // When doing fragmentation, it is necessary to have some degree of
        // flow control not to overrun the receive buffer.
        uint32_t size = 32536;
        uint32_t periodInMs = 500;
        writer.add_throughput_controller_descriptor_to_pparams(size, periodInMs);

        auto testTransport = std::make_shared<UDPv4TransportDescriptor>();
        testTransport->maxMessageSize = 32000;
        testTransport->sendBufferSize = 65536;
        testTransport->receiveBufferSize = 65536;
        writer.disable_builtin_transport();
        writer.add_user_transport_to_pparams(testTransport);
        writer.history_depth(10).asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

        ASSERT_TRUE(writer.isInitialized());

        // Because its volatile the durability
        // Wait for discovery.
        writer.wait_discovery();
        reader.wait_discovery();

        auto data = default_data64kb_data_generator();

        reader.startReception(data);

        // Send data
        writer.send(data);
        // In this test all data should be sent.
        ASSERT_TRUE(data.empty());
        // Block reader until reception finished or timeout.
        std::this_thread::sleep_for(std::chrono::seconds(3));
        size_t current_received = reader.getReceivedCount();
        ASSERT_GE(current_received, static_cast<size_t>(1));
        ASSERT_LE(current_received, static_cast<size_t>(3));

    }
    // ThroghputController size smaller than maxMessageSize.
    {
        PubSubReader<Data64kbType> reader(TEST_TOPIC_NAME);
        PubSubWriter<Data64kbType> writer(TEST_TOPIC_NAME);

        reader.history_depth(10).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

        ASSERT_TRUE(reader.isInitialized());

        // When doing fragmentation, it is necessary to have some degree of
        // flow control not to overrun the receive buffer.
        uint32_t size = 32000;
        uint32_t periodInMs = 500;
        writer.add_throughput_controller_descriptor_to_pparams(size, periodInMs);

        auto testTransport = std::make_shared<UDPv4TransportDescriptor>();
        testTransport->maxMessageSize = 32536;
        testTransport->sendBufferSize = 65536;
        testTransport->receiveBufferSize = 65536;
        writer.disable_builtin_transport();
        writer.add_user_transport_to_pparams(testTransport);
        writer.history_depth(10).
                asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).init();

        ASSERT_TRUE(writer.isInitialized());

        // Because its volatile the durability
        // Wait for discovery.
        writer.wait_discovery();
        reader.wait_discovery();

        auto data = default_data64kb_data_generator();

        reader.startReception(data);

        // Send data
        writer.send(data);
        // In this test all data should be sent.
        ASSERT_TRUE(data.empty());
        // Block reader until reception finished or timeout.
        std::this_thread::sleep_for(std::chrono::seconds(3));
        size_t current_received = reader.getReceivedCount();
        ASSERT_GE(current_received, static_cast<size_t>(1));
        ASSERT_LE(current_received, static_cast<size_t>(3));
    }
}


#ifdef INSTANTIATE_TEST_SUITE_P
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_SUITE_P(x, y, z, w)
#else
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_CASE_P(x, y, z, w)
#endif // ifdef INSTANTIATE_TEST_SUITE_P

GTEST_INSTANTIATE_TEST_MACRO(PubSubFragments,
        PubSubFragments,
        testing::Values(TRANSPORT, INTRAPROCESS, DATASHARING),
        [](const testing::TestParamInfo<PubSubFragments::ParamType>& info)
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
