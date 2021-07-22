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
 * @file DirectMessageSender.cpp
 *
 */

#include <rtps/builtin/discovery/participant/DirectMessageSender.hpp>

#include <fastrtps/utils/IPLocator.h>
#include <fastdds/rtps/writer/RTPSWriter.h>
#include <rtps/participant/RTPSParticipantImpl.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {


DirectMessageSender::DirectMessageSender(
        RTPSParticipantImpl* participant,
        std::vector<GUID_t>* guids,
        LocatorList_t* locators)
    : participant_(participant)
    , guids_(guids)
    , locators_(locators)
{
    for (const GUID_t& guid : *guids)
    {
        if (std::find(participant_guids_.begin(), participant_guids_.end(), guid.guidPrefix) ==
                participant_guids_.end())
        {
            participant_guids_.push_back(guid.guidPrefix);
        }
    }
}

/**
 * Check if the destinations managed by this sender interface have changed.
 *
 * @return true if destinations have changed, false otherwise.
 */
bool DirectMessageSender::destinations_have_changed() const
{
    return false;
}

/**
 * Get a GUID prefix representing all destinations.
 *
 * @return When all the destinations share the same prefix (i.e. belong to the same participant)
 * that prefix is returned. When there are no destinations, or they belong to different
 * participants, c_GuidPrefix_Unknown is returned.
 */
GuidPrefix_t DirectMessageSender::destination_guid_prefix() const
{
    return participant_guids_.size() == 1 ? participant_guids_.at(0) : c_GuidPrefix_Unknown;
}

/**
 * Get the GUID prefix of all the destination participants.
 *
 * @return a const reference to a vector with the GUID prefix of all destination participants.
 */
const std::vector<GuidPrefix_t>& DirectMessageSender::remote_participants() const
{
    return participant_guids_;
}

/**
 * Get the GUID of all destinations.
 *
 * @return a const reference to a vector with the GUID of all destinations.
 */
const std::vector<GUID_t>& DirectMessageSender::remote_guids() const
{
    return *guids_;
}

/**
 * Send a message through this interface.
 *
 * @param message Pointer to the buffer with the message already serialized.
 * @param max_blocking_time_point Future timepoint where blocking send should end.
 */
bool DirectMessageSender::send(
        CDRMessage_t* message,
        std::chrono::steady_clock::time_point max_blocking_time_point) const
{
    return participant_->sendSync(message, participant_->getGuid(),
                   Locators(locators_->begin()), Locators(locators_->end()), max_blocking_time_point);
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
