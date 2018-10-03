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

/*!
 * @file ParticipantSecurityAttributes.h
 */
#ifndef __RTPS_SECURITY_ACCESSCONTROL_PARTICIPANTSECURITYATTRIBUTES_H__
#define __RTPS_SECURITY_ACCESSCONTROL_PARTICIPANTSECURITYATTRIBUTES_H__

namespace eprosima {
namespace fastrtps {
namespace rtps {
namespace security {

typedef uint32_t ParticipantSecurityAttributesMask;

#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED       (0x00000001UL << 0)
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED  (0x00000001UL << 1)
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED (0x00000001UL << 2)
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID                (0x00000001UL << 31)

struct ParticipantSecurityAttributes
{
    ParticipantSecurityAttributes() : 
        allow_unauthenticated_participants(false), is_access_protected(true), 
        is_rtps_protected(false), is_discovery_protected(false), is_liveliness_protected(false) 
    {}

    explicit ParticipantSecurityAttributes(const ParticipantSecurityAttributesMask mask) :
        allow_unauthenticated_participants(false), is_access_protected(true),
        is_rtps_protected((mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED) != 0),
        is_discovery_protected((mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED) != 0),
        is_liveliness_protected((mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED) != 0)
    {}

    bool allow_unauthenticated_participants;

    bool is_access_protected;

    bool is_rtps_protected;

    bool is_discovery_protected;

    bool is_liveliness_protected;

    ParticipantSecurityAttributesMask mask() const
    {
        ParticipantSecurityAttributesMask rv = PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
        if (is_rtps_protected) rv |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED;
        if (is_discovery_protected) rv |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
        if (is_liveliness_protected) rv |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;
        return rv;
    }
};

}
}
}
}

#endif // __RTPS_SECURITY_ACCESSCONTROL_PARTICIPANTSECURITYATTRIBUTES_H__
