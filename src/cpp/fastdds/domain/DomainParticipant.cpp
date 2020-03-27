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
 * @file DomainParticipant.cpp
 *
 */

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/domain/DomainParticipantImpl.hpp>

using namespace eprosima;
using namespace eprosima::fastdds::dds;

DomainParticipant::DomainParticipant()
    : impl_(nullptr)
{
}

DomainParticipant::~DomainParticipant()
{
}

ReturnCode_t DomainParticipant::set_listener(
        DomainParticipantListener* listener)
{
    return impl_->set_listener(listener);
}

const DomainParticipantListener* DomainParticipant::get_listener() const
{
    return impl_->get_listener();
}

Publisher* DomainParticipant::create_publisher(
        const fastdds::dds::PublisherQos& qos,
        const fastrtps::PublisherAttributes& att,
        PublisherListener* listen)
{
    return impl_->create_publisher(qos, att, listen);
}

ReturnCode_t DomainParticipant::delete_publisher(
        Publisher* publisher)
{
    return impl_->delete_publisher(publisher);
}

Subscriber* DomainParticipant::create_subscriber(
        const fastdds::dds::SubscriberQos& qos,
        const fastrtps::SubscriberAttributes& att,
        SubscriberListener* listen)
{
    return impl_->create_subscriber(qos, att, listen);
}

ReturnCode_t DomainParticipant::delete_subscriber(
        Subscriber* subscriber)
{
    return impl_->delete_subscriber(subscriber);
}

bool DomainParticipant::register_type(
        TypeSupport type,
        const std::string& type_name)
{
    return impl_->register_type(type, type_name);
}

bool DomainParticipant::register_type(
        TypeSupport type)
{
    return impl_->register_type(type, type.get_type_name());
}

bool DomainParticipant::unregister_type(
        const char* typeName)
{
    return impl_->unregister_type(typeName);
}


void DomainParticipant::enable()
{
    impl_->enable();
}

/* TODO
Subscriber* DomainParticipant::get_builtin_subscriber()
{
    return impl_->get_builtin_subscriber();
}
*/

/* TODO
bool DomainParticipant::ignore_participant(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    return impl_->ignore_participant(handle);
}
*/

/* TODO
bool DomainParticipant::ignore_topic(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    return impl_->ignore_topic(handle);
}
*/

/* TODO
bool DomainParticipant::ignore_publication(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    return impl_->ignore_publication(handle);
}
*/

/* TODO
bool DomainParticipant::ignore_subscription(
        const fastrtps::rtps::InstanceHandle_t& handle)
{
    return impl_->ignore_subscription(handle);
}
*/

uint8_t DomainParticipant::get_domain_id() const
{
    return impl_->get_domain_id();
}

/* TODO
bool DomainParticipant::delete_contained_entities()
{
    return impl_->delete_contained_entities();
}
*/

ReturnCode_t DomainParticipant::assert_liveliness()
{
    return impl_->assert_liveliness();
}

ReturnCode_t DomainParticipant::set_default_publisher_qos(
        const fastdds::dds::PublisherQos& qos)
{
    return impl_->set_default_publisher_qos(qos);
}

const fastdds::dds::PublisherQos& DomainParticipant::get_default_publisher_qos() const
{
    return impl_->get_default_publisher_qos();
}

ReturnCode_t DomainParticipant::get_default_publisher_qos(
        fastdds::dds::PublisherQos& qos) const
{
    qos = impl_->get_default_publisher_qos();
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipant::set_default_subscriber_qos(
        const fastdds::dds::SubscriberQos& qos)
{
    return impl_->set_default_subscriber_qos(qos);
}

const fastdds::dds::SubscriberQos& DomainParticipant::get_default_subscriber_qos() const
{
    return impl_->get_default_subscriber_qos();
}

ReturnCode_t DomainParticipant::get_default_subscriber_qos(
        fastdds::dds::SubscriberQos& qos) const
{
    qos = impl_->get_default_subscriber_qos();
    return ReturnCode_t::RETCODE_OK;
}

/* TODO
bool DomainParticipant::get_discovered_participants(
        std::vector<fastrtps::rtps::InstanceHandle_t>& participant_handles) const
{
    return impl_->get_discovered_participants(participant_handles);
}
*/

/* TODO
bool DomainParticipant::get_discovered_topics(
        std::vector<fastrtps::rtps::InstanceHandle_t>& topic_handles) const
{
    return impl_->get_discovered_topics(topic_handles);
}
*/

bool DomainParticipant::contains_entity(
        const fastrtps::rtps::InstanceHandle_t& handle,
        bool recursive) const
{
    return impl_->contains_entity(handle, recursive);
}

ReturnCode_t DomainParticipant::get_current_time(
        fastrtps::Time_t& current_time) const
{
    return impl_->get_current_time(current_time);
}

TypeSupport DomainParticipant::find_type(
        const std::string& type_name) const
{
    return impl_->find_type(type_name);
}

const fastrtps::rtps::InstanceHandle_t& DomainParticipant::get_instance_handle() const
{
    return impl_->get_instance_handle();
}

const fastrtps::rtps::GUID_t& DomainParticipant::guid() const
{
    return impl_->guid();
}

const fastrtps::ParticipantAttributes& DomainParticipant::get_attributes() const
{
    return impl_->get_attributes();
}

std::vector<std::string> DomainParticipant::get_participant_names() const
{
    return impl_->get_participant_names();
}

bool DomainParticipant::new_remote_endpoint_discovered(
    const fastrtps::rtps::GUID_t& partguid,
    uint16_t userId,
    fastrtps::rtps::EndpointKind_t kind)
{
    return impl_->new_remote_endpoint_discovered(partguid, userId, kind);
}

fastrtps::rtps::ResourceEvent& DomainParticipant::get_resource_event() const
{
    return impl_->get_resource_event();
}

fastrtps::rtps::SampleIdentity DomainParticipant::get_type_dependencies(
        const fastrtps::types::TypeIdentifierSeq& in) const
{
    return impl_->get_type_dependencies(in);
}

fastrtps::rtps::SampleIdentity DomainParticipant::get_types(
        const fastrtps::types::TypeIdentifierSeq& in) const
{
    return impl_->get_types(in);
}

bool DomainParticipant::register_remote_type(
        const fastrtps::types::TypeInformation& type_information,
        const std::string& type_name,
        std::function<void(const std::string& name, const fastrtps::types::DynamicType_ptr type)>& callback)
{
    return impl_->register_remote_type(type_information, type_name, callback);
}
