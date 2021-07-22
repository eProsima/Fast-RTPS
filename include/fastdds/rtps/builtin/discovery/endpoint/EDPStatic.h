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
 * @file EDPStatic.h
 *
 */

#ifndef _FASTDDS_RTPS_EDPSTATIC_H_
#define _FASTDDS_RTPS_EDPSTATIC_H_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/builtin/discovery/endpoint/EDP.h>

namespace eprosima {
namespace fastrtps {
namespace xmlparser {
class XMLEndpointParser;
}
namespace rtps {


/**
 * Class EDPStaticProperty, used to read and write the strings from the properties used to transmit the EntityId_t.
 *@ingroup DISCOVERY_MODULE
 */
class EDPStaticProperty
{
public:

    EDPStaticProperty()
        : m_userId(0)
    {
    }

    ~EDPStaticProperty()
    {
    }

    //!Endpoint type
    std::string m_endpointType;
    //!Status
    std::string m_status;
    //!User ID as string
    std::string m_userIdStr;
    //!User ID
    uint16_t m_userId;
    //!Entity ID
    EntityId_t m_entityId;
    /**
     * Convert information to a property
     * @param type Type of endpoint
     * @param status Status of the endpoint
     * @param id User Id
     * @param ent EntityId
     * @return Pair of two strings.
     */
    static std::pair<std::string, std::string> toProperty(
            std::string type,
            std::string status,
            uint16_t id,
            const EntityId_t& ent);
    /**
     * @param in_property Input property-
     * @return True if correctly read
     */
    bool fromProperty(
            std::pair<std::string, std::string> in_property);
};

/**
 * Class EDPStatic, implements a static endpoint discovery module.
 * @ingroup DISCOVERYMODULE
 */
class EDPStatic : public EDP
{
public:

    /**
     * Constructor.
     * @param p Pointer to the PDPSimple.
     * @param part Pointer to the RTPSParticipantImpl.
     */
    EDPStatic(
            PDP* p,
            RTPSParticipantImpl* part);
    virtual ~EDPStatic();
    /**
     * Abstract method to initialize the EDP.
     * @param attributes DiscoveryAttributes structure.
     * @return True if correct.
     */
    bool initEDP(
            BuiltinAttributes& attributes) override;
    /**
     * Abstract method that assigns remote endpoints when a new RTPSParticipantProxyData is discovered.
     * @param pdata Pointer to the ParticipantProxyData.
     */
    void assignRemoteEndpoints(
            const ParticipantProxyData& pdata) override;
    /**
     * Abstract method that removes a local Reader from the discovery method
     * @param R Pointer to the Reader to remove.
     * @return True if correctly removed.
     */
    bool removeLocalReader(
            RTPSReader* R) override;
    /**
     * Abstract method that removes a local Writer from the discovery method
     * @param W Pointer to the Writer to remove.
     * @return True if correctly removed.
     */
    bool removeLocalWriter(
            RTPSWriter* W) override;

    /**
     * After a new local ReaderProxyData has been created some processing is needed (depends on the implementation).
     * @param reader Pointer to the RTPSReader object.
     * @param rdata Pointer to the ReaderProxyData object.
     * @return True if correct.
     */
    bool processLocalReaderProxyData(
            RTPSReader* reader,
            ReaderProxyData* rdata) override;
    /**
     * After a new local WriterProxyData has been created some processing is needed (depends on the implementation).
     * @param writer Pointer to the RTPSWriter object.
     * @param wdata Pointer to the Writer ProxyData object.
     * @return True if correct.
     */
    bool processLocalWriterProxyData(
            RTPSWriter* writer,
            WriterProxyData* wdata) override;

    /**
     * New Remote Writer has been found and this method process it and calls the pairing methods.
     * @param participant_guid  GUID of the participant.
     * @param participant_name  Name of the participant.
     * @param user_id           User Id.
     * @param ent_id            Entity Id.
     * @return True if correct.
     */
    bool newRemoteWriter(
            const GUID_t& participant_guid,
            const string_255& participant_name,
            uint16_t user_id,
            EntityId_t ent_id = c_EntityId_Unknown,
            const GUID_t& persistence_guid = GUID_t::unknown());
    /**
     * New Remote Reader has been found and this method process it and calls the pairing methods.
     * @param participant_guid  GUID of the participant.
     * @param participant_name  Name of the participant.
     * @param user_id           User Id.
     * @param ent_id            Entity Id.
     * @return true if correct.
     */
    bool newRemoteReader(
            const GUID_t& participant_guid,
            const string_255& participant_name,
            uint16_t user_id,
            EntityId_t ent_id = c_EntityId_Unknown);

    /**
     * This method checks the provided entityId against the topic type to see if it matches
     * @param rdata Pointer to the readerProxyData
     * @return True if its correct.
     **/
    bool checkEntityId(
            ReaderProxyData* rdata);
    /**
     * This method checks the provided entityId against the topic type to see if it matches
     * @param wdata Pointer to the writerProxyData
     * @return True if its correct.
     **/
    bool checkEntityId(
            WriterProxyData* wdata);

private:

    xmlparser::XMLEndpointParser* mp_edpXML;
    BuiltinAttributes m_attributes;
};

}
} /* namespace rtps */
} /* namespace eprosima */

#endif
#endif /* _FASTDDS_RTPS_EDPSTATIC_H_ */
