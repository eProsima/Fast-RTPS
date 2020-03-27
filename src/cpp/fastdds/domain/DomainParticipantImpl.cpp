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
 * @file DomainParticipantImpl.cpp
 *
 */

#include <fastdds/domain/DomainParticipantImpl.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.h>
#include <fastdds/rtps/reader/ReaderDiscoveryInfo.h>
#include <fastdds/rtps/writer/WriterDiscoveryInfo.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>

#include <fastdds/rtps/participant/RTPSParticipant.h>

#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastdds/publisher/PublisherImpl.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>

#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastdds/subscriber/SubscriberImpl.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include <fastdds/dds/topic/DataReader.hpp>
#include <fastdds/dds/topic/DataWriter.hpp>

#include <fastdds/dds/builtin/typelookup/TypeLookupManager.hpp>

#include <fastdds/rtps/RTPSDomain.h>

#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/DynamicTypeBuilderFactory.h>
#include <fastrtps/types/DynamicPubSubType.h>
#include <fastrtps/types/DynamicType.h>
#include <fastrtps/types/DynamicTypeMember.h>

#include <fastdds/dds/log/Log.hpp>

#include <chrono>

using namespace eprosima;
using namespace eprosima::fastdds::dds;

using fastrtps::ParticipantAttributes;
using fastrtps::rtps::RTPSDomain;
using fastrtps::rtps::RTPSParticipant;
using fastrtps::rtps::ParticipantDiscoveryInfo;
#if HAVE_SECURITY
using fastrtps::rtps::ParticipantAuthenticationInfo;
#endif
using fastrtps::rtps::ReaderDiscoveryInfo;
using fastrtps::rtps::ReaderProxyData;
using fastrtps::rtps::WriterDiscoveryInfo;
using fastrtps::rtps::WriterProxyData;
using fastrtps::rtps::InstanceHandle_t;
using fastrtps::rtps::GUID_t;
using fastrtps::rtps::EndpointKind_t;
using fastrtps::rtps::ResourceEvent;
using eprosima::fastdds::dds::Log;

DomainParticipantImpl::DomainParticipantImpl(
        const ParticipantAttributes& patt,
        DomainParticipant* pspart,
        DomainParticipantListener* listen)
    : att_(patt)
    , rtps_participant_(nullptr)
    , participant_(pspart)
    , listener_(listen)
#pragma warning (disable : 4355 )
    , rtps_listener_(this)
{
    participant_->impl_ = this;
}

void DomainParticipantImpl::enable()
{
    rtps_participant_->enable();
}

void DomainParticipantImpl::disable()
{
    participant_->set_listener(nullptr);
    rtps_listener_.participant_ = nullptr;
    rtps_participant_->set_listener(nullptr);
    {
        std::lock_guard<std::mutex> lock(mtx_pubs_);
        for (auto pub_it = publishers_.begin(); pub_it != publishers_.end(); ++pub_it)
        {
            pub_it->second->disable();
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx_subs_);
        for (auto sub_it = subscribers_.begin(); sub_it != subscribers_.end(); ++sub_it)
        {
            sub_it->second->disable();
        }
    }
}

DomainParticipantImpl::~DomainParticipantImpl()
{
    {
        std::lock_guard<std::mutex> lock(mtx_pubs_);
        for (auto pub_it = publishers_.begin(); pub_it != publishers_.end(); ++pub_it)
        {
            delete pub_it->second;
        }
        publishers_.clear();
        publishers_by_handle_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(mtx_subs_);

        for (auto sub_it = subscribers_.begin(); sub_it != subscribers_.end(); ++sub_it)
        {
            delete sub_it->second;
        }
        subscribers_.clear();
        subscribers_by_handle_.clear();
    }

    if (rtps_participant_ != nullptr)
    {
        RTPSDomain::removeRTPSParticipant(rtps_participant_);
    }

    {
        std::lock_guard<std::mutex> lock(mtx_types_);
        types_.clear();
    }

    delete participant_;
    participant_ = nullptr;
}


ReturnCode_t DomainParticipantImpl::delete_publisher(
        Publisher* pub)
{
    if (participant_ != pub->get_participant())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    std::lock_guard<std::mutex> lock(mtx_pubs_);
    auto pit = publishers_.find(pub);

    if (pit != publishers_.end() && pub->get_instance_handle() == pit->second->get_instance_handle())
    {
        if (pub->has_datawriters())
        {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        pit->second->set_listener(nullptr);
        publishers_by_handle_.erase(publishers_by_handle_.find(pit->second->get_instance_handle()));
        delete pit->second;
        publishers_.erase(pit);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DomainParticipantImpl::delete_subscriber(
        Subscriber* sub)
{
    if (participant_ != sub->get_participant())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    std::lock_guard<std::mutex> lock(mtx_subs_);
    auto sit = subscribers_.find(sub);

    if (sit != subscribers_.end() && sub->get_instance_handle() == sit->second->get_instance_handle())
    {
        if (sub->has_datareaders())
        {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        sit->second->set_listener(nullptr);
        subscribers_by_handle_.erase(subscribers_by_handle_.find(sit->second->get_instance_handle()));
        delete sit->second;
        subscribers_.erase(sit);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_ERROR;
}

const InstanceHandle_t& DomainParticipantImpl::get_instance_handle() const
{
    return static_cast<const InstanceHandle_t&>(rtps_participant_->getGuid());
}

const GUID_t& DomainParticipantImpl::guid() const
{
    return rtps_participant_->getGuid();
}

Publisher* DomainParticipantImpl::create_publisher(
        const PublisherQos& qos,
        const fastrtps::PublisherAttributes& att,
        PublisherListener* listen)
{
    if (att_.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol)
    {
        if (att.getUserDefinedID() <= 0)
        {
            logError(PARTICIPANT,"Static EDP requires user defined Id");
            return nullptr;
        }
    }

    if (!att.unicastLocatorList.isValid())
    {
        logError(PARTICIPANT,"Unicast Locator List for Publisher contains invalid Locator");
        return nullptr;
    }

    if (!att.multicastLocatorList.isValid())
    {
        logError(PARTICIPANT," Multicast Locator List for Publisher contains invalid Locator");
        return nullptr;
    }

    if (!att.remoteLocatorList.isValid())
    {
        logError(PARTICIPANT,"Remote Locator List for Publisher contains invalid Locator");
        return nullptr;
    }

    if (!qos.check_qos() || !att.qos.checkQos() || !att.topic.checkQos())
    {
        return nullptr;
    }

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    PublisherImpl* pubimpl = new PublisherImpl(this, qos, att, listen);
    Publisher* pub = new Publisher(pubimpl);
    pubimpl->user_publisher_ = pub;
    pubimpl->rtps_participant_ = rtps_participant_;

    // Create InstanceHandle for the new publisher
    GUID_t pub_guid = guid();
    do
    {
        pub_guid.entityId = fastrtps::rtps::c_EntityId_Unknown;
        rtps_participant_->get_new_entity_id(pub_guid.entityId);
    } while (exists_entity_id(pub_guid.entityId));

    InstanceHandle_t pub_handle(pub_guid);
    pubimpl->handle_ = pub_handle;

    //SAVE THE PUBLISHER INTO MAPS
    std::lock_guard<std::mutex> lock(mtx_pubs_);
    publishers_by_handle_[pub_handle] = pub;
    publishers_[pub] = pubimpl;

    if (att.topic.auto_fill_type_object || att.topic.auto_fill_type_information)
    {
        register_dynamic_type_to_factories(att.topic.getTopicDataType().to_string());
    }

    return pub;
}

/* TODO
Subscriber* DomainParticipantImpl::get_builtin_subscriber()
{
    logError(PARTICIPANT, "Not implemented.");
    return nullptr;
}
*/

/* TODO
bool DomainParticipantImpl::ignore_participant(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

/* TODO
bool DomainParticipantImpl::ignore_topic(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

/* TODO
bool DomainParticipantImpl::ignore_publication(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

/* TODO
bool DomainParticipantImpl::ignore_subscription(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    (void)handle;
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

uint8_t DomainParticipantImpl::get_domain_id() const
{
    return static_cast<uint8_t>(att_.rtps.builtin.domainId);
}

/* TODO
bool DomainParticipantImpl::delete_contained_entities()
{
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

ReturnCode_t DomainParticipantImpl::assert_liveliness()
{
    if (rtps_participant_->wlp() != nullptr)
    {
        if (rtps_participant_->wlp()->assert_liveliness_manual_by_participant())
        {
            return ReturnCode_t::RETCODE_OK;
        }
    }
    else
    {
        logError(PARTICIPANT, "Invalid WLP, cannot assert liveliness of participant");
    }
    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DomainParticipantImpl::set_default_publisher_qos(
        const fastdds::dds::PublisherQos& qos)
{
    if (&qos == &PUBLISHER_QOS_DEFAULT)
    {
        default_pub_qos_.set_qos(PUBLISHER_QOS_DEFAULT, true);
        return ReturnCode_t::RETCODE_OK;
    }
    else if (qos.check_qos())
    {
        default_pub_qos_.set_qos(qos, true);
        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
}

const fastdds::dds::PublisherQos& DomainParticipantImpl::get_default_publisher_qos() const
{
    return default_pub_qos_;
}

ReturnCode_t DomainParticipantImpl::set_default_subscriber_qos(
        const fastdds::dds::SubscriberQos& qos)
{
    if (&qos == &SUBSCRIBER_QOS_DEFAULT)
    {
        default_sub_qos_.set_qos(SUBSCRIBER_QOS_DEFAULT, true);
        return ReturnCode_t::RETCODE_OK;
    }
    else if (qos.check_qos())
    {
        default_sub_qos_.set_qos(qos, false);
        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
}

const fastdds::dds::SubscriberQos& DomainParticipantImpl::get_default_subscriber_qos() const
{
    return default_sub_qos_;
}

/* TODO
bool DomainParticipantImpl::get_discovered_participants(
        std::vector<fastrtps::rtps::InstanceHandle_t>& participant_handles) const
{
    (void)participant_handles;
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

/* TODO
bool DomainParticipantImpl::get_discovered_topics(
        std::vector<fastrtps::rtps::InstanceHandle_t>& topic_handles) const
{
    (void)topic_handles;
    logError(PARTICIPANT, "Not implemented.");
    return false;
}
*/

bool DomainParticipantImpl::contains_entity(
        const fastrtps::rtps::InstanceHandle_t& handle,
        bool recursive) const
{
    // Look for publishers
    {
        std::lock_guard<std::mutex> lock(mtx_pubs_);
        if (publishers_by_handle_.find(handle) != publishers_by_handle_.end())
        {
            return true;
        }
    }

    // Look for subscribers
    {
        std::lock_guard<std::mutex> lock(mtx_subs_);
        if (subscribers_by_handle_.find(handle) != subscribers_by_handle_.end())
        {
            return true;
        }
    }

    if (recursive)
    {
        // Look into publishers
        {
            std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto pit : publishers_)
            {
                if (pit.second->contains_entity(handle))
                {
                    return true;
                }
            }
        }

        // Look into subscribers
        std::vector<DataReader*> readers;
        {
            for (auto sit : subscribers_)
            {
                if (sit.second->contains_entity(handle))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

ReturnCode_t DomainParticipantImpl::get_current_time(
        fastrtps::Time_t& current_time) const
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= seconds;
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

    current_time.seconds = static_cast<int32_t>(seconds.count());
    current_time.nanosec = static_cast<uint32_t>(nanos.count());

    return ReturnCode_t::RETCODE_OK;
}

const DomainParticipant* DomainParticipantImpl::get_participant() const
{
    return participant_;
}

DomainParticipant* DomainParticipantImpl::get_participant()
{
    return participant_;
}

std::vector<std::string> DomainParticipantImpl::get_participant_names() const
{
    return rtps_participant_->getParticipantNames();
}

Subscriber* DomainParticipantImpl::create_subscriber(
        const SubscriberQos& qos,
        const fastrtps::SubscriberAttributes& att,
        SubscriberListener* listen)
{
    if (att_.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol)
    {
        if (att.getUserDefinedID() <= 0)
        {
            logError(PARTICIPANT,"Static EDP requires user defined Id");
            return nullptr;
        }
    }

    if (!att.unicastLocatorList.isValid())
    {
        logError(PARTICIPANT,"Unicast Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }

    if (!att.multicastLocatorList.isValid())
    {
        logError(PARTICIPANT," Multicast Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }

    if (!att.remoteLocatorList.isValid())
    {
        logError(PARTICIPANT,"Output Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }

    if (!qos.check_qos() || !att.qos.checkQos() || !att.topic.checkQos())
    {
        return nullptr;
    }

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    SubscriberImpl* subimpl = new SubscriberImpl(this, qos, att, listen);
    Subscriber* sub = new Subscriber(subimpl);
    subimpl->user_subscriber_ = sub;
    subimpl->rtps_participant_ = this->rtps_participant_;

    // Create InstanceHandle for the new subscriber
    GUID_t sub_guid = guid();
    do
    {
        sub_guid.entityId = fastrtps::rtps::c_EntityId_Unknown;
        rtps_participant_->get_new_entity_id(sub_guid.entityId);
    } while (exists_entity_id(sub_guid.entityId));

    InstanceHandle_t sub_handle(sub_guid);
    subimpl->handle_ = sub_handle;

    //SAVE THE PUBLISHER INTO MAPS
    std::lock_guard<std::mutex> lock(mtx_subs_);
    subscribers_by_handle_[sub_handle] = sub;
    subscribers_[sub] = subimpl;

    if (att.topic.auto_fill_type_object || att.topic.auto_fill_type_information)
    {
        register_dynamic_type_to_factories(att.topic.getTopicDataType().to_string());
    }

    return sub;
}


const TypeSupport DomainParticipantImpl::find_type(
        const std::string& type_name) const
{
    std::lock_guard<std::mutex> lock(mtx_types_);

    auto type_it = types_.find(type_name);

    if (type_it != types_.end())
    {
        return type_it->second;
    }

    return TypeSupport(nullptr);
}

bool DomainParticipantImpl::register_type(
        const TypeSupport type,
        const std::string& type_name)
{
    TypeSupport t = find_type(type_name);

    if (t != nullptr)
    {
        if (t == type)
        {
            return true;
        }

        logError(PARTICIPANT, "Another type with the same name '" << type_name << "' is already registered.");
    }

    if (type_name.size() <= 0)
    {
        logError(PARTICIPANT, "Registered Type must have a name");
        return false;
    }

    logInfo(PARTICIPANT, "Type " << type_name << " registered.");
    std::lock_guard<std::mutex> lock(mtx_types_);
    types_.insert(std::make_pair(type_name, type));

    return true;
}

bool DomainParticipantImpl::register_dynamic_type_to_factories(
    const std::string& type_name) const
{
    using namespace  eprosima::fastrtps::types;
    TypeSupport t = find_type(type_name);
    DynamicPubSubType* dpst = dynamic_cast<DynamicPubSubType*>(t.get());
    if (dpst != nullptr) // Registering a dynamic type.
    {
        TypeObjectFactory* objectFactory = TypeObjectFactory::get_instance();
        DynamicTypeBuilderFactory* dynFactory = DynamicTypeBuilderFactory::get_instance();
        const TypeIdentifier* id = objectFactory->get_type_identifier_trying_complete(type_name);
        if (id == nullptr)
        {
            std::map<MemberId, DynamicTypeMember*> membersMap;
            dpst->GetDynamicType()->get_all_members(membersMap);
            std::vector<const MemberDescriptor*> members;
            for (auto it : membersMap)
            {
                members.push_back(it.second->get_descriptor());
            }
            TypeObject typeObj;
            dynFactory->build_type_object(dpst->GetDynamicType()->get_type_descriptor(), typeObj, &members);
            // Minimal too
            dynFactory->build_type_object(dpst->GetDynamicType()->get_type_descriptor(), typeObj, &members, false);
            const TypeIdentifier *type_id2 = objectFactory->get_type_identifier(dpst->getName());
            const TypeObject *type_obj = objectFactory->get_type_object(dpst->getName());
            if (type_id2 == nullptr)
            {
                logError(DOMAIN_PARTICIPANT, "Cannot register dynamic type " << dpst->getName());
            }
            else
            {
                objectFactory->add_type_object(dpst->getName(), type_id2, type_obj);

                // Complete, just to make sure it is generated
                const TypeIdentifier *type_id_complete = objectFactory->get_type_identifier(dpst->getName(), true);
                const TypeObject *type_obj_complete = objectFactory->get_type_object(dpst->getName(), true);
                objectFactory->add_type_object(dpst->getName(), type_id_complete, type_obj_complete); // Add complete
                return true;
            }
        }
    }

    return false; // Isn't a registered dynamic type.
}

bool DomainParticipantImpl::unregister_type(
        const char* type_name)
{
    TypeSupport t = find_type(type_name);

    if (t.empty())
    {
        return true; // Not registered, so unregistering complete.
    }

    {
        // Check is any subscriber is using the type
        std::lock_guard<std::mutex> lock(mtx_subs_);

        for (auto sit : subscribers_)
        {
            if (sit.second->type_in_use(type_name))
            {
                return false; // Is in use
            }
        }
    }

    {
        // Check is any publisher is using the type
        std::lock_guard<std::mutex> lock(mtx_pubs_);

        for (auto pit : publishers_)
        {
            if (pit.second->type_in_use(type_name))
            {
                return false; // Is in use
            }
        }
    }

    std::lock_guard<std::mutex> lock(mtx_types_);
    types_.erase(type_name);

    return true;
}

void DomainParticipantImpl::MyRTPSParticipantListener::onParticipantDiscovery(
        RTPSParticipant*,
        ParticipantDiscoveryInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_participant_discovery(participant_->participant_, std::move(info));
    }
}

#if HAVE_SECURITY
void DomainParticipantImpl::MyRTPSParticipantListener::onParticipantAuthentication(
        RTPSParticipant*,
        ParticipantAuthenticationInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->onParticipantAuthentication(participant_->participant_, std::move(info));
    }
}
#endif

void DomainParticipantImpl::MyRTPSParticipantListener::onReaderDiscovery(
        RTPSParticipant*,
        ReaderDiscoveryInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_subscriber_discovery(participant_->participant_, std::move(info));
    }
}

void DomainParticipantImpl::MyRTPSParticipantListener::onWriterDiscovery(
        RTPSParticipant*,
        WriterDiscoveryInfo&& info)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_publisher_discovery(participant_->participant_, std::move(info));
    }
}

void DomainParticipantImpl::MyRTPSParticipantListener::on_type_discovery(
        RTPSParticipant*,
        const fastrtps::rtps::SampleIdentity& request_sample_id,
        const fastrtps::string_255& topic,
        const fastrtps::types::TypeIdentifier* identifier,
        const fastrtps::types::TypeObject* object,
        fastrtps::types::DynamicType_ptr dyn_type)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_type_discovery(
            participant_->participant_,
            request_sample_id,
            topic,
            identifier,
            object,
            dyn_type);
    }

    participant_->check_get_type_request(request_sample_id, identifier, object, dyn_type);
}

void DomainParticipantImpl::MyRTPSParticipantListener::on_type_dependencies_reply(
        RTPSParticipant*,
        const fastrtps::rtps::SampleIdentity& request_sample_id,
        const fastrtps::types::TypeIdentifierWithSizeSeq& dependencies)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        participant_->listener_->on_type_dependencies_reply(
            participant_->participant_, request_sample_id, dependencies);
    }

    participant_->check_get_dependencies_request(request_sample_id, dependencies);
}

void DomainParticipantImpl::MyRTPSParticipantListener::on_type_information_received(
        RTPSParticipant*,
        const fastrtps::string_255& topic_name,
        const fastrtps::string_255& type_name,
        const fastrtps::types::TypeInformation& type_information)
{
    if (participant_ != nullptr && participant_->listener_ != nullptr)
    {
        if (type_information.complete().typeid_with_size().type_id()._d() > 0
            || type_information.minimal().typeid_with_size().type_id()._d() > 0)
        {
            participant_->listener_->on_type_information_received(
                participant_->participant_, topic_name, type_name, type_information);
        }
    }
}

bool DomainParticipantImpl::new_remote_endpoint_discovered(
        const GUID_t& partguid,
        uint16_t endpointId,
        EndpointKind_t kind)
{
    if (kind == fastrtps::rtps::WRITER)
    {
        return rtps_participant_->newRemoteWriterDiscovered(partguid, static_cast<int16_t>(endpointId));
    }
    else
    {
        return rtps_participant_->newRemoteReaderDiscovered(partguid, static_cast<int16_t>(endpointId));
    }
}

ResourceEvent& DomainParticipantImpl::get_resource_event() const
{
    return rtps_participant_->get_resource_event();
}

bool DomainParticipantImpl::exists_entity_id(
        const fastrtps::rtps::EntityId_t& entity_id) const
{
    GUID_t g = guid();
    g.entityId = entity_id;
    InstanceHandle_t instance(g);

    return contains_entity(instance, false);
}

fastrtps::rtps::SampleIdentity DomainParticipantImpl::get_type_dependencies(
        const fastrtps::types::TypeIdentifierSeq& in) const
{
    return rtps_participant_->typelookup_manager()->get_type_dependencies(in);
}

fastrtps::rtps::SampleIdentity DomainParticipantImpl::get_types(
        const fastrtps::types::TypeIdentifierSeq& in) const
{
    return rtps_participant_->typelookup_manager()->get_types(in);
}

bool DomainParticipantImpl::register_remote_type(
        const fastrtps::types::TypeInformation& type_information,
        const std::string& type_name,
        std::function<void(const std::string& name, const fastrtps::types::DynamicType_ptr type)>& callback)
{
    using namespace fastrtps::types;
    TypeObjectFactory* factory = TypeObjectFactory::get_instance();
    // Check if plain
    if (type_information.complete().typeid_with_size().type_id()._d() < EK_MINIMAL)
    {
        DynamicType_ptr dyn = factory->build_dynamic_type(
            type_name,
            &type_information.complete().typeid_with_size().type_id());

        if (nullptr != dyn)
        {
            if (register_dynamic_type(dyn))
            {
                //callback(type_name, dyn); // For plain types, don't call the callback
                return true;
            }
            return false;
        }
        // If cannot create the dynamic type, probably is because it depend on unknown types.
        // We must continue.
    }

    // Check if already available
    TypeObject obj;
    factory->typelookup_get_type(
        type_information.complete().typeid_with_size().type_id(),
        obj);

    if (obj._d() != 0)
    {
        DynamicType_ptr dyn = factory->build_dynamic_type(
            type_name,
            &type_information.complete().typeid_with_size().type_id(),
            &obj);

        if (nullptr != dyn)
        {
            if (register_dynamic_type(dyn))
            {
                //callback(type_name, dyn); // If the type is already registered, don't call the callback.
                return true;
            }
            return false;
        }
    }
    else if (rtps_participant_->typelookup_manager() != nullptr)
    {
        TypeIdentifierSeq dependencies;
        TypeIdentifierSeq retrieve_objects;

        fill_pending_dependencies(type_information.complete().dependent_typeids(), dependencies, retrieve_objects);

        fastrtps::rtps::SampleIdentity request_dependencies;
        fastrtps::rtps::SampleIdentity request_objects;

        // Lock now, we don't want to process the reply before we add the requests' ID to the maps.
        std::lock_guard<std::mutex> lock(mtx_request_cb_);

        // If any pending dependency exists, retrieve it.
        if (!dependencies.empty())
        {
            request_dependencies = get_type_dependencies(dependencies);
        }

        // If any pending TypeObject exists, retrieve it
        if (!retrieve_objects.empty())
        {
            request_objects = get_types(retrieve_objects);
        }

        // If no more dependencies but failed to create, probably we only need the TypeObject
        dependencies.clear(); // Reuse the same vector.
        dependencies.push_back(type_information.complete().typeid_with_size().type_id());
        fastrtps::rtps::SampleIdentity requestId = get_types(dependencies);

        // Add everything to maps
        register_callbacks_.emplace(std::make_pair(requestId, std::make_pair(type_name, callback)));
        std::vector<fastrtps::rtps::SampleIdentity> vector;
        vector.push_back(requestId); // Add itself

        if (builtin::INVALID_SAMPLE_IDENTITY != request_dependencies)
        {
            vector.push_back(request_dependencies);
            child_requests_.emplace(std::make_pair(request_dependencies, requestId));
        }

        if (builtin::INVALID_SAMPLE_IDENTITY != request_objects)
        {
            vector.push_back(request_objects);
            child_requests_.emplace(std::make_pair(request_objects, requestId));
        }

        // Move the filled vector to the map
        parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));

        return false;
    }
    return false;
}

bool DomainParticipantImpl::check_get_type_request(
        const fastrtps::rtps::SampleIdentity& requestId,
        const fastrtps::types::TypeIdentifier* identifier,
        const fastrtps::types::TypeObject* object,
        fastrtps::types::DynamicType_ptr dyn_type)
{
    // Maybe we have a pending request?
    if (builtin::INVALID_SAMPLE_IDENTITY != requestId)
    {
        // First level request?
        std::lock_guard<std::mutex> lock(mtx_request_cb_);

        auto cb_it = register_callbacks_.find(requestId);

        if (cb_it != register_callbacks_.end())
        {
            const std::string& name = cb_it->second.first;
            const auto& callback = cb_it->second.second;

            if (nullptr != dyn_type)
            {
                dyn_type->set_name(name);
                if (register_dynamic_type(dyn_type))
                {
                    callback(name, dyn_type);
                    remove_parent_request(requestId);
                    return true;
                }
            }

            // Exists the request, but the provided dyn_type isn't valid.
            // Register the received TypeObject into factory and recreate the DynamicType.
            fastrtps::types::TypeObjectFactory::get_instance()->add_type_object(name, identifier, object);

            auto pending = parent_requests_.find(requestId);
            if (pending != parent_requests_.end() && pending->second.size() < 2) // Exists and everything is solved.
            {
                fastrtps::types::DynamicType_ptr dynamic =
                    fastrtps::types::TypeObjectFactory::get_instance()->build_dynamic_type(name, identifier, object);

                if (nullptr != dynamic)
                {
                    if (register_dynamic_type(dynamic))
                    {
                        callback(name, dynamic);
                        remove_parent_request(requestId);
                        return true;
                    }
                }
            }
            // Failed, cannot register the type yet, probably child request still pending.
            return false;
        }

        // Child request?
        auto child_it = child_requests_.find(requestId);

        if (child_it != child_requests_.end())
        {
            // Register received TypeObject into factory, remove the iterator from the map and check our parent.
            fastrtps::types::TypeObjectFactory::get_instance()->add_type_object(
                get_inner_type_name(requestId), identifier, object);
            remove_child_request(requestId);
        }
    }
    return false;
}

void DomainParticipantImpl::fill_pending_dependencies(
        const fastrtps::types::TypeIdentifierWithSizeSeq& dependencies,
        fastrtps::types::TypeIdentifierSeq& pending_identifiers,
        fastrtps::types::TypeIdentifierSeq& pending_objects) const
{
    using namespace fastrtps::types;
    for (const TypeIdentifierWithSize& tiws : dependencies)
    {
        // Check that we don't know that dependency
        if (!TypeObjectFactory::get_instance()->typelookup_check_type_identifier(tiws.type_id()))
        {
            pending_identifiers.push_back(tiws.type_id());
        }
        // Check if we need to retrieve the TypeObject
        if (tiws.type_id()._d() >= EK_MINIMAL)
        {
            TypeObject obj;
            TypeObjectFactory::get_instance()->typelookup_get_type(tiws.type_id(), obj);
            if (obj._d() == 0)
            {
                // Failed, so we must retrieve it.
                pending_objects.push_back(tiws.type_id());
            }
        }
    }
}

bool DomainParticipantImpl::check_get_dependencies_request(
        const fastrtps::rtps::SampleIdentity& requestId,
        const fastrtps::types::TypeIdentifierWithSizeSeq& dependencies)
{
    using namespace fastrtps::types;

    // Maybe we have a pending request?
    if (builtin::INVALID_SAMPLE_IDENTITY != requestId)
    {
        TypeIdentifierSeq next_dependencies;
        TypeIdentifierSeq retrieve_objects;

        // First level request?
        std::lock_guard<std::mutex> lock(mtx_request_cb_);

        auto cb_it = register_callbacks_.find(requestId);

        if (cb_it != register_callbacks_.end())
        {
            fill_pending_dependencies(dependencies, next_dependencies, retrieve_objects);

            // If any pending dependency exists, retrieve it
            if (!next_dependencies.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_type_dependencies(next_dependencies);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            // Add received dependencies to the factory
            for (const TypeIdentifierWithSize& tiws : dependencies)
            {
                if (tiws.type_id()._d() >= EK_MINIMAL)
                {
                    // This dependency needs a TypeObject
                    retrieve_objects.push_back(tiws.type_id());
                }
                else
                {
                    TypeObjectFactory::get_instance()->add_type_identifier(
                        get_inner_type_name(requestId), &tiws.type_id());
                }
            }

            // If any pending TypeObject exists, retrieve it
            if (!retrieve_objects.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_types(retrieve_objects);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            if (next_dependencies.empty() && retrieve_objects.empty())
            {
                // Finished?
                on_child_requests_finished(requestId);
                return true;
            }

            return false;
        }

        // Child request?
        auto child_it = child_requests_.find(requestId);

        if (child_it != child_requests_.end())
        {
            fill_pending_dependencies(dependencies, next_dependencies, retrieve_objects);

            // If any pending dependency exists, retrieve it
            if (!next_dependencies.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_type_dependencies(next_dependencies);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            // Add received dependencies to the factory
            for (const TypeIdentifierWithSize& tiws : dependencies)
            {
                if (tiws.type_id()._d() >= EK_MINIMAL)
                {
                    // This dependency needs a TypeObject
                    retrieve_objects.push_back(tiws.type_id());
                }
                else
                {
                    TypeObjectFactory::get_instance()->add_type_identifier(
                        get_inner_type_name(requestId), &tiws.type_id());
                }
            }

            // If any pending TypeObject exists, retrieve it
            if (!retrieve_objects.empty())
            {
                fastrtps::rtps::SampleIdentity child_request = get_types(retrieve_objects);
                std::vector<fastrtps::rtps::SampleIdentity> vector;
                vector.push_back(child_request);
                parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
                child_requests_.emplace(std::make_pair(child_request, requestId));
            }

            if (next_dependencies.empty() && retrieve_objects.empty())
            {
                remove_child_request(requestId);
                return true;
            }

            return false;
        }
    }
    return false;
}

bool DomainParticipantImpl::register_dynamic_type(
        fastrtps::types::DynamicType_ptr dyn_type)
{
    TypeSupport type(new fastrtps::types::DynamicPubSubType(dyn_type));
    return participant_->register_type(type);
}

void DomainParticipantImpl::remove_parent_request(
        const fastrtps::rtps::SampleIdentity& request)
{
    // If a parent request if going to be deleted, delete all its children too.
    auto cb_it = register_callbacks_.find(request);
    auto parent_it = parent_requests_.find(request);

    if (parent_requests_.end() != parent_it)
    {
        for (const fastrtps::rtps::SampleIdentity& child_id : parent_it->second)
        {
            auto child_it = child_requests_.find(child_id);
            if (child_requests_.end() != child_it)
            {
                child_requests_.erase(child_it);
            }
        }
        parent_requests_.erase(parent_it);
    }

    if (register_callbacks_.end() != cb_it)
    {
        register_callbacks_.erase(cb_it);
    }
}

void DomainParticipantImpl::remove_child_request(
        const fastrtps::rtps::SampleIdentity& request)
{
    auto child_it = child_requests_.find(request);
    if (child_requests_.end() != child_it)
    {
        fastrtps::rtps::SampleIdentity parent_request = child_it->second;
        child_requests_.erase(child_it);

        auto parent_it = parent_requests_.find(parent_request);
        if (parent_requests_.end() != parent_it)
        {
            std::vector<fastrtps::rtps::SampleIdentity>& pending = parent_it->second;
            pending.erase(std::find(pending.begin(), pending.end(), request));
            if (pending.empty())
            {
                parent_requests_.erase(parent_it);
            }
        }

        on_child_requests_finished(parent_request);
    }
}

void DomainParticipantImpl::on_child_requests_finished(
        const fastrtps::rtps::SampleIdentity& parent)
{
    auto pending_requests_it = parent_requests_.find(parent);
    // Do I have no more pending childs?
    if (parent_requests_.end() == pending_requests_it || pending_requests_it->second.empty())
    {
        // Am I a children?
        auto child_it = child_requests_.find(parent);
        if (child_requests_.end() != child_it)
        {
            remove_child_request(parent);
        }
        else
        {
            // Or a top-level request?
            auto cb_it = register_callbacks_.find(parent);
            if (pending_requests_it->second.size() < 2)
            {
                parent_requests_.erase(pending_requests_it);
            }
            cb_it->second.second(cb_it->second.first, fastrtps::types::DynamicType_ptr(nullptr)); // Everything should be already registered
            register_callbacks_.erase(cb_it);
        }
    }
}

std::string DomainParticipantImpl::get_inner_type_name(
        const fastrtps::rtps::SampleIdentity& id) const
{
    std::stringstream ss;
    ss << "type_" << id.writer_guid() << "_" << id.sequence_number();
    std::string str = ss.str();
    std::transform(str.begin(), str.end(), str.begin(),
       [](unsigned char c)
       {
           return static_cast<char>(std::tolower(c));
       });
    str.erase(std::remove(str.begin(), str.end(), '.'), str.end());
    std::replace(str.begin(), str.end(), '|' ,'_');
    return str;
}
