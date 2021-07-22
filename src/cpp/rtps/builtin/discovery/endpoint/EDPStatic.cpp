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
 * @file EDPStatic.cpp
 *
 */

#include <fastdds/rtps/builtin/discovery/endpoint/EDPStatic.h>
#include <fastdds/rtps/builtin/discovery/participant/PDPSimple.h>
#include <fastrtps/xmlparser/XMLEndpointParser.h>

#include <fastdds/rtps/builtin/data/WriterProxyData.h>
#include <fastdds/rtps/builtin/data/ReaderProxyData.h>
#include <fastdds/rtps/builtin/data/ParticipantProxyData.h>

#include <fastdds/rtps/reader/RTPSReader.h>
#include <fastdds/rtps/writer/RTPSWriter.h>

#include <fastdds/dds/log/Log.hpp>

#include <rtps/participant/RTPSParticipantImpl.h>

#include <mutex>
#include <sstream>
#include <tinyxml2.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {


EDPStatic::EDPStatic(
        PDP* p,
        RTPSParticipantImpl* part)
    : EDP(p, part)
    , mp_edpXML(nullptr)
{
}

EDPStatic::~EDPStatic()
{
    if (mp_edpXML != nullptr)
    {
        delete(mp_edpXML);
    }
}

bool EDPStatic::initEDP(
        BuiltinAttributes& attributes)
{
    logInfo(RTPS_EDP, "Beginning STATIC EndpointDiscoveryProtocol");

    bool returned_value = false;
    m_attributes = attributes;
    mp_edpXML = new xmlparser::XMLEndpointParser();
    std::string content(m_attributes.discovery_config.static_edp_xml_config());
    if (0 == content.rfind("data://", 0))
    {
        tinyxml2::XMLDocument xml_document;
        if (tinyxml2::XMLError::XML_SUCCESS == xml_document.Parse(content.c_str() + 7, content.size() - 7))
        {
            returned_value = (this->mp_edpXML->loadXMLNode(xml_document) == xmlparser::XMLP_ret::XML_OK);
        }
    }
    else if (0 == content.rfind("file://", 0))
    {
        std::string file_name = content.substr(7);
        returned_value =  (this->mp_edpXML->loadXMLFile(file_name) == xmlparser::XMLP_ret::XML_OK);
    }

    return returned_value;
}

std::pair<std::string, std::string> EDPStaticProperty::toProperty(
        std::string type,
        std::string status,
        uint16_t id,
        const EntityId_t& ent)
{
    std::pair<std::string, std::string> prop;
    std::stringstream ss;
    ss << "eProsimaEDPStatic_" << type << "_" << status << "_ID_" << id;
    prop.first = ss.str();
    ss.clear();
    ss.str(std::string());
    ss << (int)ent.value[0] << ".";
    ss << (int)ent.value[1] << ".";
    ss << (int)ent.value[2] << ".";
    ss << (int)ent.value[3];
    prop.second = ss.str();
    return prop;
}

bool EDPStaticProperty::fromProperty(
        std::pair<std::string, std::string> prop)
{
    if (prop.first.substr(0, 17) == "eProsimaEDPStatic" && prop.first.substr(31, 2) == "ID")
    {
        this->m_endpointType = prop.first.substr(18, 6);
        this->m_status = prop.first.substr(25, 5);
        this->m_userIdStr = prop.first.substr(34, 100);
        std::stringstream ss;
        ss << m_userIdStr;
        ss >> m_userId;
        ss.clear();
        ss.str(std::string());
        ss << prop.second;
        int a, b, c, d;
        char ch;
        ss >> a >> ch >> b >> ch >> c >> ch >> d;
        m_entityId.value[0] = (octet)a; m_entityId.value[1] = (octet)b;
        m_entityId.value[2] = (octet)c; m_entityId.value[3] = (octet)d;
        return true;
    }
    return false;
}

bool EDPStatic::processLocalReaderProxyData(
        RTPSReader*,
        ReaderProxyData* rdata)
{
    logInfo(RTPS_EDP, rdata->guid().entityId << " in topic: " << rdata->topicName());
    mp_PDP->getMutex()->lock();
    //Add the property list entry to our local pdp
    ParticipantProxyData* localpdata = this->mp_PDP->getLocalParticipantProxyData();
    localpdata->m_properties.push_back(EDPStaticProperty::toProperty("Reader", "ALIVE", rdata->userDefinedId(),
            rdata->guid().entityId));
    mp_PDP->getMutex()->unlock();
    this->mp_PDP->announceParticipantState(true);
    return true;
}

bool EDPStatic::processLocalWriterProxyData(
        RTPSWriter*,
        WriterProxyData* wdata)
{
    logInfo(RTPS_EDP, wdata->guid().entityId << " in topic: " << wdata->topicName());
    mp_PDP->getMutex()->lock();
    //Add the property list entry to our local pdp
    ParticipantProxyData* localpdata = this->mp_PDP->getLocalParticipantProxyData();
    localpdata->m_properties.push_back(EDPStaticProperty::toProperty("Writer", "ALIVE",
            wdata->userDefinedId(), wdata->guid().entityId));
    mp_PDP->getMutex()->unlock();
    this->mp_PDP->announceParticipantState(true);
    return true;
}

bool EDPStatic::removeLocalReader(
        RTPSReader* R)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_PDP->getMutex());
    ParticipantProxyData* localpdata = this->mp_PDP->getLocalParticipantProxyData();
    for (ParameterPropertyList_t::iterator pit = localpdata->m_properties.begin();
            pit != localpdata->m_properties.end(); ++pit)
    {
        EDPStaticProperty staticproperty;
        if (staticproperty.fromProperty((*pit).pair()))
        {
            if (staticproperty.m_entityId == R->getGuid().entityId)
            {
                auto new_property = EDPStaticProperty::toProperty("Reader", "ENDED",
                                R->getAttributes().getUserDefinedID(), R->getGuid().entityId);
                if (!pit->modify(new_property))
                {
                    logError(RTPS_EDP, "Failed to change property <"
                            << pit->first() << " | " << pit->second() << "> to <"
                            << new_property.first << " | " << new_property.second << ">");
                }
            }
        }
    }
    return false;
}

bool EDPStatic::removeLocalWriter(
        RTPSWriter* W)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_PDP->getMutex());
    ParticipantProxyData* localpdata = this->mp_PDP->getLocalParticipantProxyData();
    for (ParameterPropertyList_t::iterator pit = localpdata->m_properties.begin();
            pit != localpdata->m_properties.end(); ++pit)
    {
        EDPStaticProperty staticproperty;
        if (staticproperty.fromProperty((*pit).pair()))
        {
            if (staticproperty.m_entityId == W->getGuid().entityId)
            {
                auto new_property = EDPStaticProperty::toProperty("Writer", "ENDED",
                                W->getAttributes().getUserDefinedID(), W->getGuid().entityId);
                if (!pit->modify(new_property))
                {
                    logError(RTPS_EDP, "Failed to change property <"
                            << pit->first() << " | " << pit->second() << "> to <"
                            << new_property.first << " | " << new_property.second << ">");
                }
            }
        }
    }
    return false;
}

void EDPStatic::assignRemoteEndpoints(
        const ParticipantProxyData& pdata)
{
    GUID_t persistence_guid;

    // Fill persistence GUID if present in UserData.
    bool is_persistent_guid = (18 <= pdata.m_userData.size()) &&
            ('V' == pdata.m_userData.at(0)) && ('G' == pdata.m_userData.at(1)) && ('W' == pdata.m_userData.at(2));
    if (is_persistent_guid)
    {
        persistence_guid.guidPrefix.value[0] = pdata.m_userData.at(3);
        persistence_guid.guidPrefix.value[1] = pdata.m_userData.at(4);
        persistence_guid.guidPrefix.value[2] = pdata.m_userData.at(5);
        persistence_guid.guidPrefix.value[3] = pdata.m_userData.at(6);
        persistence_guid.guidPrefix.value[4] = pdata.m_userData.at(7);
        persistence_guid.guidPrefix.value[5] = pdata.m_userData.at(8);
        persistence_guid.guidPrefix.value[6] = pdata.m_userData.at(9);
        persistence_guid.guidPrefix.value[7] = pdata.m_userData.at(10);
        persistence_guid.guidPrefix.value[8] = pdata.m_userData.at(11);
        persistence_guid.guidPrefix.value[9] = pdata.m_userData.at(12);
        persistence_guid.guidPrefix.value[10] = pdata.m_userData.at(13);
        persistence_guid.guidPrefix.value[11] = pdata.m_userData.at(14);
        persistence_guid.entityId.value[0] = pdata.m_userData.at(15);
        persistence_guid.entityId.value[1] = pdata.m_userData.at(16);
        persistence_guid.entityId.value[2] = pdata.m_userData.at(17);
    }

    for (ParameterPropertyList_t::const_iterator pit = pdata.m_properties.begin();
            pit != pdata.m_properties.end(); ++pit)
    {
        persistence_guid.entityId.value[3] = 0;

        //cout << "STATIC EDP READING PROPERTY " << pit->first << "// " << pit->second << endl;
        EDPStaticProperty staticproperty;
        if (staticproperty.fromProperty((*pit).pair()))
        {
            if (staticproperty.m_endpointType == "Reader" && staticproperty.m_status == "ALIVE")
            {
                GUID_t guid(pdata.m_guid.guidPrefix, staticproperty.m_entityId);
                if (!this->mp_PDP->has_reader_proxy_data(guid))//IF NOT FOUND, we CREATE AND PAIR IT
                {
                    newRemoteReader(pdata.m_guid, pdata.m_participantName,
                            staticproperty.m_userId, staticproperty.m_entityId);
                }
            }
            else if (staticproperty.m_endpointType == "Writer" && staticproperty.m_status == "ALIVE")
            {
                GUID_t guid(pdata.m_guid.guidPrefix, staticproperty.m_entityId);
                if (!this->mp_PDP->has_writer_proxy_data(guid))//IF NOT FOUND, we CREATE AND PAIR IT
                {
                    if (is_persistent_guid)
                    {
                        persistence_guid.entityId.value[3] = static_cast<uint8_t>(staticproperty.m_userId);
                    }
                    newRemoteWriter(pdata.m_guid, pdata.m_participantName,
                            staticproperty.m_userId, staticproperty.m_entityId, persistence_guid);
                }
            }
            else if (staticproperty.m_endpointType == "Reader" && staticproperty.m_status == "ENDED")
            {
                GUID_t guid(pdata.m_guid.guidPrefix, staticproperty.m_entityId);
                this->mp_PDP->removeReaderProxyData(guid);
            }
            else if (staticproperty.m_endpointType == "Writer" && staticproperty.m_status == "ENDED")
            {
                GUID_t guid(pdata.m_guid.guidPrefix, staticproperty.m_entityId);
                this->mp_PDP->removeWriterProxyData(guid);
            }
            else
            {
                logWarning(RTPS_EDP, "Property with type: " << staticproperty.m_endpointType
                                                            << " and status " << staticproperty.m_status <<
                        " not recognized");
            }
        }
        else
        {

        }
    }
}

bool EDPStatic::newRemoteReader(
        const GUID_t& participant_guid,
        const string_255& participant_name,
        uint16_t user_id,
        EntityId_t ent_id)
{
    ReaderProxyData* rpd = NULL;
    if (mp_edpXML->lookforReader(participant_name, user_id, &rpd) == xmlparser::XMLP_ret::XML_OK)
    {
        logInfo(RTPS_EDP, "Activating: " << rpd->guid().entityId << " in topic " << rpd->topicName());
        GUID_t reader_guid(participant_guid.guidPrefix, ent_id != c_EntityId_Unknown ? ent_id : rpd->guid().entityId);

        auto init_fun = [this, participant_guid, reader_guid, rpd](
            ReaderProxyData* newRPD,
            bool updating,
            const ParticipantProxyData& participant_data)
                {
                    // Should be a new reader
                    (void)updating;
                    assert(!updating);

                    *newRPD = *rpd;
                    newRPD->guid(reader_guid);
                    if (!checkEntityId(newRPD))
                    {
                        logError(RTPS_EDP, "The provided entityId for Reader with ID: "
                                << newRPD->userDefinedId() << " does not match the topic Kind");
                        return false;
                    }
                    newRPD->key() = newRPD->guid();
                    newRPD->RTPSParticipantKey() = participant_guid;
                    if (!newRPD->has_locators())
                    {
                        const NetworkFactory& network = mp_RTPSParticipant->network_factory();
                        newRPD->set_remote_locators(participant_data.default_locators, network, true);
                    }

                    return true;
                };

        GUID_t temp_participant_guid;
        ReaderProxyData* reader_data = this->mp_PDP->addReaderProxyData(reader_guid, temp_participant_guid, init_fun);
        if (reader_data != nullptr)
        {
            this->pairing_reader_proxy_with_any_local_writer(participant_guid, reader_data);
            return true;
        }
    }
    return false;
}

bool EDPStatic::newRemoteWriter(
        const GUID_t& participant_guid,
        const string_255& participant_name,
        uint16_t user_id,
        EntityId_t ent_id,
        const GUID_t& persistence_guid)
{
    WriterProxyData* wpd = NULL;
    if (mp_edpXML->lookforWriter(participant_name, user_id, &wpd) == xmlparser::XMLP_ret::XML_OK)
    {
        logInfo(RTPS_EDP, "Activating: " << wpd->guid().entityId << " in topic " << wpd->topicName());
        GUID_t writer_guid(participant_guid.guidPrefix, ent_id != c_EntityId_Unknown ? ent_id : wpd->guid().entityId);

        auto init_fun = [this, participant_guid, writer_guid, wpd, persistence_guid](
            WriterProxyData* newWPD,
            bool updating,
            const ParticipantProxyData& participant_data)
                {
                    // Should be a new reader
                    (void)updating;
                    assert(!updating);

                    *newWPD = *wpd;
                    newWPD->guid(writer_guid);
                    if (!checkEntityId(newWPD))
                    {
                        logError(RTPS_EDP, "The provided entityId for Writer with User ID: "
                                << newWPD->userDefinedId() << " does not match the topic Kind");
                        return false;
                    }
                    newWPD->key() = newWPD->guid();
                    newWPD->RTPSParticipantKey() = participant_guid;
                    if (!newWPD->has_locators())
                    {
                        const NetworkFactory& network = mp_RTPSParticipant->network_factory();
                        newWPD->set_remote_locators(participant_data.default_locators, network, true);
                    }
                    newWPD->persistence_guid(persistence_guid);

                    return true;
                };
        GUID_t temp_participant_guid;
        WriterProxyData* writer_data = this->mp_PDP->addWriterProxyData(writer_guid, temp_participant_guid, init_fun);
        if (writer_data != nullptr)
        {
            this->pairing_writer_proxy_with_any_local_reader(participant_guid, writer_data);
            return true;
        }
    }
    return false;
}

bool EDPStatic::checkEntityId(
        ReaderProxyData* rdata)
{
    if (rdata->topicKind() == WITH_KEY && rdata->guid().entityId.value[3] == 0x07)
    {
        return true;
    }
    if (rdata->topicKind() == NO_KEY && rdata->guid().entityId.value[3] == 0x04)
    {
        return true;
    }
    return false;
}

bool EDPStatic::checkEntityId(
        WriterProxyData* wdata)
{
    if (wdata->topicKind() == WITH_KEY && wdata->guid().entityId.value[3] == 0x02)
    {
        return true;
    }
    if (wdata->topicKind() == NO_KEY && wdata->guid().entityId.value[3] == 0x03)
    {
        return true;
    }
    return false;
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
