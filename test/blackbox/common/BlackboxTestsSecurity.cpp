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

#if HAVE_SECURITY

#include "PubSubReader.hpp"
#include "PubSubWriter.hpp"
#include "PubSubWriterReader.hpp"
#include "PubSubParticipant.hpp"

#include <gtest/gtest.h>

#include <fastrtps/transport/test_UDPv4Transport.h>
#include <fastrtps/xmlparser/XMLProfileManager.h>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

static const char* certs_path = nullptr;

class Security : public testing::TestWithParam<bool>
{
public:

    void SetUp() override
    {
        LibrarySettingsAttributes library_settings;
        if (GetParam())
        {
            library_settings.intraprocess_delivery = IntraprocessDeliveryType::INTRAPROCESS_FULL;
            xmlparser::XMLProfileManager::library_settings(library_settings);
        }
    }

    void TearDown() override
    {
        LibrarySettingsAttributes library_settings;
        if (GetParam())
        {
            library_settings.intraprocess_delivery = IntraprocessDeliveryType::INTRAPROCESS_OFF;
            xmlparser::XMLProfileManager::library_settings(library_settings);
        }
    }

};

TEST_P(Security, BuiltinAuthenticationPlugin_PKIDH_validation_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));

    writer.history_depth(10).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_all();
}

// Used to detect Github issue #106
TEST_P(Security, BuiltinAuthenticationPlugin_PKIDH_validation_ok_same_participant)
{
    PubSubWriterReader<HelloWorldType> wreader(TEST_TOPIC_NAME);

    PropertyPolicy property_policy;

    property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));

    wreader.sub_history_depth(10).sub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);
    wreader.property_policy(property_policy).init();

    ASSERT_TRUE(wreader.isInitialized());

    // Wait for discovery.
    wreader.wait_discovery();

    auto data = default_helloworld_data_generator();

    wreader.startReception(data);

    // Send data
    wreader.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    wreader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationPlugin_PKIDH_validation_fail)
{
    {
        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

        PropertyPolicy pub_property_policy;

        reader.history_depth(10).ignore_participant_flags(eprosima::fastrtps::rtps::NO_FILTER).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

        ASSERT_TRUE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));

        writer.history_depth(10).
                property_policy(pub_property_policy).init();

        ASSERT_TRUE(writer.isInitialized());

        // Wait for authorization
        writer.waitUnauthorized();
    }
    {
        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

        PropertyPolicy sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));

        reader.history_depth(10).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
                property_policy(sub_property_policy).init();

        ASSERT_TRUE(reader.isInitialized());

        writer.history_depth(10).init();

        ASSERT_TRUE(writer.isInitialized());

        // Wait for authorization
        reader.waitUnauthorized();
    }
}

TEST_P(Security, BuiltinAuthenticationPlugin_PKIDH_lossy_conditions)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_property_policy).init();

    // To simulate lossy conditions, we are going to remove the default
    // bultin transport, and instead use a lossy shim layer variant.
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    testTransport->sendBufferSize = 65536;
    testTransport->receiveBufferSize = 65536;
    // We drop 20% of all data frags
    testTransport->dropDataMessagesPercentage = 40;
    testTransport->dropLogLength = 10;
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(testTransport);

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));

    writer.history_depth(10).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_rtps_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_shm_transport_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    auto shm_transport = std::make_shared<eprosima::fastdds::rtps::SharedMemTransportDescriptor>();
    auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
    const uint32_t segment_size = 1024 * 1024;
    shm_transport->segment_size(segment_size);
    shm_transport->max_message_size(segment_size);
    reader.disable_builtin_transport();
    reader.add_user_transport_to_pparams(shm_transport);
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(shm_transport);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_shm_udp_transport_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    auto shm_transport = std::make_shared<eprosima::fastdds::rtps::SharedMemTransportDescriptor>();
    auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
    const uint32_t segment_size = 1024 * 1024;
    shm_transport->segment_size(segment_size);
    shm_transport->max_message_size(segment_size);
    reader.disable_builtin_transport();
    reader.add_user_transport_to_pparams(shm_transport);
    reader.add_user_transport_to_pparams(udp_transport);
    writer.disable_builtin_transport();
    writer.add_user_transport_to_pparams(shm_transport);
    writer.add_user_transport_to_pparams(udp_transport);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_rtps_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_all();
}

// Used to detect Github issue #106
TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_rtps_ok_same_participant)
{
    PubSubWriterReader<HelloWorldType> wreader(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy,
            property_policy;

    property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    wreader.property_policy(property_policy).init();

    ASSERT_TRUE(wreader.isInitialized());

    // Wait for discovery.
    wreader.wait_discovery();

    auto data = default_helloworld_data_generator();

    wreader.startReception(data);

    // Send data
    wreader.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    wreader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_rtps_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_rtps_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 500;

    writer.history_depth(5).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_rtps_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_submessage_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_submessage_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_all();
}

// Used to detect Github issue #106
TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_submessage_ok_same_participant)
{
    PubSubWriterReader<HelloWorldType> wreader(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy,
            property_policy;

    property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    wreader.sub_history_depth(10).sub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);
    wreader.property_policy(property_policy).
            pub_property_policy(pub_property_policy).
            sub_property_policy(sub_property_policy).init();

    ASSERT_TRUE(wreader.isInitialized());

    // Wait for discovery.
    wreader.wait_discovery();

    auto data = default_helloworld_data_generator();

    wreader.startReception(data);

    // Send data
    wreader.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    wreader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_submessage_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_submessage_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_submessage_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 500;

    writer.history_depth(5).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_submessage_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_payload_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_payload_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_all();
}

// Used to detect Github issue #106
TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_payload_ok_same_participant)
{
    PubSubWriterReader<HelloWorldType> wreader(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy,
            property_policy;

    property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    wreader.sub_history_depth(10).sub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);
    wreader.property_policy(property_policy).
            pub_property_policy(pub_property_policy).
            sub_property_policy(sub_property_policy).init();

    ASSERT_TRUE(wreader.isInitialized());

    // Wait for discovery.
    wreader.wait_discovery();

    auto data = default_helloworld_data_generator();

    wreader.startReception(data);

    // Send data
    wreader.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    wreader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_payload_ok_same_participant_300kb)
{
    PubSubWriterReader<Data1mbType> wreader(TEST_TOPIC_NAME);

    PropertyPolicy pub_property_policy, sub_property_policy, property_policy;

    property_policy.properties().emplace_back(Property("dds.sec.auth.plugin", "builtin.PKI-DH"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    wreader.sub_history_depth(10).sub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);
    wreader.pub_history_depth(10).pub_reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS);
    wreader.sub_durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS);
    wreader.pub_durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS);
    wreader.property_policy(property_policy).
            pub_property_policy(pub_property_policy).
            sub_property_policy(sub_property_policy).init();

    ASSERT_TRUE(wreader.isInitialized());

    // Wait for discovery.
    wreader.wait_discovery();

    auto data = default_data300kb_data_generator();

    wreader.startReception(data);

    // Send data
    wreader.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    wreader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_payload_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_payload_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_payload_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 500;

    writer.history_depth(5).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_payload_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_all_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_all_ok)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_all_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_all_large_string)
{
    PubSubReader<StringType> reader(TEST_TOPIC_NAME);
    PubSubWriter<StringType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(10).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_large_string_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_all();
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_besteffort_all_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 1000;

    writer.history_depth(5).
            reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_at_least(2);
}

TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_all_data300kb)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    // When doing fragmentation, it is necessary to have some degree of
    // flow control not to overrun the receive buffer.
    uint32_t bytesPerPeriod = 65536;
    uint32_t periodInMs = 50;

    writer.history_depth(5).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            add_throughput_controller_descriptor_to_pparams(bytesPerPeriod, periodInMs).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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

// Regression test of Refs #2457
TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_reliable_all_data300kb_mix)
{
    PubSubReader<Data1mbType> reader(TEST_TOPIC_NAME);
    PubSubWriter<Data1mbType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.history_depth(5).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(2).resource_limits_max_samples(2).resource_limits_allocated_samples(2).
            asynchronously(eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_data300kb_mix_data_generator(10);

    reader.startReception(data);

    size_t count = 0;
    for (auto data_sample : data)
    {
        // Send data
        writer.send_sample(data_sample);
        ++count;
        if (count % 2 == 0)
        {
            // Block reader until reception finished or timeout.
            reader.block_for_at_least(count);
        }
    }
}

// Regression test of Refs #2457, Github ros2 #438.
TEST_P(Security, BuiltinAuthenticationAndCryptoPlugin_user_data)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    PropertyPolicy pub_part_property_policy, sub_part_property_policy,
            pub_property_policy, sub_property_policy;

    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    pub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    writer.history_depth(100).
            userData({ 'a', 'b', 'c', 'd', 'e' }).
            property_policy(pub_part_property_policy).
            entity_property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_part_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_part_property_policy.properties().emplace_back("rtps.participant.rtps_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    sub_property_policy.properties().emplace_back("rtps.endpoint.payload_protection_kind", "ENCRYPT");

    reader.setOnDiscoveryFunction([&writer](const ParticipantDiscoveryInfo& info) -> bool
            {
                if (info.info.m_guid == writer.participant_guid())
                {
                    std::cout << "Received USER_DATA from the writer: ";
                    for (auto i : info.info.m_userData)
                    {
                        std::cout << i << ' ';
                    }
                    return info.info.m_userData == std::vector<octet>({ 'a', 'b', 'c', 'd', 'e' });
                }

                return false;
            });

    reader.history_depth(100).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_part_property_policy).
            entity_property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

    reader.wait_discovery();
    writer.wait_discovery();

    reader.wait_discovery_result();
}

TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_governance_rule_order)
{
    {
        // Governance rule for topic *HelloWorldTopic* with enable_read/write_access_contrl set to false
        // Governance rule for topic * with enable_read/write_access_contrl set to true
        // Permission denied for topic HelloWorldTopic
        // Creation of reader and writer is allowed
        PubSubReader<HelloWorldType> reader("HelloWorldTopic");
        PubSubWriter<HelloWorldType> writer("HelloWorldTopic");
        std::string governance_file("governance_rule_order_test.smime");

        PropertyPolicy pub_property_policy, sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions.smime"));

        reader.history_depth(10).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
                property_policy(sub_property_policy).init();

        ASSERT_TRUE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions.smime"));

        writer.history_depth(10).
                property_policy(pub_property_policy).init();

        ASSERT_TRUE(writer.isInitialized());

        // Wait for authorization
        reader.waitAuthorized();
        writer.waitAuthorized();

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
        reader.block_for_all();
    }

    {
        // Governance rule for topic * with enable_read/write_access_contrl set to true
        // Governance rule for topic *HelloWorldTopic* with enable_read/write_access_contrl set to false
        // Permission denied for topic HelloWorldTopic
        // Creation of reader and writer is denied
        PubSubReader<HelloWorldType> reader("HelloWorldTopic");
        PubSubWriter<HelloWorldType> writer("HelloWorldTopic");
        std::string governance_file("governance_rule_order_test_inverse.smime");

        PropertyPolicy pub_property_policy, sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions.smime"));

        reader.property_policy(sub_property_policy).init();

        ASSERT_FALSE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions.smime"));

        writer.property_policy(pub_property_policy).init();

        ASSERT_FALSE(writer.isInitialized());
    }
}

TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_multiple_endpoints_matching)
{
    {
        std::string governance_file("governance_helloworld_all_enable.smime");
        std::string permissions_file("permissions_helloworld.smime");

        PropertyPolicy pub_property_policy;
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/" + permissions_file));

        PubSubParticipant<HelloWorldType> publishers(3u, 0u, 9u, 0u);
        publishers.property_policy(pub_property_policy)
                .pub_topic_name("HelloWorldTopic");
        ASSERT_TRUE(publishers.init_participant());

        // Initializing two publishers in the same participant
        ASSERT_TRUE(publishers.init_publisher(0u));
        ASSERT_TRUE(publishers.init_publisher(1u));

        PropertyPolicy sub_property_policy;
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/" + permissions_file));

        PubSubParticipant<HelloWorldType> subscribers(0u, 3u, 0u, 9u);
        subscribers.property_policy(sub_property_policy)
                .sub_topic_name("HelloWorldTopic");
        ASSERT_TRUE(subscribers.init_participant());

        // Initializing two subscribers in the same participant
        ASSERT_TRUE(subscribers.init_subscriber(0u));
        ASSERT_TRUE(subscribers.init_subscriber(1u));

        // Wait for discovery: 2 subs x 2 pubs
        publishers.pub_wait_discovery(4u);
        subscribers.sub_wait_discovery(4u);

        // Initializing one late joiner in the participants
        ASSERT_TRUE(subscribers.init_subscriber(2u));
        ASSERT_TRUE(publishers.init_publisher(2u));

        // Wait for discovery: 3 subs x 3 pubs
        publishers.pub_wait_discovery();
        subscribers.sub_wait_discovery();
    }
}

// Regression test of Refs #5346, Github #441.
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_fail_on_topic_wildcards)
{
    {
        // Wildcards are only considered on PERMISSIONS, Topic values should be treated as plain strings
        PubSubReader<HelloWorldType> reader("*");
        PubSubWriter<HelloWorldType> writer("*");
        std::string governance_file("governance_helloworld_all_enable.smime");

        PropertyPolicy pub_property_policy, sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld.smime"));

        reader.setManualTopicName("*").
                property_policy(sub_property_policy).init();
        ASSERT_FALSE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld.smime"));

        writer.setManualTopicName("*").
                property_policy(pub_property_policy).init();

        ASSERT_FALSE(writer.isInitialized());
    }

    {
        // Wildcards are only considered on PERMISSIONS, Topic values should be treated as plain strings
        PubSubReader<HelloWorldType> reader("HelloWorldTopic");
        PubSubWriter<HelloWorldType> writer("HelloWorldTopic");
        std::string governance_file("governance_helloworld_all_enable.smime");

        PropertyPolicy pub_property_policy, sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld.smime"));

        reader.history_depth(10).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
                property_policy(sub_property_policy).init();

        ASSERT_TRUE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld.smime"));

        writer.history_depth(10).
                property_policy(pub_property_policy).init();

        ASSERT_TRUE(writer.isInitialized());

        // Wait for authorization
        reader.waitAuthorized();
        writer.waitAuthorized();

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
        reader.block_for_all();
    }
}

// Regression test of Refs #5346, Github #441.
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_fail_on_partition_wildcards)
{
    {
        // Wildcards are only considered on PERMISSIONS, partition values should be treated as plain strings
        PubSubReader<HelloWorldType> reader("HelloWorldTopic");
        PubSubWriter<HelloWorldType> writer("HelloWorldTopic");
        std::string governance_file("governance_helloworld_all_enable.smime");

        PropertyPolicy pub_property_policy, sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld_partitions.smime"));

        reader.partition("*").
                property_policy(sub_property_policy).init();
        ASSERT_FALSE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld_partitions.smime"));

        writer.partition("*").
                property_policy(pub_property_policy).init();

        ASSERT_FALSE(writer.isInitialized());
    }

    {
        // Wildcards are only considered on PERMISSIONS, partition values should be treated as plain strings
        PubSubReader<HelloWorldType> reader("HelloWorldTopic");
        PubSubWriter<HelloWorldType> writer("HelloWorldTopic");
        std::string governance_file("governance_helloworld_all_enable.smime");

        PropertyPolicy pub_property_policy, sub_property_policy;

        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainsubcert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainsubkey.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        sub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld_partitions.smime"));

        reader.history_depth(10).
                reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
                property_policy(sub_property_policy).
                partition("Partition1").init();

        ASSERT_TRUE(reader.isInitialized());

        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
                "builtin.PKI-DH"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
                "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                "file://" + std::string(certs_path) + "/mainpubcert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
                "file://" + std::string(certs_path) + "/mainpubkey.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
                "builtin.AES-GCM-GMAC"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
                "builtin.Access-Permissions"));
        pub_property_policy.properties().emplace_back(Property(
                    "dds.sec.access.builtin.Access-Permissions.permissions_ca",
                    "file://" + std::string(certs_path) + "/maincacert.pem"));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
                "file://" + std::string(certs_path) + "/" + governance_file));
        pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
                "file://" + std::string(certs_path) + "/permissions_helloworld_partitions.smime"));

        writer.history_depth(10).
                property_policy(pub_property_policy).
                partition("Partition*").init();

        ASSERT_TRUE(writer.isInitialized());

        // Wait for authorization
        reader.waitAuthorized();
        writer.waitAuthorized();

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
        reader.block_for_all();
    }
}

static void BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(
        PubSubReader<HelloWorldType>& reader,
        PubSubWriter<HelloWorldType>& writer,
        const std::string& governance_file)
{
    PropertyPolicy pub_property_policy, sub_property_policy;

    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainsubcert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainsubkey.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
            "builtin.Access-Permissions"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
            "file://" + std::string(certs_path) + "/" + governance_file));
    sub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
            "file://" + std::string(certs_path) + "/permissions.smime"));

    reader.history_depth(10).
            reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
            property_policy(sub_property_policy).init();

    ASSERT_TRUE(reader.isInitialized());

    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.plugin",
            "builtin.PKI-DH"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://" + std::string(certs_path) + "/mainpubcert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://" + std::string(certs_path) + "/mainpubkey.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.access.plugin",
            "builtin.Access-Permissions"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions_ca",
            "file://" + std::string(certs_path) + "/maincacert.pem"));
    pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.governance",
            "file://" + std::string(certs_path) + "/" + governance_file));
    pub_property_policy.properties().emplace_back(Property("dds.sec.access.builtin.Access-Permissions.permissions",
            "file://" + std::string(certs_path) + "/permissions.smime"));

    writer.history_depth(10).
            property_policy(pub_property_policy).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for authorization
    reader.waitAuthorized();
    writer.waitAuthorized();

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
    reader.block_for_all();
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}


// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryDisableAccessNone_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}


// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsDisableDiscoveryEnableAccessNone_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_disable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessEncrypt_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryDisableAccessNone_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_disable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessEncrypt_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_encrypt.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_enable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_disable_discovery_enable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_disable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_enable_discovery_disable_access_encrypt)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_enable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_disable_discovery_enable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_disable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

// *INDENT-OFF*
TEST_P(Security, BuiltinAuthenticationAndAccessAndCryptoPlugin_PermissionsEnableDiscoveryEnableAccessNone_validation_ok_enable_discovery_disable_access_none)
// *INDENT-ON*
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    std::string governance_file("governance_enable_discovery_enable_access_none.smime");

    BuiltinAuthenticationAndAccessAndCryptoPlugin_Permissions_validation_ok_common(reader, writer, governance_file);
}

void blackbox_security_init()
{
    certs_path = std::getenv("CERTS_PATH");

    if (certs_path == nullptr)
    {
        std::cout << "Cannot get enviroment variable CERTS_PATH" << std::endl;
        exit(-1);
    }
}

#ifdef INSTANTIATE_TEST_SUITE_P
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_SUITE_P(x, y, z, w)
#else
#define GTEST_INSTANTIATE_TEST_MACRO(x, y, z, w) INSTANTIATE_TEST_CASE_P(x, y, z, w)
#endif // ifdef INSTANTIATE_TEST_SUITE_P

GTEST_INSTANTIATE_TEST_MACRO(Security,
        Security,
        testing::Values(false, true),
        [](const testing::TestParamInfo<Security::ParamType>& info)
        {
            if (info.param)
            {
                return "Intraprocess";
            }
            return "NonIntraprocess";
        });


#endif // if HAVE_SECURITY
