// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file RTPSMessageGroup.h
 *
 */

#ifndef _FASTDDS_RTPS_RTPSMESSAGEGROUP_H_
#define _FASTDDS_RTPS_RTPSMESSAGEGROUP_H_

#include <gmock/gmock.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class RTPSParticipantImpl;
class Endpoint;
class RTPSMessageSenderInterface;

class RTPSMessageGroup
{
public:

    RTPSMessageGroup(
            RTPSParticipantImpl*)
    {
    }

    RTPSMessageGroup(
            RTPSParticipantImpl*,
            Endpoint*,
            const RTPSMessageSenderInterface*)
    {
    }

    MOCK_METHOD0(flush_and_reset, void());

    MOCK_METHOD0(get_current_bytes_processed, uint32_t());

    MOCK_METHOD0(reset_current_bytes_processed, void());

    void sender(
            Endpoint*,
            const RTPSMessageSenderInterface*) const
    {
    }

    void set_sent_bytes_limitation(
            uint32_t) const
    {
    }

};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // _FASTDDS_RTPS_RTPSMESSAGEGROUP_H_
