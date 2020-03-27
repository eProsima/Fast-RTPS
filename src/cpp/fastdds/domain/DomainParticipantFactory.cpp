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
 * @file DomainParticipantFactory.cpp
 *
 */

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/RTPSDomain.h>
#include <fastdds/rtps/participant/RTPSParticipant.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/domain/DomainParticipantImpl.hpp>

#include <fastdds/dds/log/Log.hpp>

#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <fastrtps/types/DynamicTypeBuilderFactory.h>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/TypeObjectFactory.h>

using namespace eprosima::fastrtps::xmlparser;

using eprosima::fastrtps::ParticipantAttributes;
using eprosima::fastdds::dds::Log;

using eprosima::fastrtps::rtps::RTPSDomain;
using eprosima::fastrtps::rtps::RTPSParticipant;

namespace eprosima {
namespace fastdds {
namespace dds {

class DomainParticipantFactoryReleaser
{
public:
    ~DomainParticipantFactoryReleaser()
    {
        DomainParticipantFactory::delete_instance();
    }
};

static bool g_instance_initialized = false;
static std::mutex g_mtx;
static DomainParticipantFactoryReleaser s_releaser;
static DomainParticipantFactory* g_instance = nullptr;

DomainParticipantFactory::DomainParticipantFactory()
    : default_xml_profiles_loaded(false)
{
}

DomainParticipantFactory::~DomainParticipantFactory()
{
    {
        std::lock_guard<std::mutex> guard(mtx_participants_);
        for (auto it : participants_)
        {
            for (auto pit : it.second)
            {
                pit->disable();
            }
        }
        // Delete participants
        for (auto it : participants_)
        {
            for (auto pit = it.second.begin(); pit != it.second.end(); ++pit)
            {
                delete *pit;
            }
        }
        participants_.clear();
    }

    // Deletes DynamicTypes and TypeObject factories
    fastrtps::types::DynamicTypeBuilderFactory::delete_instance();
    fastrtps::types::DynamicDataFactory::delete_instance();
    fastrtps::types::TypeObjectFactory::delete_instance();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    eprosima::fastdds::dds::Log::KillThread();
}

DomainParticipantFactory* DomainParticipantFactory::get_instance()
{
    if (!g_instance_initialized)
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        if (g_instance == nullptr)
        {
            g_instance = new DomainParticipantFactory();
            g_instance_initialized = true;
        }
    }
    return g_instance;
}

bool DomainParticipantFactory::delete_instance()
{
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_instance_initialized && g_instance != nullptr)
    {
        delete g_instance;
        g_instance = nullptr;
        g_instance_initialized = false;
        return true;
    }
    return false;
}

ReturnCode_t DomainParticipantFactory::delete_participant(
        DomainParticipant* part)
{
    using PartVectorIt = std::vector<DomainParticipantImpl*>::iterator;
    using VectorIt = std::map<uint8_t, std::vector<DomainParticipantImpl*>>::iterator;

    if (part->contains_entity(part->get_instance_handle()))
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    if (part != nullptr)
    {
        std::lock_guard<std::mutex> guard(mtx_participants_);

        VectorIt vit = participants_.find(part->get_domain_id());

        if (vit != participants_.end())
        {
            for (PartVectorIt pit = vit->second.begin(); pit != vit->second.end();)
            {
                if ((*pit)->get_participant() == part
                    || (*pit) ->get_participant()->guid() == part->guid())
                {
                    delete (*pit);
                    PartVectorIt next_it = vit->second.erase(pit);
                    pit = next_it;
                }
                else
                {
                    ++pit;
                }
            }
            if (vit->second.empty())
            {
                participants_.erase(vit);
            }
            return ReturnCode_t::RETCODE_OK;
        }
    }
    return ReturnCode_t::RETCODE_ERROR;
}

DomainParticipant* DomainParticipantFactory::create_participant(
        const std::string& participant_profile,
        DomainParticipantListener* listen)
{
    if (false == default_xml_profiles_loaded)
    {
        XMLProfileManager::loadDefaultXMLFile();
        default_xml_profiles_loaded = true;
    }

    ParticipantAttributes participant_att;
    if (XMLP_ret::XML_ERROR == XMLProfileManager::fillParticipantAttributes(participant_profile, participant_att))
    {
        logError(DOMAIN_PARTICIPANT_FACTORY, "Problem loading profile '" << participant_profile << "'");
        return nullptr;
    }

    return create_participant(participant_att, listen);
}

DomainParticipant* DomainParticipantFactory::create_participant(
        const fastrtps::ParticipantAttributes& att,
        DomainParticipantListener* listen)
{
    return create_participant(att, true, listen);
}

DomainParticipant* DomainParticipantFactory::create_participant(
        const ParticipantAttributes& att,
        bool enabled,
        DomainParticipantListener* listen)
{
    uint8_t domain_id = static_cast<uint8_t>(att.rtps.builtin.domainId);
    DomainParticipant* dom_part = new DomainParticipant();
    DomainParticipantImpl* dom_part_impl = new DomainParticipantImpl(att, dom_part, listen);
    RTPSParticipant* part = RTPSDomain::createParticipant(att.rtps, enabled, &dom_part_impl->rtps_listener_);

    if (part == nullptr)
    {
        logError(DOMAIN_PARTICIPANT_FACTORY, "Problem creating RTPSParticipant");
        delete dom_part_impl;
        return nullptr;
    }

    dom_part_impl->rtps_participant_ = part;

    {
        std::lock_guard<std::mutex> guard(mtx_participants_);
        using VectorIt = std::map<uint8_t, std::vector<DomainParticipantImpl*>>::iterator;
        VectorIt vector_it = participants_.find(domain_id);

        if (vector_it == participants_.end())
        {
            // Insert the vector
            std::vector<DomainParticipantImpl*> new_vector;
            auto pair_it = participants_.insert(std::make_pair(domain_id, std::move(new_vector)));
            vector_it = pair_it.first;
        }

        vector_it->second.push_back(dom_part_impl);
    }

    part->set_check_type_function(
        [dom_part](const std::string& type_name) -> bool
        {
            return dom_part->find_type(type_name).get() != nullptr;
        });

    return dom_part;
}

DomainParticipant* DomainParticipantFactory::lookup_participant(
        uint8_t domain_id) const
{
    std::lock_guard<std::mutex> guard(mtx_participants_);

    auto it = participants_.find(domain_id);
    if (it != participants_.end() && it->second.size() > 0)
    {
        return it->second.front()->get_participant();
    }

    return nullptr;
}

std::vector<DomainParticipant*> DomainParticipantFactory::lookup_participants(
    uint8_t domain_id) const
{
    std::lock_guard<std::mutex> guard(mtx_participants_);

    std::vector<DomainParticipant*> result;
    auto it = participants_.find(domain_id);
    if (it != participants_.end())
    {
        const std::vector<DomainParticipantImpl*>& v = it->second;
        for (auto pit = v.begin(); pit != v.end(); ++pit)
        {
            result.push_back((*pit)->get_participant());
        }
    }

    return result;
}

ReturnCode_t DomainParticipantFactory::get_default_participant_qos(
        ParticipantAttributes& participant_attributes) const
{
    if (false == default_xml_profiles_loaded)
    {
        XMLProfileManager::loadDefaultXMLFile();
        default_xml_profiles_loaded = true;
    }

    XMLProfileManager::getDefaultParticipantAttributes(participant_attributes);
    return ReturnCode_t::RETCODE_OK;
}

/* TODO
bool DomainParticipantFactory::set_default_participant_qos(
        const fastrtps::ParticipantAttributes &participant_qos)
{
    // TODO XMLProfileManager::setDefault...
    (void)participant_qos;
    logError(DOMAIN_PARTICIPANT_FACTORY, "Not implemented.");
    return false;
}
*/

bool DomainParticipantFactory::load_XML_profiles_file(
        const std::string& xml_profile_file)
{
    if (false == default_xml_profiles_loaded)
    {
        XMLProfileManager::loadDefaultXMLFile();
        default_xml_profiles_loaded = true;
    }

    if (XMLP_ret::XML_ERROR == XMLProfileManager::loadXMLFile(xml_profile_file))
    {
        logError(DOMAIN, "Problem loading XML file '" << xml_profile_file << "'");
        return false;
    }
    return true;
}

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */
