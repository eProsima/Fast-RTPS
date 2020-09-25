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

/**
 * @file DiscoveryDataQueueInfo.cpp
 *
 */

#include "./DiscoveryDataQueueInfo.hpp"

namespace eprosima {
namespace fastdds {
namespace rtps {
namespace ddb {

eprosima::fastrtps::rtps::CacheChange_t* DiscoveryDataQueueInfo::change()
{
    return change_;
}

eprosima::fastrtps::string_255 DiscoveryDataQueueInfo::topic()
{
    return topic_;
}

eprosima::fastrtps::rtps::GUID_t DiscoveryDataQueueInfo::associated_entity()
{
    return associated_entity_;
}




} /* namespace ddb */
} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */
