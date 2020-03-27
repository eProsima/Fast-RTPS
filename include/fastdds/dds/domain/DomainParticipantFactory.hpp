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
 * @file DomainParticipantFactory.hpp
 *
 */

#ifndef _FASTDDS_DOMAINPARTICIPANT_HPP_
#define _FASTDDS_DOMAINPARTICIPANT_HPP_

#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/types/TypesBase.h>

#include <mutex>
#include <map>

using eprosima::fastrtps::types::ReturnCode_t;

namespace eprosima {
namespace fastdds {
namespace dds {

class DomainParticipantListener;
class DomainParticipant;
class DomainParticipantImpl;

/**
 * Class DomainParticipantFactory
 *  @ingroup FASTDDS_MODULE
 */
class DomainParticipantFactory
{

public:

    /**
     * Returns the DomainParticipantFactory singleton.
     * @return The DomainParticipantFactory singleton.
     */
    RTPS_DllAPI static DomainParticipantFactory* get_instance();

    /**
     * Create a Participant from a profile name.
     * @param participant_profile Participant profile name.
     * @param listen ParticipantListener Pointer.
     * @return Participant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipant* create_participant(
            const std::string& participant_profile,
            DomainParticipantListener* listen = nullptr);

    /**
     * Create a Participant.
     * @param att Participant Attributes.
     * @param listen ParticipantListener Pointer.
     * @return Participant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipant* create_participant(
            const fastrtps::ParticipantAttributes& att,
            DomainParticipantListener* listen = nullptr);


    /**
     * Create a Participant.
     * @param att Participant Attributes.
     * @param enabled True if the participant should be enabled on creation.
     * @param listen ParticipantListener Pointer.
     * @return Participant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipant* create_participant(
            const fastrtps::ParticipantAttributes& att,
            bool enabled,
            DomainParticipantListener* listen = nullptr);

/**
     * This operation retrieves a previously created DomainParticipant belonging to specified domain_id.
     * If no such DomainParticipant exists, the operation will return 'nullptr'.
     * If multiple DomainParticipant entities belonging to that domain_id exist,
     * then the operation will return one of them. It is not specified which one.
     * @param domain_id
     * @return previously created DomainParticipant
     */
    RTPS_DllAPI DomainParticipant* lookup_participant(
            uint8_t domain_id) const;

    /**
     * Returns all participants that belongs to the specified domain_id.
     * @param domain_id
     * @return
     */
    RTPS_DllAPI std::vector<DomainParticipant*> lookup_participants(
        uint8_t domain_id) const;

    //!Fills participant_attributes with the default values.
    RTPS_DllAPI ReturnCode_t get_default_participant_qos(
            fastrtps::ParticipantAttributes& participant_qos) const;

    /* TODO
    RTPS_DllAPI ReturnCode_t set_default_participant_qos(
            const fastrtps::ParticipantAttributes& participant_qos);
    */

    /**
     * Remove a Participant and all associated publishers and subscribers.
     * @param part Pointer to the participant.
     * @return True if correctly removed.
     */
    RTPS_DllAPI ReturnCode_t delete_participant(
            DomainParticipant* part);

    /**
     * Load profiles from XML file.
     * @param xml_profile_file XML profile file.
     * @return True if correctly loaded.
     */
    RTPS_DllAPI bool load_XML_profiles_file(
            const std::string& xml_profile_file);

    // TODO set/get DomainParticipantFactoryQos

private:

    friend class DomainParticipantFactoryReleaser;

    std::map<uint8_t, std::vector<DomainParticipantImpl*>> participants_;

    DomainParticipantFactory();

    virtual ~DomainParticipantFactory();

    static bool delete_instance();

    mutable std::mutex mtx_participants_;

    mutable bool default_xml_profiles_loaded;


};

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */

#endif /* _FASTDDS_DOMAINPARTICIPANT_HPP_*/
