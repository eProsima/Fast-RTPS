// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/rtps/common/Locator.h>
#include <fastrtps/rtps/common/RemoteLocators.hpp>
#include <fastrtps/utils/collections/ResourceLimitedVector.hpp>
#include <fastrtps/rtps/common/LocatorListComparisons.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace eprosima::fastrtps::rtps;

// Checks whether the address of two locators are equal byte to byte
static bool address_match(
        const Locator_t& loc1,
        const Locator_t& loc2)
{
    for (int i = 0; i < 16; i++)
    {
        if (loc1.address[i] != loc2.address[i])
        {
            return false;
        }
    }
    return true;
}

// Reset locator address to 0
void reset_locator_address(
        Locator_t& locator)
{
    for (size_t i = 0; i < 16; ++i)
    {
        locator.address[i] = 0;
    }
}

class IPLocatorTests : public ::testing::Test
{

public:

    // different std ips to use in tests
    const std::string ipv4_address = "74.65.86.73";
    const std::string ipv4_address_repeated = "74.65.86.073";
    const std::string ipv4_address_2 = "106.97.118.105";
    const std::string ipv4_lo_address = "127.0.0.1";

    const std::string ipv6_address = "4a41:5649::50:4152:4953";
    const std::string ipv6_address_repeated = "4a41:5649:0000:0000:0000:0050:4152:4953";
    const std::string ipv6_address_2 = "4a41:5649::150:4152:4953";
    const std::string ipv6_address_full = "4a41:5649:85a3:a8d3:1319:0050:4152:4953";
    const std::string ipv6_lo_address = "::1";

    const std::string ipv4_multicast_address = "224.0.0.0";
    const std::string ipv4_multicast_address_2 = "239.255.255.255";

    const std::string ipv6_multicast_address = "FF00::";
    const std::string ipv6_multicast_address_2 = "FFFF::";
    const std::string ipv6_multicast_address_3 = "FF0A::1:2:3";

    const std::string ipv4_any = "0.0.0.0";
    const std::string ipv4_invalid = "0.0.0.0";
    const std::string ipv6_any = "::";
    const std::string ipv6_invalid = "0:0:0:0:0:0:0:0";

    const uint32_t port1 = 6666;
    const uint32_t port2 = 7400;

    const uint16_t rtps_port1 = 6666;
    const uint16_t rtps_port2 = 7400;
};

/*******************
* IPLocator Tests *
*******************/

/*
 * Check to create an IPv4 by string
 * All the tests of ipv4 depends on the correct functionality of this function
 */
TEST_F(IPLocatorTests, setIPv4_from_string)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv4;

    {
        // Empty string
        ASSERT_TRUE(IPLocator::setIPv4(locator, "0.0.0.0"));
        std::vector<unsigned int> vec{0, 0, 0, 0};
        for (int i = 0; i < 12; i++)
        {
            ASSERT_EQ(locator.address[i], 0u);
        }
        for (int i = 0; i < 4; i++)
        {
            ASSERT_EQ(locator.address[i + 12], vec[i]);
        }
    }

    {
        // Std random string
        ASSERT_TRUE(IPLocator::setIPv4(locator, "1.2.3.4"));
        std::vector<unsigned int> vec{1, 2, 3, 4};

        for (int i = 0; i < 12; i++)
        {
            ASSERT_EQ(locator.address[i], 0u);
        }
        for (int i = 0; i < 4; i++)
        {
            ASSERT_EQ(locator.address[i + 12], vec[i]);
        }
    }

    {
        // Std string with 0 forwarding number
        ASSERT_TRUE(IPLocator::setIPv4(locator, "01.002.0003.000104"));
        std::vector<unsigned int> vec{1, 2, 3, 104};

        for (int i = 0; i < 12; i++)
        {
            ASSERT_EQ(locator.address[i], 0u);
        }
        for (int i = 0; i < 4; i++)
        {
            ASSERT_EQ(locator.address[i + 12], vec[i]);
        }
    }

    {
        // Multicast address
        ASSERT_TRUE(IPLocator::setIPv4(locator, "255.255.255.255"));
        std::vector<unsigned int> vec{255, 255, 255, 255};

        for (int i = 0; i < 12; i++)
        {
            ASSERT_EQ(locator.address[i], 0u);
        }
        for (int i = 0; i < 4; i++)
        {
            ASSERT_EQ(locator.address[i + 12], vec[i]);
        }
    }

    {
        // Error cases
        ASSERT_FALSE(IPLocator::setIPv4(locator, "1.1.1.256")); // Too high number
        ASSERT_FALSE(IPLocator::setIPv4(locator, "1.1.1"));     // Too few args
        ASSERT_FALSE(IPLocator::setIPv4(locator, "1.1.1.1.1")); // Too much args

        // Change to IPv6
        locator.kind = LOCATOR_KIND_UDPv6;
        ASSERT_FALSE(IPLocator::setIPv4(locator, "255.255.255.255"));
    }
}

/*
 * Check to create an empty IPv6 by string
 * All the tests of ipv6 depends on the correct functionality setIPv6
 */
TEST_F(IPLocatorTests, setIPv6_from_string_empty)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;

    // Empty string
    std::vector<unsigned int> vec(16, 0);
    std::vector<std::string> str_vec
    {
        "::",
        "::0",
        "0::0",
        "0:0:0:0::0:0",
        "0000::0000",
        "0000:0000:0000:0000:0000:0000:0000:0000"
    };

    for (std::string s : str_vec)
    {
        ASSERT_TRUE(IPLocator::setIPv6(locator, s));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }
    }
}

/*
 * Check to create random IPv6 by string
 * All the tests of ipv6 depends on the correct functionality setIPv6
 */
TEST_F(IPLocatorTests, setIPv6_from_string_std)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;

    // Localhost address
    {
        // Localhost IPv6 = 0:0:0:0:0:0:0:1
        std::vector<unsigned int> vec(15, 0);
        vec.push_back(1);

        ASSERT_TRUE(IPLocator::setIPv6(locator, "::1"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        ASSERT_TRUE(IPLocator::setIPv6(locator, "0:0::0:0001"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }
    }

    // Std random address
    {
        std::vector<unsigned int> vec(16, 0);
        vec[2] = 2;
        vec[3] = 3;
        vec[6] = 6;
        vec[7] = 7;
        vec[12] = 12;
        vec[13] = 13;

        // Full hexadecimal address
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0000:0203:0000:0607:0000:0000:0c0d:0000"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Removing forwarding zeros
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0:203:0:607:0:0:c0d:0"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Removing some forwarding zeros
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0000:203:0:0607:000:0:c0d:00"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Initial zero block
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "::203:0:607:0:0:c0d:0"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Final zero block
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0:203:0:607:0:0:c0d::"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Middle zero block
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0:203::607:0:0:c0d:0"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Another middle zero block
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0:203:0:607::c0d:0"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }

        // Uppercase
        ASSERT_TRUE(IPLocator::setIPv6(
                    locator,
                    "0:203:0:607::C0D:0"));
        for (int i = 0; i < 16; i++)
        {
            ASSERT_EQ(locator.address[i], vec[i]);
        }
    }
}

/*
 * Check to create invalid IPv6 by string
 * All the tests of ipv6 depends on the correct functionality setIPv6
 */
TEST_F(IPLocatorTests, setIPv6_from_string_invalid)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;

    ASSERT_FALSE(IPLocator::setIPv6(locator, ":"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "::1:"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, ":1::"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "::1::"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1::1::1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:::1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:1:1:1:1:1:1:1:1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:1:1:1:1:1:1::1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "::::"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, ":1:"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, ":1:0:0:0:0:0:0"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:0:0:0:0:0:1:"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "10000::"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "::10000"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:10000::1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:10000:0:0:0:0:0:1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1::0:10000:0:1"));
    ASSERT_FALSE(IPLocator::setIPv6(locator, "1:10::10:1,"));

    // Change to IPv4
    locator.kind = LOCATOR_KIND_UDPv4;
    ASSERT_FALSE(IPLocator::setIPv4(locator, "::1"));
}


/*
 * Check isIPv4 method by correct and incorrect IPv4
 */
TEST_F(IPLocatorTests, isIPv4)
{
    // Not valid strings for IPv4
    std::vector<std::string> correct_ipv4
    {
        "127.0.0.1",
        "0.0.0.0",
        "255.255.255.255",
        "192.168.1.254"
    };

    for (std::string ipv4 : correct_ipv4)
    {
        bool isIPv4_result = IPLocator::isIPv4(ipv4);
        if (!isIPv4_result)
        {
            std::cout << "Error in isIPv4 for case: " << ipv4 << std::endl;
        }
        ASSERT_TRUE(isIPv4_result);
    }

    // Not valid strings for IPv4
    std::vector<std::string> incorrect_ipv4
    {
        "::",
        "::1",
        "192.168.1",
        "192.168.1.256",
        "www",
        "www.eprosima.com",
        "localhost"
    };

    for (std::string ipv4 : incorrect_ipv4)
    {
        bool isIPv4_result = IPLocator::isIPv4(ipv4);
        if (isIPv4_result)
        {
            std::cout << "Error in isIPv4 for case: " << ipv4 << std::endl;
        }
        ASSERT_FALSE(isIPv4_result);
    }
}

/*
 * Check isIPv6 method by correct and incorrect IPv6
 */
TEST_F(IPLocatorTests, isIPv6)
{
    // Not valid strings for IPv6
    std::vector<std::string> correct_ipv6
    {
        "::1",
        "0:0::0:0001",
        "0000:0203:0000:0607:0000:0000:0c0d:0000",
        "0:203:0:607:0:0:c0d:0",
        "0000:203:0:0607:000:0:c0d:00",
        "::203:0:607:0:0:c0d:0",
        "0:203:0:607:0:0:c0d::",
        "0:203::607:0:0:c0d:0",
        "0:203:0:607::c0d:0",
        "0:203:0:607::C0D:0",
        "::",
        "::0",
        "0::0",
        "0:0:0:0::0:0",
        "0000::0000",
        "0000:0000:0000:0000:0000:0000:0000:0000",
    };

    for (std::string ipv6 : correct_ipv6)
    {
        bool isIPv6_result = IPLocator::isIPv6(ipv6);
        if (!isIPv6_result)
        {
            std::cout << "Error in isIPv6 for case: " << ipv6 << std::endl;
        }
        ASSERT_TRUE(isIPv6_result);
    }

    // Not valid strings for IPv6
    std::vector<std::string> incorrect_ipv6
    {
        ":",
        "::1:",
        ":1::",
        "::1::",
        "1::1::1",
        "1:::1",
        "1:1:1:1:1:1:1:1:1",
        "1:1:1:1:1:1:1::1",
        "::::",
        ":1:",
        ":1:0:0:0:0:0:0",
        "1:0:0:0:0:0:1:",
        "0000000::",
        "10000::",
        "::10000",
        "1:10000::1",
        "1:10000:0:0:0:0:0:1",
        "1::0:10000:0:1",
        "ABZ::",
        "::ABZ",
        "1:ABZ::1",
        "1:ABZ:0:0:0:0:0:1",
        "1::0:ABZ:0:1",
        "1:10::10:1,",
        "www",
        "www.eprosima.com",
        "localhost"
    };

    for (std::string ipv6 : incorrect_ipv6)
    {
        bool isIPv6_result = IPLocator::isIPv6(ipv6);
        if (isIPv6_result)
        {
            std::cout << "Error in isIPv6 for case: " << ipv6 << std::endl;
        }
        EXPECT_FALSE(isIPv6_result);
    }
}

/*
 * Check to create locator of all kinds
 */
TEST_F(IPLocatorTests, createLocator)
{
    int32_t kind;
    Locator_t res_locator;
    Locator_t probe_locator;

    // create UDP IPv4
    kind = LOCATOR_KIND_UDPv4;
    probe_locator.kind = kind;
    IPLocator::createLocator(kind, ipv4_address, port1, res_locator);
    ASSERT_EQ(res_locator.kind, kind);
    ASSERT_EQ(res_locator.port, port1);
    IPLocator::setIPv4(probe_locator, ipv4_address_repeated);
    ASSERT_TRUE(address_match(probe_locator, res_locator));

    // create TCP IPv4
    kind = LOCATOR_KIND_TCPv4;
    probe_locator.kind = kind;
    IPLocator::createLocator(kind, ipv4_lo_address, port2, res_locator);
    ASSERT_EQ(res_locator.kind, kind);
    ASSERT_EQ(res_locator.port, port2);
    IPLocator::setIPv4(probe_locator, ipv4_lo_address);
    ASSERT_TRUE(address_match(probe_locator, res_locator));

    // create UDP IPv6
    kind = LOCATOR_KIND_UDPv6;
    probe_locator.kind = kind;
    IPLocator::createLocator(kind, ipv6_address, port1, res_locator);
    ASSERT_EQ(res_locator.kind, kind);
    ASSERT_EQ(res_locator.port, port1);
    IPLocator::setIPv6(probe_locator, ipv6_address_repeated);
    ASSERT_TRUE(address_match(probe_locator, res_locator));

    // create TCP IPv6
    kind = LOCATOR_KIND_TCPv6;
    probe_locator.kind = kind;
    IPLocator::createLocator(kind, ipv6_any, port2, res_locator);
    ASSERT_EQ(res_locator.kind, kind);
    ASSERT_EQ(res_locator.port, port2);
    IPLocator::setIPv6(probe_locator, ipv6_invalid);
    ASSERT_TRUE(address_match(probe_locator, res_locator));

    // create SHM
    kind = LOCATOR_KIND_SHM;
    probe_locator.kind = kind;
    IPLocator::createLocator(kind, ipv6_address, port1, res_locator);
    ASSERT_EQ(res_locator.kind, kind);
    ASSERT_EQ(res_locator.port, port1);
    IPLocator::setIPv6(probe_locator, ipv6_invalid);
    ASSERT_TRUE(address_match(probe_locator, res_locator));
}

/*
 * Check to set IPv4 by
 *  byte array
 *  octets
 *  other locator
 */
TEST_F(IPLocatorTests, setIPv4)
{
    Locator_t locator;
    Locator_t probe_locator;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_lo_address, port1, locator);

    // setIPv4 char*
    {
        IPLocator::setIPv4(probe_locator, ipv4_lo_address);
        unsigned char arr[4]{127, 0, 0, 1};
        ASSERT_TRUE(IPLocator::setIPv4(locator, arr));
        ASSERT_TRUE(address_match(probe_locator, locator));

        // try set IPv6
        locator.kind = LOCATOR_KIND_UDPv6;
        ASSERT_FALSE(IPLocator::setIPv4(locator, arr));
        locator.kind = LOCATOR_KIND_UDPv4;
    }

    // setIPv4 4xoctet
    {
        IPLocator::setIPv4(probe_locator, ipv4_address_2);
        ASSERT_TRUE(IPLocator::setIPv4(locator, 106, 97, 118, 105));
        ASSERT_TRUE(address_match(probe_locator, locator));

        // try set IPv6
        locator.kind = LOCATOR_KIND_UDPv6;
        ASSERT_FALSE(IPLocator::setIPv4(locator, 106, 97, 118, 105));
        locator.kind = LOCATOR_KIND_UDPv4;
    }

    // setIPv4 locator
    {
        Locator_t ipv4_locator;
        IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address_2, port1, ipv4_locator);
        ASSERT_TRUE(IPLocator::setIPv4(locator, ipv4_locator));
        ASSERT_TRUE(address_match(ipv4_locator, locator));

        // try set IPv6
        locator.kind = LOCATOR_KIND_UDPv6;
        ASSERT_FALSE(IPLocator::setIPv4(locator, ipv4_locator));
    }
}

/*
 * Check to set IPv6 by
 *  byte array
 *  octets
 *  other locator
 */
TEST_F(IPLocatorTests, setIPv6)
{
    Locator_t locator;
    Locator_t probe_locator;
    probe_locator.kind = LOCATOR_KIND_UDPv6;
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_lo_address, port1, locator);

    // setIPv6 char*
    {
        IPLocator::setIPv6(probe_locator, ipv6_lo_address);
        unsigned char arr[16]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        ASSERT_TRUE(IPLocator::setIPv6(locator, arr));
        ASSERT_TRUE(address_match(probe_locator, locator));

        // try set IPv4
        locator.kind = LOCATOR_KIND_UDPv4;
        ASSERT_FALSE(IPLocator::setIPv6(locator, arr));
        locator.kind = LOCATOR_KIND_UDPv6;
    }

    // setIPv6 4xoctet
    {
        IPLocator::setIPv6(probe_locator, ipv6_address);
        ASSERT_TRUE(IPLocator::setIPv6(locator, 0x4a41, 0x5649, 0x0000, 0x0000, 0x0000, 0x0050, 0x4152, 0x4953));
        ASSERT_TRUE(address_match(probe_locator, locator));

        // try set IPv4
        locator.kind = LOCATOR_KIND_UDPv4;
        ASSERT_FALSE(IPLocator::setIPv6(locator, 0x4a41, 0x5649, 0x0000, 0x0000, 0x0000, 0x0050, 0x4152, 0x4953));
        locator.kind = LOCATOR_KIND_UDPv6;
    }

    // setIPv6 locator
    {
        Locator_t ipv6_locator;
        IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_address, port1, ipv6_locator);
        ASSERT_TRUE(IPLocator::setIPv6(locator, ipv6_locator));
        ASSERT_TRUE(address_match(ipv6_locator, locator));

        // try set IPv4
        locator.kind = LOCATOR_KIND_UDPv4;
        ASSERT_FALSE(IPLocator::setIPv6(locator, ipv6_locator));
    }
}

/*
 * Check to get IPv4 by
 */
TEST_F(IPLocatorTests, getIPv4)
{
    Locator_t locator;
    IPLocator::setIPv4(locator, ipv4_lo_address);
    auto res = IPLocator::getIPv4(locator);
    ASSERT_EQ(res[0], 127u);
    ASSERT_EQ(res[1], 0u);
    ASSERT_EQ(res[2], 0u);
    ASSERT_EQ(res[3], 1u);
}

/*
 * Check to get IPv6 by
 */
TEST_F(IPLocatorTests, getIPv6)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;
    IPLocator::setIPv6(locator, ipv6_lo_address);
    auto res = IPLocator::getIPv6(locator);
    for (int i = 0; i < 15; i++)
    {
        ASSERT_EQ(res[i], 0u);
    }
    ASSERT_EQ(res[15], 1u);
}

/*
 * Check if locator has a valid IP for every kind
 */
TEST_F(IPLocatorTests, hasIP)
{
    Locator_t locator;

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator);
    ASSERT_TRUE(IPLocator::hasIPv4(locator));
    ASSERT_FALSE(IPLocator::hasIPv6(locator));

    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_lo_address, port1, locator);
    ASSERT_TRUE(IPLocator::hasIPv4(locator));
    ASSERT_FALSE(IPLocator::hasIPv6(locator));

    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_invalid, port1, locator);
    ASSERT_FALSE(IPLocator::hasIPv4(locator));
    ASSERT_FALSE(IPLocator::hasIPv6(locator));

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address, port1, locator);
    ASSERT_TRUE(IPLocator::hasIPv6(locator));
    ASSERT_FALSE(IPLocator::hasIPv4(locator));

    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_lo_address, port1, locator);
    ASSERT_TRUE(IPLocator::hasIPv6(locator));
    ASSERT_FALSE(IPLocator::hasIPv4(locator));

    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_invalid, port1, locator);
    ASSERT_FALSE(IPLocator::hasIPv6(locator));
    ASSERT_FALSE(IPLocator::hasIPv4(locator));

    IPLocator::createLocator(LOCATOR_PORT_INVALID, ipv4_address, port1, locator);
    ASSERT_FALSE(IPLocator::hasIPv4(locator));
    ASSERT_FALSE(IPLocator::hasIPv6(locator));

    IPLocator::createLocator(LOCATOR_KIND_SHM, ipv6_address, port1, locator);
    ASSERT_FALSE(IPLocator::hasIPv4(locator));
    ASSERT_FALSE(IPLocator::hasIPv6(locator));
}

/*
 * Check string conversion from an IPv4 address
 */
TEST_F(IPLocatorTests, toIPv4string)
{
    Locator_t locator;
    IPLocator::setIPv4(locator, 127, 0, 0, 1);
    ASSERT_EQ(IPLocator::toIPv4string(locator), ipv4_lo_address);
    IPLocator::setIPv4(locator, ipv4_address.c_str());
    ASSERT_EQ(IPLocator::toIPv4string(locator), ipv4_address);
}

/*
 * Check string conversion from an IPv6 address
 */
TEST_F(IPLocatorTests, toIPv6string)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;
    IPLocator::setIPv6(locator, 0, 0, 0, 0, 0, 0, 0, 1);
    ASSERT_EQ(IPLocator::toIPv6string(locator), ipv6_lo_address);
    IPLocator::setIPv6(locator, ipv6_address_repeated.c_str());
    ASSERT_EQ(IPLocator::toIPv6string(locator), ipv6_address);

    IPLocator::setIPv6(locator, "11::");
    ASSERT_EQ(IPLocator::toIPv6string(locator), "11::");
    IPLocator::setIPv6(locator, "::11");
    ASSERT_EQ(IPLocator::toIPv6string(locator), "::11");
    IPLocator::setIPv6(locator, "11::01");
    ASSERT_EQ(IPLocator::toIPv6string(locator), "11::1");
    IPLocator::setIPv6(locator, "0::01");
    ASSERT_EQ(IPLocator::toIPv6string(locator), "::1");
    IPLocator::setIPv6(locator, "0::0:0");
    ASSERT_EQ(IPLocator::toIPv6string(locator), "::");
}

/*
 * Check to copy a IPv4
 */
TEST_F(IPLocatorTests, copyIPv4)
{
    Locator_t locator;
    IPLocator::setIPv4(locator, ipv4_lo_address);
    unsigned char arr[4];
    ASSERT_TRUE(IPLocator::copyIPv4(locator, arr));
    ASSERT_EQ(arr[0], 127u);
    ASSERT_EQ(arr[1], 0u);
    ASSERT_EQ(arr[2], 0u);
    ASSERT_EQ(arr[3], 1u);
}

/*
 * Check to copy a IPv6
 */
TEST_F(IPLocatorTests, copyIPv6)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;
    IPLocator::setIPv6(locator, ipv6_lo_address);
    unsigned char arr[16];
    ASSERT_TRUE(IPLocator::copyIPv6(locator, arr));
    for (int i = 0; i < 15; i++)
    {
        ASSERT_EQ(arr[i], 0u);
    }
    ASSERT_EQ(arr[15], 1u);
}

/*
 * Check to set ip of any kind
 */
TEST_F(IPLocatorTests, ip)
{
    Locator_t locator;

    // UDPv4
    locator.kind = LOCATOR_KIND_UDPv4;
    ASSERT_TRUE(IPLocator::ip(locator, ipv4_address_repeated));
    ASSERT_EQ(IPLocator::toIPv4string(locator), ipv4_address);

    // TCPv4
    locator.kind = LOCATOR_KIND_TCPv4;
    ASSERT_TRUE(IPLocator::ip(locator, ipv4_lo_address));
    ASSERT_EQ(IPLocator::toIPv4string(locator), ipv4_lo_address);

    // UDPv6
    locator.kind = LOCATOR_KIND_UDPv6;
    ASSERT_TRUE(IPLocator::ip(locator, ipv6_address_repeated));
    ASSERT_EQ(IPLocator::toIPv6string(locator), ipv6_address);

    // TCPv6
    locator.kind = LOCATOR_KIND_TCPv6;
    ASSERT_TRUE(IPLocator::ip(locator, ipv6_lo_address));
    ASSERT_EQ(IPLocator::toIPv6string(locator), ipv6_lo_address);

    // SHM
    locator.kind = LOCATOR_KIND_SHM;
    ASSERT_FALSE(IPLocator::ip(locator, ipv6_lo_address));
}

/*
 * Check address string for any kind
 */
TEST_F(IPLocatorTests, ip_to_string)
{
    Locator_t locator;

    {
        // v4
        locator.kind = LOCATOR_KIND_UDPv4;
        IPLocator::setIPv4(locator, 127, 0, 0, 1);
        ASSERT_EQ(IPLocator::ip_to_string(locator), ipv4_lo_address);

        locator.kind = LOCATOR_KIND_TCPv4;
        IPLocator::setIPv4(locator, ipv4_address_repeated);
        ASSERT_EQ(IPLocator::ip_to_string(locator), ipv4_address);
    }

    {
        // v6
        locator.kind = LOCATOR_KIND_UDPv6;
        IPLocator::setIPv6(locator, 0, 0, 0, 0, 0, 0, 0, 1);
        ASSERT_EQ(IPLocator::ip_to_string(locator), ipv6_lo_address);

        locator.kind = LOCATOR_KIND_TCPv6;
        IPLocator::setIPv6(locator, ipv6_address_repeated);
        ASSERT_EQ(IPLocator::ip_to_string(locator), ipv6_address);

        // RFRIEDM tests
        // Zero Compression of 2001:DB8:0:0:8:800:200C:417A
        reset_locator_address(locator);
        locator.address[0] = 0x20;
        locator.address[1] = 0x01;
        locator.address[2] = 0x0d;
        locator.address[3] = 0xb8;
        locator.address[9] = 0x08;
        locator.address[10] = 0x08;
        locator.address[12] = 0x20;
        locator.address[13] = 0x0c;
        locator.address[14] = 0x41;
        locator.address[15] = 0x7a;
        ASSERT_EQ("2001:db8::8:800:200c:417a", IPLocator::ip_to_string(locator));

        // Zero Compression of FF01:0:0:0:0:0:0:101
        reset_locator_address(locator);
        locator.address[0] = 0xff;
        locator.address[1] = 0x01;
        locator.address[14] = 0x01;
        locator.address[15] = 0x01;
        ASSERT_EQ("ff01::101", IPLocator::ip_to_string(locator));

        // Zero Compression of 0:0:0:0:0:0:0:1
        reset_locator_address(locator);
        locator.address[15] = 0x01;
        ASSERT_EQ("::1", IPLocator::ip_to_string(locator));

        // Zero Compression of 0:0:0:0:0:0:0:0
        reset_locator_address(locator);
        locator.address[15] = 0x00;
        ASSERT_EQ("::", IPLocator::ip_to_string(locator));

        // Trailing Zeros
        reset_locator_address(locator);
        locator.address[14] = 0x10;
        ASSERT_EQ("::1000", IPLocator::ip_to_string(locator));
        locator.address[15] = 0x20;
        ASSERT_EQ("::1020", IPLocator::ip_to_string(locator));

        // Embedded Zeros in 2001:DB8:a::
        reset_locator_address(locator);
        locator.address[0] = 0x20;
        locator.address[1] = 0x01;
        locator.address[2] = 0x0d;
        locator.address[3] = 0xb8;
        locator.address[5] = 0xa;
        ASSERT_EQ("2001:db8:a::", IPLocator::ip_to_string(locator));

        locator.address[15] = 0x01;
        ASSERT_EQ("2001:db8:a::1", IPLocator::ip_to_string(locator));
        locator.address[15] = 0x10;
        ASSERT_EQ("2001:db8:a::10", IPLocator::ip_to_string(locator));
        locator.address[15] = 0;
        locator.address[14] = 0x01;
        ASSERT_EQ("2001:db8:a::100", IPLocator::ip_to_string(locator));
        locator.address[14] = 0x10;
        ASSERT_EQ("2001:db8:a::1000", IPLocator::ip_to_string(locator));
        locator.address[14] = 0;

        locator.address[13] = 0x01;
        ASSERT_EQ("2001:db8:a::1:0", IPLocator::ip_to_string(locator));
        locator.address[13] = 0x10;
        ASSERT_EQ("2001:db8:a::10:0", IPLocator::ip_to_string(locator));
        locator.address[13] = 0;


        // Do not compress a single block of zeros
        locator.address[9] = 0x01;
        locator.address[11] = 0x01;
        locator.address[13] = 0x01;
        ASSERT_EQ("2001:db8:a:0:1:1:1:0", IPLocator::ip_to_string(locator));
        locator.address[9] = 0;
        locator.address[11] = 0;
        locator.address[13] = 0;

        // 2001:db8:a:0:0:1:0:0 special case for two equal compressible blocks
        // When this occurs, it's recommended to collapse the left
        locator.address[11] = 0x01;
        ASSERT_EQ("2001:db8:a::1:0:0", IPLocator::ip_to_string(locator));
        locator.address[11] = 0;

        // Compress larger block
        locator.address[9] = 0x01;
        ASSERT_EQ("2001:db8:a:0:1::", IPLocator::ip_to_string(locator));
        locator.address[9] = 0;

        locator.address[7] = 0x01;
        ASSERT_EQ("2001:db8:a:1::", IPLocator::ip_to_string(locator));
        locator.address[7] = 0;

        // Compress the larger block v.2
        // IP 0:0:0:a:0:0:0:0 = 0:0:0:a::
        reset_locator_address(locator);
        locator.address[7] = 0x0a;
        ASSERT_EQ("0:0:0:a::", IPLocator::ip_to_string(locator));
        locator.address[7] = 0;

        // IP 0:0:0:0:a:0:0:0 = ::a:0:0:0
        locator.address[9] = 0x0a;
        ASSERT_EQ("::a:0:0:0", IPLocator::ip_to_string(locator));
        locator.address[9] = 0;

        // IP 0:0:0:a:b:0:0:0 = ::a:b:0:0:0
        locator.address[7] = 0x0a;
        locator.address[9] = 0x0b;
        ASSERT_EQ("::a:b:0:0:0", IPLocator::ip_to_string(locator));

        // IP 100:0:0:a:b:0:0:0 = 100:0:0::a:b::
        locator.address[0] = 0x01;
        ASSERT_EQ("100:0:0:a:b::", IPLocator::ip_to_string(locator));
    }

    {
        // shm
        locator.kind = LOCATOR_KIND_SHM;
        ASSERT_EQ(IPLocator::ip_to_string(locator), "");
    }
}

/*
 * Check to set and get logical port
 */
TEST_F(IPLocatorTests, logicalPort)
{
    Locator_t locator;
    ASSERT_TRUE(IPLocator::setLogicalPort(locator, rtps_port1));
    ASSERT_EQ(IPLocator::getLogicalPort(locator), rtps_port1);
    ASSERT_NE(IPLocator::getLogicalPort(locator), rtps_port2);
    ASSERT_TRUE(IPLocator::setLogicalPort(locator, rtps_port2));
    ASSERT_EQ(IPLocator::getLogicalPort(locator), rtps_port2);
}

/*
 * Check to set and get physical port
 */
TEST_F(IPLocatorTests, physicalPort)
{
    Locator_t locator;
    ASSERT_TRUE(IPLocator::setPhysicalPort(locator, rtps_port1));
    ASSERT_EQ(IPLocator::getPhysicalPort(locator), rtps_port1);
    ASSERT_NE(IPLocator::getPhysicalPort(locator), rtps_port2);
    ASSERT_TRUE(IPLocator::setPhysicalPort(locator, rtps_port2));
    ASSERT_EQ(IPLocator::getPhysicalPort(locator), rtps_port2);
}

/*
 * Check to set and get wan
 */
TEST_F(IPLocatorTests, wan)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator);
    ASSERT_TRUE(IPLocator::setWan(locator, "0.1.2.3"));
    for (unsigned int i = 0; i < 4; i++)
    {
        ASSERT_EQ(locator.address[8 + i], i);
    }

    ASSERT_TRUE(IPLocator::setWan(locator, 3u, 2u, 1u, 0u));
    for (unsigned int i = 0; i < 4; i++)
    {
        ASSERT_EQ(locator.address[8 + i], 3 - i);
    }

    for (unsigned int i = 0; i < 4; i++)
    {
        ASSERT_EQ(IPLocator::getWan(locator)[i], 3 - i);
    }
}

/*
 * Check if wan is set
 */
TEST_F(IPLocatorTests, hasWan)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator);
    IPLocator::setWan(locator, "1.1.1.1");
    ASSERT_TRUE(IPLocator::hasWan(locator));
    IPLocator::setWan(locator, "0.1.1.1");
    ASSERT_TRUE(IPLocator::hasWan(locator));
    IPLocator::setWan(locator, "0.0.1.1");
    ASSERT_TRUE(IPLocator::hasWan(locator));
    IPLocator::setWan(locator, "0.0.0.1");
    ASSERT_TRUE(IPLocator::hasWan(locator));
    IPLocator::setWan(locator, "0.0.0.0");
    ASSERT_FALSE(IPLocator::hasWan(locator));
}

/*
 * Check Wan string conversion
 */
TEST_F(IPLocatorTests, toWanstring)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator);
    IPLocator::setWan(locator, "0.1.2.3");
    ASSERT_EQ(IPLocator::toWanstring(locator), "0.1.2.3");
}

/*
 * Check to set and get lan
 */
TEST_F(IPLocatorTests, lanID)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator);
    ASSERT_TRUE(IPLocator::setLanID(locator, "0.1.2.3.4.5.6.7"));
    for (int i = 0; i < 8; i++)
    {
        ASSERT_EQ(locator.address[i], i);
    }
    locator.kind = LOCATOR_KIND_UDPv4;
    ASSERT_FALSE(IPLocator::setLanID(locator, "0.1.2.3.4.5.6.7"));

    for (int i = 0; i < 8; i++)
    {
        ASSERT_EQ(IPLocator::getLanID(locator)[i], i);
    }
}

/*
 * Check LAN to string
 */
TEST_F(IPLocatorTests, toLanIDstring)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator);
    IPLocator::setLanID(locator, "0.1.2.3.4.5.6.7");
    ASSERT_EQ(IPLocator::toLanIDstring(locator), "0.1.2.3.4.5.6.7");

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator);
    IPLocator::setWan(locator, ipv4_lo_address);
    ASSERT_EQ(IPLocator::toLanIDstring(locator), "");
}

/*
 * Check conversion to Physical Locator
 */
TEST_F(IPLocatorTests, toPhysicalLocator)
{
    Locator_t locator;
    ASSERT_TRUE(IPLocator::setLogicalPort(locator, rtps_port1));
    ASSERT_EQ(IPLocator::getLogicalPort(locator), rtps_port1);
    locator = IPLocator::toPhysicalLocator(locator);
    ASSERT_NE(IPLocator::getLogicalPort(locator), rtps_port1);
}

/*
 * Check ip is equal wan
 */
TEST_F(IPLocatorTests, ip_equals_wan)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator);
    IPLocator::setWan(locator, ipv4_address);
    ASSERT_TRUE(IPLocator::ip_equals_wan(locator));

    IPLocator::setWan(locator, ipv4_lo_address);
    ASSERT_FALSE(IPLocator::ip_equals_wan(locator));
}

/*
 * Check to set and get RTPS port
 */
TEST_F(IPLocatorTests, setPortRTPS)
{
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator);
    ASSERT_TRUE(IPLocator::setPortRTPS(locator, rtps_port2));
    ASSERT_EQ(IPLocator::getPortRTPS(locator), static_cast<uint16_t>(rtps_port2));

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address, port2, locator);
    IPLocator::setPortRTPS(locator, rtps_port1);
    ASSERT_EQ(IPLocator::getPortRTPS(locator), static_cast<uint16_t>(rtps_port1));

    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address_2, port1, locator);
    ASSERT_TRUE(IPLocator::setPortRTPS(locator, rtps_port2));
    ASSERT_EQ(IPLocator::getPortRTPS(locator), static_cast<uint16_t>(rtps_port2));

    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_address_2, port2, locator);
    ASSERT_TRUE(IPLocator::setPortRTPS(locator, rtps_port1));
    ASSERT_EQ(IPLocator::getPortRTPS(locator), static_cast<uint16_t>(rtps_port1));

    IPLocator::createLocator(LOCATOR_KIND_SHM, ipv4_lo_address, port1, locator);
    ASSERT_FALSE(IPLocator::setPortRTPS(locator, rtps_port1));
    ASSERT_EQ(IPLocator::getPortRTPS(locator), 0u);
}

/*
 * Check whether an address is localhost
 */
TEST_F(IPLocatorTests, isLocal)
{
    // ipv4 UDP
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv4;
    IPLocator::setIPv4(locator, ipv4_address);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv4(locator, ipv4_any);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv4(locator, ipv4_lo_address);
    ASSERT_TRUE(IPLocator::isLocal(locator));

    // ipv4 TCP
    locator.kind = LOCATOR_KIND_TCPv4;
    IPLocator::setIPv4(locator, ipv4_address);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv4(locator, ipv4_any);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv4(locator, ipv4_lo_address);
    ASSERT_TRUE(IPLocator::isLocal(locator));

    // ipv6 UDP
    locator.kind = LOCATOR_KIND_UDPv6;
    IPLocator::setIPv6(locator, ipv6_address);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv6(locator, ipv6_any);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv6(locator, ipv6_lo_address);
    ASSERT_TRUE(IPLocator::isLocal(locator));

    // ipv6 TCP
    locator.kind = LOCATOR_KIND_TCPv6;
    IPLocator::setIPv6(locator, ipv6_address);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv6(locator, ipv6_any);
    ASSERT_FALSE(IPLocator::isLocal(locator));
    IPLocator::setIPv6(locator, ipv6_lo_address);
    ASSERT_TRUE(IPLocator::isLocal(locator));
}

/*
 * Check whether an address is any
 */
TEST_F(IPLocatorTests, isAny)
{
    // IPv4
    Locator_t locator;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator);
    ASSERT_FALSE(IPLocator::isAny(locator));
    IPLocator::setIPv4(locator, ipv4_any);
    ASSERT_TRUE(IPLocator::isAny(locator));

    // IPv6
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address, port1, locator);
    ASSERT_FALSE(IPLocator::isAny(locator));
    IPLocator::setIPv6(locator, ipv6_any);
    ASSERT_TRUE(IPLocator::isAny(locator));
}

/*
 * Check address comparasion
 */
TEST_F(IPLocatorTests, compareAddress)
{
    Locator_t locator1, locator2;

    // UDP v4
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator1);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address_repeated, port1, locator2);
    ASSERT_TRUE(IPLocator::compareAddress(locator1, locator2));
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address_2, port1, locator2);
    ASSERT_FALSE(IPLocator::compareAddress(locator1, locator2));

    // TCP v4
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator1);
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address_repeated, port1, locator2);
    ASSERT_TRUE(IPLocator::compareAddress(locator1, locator2));
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address_2, port1, locator2);
    ASSERT_FALSE(IPLocator::compareAddress(locator1, locator2));

    // UDP v6
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address, port1, locator1);
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address_repeated, port1, locator2);
    ASSERT_TRUE(IPLocator::compareAddress(locator1, locator2));
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address_2, port1, locator2);
    ASSERT_FALSE(IPLocator::compareAddress(locator1, locator2));

    // TCP v6
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_address, port1, locator1);
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_address_repeated, port1, locator2);
    ASSERT_TRUE(IPLocator::compareAddress(locator1, locator2));
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_address_2, port1, locator2);
    ASSERT_FALSE(IPLocator::compareAddress(locator1, locator2));
}

/*
 * Check address and port comparasion
 */
TEST_F(IPLocatorTests, compareAddressAndPhysicalPort)
{
    Locator_t locator1, locator2;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator1);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, ipv4_address, port1, locator2);
    ASSERT_TRUE(IPLocator::compareAddressAndPhysicalPort(locator1, locator2));
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, ipv4_address, port1, locator2);
    ASSERT_FALSE(IPLocator::compareAddressAndPhysicalPort(locator1, locator2));

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address, port1, locator1);
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, ipv6_address, port1, locator2);
    ASSERT_TRUE(IPLocator::compareAddressAndPhysicalPort(locator1, locator2));
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, ipv6_address, port1, locator2);
    ASSERT_FALSE(IPLocator::compareAddressAndPhysicalPort(locator1, locator2));
}

/*
 * Check locator serialization
 */
TEST_F(IPLocatorTests, to_string)
{
    Locator_t locator;

    // Invalid
    IPLocator::createLocator(LOCATOR_KIND_INVALID, "1", 10u, locator);
    ASSERT_EQ(IPLocator::to_string(locator), "Invalid_locator:[_]:0");

    // UDPv4
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "0.1.0.1", 1u, locator);
    ASSERT_EQ(IPLocator::to_string(locator), "UDPv4:[0.1.0.1]:1");

    // TCPv4
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "0.0.1.1", 2u, locator);
    ASSERT_EQ(IPLocator::to_string(locator), "TCPv4:[0.0.1.1]:2");

    // UDPv6
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "200::", 3u, locator);
    ASSERT_EQ(IPLocator::to_string(locator), "UDPv6:[200::]:3");

    // TCPv6
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::2", 4u, locator);
    ASSERT_EQ(IPLocator::to_string(locator), "TCPv6:[::2]:4");

    // SHM
    IPLocator::createLocator(LOCATOR_KIND_SHM, "", 5u, locator);
    ASSERT_EQ(IPLocator::to_string(locator), "SHM:[_]:5");

    // SHM M
    IPLocator::createLocator(LOCATOR_KIND_SHM, "", 6u, locator);
    locator.address[0] = 'M';
    ASSERT_EQ(IPLocator::to_string(locator), "SHM:[M]:6");
}

/*
 * Check whether an address is multicast
 */
TEST_F(IPLocatorTests, isMulticast)
{
    Locator_t locator;

    // UDP v4
    locator.kind = LOCATOR_KIND_UDPv4;
    IPLocator::setIPv4(locator, ipv4_address);
    ASSERT_FALSE(IPLocator::isMulticast(locator));
    IPLocator::setIPv4(locator, ipv4_multicast_address);
    ASSERT_TRUE(IPLocator::isMulticast(locator));
    IPLocator::setIPv4(locator, ipv4_multicast_address_2);
    ASSERT_TRUE(IPLocator::isMulticast(locator));

    // UDP v6
    locator.kind = LOCATOR_KIND_UDPv6;
    IPLocator::setIPv6(locator, ipv6_address);
    ASSERT_FALSE(IPLocator::isMulticast(locator));
    IPLocator::setIPv6(locator, ipv6_multicast_address);
    ASSERT_TRUE(IPLocator::isMulticast(locator));
    IPLocator::setIPv6(locator, ipv6_multicast_address_2);
    ASSERT_TRUE(IPLocator::isMulticast(locator));
    IPLocator::setIPv6(locator, ipv6_multicast_address_3);
    ASSERT_TRUE(IPLocator::isMulticast(locator));

    // TCP
    locator.kind = LOCATOR_KIND_TCPv4;
    ASSERT_FALSE(IPLocator::isMulticast(locator));
}

/*
 * Check to create a full IPv4 address by string
 */
TEST_F(IPLocatorTests, setIPv4address)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_TCPv4;

    ASSERT_TRUE(IPLocator::setIPv4address(locator, "1.2.3.4.5.6.7.8", "9.10.11.12", "13.14.15.16"));
    for (unsigned int i = 0; i < 16; i++)
    {
        ASSERT_EQ(locator.address[i], i + 1);
    }

    ASSERT_FALSE(IPLocator::setIPv4address(locator, "1.2.3.4.5.6.7", "9.10.11.12", "13.14.15.16"));
    ASSERT_FALSE(IPLocator::setIPv4address(locator, "1.2.3.4.5.6.7.8", "9.10.11", "13.14.15.16"));
    ASSERT_FALSE(IPLocator::setIPv4address(locator, "1.2.3.4.5.6.7.8", "9.10.11.12", "13.14.15"));

    locator.kind = LOCATOR_KIND_TCPv6;
    ASSERT_FALSE(IPLocator::setIPv4address(locator, "1.2.3.4.5.6.7.8", "9.10.11.12", "13.14.15.16"));
}

/*******************
* Locator_t Tests *
*******************/

/*
 * Check creation of a locator with port constructor
 */
TEST(LocatorTests, locator_port_constructor)
{
    Locator_t locator(314u);
    ASSERT_EQ(locator.port, 314u);
    ASSERT_EQ(locator.kind, LOCATOR_KIND_UDPv4);
    ASSERT_FALSE(IPLocator::hasIPv4(locator));
}

/*
 * Check set_addres Locator_t function
 */
TEST(LocatorTests, locator_set_address)
{
    Locator_t locator;
    Locator_t locator_copy;
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::2", 3, locator_copy);

    ASSERT_TRUE(locator.set_address(locator_copy));

    ASSERT_EQ(locator.address[0], 0u);
    ASSERT_EQ(locator.address[10], 0u);
    ASSERT_EQ(locator.address[15], 2u);
}

/*
 * Check Locator_t operators
 */
TEST(LocatorTests, locator_minor)
{
    Locator_t locator1;
    Locator_t locator2;
    Locator_t locator3;
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::2", 3, locator1);
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::3", 3, locator2);
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::3", 3, locator3);

    ASSERT_TRUE(locator1 < locator2);
    ASSERT_FALSE(locator2 < locator1);
    ASSERT_FALSE(locator1 < locator1);

    ASSERT_TRUE(locator1 != locator2);
    ASSERT_TRUE(locator2 != locator1);
    ASSERT_FALSE(locator2 != locator2);
    ASSERT_FALSE(locator2 != locator3);
}

/*
 * Check creation of a locator from string
 * Serialization is tested in IPLocatorTests::to_string
 */
TEST(LocatorTests, locator_deserialization)
{
    Locator_t locator, locator_res;
    std::stringstream ss;
    // std::stringstream::str construct stringstream from string
    // std::stringstream::clear clear stringstream flags to reuse it

    // UDPv4
    ss.str("UDPv4:[0.1.0.1]:1");
    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "0.1.0.1", 1, locator);
    ASSERT_EQ(locator, locator_res);

    // TCPv4
    ss.clear();
    ss.str("TCPv4:[0.0.1.1]:2");
    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "0.0.1.1", 2, locator);
    ASSERT_EQ(locator, locator_res);

    // UDPv6
    ss.clear();
    ss.str("UDPv6:[200::]:3");
    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "200::", 3, locator);
    ASSERT_EQ(locator, locator_res);

    // TCPv6
    ss.clear();
    ss.str("TCPv6:[::2]:4");
    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::2", 4, locator);
    ASSERT_EQ(locator, locator_res);

    // SHM
    ss.clear();
    ss.str("SHM:[_]:5");
    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_SHM, "", 5, locator);
    ASSERT_EQ(locator, locator_res);

    // Deserializate 2 locators
    ss.clear();
    ss.str("UDPv4:[0.1.0.1]:1,TCPv4:[0.0.1.1]:2");
    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "0.1.0.1", 1, locator);
    ASSERT_EQ(locator, locator_res);

    char coma;
    ss >> coma;

    ss >> locator_res;
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "0.0.1.1", 2, locator);
    ASSERT_EQ(locator, locator_res);
}

/*
 * Check IsAddressDefined function for IPv6
 */
TEST(LocatorTests, IsAddressDefined_v6)
{
    Locator_t locator;

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::1", 1, locator);
    ASSERT_TRUE(IsAddressDefined(locator));

    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::", 2, locator);
    ASSERT_FALSE(IsAddressDefined(locator));
}

/*
 * Check LocatorList copy constructor
 */
TEST(LocatorTests, LocatorList_copy_constructor)
{
    Locator_t locator;
    LocatorList_t locator_list_1;

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::1", 1, locator);
    locator_list_1.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::2", 2, locator);
    locator_list_1.push_back(locator);

    LocatorList_t locator_list_2(locator_list_1);
    ASSERT_EQ(locator_list_2.size(), 2u);
    ASSERT_EQ(locator_list_2.begin()->kind, LOCATOR_KIND_UDPv6);
    ASSERT_EQ((locator_list_2.begin() + 1)->kind, LOCATOR_KIND_TCPv6);
}

/*
 * Check == operator for LocatorLists
 */
TEST(LocatorTests, LocatorList_equal)
{
    Locator_t locator;
    LocatorList_t locator_list_1;
    LocatorList_t locator_list_2;

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::1", 1, locator);
    locator_list_1.push_back(locator);
    locator_list_2.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::1", 2, locator);
    locator_list_1.push_back(locator);
    ASSERT_FALSE(locator_list_1 == locator_list_2);

    locator_list_2.push_back(locator);
    ASSERT_TRUE(locator_list_1 == locator_list_2);

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::2", 1, locator);
    locator_list_1.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::2", 2, locator);
    locator_list_2.push_back(locator);
    ASSERT_FALSE(locator_list_1 == locator_list_2);

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::2", 1, locator);
    locator_list_2.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "::2", 2, locator);
    locator_list_1.push_back(locator);
    ASSERT_TRUE(locator_list_1 == locator_list_2);
}

/*
 * Check push_back for LocatorLists when a locator is already within the list
 */
TEST(LocatorTests, push_back_negative)
{
    Locator_t locator_1;
    Locator_t locator_2;
    LocatorList_t locator_list;

    ASSERT_EQ(locator_list.size(), 0u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::1", 1, locator_1);
    locator_list.push_back(locator_1);
    ASSERT_EQ(locator_list.size(), 1u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::1", 1, locator_2);
    locator_list.push_back(locator_2);
    ASSERT_EQ(locator_list.size(), 1u);
}

/*
 * Check isValid for LocatorList in negative case
 */
TEST(LocatorTests, isValid_negative)
{
    Locator_t locator;
    LocatorList_t locator_list;

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "::1", 1, locator);
    locator_list.push_back(locator);
    ASSERT_TRUE(locator_list.isValid());

    IPLocator::createLocator(LOCATOR_KIND_INVALID, "", 2, locator);
    locator_list.push_back(locator);
    ASSERT_FALSE(locator_list.isValid());
}

/*
 * Check LocatorList serialization
 */
TEST(LocatorTests, LocatorList_serialization)
{
    Locator_t locator;
    LocatorList_t locator_list;
    std::stringstream ss_empty;
    std::stringstream ss_filled;

    // Empty list
    ss_empty << locator_list;
    ASSERT_EQ(ss_empty.str(), "[_]");

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.1.1.1", 1, locator);
    locator_list.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "2.2.2.2", 2, locator);
    locator_list.push_back(locator);

    // Full list
    ss_filled << locator_list;
    ASSERT_EQ(ss_filled.str(), "[UDPv4:[1.1.1.1]:1,TCPv4:[2.2.2.2]:2]");
}

/*
 * Check LocatorList deserialization
 */
TEST(LocatorTests, LocatorList_deserialization)
{
    Locator_t locator;
    LocatorList_t locator_list;
    std::stringstream ss;

    ASSERT_EQ(locator_list.size(), 0u);

    // Empty list
    ss.str("[_]");
    ss >> locator_list;
    ASSERT_EQ(locator_list.size(), 0u);

    // Filled list
    ss.clear();
    ss.str("[UDPv4:[1.1.1.1]:1,TCPv4:[2.2.2.2]:2]");
    ss >> locator_list;
    ASSERT_EQ(locator_list.size(), 2u);

    // Error
    ss.clear();
    ss.str("[UDP_ERROR:]");
    ss >> locator_list;
    ASSERT_EQ(locator_list.size(), 0u);
}

/*******************************
* RemoteLocators Tests *
*******************************/

/*
 * Check RemoteLocators add unicast locator that already exists
 */
TEST(RemoteLocatorsTests, add_unicast_locator_repetead)
{
    eprosima::fastrtps::rtps::RemoteLocatorList rll;
    Locator_t locator_1;
    Locator_t locator_2;
    Locator_t locator_3;
    ASSERT_EQ(rll.unicast.size(), 0u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 1, locator_1);
    rll.add_unicast_locator(locator_1);
    ASSERT_EQ(rll.unicast.size(), 1u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "4.3.2.1", 1, locator_2);
    rll.add_unicast_locator(locator_2);
    ASSERT_EQ(rll.unicast.size(), 2u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 1, locator_3);
    rll.add_unicast_locator(locator_3);
    ASSERT_EQ(rll.unicast.size(), 2u);
}

/*
 * Check RemoteLocators add multicast locator that already exists
 */
TEST(RemoteLocatorsTests, add_multicast_locator_repetead)
{
    eprosima::fastrtps::rtps::RemoteLocatorList rll;
    Locator_t locator_1, locator_2, locator_3;
    ASSERT_EQ(rll.multicast.size(), 0u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 1, locator_1);
    rll.add_multicast_locator(locator_1);
    ASSERT_EQ(rll.multicast.size(), 1u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "4.3.2.1", 1, locator_2);
    rll.add_multicast_locator(locator_2);
    ASSERT_EQ(rll.multicast.size(), 2u);

    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 1, locator_3);
    rll.add_multicast_locator(locator_3);
    ASSERT_EQ(rll.multicast.size(), 2u);
}

/*
 * Check RemoteLocators >> operator
 */
TEST(RemoteLocatorsTests, RemoteLocator_serialization)
{
    eprosima::fastrtps::rtps::RemoteLocatorList rll;
    Locator_t locator;
    std::string serialized;
    std::stringstream serialized_ss;
    std::stringstream empty_serialized_ss;
    std::stringstream multicast_ss;
    std::stringstream unicast_ss;

    // Check empty List
    empty_serialized_ss << rll;
    ASSERT_EQ(empty_serialized_ss.str(), "{}");

    // Add multicast locators
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "224.0.0.0", 1, locator);
    rll.add_multicast_locator(locator);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "239.255.255.255", 2, locator);
    rll.add_multicast_locator(locator);

    // Check multicast List
    multicast_ss << rll;
    ASSERT_EQ(multicast_ss.str(), "{MULTICAST:[UDPv4:[224.0.0.0]:1,UDPv4:[239.255.255.255]:2]}");

    // Add unicast locators
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 3, locator);
    rll.add_unicast_locator(locator);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "04.03.02.01", 4, locator);
    rll.add_unicast_locator(locator);

    // Check List
    std::string str_result = // this variable is needed to separate string in 2 lines
            "{MULTICAST:[UDPv4:[224.0.0.0]:1,UDPv4:[239.255.255.255]:2]"
            "UNICAST:[UDPv4:[1.2.3.4]:3,UDPv4:[4.3.2.1]:4]}";
    serialized_ss << rll;
    ASSERT_EQ(serialized_ss.str(), str_result);

    // Check unicast List
    eprosima::fastrtps::rtps::RemoteLocatorList rll_2;
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 3, locator);
    rll_2.add_unicast_locator(locator);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "04.03.02.01", 4, locator);
    rll_2.add_unicast_locator(locator);
    unicast_ss << rll_2;
    ASSERT_EQ(unicast_ss.str(), "{UNICAST:[UDPv4:[1.2.3.4]:3,UDPv4:[4.3.2.1]:4]}");
}

/*
 * Check RemoteLocators >> operator
 */
TEST(RemoteLocatorsTests, RemoteLocator_deserialization)
{
    eprosima::fastrtps::rtps::RemoteLocatorList rll;
    Locator_t locator;
    std::string serialized;
    std::stringstream serialized_ss;

    // Check empty List
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "224.0.0.0", 1, locator);
    rll.add_multicast_locator(locator);
    rll.add_unicast_locator(locator);
    ASSERT_EQ(rll.multicast.size(), 1u);
    ASSERT_EQ(rll.unicast.size(), 1u);

    serialized_ss.str("{}");
    serialized_ss >> rll;
    ASSERT_EQ(rll.multicast.size(), 0u);
    ASSERT_EQ(rll.unicast.size(), 0u);

    // Check Filled list
    std::string str_result = // this variable is needed to not separate string in 2 lines
            "{MULTICAST:[UDPv4:[224.0.0.0]:1,UDPv4:[239.255.255.255]:2]"
            "UNICAST:[UDPv4:[1.2.3.4]:3,UDPv4:[4.3.2.1]:4]}";
    serialized_ss.clear();
    serialized_ss.str(str_result);
    serialized_ss >> rll;
    ASSERT_EQ(rll.multicast.size(), 2u);
    ASSERT_EQ(rll.unicast.size(), 2u);

    // Check error List
    serialized_ss.clear();
    serialized_ss.str("{MUL_ERROR:[UDPv4:[224.0.0.0]:1}"); // With an invalid locator it does not fail
    serialized_ss >> rll;
    ASSERT_EQ(rll.multicast.size(), 0u);
    ASSERT_EQ(rll.unicast.size(), 0u);
}

/*******************************
* LocatorListComparison Tests *
*******************************/

/*
 * Check LocatorLists comparison
 */
TEST(LocatorListComparisonTests, locatorList_comparison)
{
    Locator_t locator;
    eprosima::fastrtps::ResourceLimitedVector<Locator_t> locator_list_1;
    eprosima::fastrtps::ResourceLimitedVector<Locator_t> locator_list_2;

    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "1.2.3.4", 1, locator);
    locator_list_1.push_back(locator);
    locator_list_2.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 2, locator);
    locator_list_1.push_back(locator);
    locator_list_2.push_back(locator);

    ASSERT_TRUE(locator_list_1 == locator_list_2);

    IPLocator::createLocator(LOCATOR_KIND_TCPv6, "100::1", 3, locator);
    locator_list_1.push_back(locator);

    ASSERT_FALSE(locator_list_1 == locator_list_2);

    IPLocator::createLocator(LOCATOR_KIND_UDPv6, "100::1", 3, locator);
    locator_list_1.push_back(locator);

    ASSERT_FALSE(locator_list_1 == locator_list_2);

    locator_list_1.clear();
    locator_list_2.clear();
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "1.2.3.4", 1, locator);
    locator_list_1.push_back(locator);
    locator_list_2.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_UDPv4, "1.2.3.4", 2, locator);
    locator_list_1.push_back(locator);
    locator_list_2.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "0.0.0.1", 4, locator);
    locator_list_1.push_back(locator);
    IPLocator::createLocator(LOCATOR_KIND_TCPv4, "0.0.0.1", 5, locator);
    locator_list_2.push_back(locator);

    ASSERT_FALSE(locator_list_1 == locator_list_2);
}

/************
* DNS Tests *
************/

/*
 * Check DNS name resolve function
 *
 * Each DNS could resolve each domain with different IPs.
 * This test creates a set of the possible IPs for each of the domains tried.
 * In case one of the IPs is correct, the domains is set as found and the test past to it.
 */
TEST(LocatorDNSTests, resolve_name)
{
    std::map<std::string, std::pair<std::set<std::string>, std::set<std::string>>> addresses =
        {
            {"localhost", {{"127.0.0.1"}, {"::1"}}},
            {"www.eprosima.com", {{"154.56.134.194"}, {}}}, // Only IPv4
            {"www.google.com", {{"216.58.215.164"}, {"2a00:1450:400e:803::2004"}}},
            {"www.google.es", {{"142.250.184.3"}, {"2a00:1450:4003:808::2003"}}},
            {"www.github.com", {{"140.82.121.4", "140.82.121.3"}, {}}},
            {"docs.ros.org", {{}, {"2605:bc80:3010:104::8cd3:962"}}} // Only IPv6
        };

    for (auto const& address : addresses)
    {
        bool found_at_least_one = false;
        std::pair<std::set<std::string>, std::set<std::string>> dns_result;

        dns_result = IPLocator::resolveNameDNS(address.first);

        for (auto ipv4 : address.second.first)
        {
            if (dns_result.first.find(ipv4) != dns_result.first.end())
            {
                found_at_least_one = true;
                break;
            }
        }

        if (!found_at_least_one)
        {
            for (auto ipv6 : address.second.second)
            {
                if (dns_result.second.find(ipv6) != dns_result.second.end())
                {
                    found_at_least_one = true;
                    break;
                }
            }
        }

        // If it arrives here is that any correct ip has been found for one case
        if (!found_at_least_one)
        {
            std::cout << "IP not found for domain: " << address.first << std::endl;
        }
        EXPECT_TRUE(found_at_least_one);
    }
}

/*
    This test uses the DNS and IPs onf the `resolve_name` test
    to check that the locators are correctly formed
*/
TEST(LocatorDNSTests, dns_locator)
{
    std::map<std::string, std::string> dns_locator_to_address =
        {
            {"IPv4:[localhost]:1024", "127.0.0.1"},
            {"IPv6:[localhost]:1024", "::1"},
            {"IPv4:[www.eprosima.com]:1024", "154.56.134.194"},
            {"IPv6:[www.eprosima.com]:1024", ""},   //invalid locator
            {"IPv4:[www.eprosima.com]:1024", "154.56.134.194"},
            {"IPv6:[www.eprosima.com]:1024", ""},   // invalid locator
            {"IPv4:[www.google.com]:1024", "216.58.215.164"},
            {"IPv6:[www.google.com]:1024", "2a00:1450:400e:803::2004"},
            {"IPv4:[www.google.es]:1024", "142.250.184.3"},
            {"IPv6:[www.google.es]:1024", "2a00:1450:4003:808::2003"},
            {"IPv4:[docs.ros.org]:1024", ""},   // invalid locator
            {"IPv6:[docs.ros.org]:1024", "2605:bc80:3010:104::8cd3:962"}
        };

    for (auto const& dns : dns_locator_to_address)
    {
        std::stringstream ss_dns(dns.first);
        Locator_t locator;
        ss_dns >> locator;
        if (dns.second.empty())
        {
            EXPECT_EQ(LOCATOR_KIND_INVALID, locator.kind);
        }
        else
        {
            std::stringstream ss_locator;
            ss_locator << locator;
            EXPECT_EQ(dns.second, ss_locator.str());
        }
    }
}

int main(
        int argc,
        char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
