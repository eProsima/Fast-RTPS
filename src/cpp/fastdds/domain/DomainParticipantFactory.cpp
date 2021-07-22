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

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/rtps/participant/RTPSParticipant.h>
#include <fastdds/rtps/RTPSDomain.h>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/DynamicTypeBuilderFactory.h>
#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <fastdds/domain/DomainParticipantImpl.hpp>
#include <rtps/history/TopicPayloadPoolRegistry.hpp>
#include <statistics/fastdds/domain/DomainParticipantImpl.hpp>





using namespace eprosima::fastrtps::xmlparser;

using eprosima::fastrtps::ParticipantAttributes;
using eprosima::fastdds::dds::Log;

using eprosima::fastrtps::rtps::RTPSDomain;
using eprosima::fastrtps::rtps::RTPSParticipant;

namespace eprosima {
namespace fastdds {
namespace dds {

static void set_qos_from_attributes(
        DomainParticipantQos& qos,
        const eprosima::fastrtps::rtps::RTPSParticipantAttributes& attr)
{
    qos.user_data().setValue(attr.userData);
    qos.allocation() = attr.allocation;
    qos.properties() = attr.properties;
    qos.wire_protocol().prefix = attr.prefix;
    qos.wire_protocol().participant_id = attr.participantID;
    qos.wire_protocol().builtin = attr.builtin;
    qos.wire_protocol().port = attr.port;
    qos.wire_protocol().throughput_controller = attr.throughputController;
    qos.wire_protocol().default_unicast_locator_list = attr.defaultUnicastLocatorList;
    qos.wire_protocol().default_multicast_locator_list = attr.defaultMulticastLocatorList;
    qos.transport().user_transports = attr.userTransports;
    qos.transport().use_builtin_transports = attr.useBuiltinTransports;
    qos.transport().send_socket_buffer_size = attr.sendSocketBufferSize;
    qos.transport().listen_socket_buffer_size = attr.listenSocketBufferSize;
    qos.name() = attr.getName();
    qos.flow_controllers() = attr.flow_controllers;
}

DomainParticipantFactory::DomainParticipantFactory()
    : default_xml_profiles_loaded(false)
    , default_participant_qos_(PARTICIPANT_QOS_DEFAULT)
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
                delete pit;
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
    // Keep a reference to the topic payload pool to avoid it to be destroyed before our own instance
    using pool_registry_ref = eprosima::fastrtps::rtps::TopicPayloadPoolRegistry::reference;
    static pool_registry_ref topic_pool_registry = eprosima::fastrtps::rtps::TopicPayloadPoolRegistry::instance();

    static DomainParticipantFactory instance;
    return &instance;
}

ReturnCode_t DomainParticipantFactory::delete_participant(
        DomainParticipant* part)
{
    using PartVectorIt = std::vector<DomainParticipantImpl*>::iterator;
    using VectorIt = std::map<DomainId_t, std::vector<DomainParticipantImpl*>>::iterator;

    if (part != nullptr)
    {
        std::lock_guard<std::mutex> guard(mtx_participants_);
#ifdef FASTDDS_STATISTICS
        // Delete builtin statistics entities
        eprosima::fastdds::statistics::dds::DomainParticipantImpl* stat_part_impl =
                static_cast<eprosima::fastdds::statistics::dds::DomainParticipantImpl*>(part->impl_);
        stat_part_impl->delete_statistics_builtin_entities();
#endif // ifdef FASTDDS_STATISTICS
        if (part->has_active_entities())
        {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }

        VectorIt vit = participants_.find(part->get_domain_id());

        if (vit != participants_.end())
        {
            for (PartVectorIt pit = vit->second.begin(); pit != vit->second.end();)
            {
                if ((*pit)->get_participant() == part
                        || (*pit)->get_participant()->guid() == part->guid())
                {
                    (*pit)->disable();
                    delete (*pit);
                    PartVectorIt next_it = vit->second.erase(pit);
                    pit = next_it;
                    break;
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
        DomainId_t did,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listen,
        const StatusMask& mask)
{
    load_profiles();

    const DomainParticipantQos& pqos = (&qos == &PARTICIPANT_QOS_DEFAULT) ? default_participant_qos_ : qos;

    DomainParticipant* dom_part = new DomainParticipant(mask);
#ifndef FASTDDS_STATISTICS
    DomainParticipantImpl* dom_part_impl = new DomainParticipantImpl(dom_part, did, pqos, listen);
#else
    eprosima::fastdds::statistics::dds::DomainParticipantImpl* dom_part_impl =
            new eprosima::fastdds::statistics::dds::DomainParticipantImpl(dom_part, did, pqos, listen);
#endif // FASTDDS_STATISTICS

    {
        std::lock_guard<std::mutex> guard(mtx_participants_);
        using VectorIt = std::map<DomainId_t, std::vector<DomainParticipantImpl*>>::iterator;
        VectorIt vector_it = participants_.find(did);

        if (vector_it == participants_.end())
        {
            // Insert the vector
            std::vector<DomainParticipantImpl*> new_vector;
            auto pair_it = participants_.insert(std::make_pair(did, std::move(new_vector)));
            vector_it = pair_it.first;
        }

        vector_it->second.push_back(dom_part_impl);
    }

    if (factory_qos_.entity_factory().autoenable_created_entities)
    {
        if (ReturnCode_t::RETCODE_OK != dom_part->enable())
        {
            delete_participant(dom_part);
            return nullptr;
        }
    }

    return dom_part;
}

DomainParticipant* DomainParticipantFactory::create_participant_with_profile(
        DomainId_t did,
        const std::string& profile_name,
        DomainParticipantListener* listen,
        const StatusMask& mask)
{
    load_profiles();

    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles
    ParticipantAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile_name, attr))
    {
        DomainParticipantQos qos = default_participant_qos_;
        set_qos_from_attributes(qos, attr.rtps);
        return create_participant(did, qos, listen, mask);
    }

    return nullptr;
}

DomainParticipant* DomainParticipantFactory::create_participant_with_profile(
        const std::string& profile_name,
        DomainParticipantListener* listen,
        const StatusMask& mask)
{
    load_profiles();

    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles
    ParticipantAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile_name, attr))
    {
        DomainParticipantQos qos = default_participant_qos_;
        set_qos_from_attributes(qos, attr.rtps);
        return create_participant(attr.domainId, qos, listen, mask);
    }

    return nullptr;
}

DomainParticipant* DomainParticipantFactory::lookup_participant(
        DomainId_t domain_id) const
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
        DomainId_t domain_id) const
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
        DomainParticipantQos& qos) const
{
    qos = default_participant_qos_;
    return ReturnCode_t::RETCODE_OK;
}

const DomainParticipantQos& DomainParticipantFactory::get_default_participant_qos() const
{
    return default_participant_qos_;
}

ReturnCode_t DomainParticipantFactory::set_default_participant_qos(
        const DomainParticipantQos& qos)
{
    if (&qos == &PARTICIPANT_QOS_DEFAULT)
    {
        reset_default_participant_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t ret_val = DomainParticipantImpl::check_qos(qos);
    if (!ret_val)
    {
        return ret_val;
    }
    DomainParticipantImpl::set_qos(default_participant_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactory::get_participant_qos_from_profile(
        const std::string& profile_name,
        DomainParticipantQos& qos) const
{
    ParticipantAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile_name, attr))
    {
        qos = default_participant_qos_;
        set_qos_from_attributes(qos, attr.rtps);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DomainParticipantFactory::load_profiles()
{
    if (false == default_xml_profiles_loaded)
    {
        XMLProfileManager::loadDefaultXMLFile();
        // Only load profile once
        default_xml_profiles_loaded = true;

        // Only change default participant qos when not explicitly set by the user
        if (default_participant_qos_ == PARTICIPANT_QOS_DEFAULT)
        {
            reset_default_participant_qos();
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactory::load_XML_profiles_file(
        const std::string& xml_profile_file)
{
    if (XMLP_ret::XML_ERROR == XMLProfileManager::loadXMLFile(xml_profile_file))
    {
        logError(DOMAIN, "Problem loading XML file '" << xml_profile_file << "'");
        return ReturnCode_t::RETCODE_ERROR;
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactory::get_qos(
        DomainParticipantFactoryQos& qos) const
{
    qos = factory_qos_;
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactory::set_qos(
        const DomainParticipantFactoryQos& qos)
{
    ReturnCode_t ret_val = check_qos(qos);
    if (!ret_val)
    {
        return ret_val;
    }
    if (!can_qos_be_updated(factory_qos_, qos))
    {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }
    set_qos(factory_qos_, qos, false);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantFactory::reset_default_participant_qos()
{
    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles
    DomainParticipantImpl::set_qos(default_participant_qos_, PARTICIPANT_QOS_DEFAULT, true);
    if (true == default_xml_profiles_loaded)
    {
        eprosima::fastrtps::ParticipantAttributes attr;
        XMLProfileManager::getDefaultParticipantAttributes(attr);
        set_qos_from_attributes(default_participant_qos_, attr.rtps);
    }
}

void DomainParticipantFactory::set_qos(
        DomainParticipantFactoryQos& to,
        const DomainParticipantFactoryQos& from,
        bool first_time)
{
    (void) first_time;
    //As all the Qos can always be updated and none of them need to be sent
    to = from;
}

ReturnCode_t DomainParticipantFactory::check_qos(
        const DomainParticipantFactoryQos& qos)
{
    (void) qos;
    //There is no restriction by the moment with the contained Qos
    return ReturnCode_t::RETCODE_OK;
}

bool DomainParticipantFactory::can_qos_be_updated(
        const DomainParticipantFactoryQos& to,
        const DomainParticipantFactoryQos& from)
{
    (void) to;
    (void) from;
    //All the DomainParticipantFactoryQos can be updated
    return true;
}

void DomainParticipantFactory::participant_has_been_deleted(
        DomainParticipantImpl* part)
{
    std::lock_guard<std::mutex> guard(mtx_participants_);
    auto it = participants_.find(part->get_domain_id());
    if (it != participants_.end())
    {
        for (auto pit = it->second.begin(); pit != it->second.end();)
        {
            if ((*pit) == part || (*pit)->guid() == part->guid())
            {
                pit = it->second.erase(pit);
            }
            else
            {
                ++pit;
            }
        }
        if (it->second.empty())
        {
            participants_.erase(it);
        }
    }
}

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */
