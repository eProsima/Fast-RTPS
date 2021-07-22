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
 * @file DirectMessageSender.hpp
 *
 */

#ifndef FASTRTPS_RTPS_BUILTIN_DISCOVERY_PARTICIPANT_DIRECTMESSAGESENDER_HPP_
#define FASTRTPS_RTPS_BUILTIN_DISCOVERY_PARTICIPANT_DIRECTMESSAGESENDER_HPP_

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/messages/RTPSMessageSenderInterface.hpp>
#include <fastdds/rtps/common/Locator.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class RTPSParticipantImpl;

/**
 * A proxy to a \ref RTPSMessageSenderInterface to send a message avoiding multicast.
 */
class DirectMessageSender : public RTPSMessageSenderInterface
{
public:

    DirectMessageSender(
            RTPSParticipantImpl* participant,
            std::vector<GUID_t>* guids,
            LocatorList_t* locators);

    virtual ~DirectMessageSender() override = default;

    /**
     * Check if the destinations managed by this sender interface have changed.
     *
     * @return true if destinations have changed, false otherwise.
     */
    virtual bool destinations_have_changed() const override;

    /**
     * Get a GUID prefix representing all destinations.
     *
     * @return When all the destinations share the same prefix (i.e. belong to the same participant)
     * that prefix is returned. When there are no destinations, or they belong to different
     * participants, c_GuidPrefix_Unknown is returned.
     */
    virtual GuidPrefix_t destination_guid_prefix() const override;

    /**
     * Get the GUID prefix of all the destination participants.
     *
     * @return a const reference to a vector with the GUID prefix of all destination participants.
     */
    virtual const std::vector<GuidPrefix_t>& remote_participants() const override;

    /**
     * Get the GUID of all destinations.
     *
     * @return a const reference to a vector with the GUID of all destinations.
     */
    virtual const std::vector<GUID_t>& remote_guids() const override;

    /**
     * Send a message through this interface.
     *
     * @param message Pointer to the buffer with the message already serialized.
     * @param max_blocking_time_point Future timepoint where blocking send should end.
     */
    virtual bool send(
            CDRMessage_t* message,
            std::chrono::steady_clock::time_point max_blocking_time_point) const override;

private:

    RTPSParticipantImpl* participant_;
    std::vector<GUID_t>* guids_;
    std::vector<GuidPrefix_t> participant_guids_;
    LocatorList_t* locators_;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#endif /* FASTRTPS_RTPS_BUILTIN_DISCOVERY_PARTICIPANT_DIRECTMESSAGESENDER_HPP_ */
