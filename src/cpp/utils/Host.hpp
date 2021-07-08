// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef UTILS_HOST_HPP_
#define UTILS_HOST_HPP_

#include <fastdds/rtps/common/Locator.h>

#include <fastrtps/utils/md5.h>
#include <fastrtps/utils/IPFinder.h>

#include <fastdds/dds/log/Log.hpp>

#include <asio/ip/host_name.hpp>

namespace eprosima {

/**
 * This singleton generates a host_id based on system interfaces
 * ip addresses or mac addresses
 */
class Host
{
public:

    static const size_t mac_id_length = 6;
    struct uint48
    {
        unsigned char value[mac_id_length];

        uint48()
        {
            memset(value, 0, mac_id_length);
        }

    };

    inline uint16_t id() const
    {
        return id_;
    }

    inline uint48 mac_id() const
    {
        return mac_id_;
    }

    static Host& instance()
    {
        static Host singleton;
        return singleton;
    }

private:

    Host()
    {
        asio::error_code ec;
        auto host_name = asio::ip::host_name(ec);

        // Compute the host id
        fastdds::rtps::LocatorList loc;
        fastrtps::rtps::IPFinder::getIP4Address(&loc);

        {
            MD5 md5;
            md5.update(host_name.c_str(), static_cast<MD5::size_type>(host_name.length()));
            for (auto& l : loc)
            {
                md5.update(l.address, sizeof(l.address));
            }
            md5.finalize();
            id_ = 0;
            for (size_t i = 0; i < sizeof(md5.digest); i += 2)
            {
                id_ ^= ((md5.digest[i] << 8) | md5.digest[i + 1]);
            }
        }

        // Compute the MAC id
        std::vector<fastrtps::rtps::IPFinder::info_MAC> macs;
        if (fastrtps::rtps::IPFinder::getAllMACAddress(&macs) &&
                macs.size() > 0)
        {
            MD5 md5;
            md5.update(host_name.c_str(), static_cast<MD5::size_type>(host_name.length()));
            for (auto& m : macs)
            {
                md5.update(m.address, sizeof(m.address));
            }
            md5.finalize();
            for (size_t i = 0, j = 0; i < sizeof(md5.digest); ++i, ++j)
            {
                if (j >= mac_id_length)
                {
                    j = 0;
                }
                mac_id_.value[j] ^= md5.digest[i];
            }
        }
        else
        {
            logWarning(UTILS, "Cannot get MAC addresses. Failing back to IP based ID");
            for (size_t i = 0; i < mac_id_length; i += 2)
            {
                mac_id_.value[i] = (id_ >> 8);
                mac_id_.value[i + 1] = (id_ & 0xFF);
            }
        }
    }

    uint16_t id_;
    uint48 mac_id_;
};

} // eprosima

#endif // UTILS_HOST_HPP_
