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

/**
 * @file RTPSMessageSenderInterface.hpp
 *
 */

#ifndef _FASTDDS_RTPS_MESSAGES_RTPSMESSAGESENDERINTERFACE_HPP_
#define _FASTDDS_RTPS_MESSAGES_RTPSMESSAGESENDERINTERFACE_HPP_

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/messages/CDRMessage.h>
#include <fastdds/rtps/common/Guid.h>

#include <vector>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
 * An interface used in \ref RTPSMessageGroup to handle destinations management
 * and message sending.
 */
class RTPSMessageSenderInterface
{
public:

    virtual ~RTPSMessageSenderInterface() = default;

    /**
     * Check if the destinations managed by this sender interface have changed.
     *
     * @return true if destinations have changed, false otherwise.
     */
    virtual bool destinations_have_changed() const = 0;

    /**
     * Get a GUID prefix representing all destinations.
     *
     * @return When all the destinations share the same prefix (i.e. belong to the same participant)
     * that prefix is returned. When there are no destinations, or they belong to different
     * participants, c_GuidPrefix_Unknown is returned.
     */
    virtual GuidPrefix_t destination_guid_prefix() const = 0;

    /**
     * Get the GUID prefix of all the destination participants.
     *
     * @return a const reference to a vector with the GUID prefix of all destination participants.
     */
    virtual const std::vector<GuidPrefix_t>& remote_participants() const = 0;

    /**
     * Get the GUID of all destinations.
     *
     * @return a const reference to a vector with the GUID of all destinations.
     */
    virtual const std::vector<GUID_t>& remote_guids() const = 0;

    /**
     * Send a message through this interface.
     *
     * @param message Pointer to the buffer with the message already serialized.
     * @param max_blocking_time_point Future timepoint where blocking send should end.
     */
    virtual bool send(
            CDRMessage_t* message,
            std::chrono::steady_clock::time_point max_blocking_time_point) const = 0;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#endif /* _FASTDDS_RTPS_MESSAGES_RTPSMESSAGESENDERINTERFACE_HPP_ */
