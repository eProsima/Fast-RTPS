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
 * @file NetworkBuffer.hpp
 */

#ifndef _FASTDDS_RTPS_NETWORK_NETWORKBUFFER_HPP
#define _FASTDDS_RTPS_NETWORK_NETWORKBUFFER_HPP

#include <cstdint>  // uint32_t
#include <cstdlib>  // size_t

namespace eprosima {
namespace fastdds {
namespace rtps {

/**
 * A slice of data to be sent to one or more transports.
 * An RTPS datagram is made up of one or more NetworkBuffer instances.
 */
struct NetworkBuffer final
{
    //! Pointer to the buffer where the data is stored.
    const void* buffer;
    //! Number of bytes to use starting at @c buffer.
    uint32_t length;
};

}  // namespace rtps
}  // namespace fastdds
}  // namespace eprosima

#endif  // _FASTDDS_RTPS_NETWORK_NETWORKBUFFER_HPP
