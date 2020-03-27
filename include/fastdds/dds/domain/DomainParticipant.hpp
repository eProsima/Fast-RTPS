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
 * @file DomainParticipant.hpp
 *
 */

#ifndef _FASTDDS_DOMAIN_PARTICIPANT_HPP_
#define _FASTDDS_DOMAIN_PARTICIPANT_HPP_

#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastrtps/types/TypeIdentifier.h>

#include <fastdds/rtps/common/Guid.h>
#include <fastdds/rtps/common/SampleIdentity.h>
#include <fastdds/rtps/attributes/RTPSParticipantAttributes.h>
#include <fastrtps/types/TypesBase.h>

#include <utility>

using eprosima::fastrtps::types::ReturnCode_t;

namespace eprosima {
namespace fastrtps {
namespace rtps {
class ResourceEvent;
} // namespace rtps

namespace types {
class TypeInformation;
} // namespace types

class ParticipantAttributes;
class PublisherAttributes;
class SubscriberAttributes;

} //namespace fastrtps

namespace fastdds {
namespace dds {

class DomainParticipantImpl;
class DomainParticipantListener;
class Publisher;
class PublisherQos;
class PublisherListener;
class Subscriber;
class SubscriberQos;
class SubscriberListener;

/**
 * Class DomainParticipant used to group Publishers and Subscribers into a single working unit.
 * @ingroup FASTDDS_MODULE
 */
class RTPS_DllAPI DomainParticipant
{
public:

    /**
     * Allows modifying the DomainParticipantListener.
     * @param listener
     * @return true if the listener was updated.
     */
    ReturnCode_t set_listener(
            DomainParticipantListener* listener);

    /**
     * Allows accessing the DomainParticipantListener.
     * @return DomainParticipantListener
     */
    const DomainParticipantListener* get_listener() const;

    /**
     * Create a Publisher in this Participant.
     * @param qos QoS of the Publisher.
     * @param att Attributes of the Publisher. TopicAttributes and WriterQos will be ignored using DDS interface.
     * @param listen Pointer to the listener.
     * @return Pointer to the created Publisher.
     */
    Publisher* create_publisher(
            const fastdds::dds::PublisherQos& qos,
            const fastrtps::PublisherAttributes& att,
            PublisherListener* listen = nullptr);

    /**
     * Deletes an existing Publisher.
     * @param publisher to be deleted.
     * @return if publisher was deleted.
     */
    ReturnCode_t delete_publisher(
            Publisher* publisher);

    /**
     * Create a Subscriber in this Participant.
     * @param qos QoS of the Subscriber.
     * @param att Attributes of the Subscriber. TopicAttributes and ReaderQos will be ignored using DDS interface.
     * @param listen Pointer to the listener.
     * @return Pointer to the created Subscriber.
     */
    Subscriber* create_subscriber(
            const fastdds::dds::SubscriberQos& qos,
            const fastrtps::SubscriberAttributes& att,
            SubscriberListener* listen = nullptr);

    /**
     * Deletes an existing Subscriber.
     * @param subscriber to be deleted.
     * @return if subscriber was deleted.
     */
    ReturnCode_t delete_subscriber(
            Subscriber* subscriber);

    /**
     * Register a type in this participant.
     * @param type TypeSupport.
     * @param type_name The name that will be used to identify the Type.
     * @return True if registered.
     */
    bool register_type(
            TypeSupport type,
            const std::string& type_name);

    /**
     * Register a type in this participant.
     * @param type TypeSupport.
     * @return True if registered.
     */
    bool register_type(
            TypeSupport type);

    /**
     * Unregister a type in this participant.
     * @param typeName Name of the type
     * @return True if unregistered.
     */
    bool unregister_type(
            const char* typeName);

    /**
     * Enable participant and start listening to incoming messages.
     */
    void enable();

    // TODO create/delete topic

    /* TODO
    Subscriber* get_builtin_subscriber();
    */

    /* TODO
    bool ignore_participant(
            const fastrtps::rtps::InstanceHandle_t& handle);
    */

    /* TODO
    bool ignore_topic(
            const fastrtps::rtps::InstanceHandle_t& handle);
    */

    /* TODO
    bool ignore_publication(
            const fastrtps::rtps::InstanceHandle_t& handle);
    */

    /* TODO
    bool ignore_subscription(
            const fastrtps::rtps::InstanceHandle_t& handle);
    */

    /**
     * This operation retrieves the domain_id used to create the DomainParticipant.
     * The domain_id identifies the DDS domain to which the DomainParticipant belongs.
     * @return The Participant's domain_id
     */
    uint8_t get_domain_id() const;

    /* TODO
    bool delete_contained_entities();
    */

    /**
     * This operation manually asserts the liveliness of the DomainParticipant.
     * This is used in combination with the LIVELINESS QoS policy to indicate to the Service that the entity
     * remains active.
     *
     * This operation needs to only be used if the DomainParticipant contains DataWriter entities with
     * the LIVELINESS set to MANUAL_BY_PARTICIPANT and it only affects the liveliness of those DataWriter entities.
     * Otherwise, it has no effect.
     *
     * NOTE: Writing data via the write operation on a DataWriter asserts liveliness on the DataWriter itself and its
     * DomainParticipant. Consequently the use of assert_liveliness is only needed if the application is not
     * writing data regularly.
     * @return if liveliness was asserted.
     */
    ReturnCode_t assert_liveliness();

    /**
     * This operation sets a default value of the Publisher QoS policies which will be used for newly created
     * Publisher entities in the case where the QoS policies are defaulted in the create_publisher operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return false.
     *
     * The special value PUBLISHER_QOS_DEFAULT may be passed to this operation to indicate that the default QoS
     * should be reset back to the initial values the factory would use, that is the values that would be used
     * if the set_default_publisher_qos operation had never been called.
     * @param qos
     * @return if given qos was applied as default.
     */
    ReturnCode_t set_default_publisher_qos(
            const fastdds::dds::PublisherQos& qos);

    /**
     * This operation retrieves the default value of the Publisher QoS, that is, the QoS policies which will be used
     * for newly created Publisher entities in the case where the QoS policies are defaulted in the
     * create_publisher operation.
     *
     * The values retrieved get_default_publisher_qos will match the set of values specified on the last successful
     * call to set_default_publisher_qos, or else, if the call was never made, the default values.
     * @return Current default publisher qos.
     */
    const fastdds::dds::PublisherQos& get_default_publisher_qos() const;

    /**
     * This operation retrieves the default value of the Publisher QoS, that is, the QoS policies which will be used
     * for newly created Publisher entities in the case where the QoS policies are defaulted in the
     * create_publisher operation.
     *
     * The values retrieved get_default_publisher_qos will match the set of values specified on the last successful
     * call to set_default_publisher_qos, or else, if the call was never made, the default values.
     * @param qos
     * @return Always true.
     */
    ReturnCode_t get_default_publisher_qos(
            fastdds::dds::PublisherQos& qos) const;

    /**
     * This operation sets a default value of the Subscriber QoS policies that will be used for newly created
     * Subscriber entities in the case where the QoS policies are defaulted in the create_subscriber operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return false.
     *
     * The special value SUBSCRIBER_QOS_DEFAULT may be passed to this operation to indicate that the default QoS
     * should be reset back to the initial values the factory would use, that is the values that would be used
     * if the set_default_subscriber_qos operation had never been called.
     * @param qos
     * @return if given qos was applied as default.
     */
    ReturnCode_t set_default_subscriber_qos(
            const fastdds::dds::SubscriberQos& qos);

    /**
     * This operation retrieves the default value of the Subscriber QoS, that is, the QoS policies which will be used
     * for newly created Subscriber entities in the case where the QoS policies are defaulted in the
     * create_subscriber operation.
     *
     * The values retrieved get_default_subscriber_qos will match the set of values specified on the last successful
     * call to set_default_subscriber_qos, or else, if the call was never made, the default values.
     * @return Current default subscriber qos.
     */
    const fastdds::dds::SubscriberQos& get_default_subscriber_qos() const;

    /**
     * This operation retrieves the default value of the Subscriber QoS, that is, the QoS policies which will be used
     * for newly created Subscriber entities in the case where the QoS policies are defaulted in the
     * create_subscriber operation.
     *
     * The values retrieved get_default_subscriber_qos will match the set of values specified on the last successful
     * call to set_default_subscriber_qos, or else, if the call was never made, the default values.
     * @param qos
     * @return Always true.
     */
    ReturnCode_t get_default_subscriber_qos(
            fastdds::dds::SubscriberQos& qos) const;

    // TODO Get/Set default Topic Qos

    /* TODO
    bool get_discovered_participants(
            std::vector<fastrtps::rtps::InstanceHandle_t>& participant_handles) const;
    */

    /* TODO
    bool get_discovered_participant_data(
            ParticipantBuiltinTopicData& participant_data,
            const fastrtps::rtps::InstanceHandle_t& participant_handle) const;
    */

    /* TODO
    bool get_discovered_topics(
            std::vector<fastrtps::rtps::InstanceHandle_t>& topic_handles) const;
    */

    /* TODO
    bool get_discovered_topic_data(
            TopicBuiltinTopicData& topic_data,
            const fastrtps::rtps::InstanceHandle_t& topic_handle) const;
    */

    /**
     * This operation checks whether or not the given handle represents an Entity that was created from the
     * DomainParticipant.
     * @param handle InstanceHandle of the entity to look for.
     * @param recursive The containment applies recursively. That is, it applies both to entities
     * (TopicDescription, Publisher, or Subscriber) created directly using the DomainParticipant as well as
     * entities created using a contained Publisher, or Subscriber as the factory, and so forth.
     * @return True if entity is contained. False otherwise.
     */
    bool contains_entity(
            const fastrtps::rtps::InstanceHandle_t& handle,
            bool recursive = true) const;

    /**
     * This operation returns the current value of the time that the service uses to time-stamp data-writes
     * and to set the reception-timestamp for the data-updates it receives.
     * @param current_time
     * @return Always true
     */
    ReturnCode_t get_current_time(
            fastrtps::Time_t& current_time) const;

    /**
     * This method gives access to a registered type based on its name.
     * @param type_name
     * @return TypeSupport
     */
    TypeSupport find_type(
            const std::string& type_name) const;

    /**
     * Returns the DomainParticipant's handle.
     * @return InstanceHandle of this DomainParticipant.
     */
    const fastrtps::rtps::InstanceHandle_t& get_instance_handle() const;

    // From here legacy RTPS methods.

    const fastrtps::rtps::GUID_t& guid() const;

    /**
     * Get the participant attributes
     * @return Participant attributes
     */
    const fastrtps::ParticipantAttributes& get_attributes() const;

    std::vector<std::string> get_participant_names() const;

    /**
     * This method can be used when using a StaticEndpointDiscovery mechanism differnet that the one
     * included in FastRTPS, for example when communicating with other implementations.
     * It indicates the Participant that an Endpoint from the XML has been discovered and
     * should be activated.
     * @param partguid Participant GUID_t.
     * @param userId User defined ID as shown in the XML file.
     * @param kind EndpointKind (WRITER or READER)
     * @return True if correctly found and activated.
     */
    bool new_remote_endpoint_discovered(
            const fastrtps::rtps::GUID_t& partguid,
            uint16_t userId,
            fastrtps::rtps::EndpointKind_t kind);

    fastrtps::rtps::ResourceEvent& get_resource_event() const;

    /**
     * When a DomainParticipant receives an incomplete list of TypeIdentifiers in a
     * PublicationBuiltinTopicData or SubscriptionBuiltinTopicData, it may request the additional type
     * dependencies by invoking the getTypeDependencies operation.
     * @param in
     * @param out
     * @return
     */
    fastrtps::rtps::SampleIdentity get_type_dependencies(
            const fastrtps::types::TypeIdentifierSeq& in) const;

    /**
     * A DomainParticipant may invoke the operation getTypes to retrieve the TypeObjects associated with a
     * list of TypeIdentifiers.
     * @param in
     * @param out
     * @return
     */
    fastrtps::rtps::SampleIdentity get_types(
            const fastrtps::types::TypeIdentifierSeq& in) const;

    /**
     * Helps the user to solve all dependencies calling internally to the typelookup service
     * and registers the resulting dynamic type.
     * The registration will be perform asynchronously and the user will be notified through the
     * given callback, which receives the type_name as unique argument.
     * If the type is already registered, the function will return true, but the callback will not be called.
     * If the given type_information is enought to build the type without using the typelookup service,
     * it will return true and the callback will be never called.
     * @param type_information
     * @param type_name
     * @param callback
     * @return true if type is already available (callback will not be called). false if type isn't available yet
     * (the callback will be called if negotiation is success, and ignored in other case).
     */
    bool register_remote_type(
            const fastrtps::types::TypeInformation& type_information,
            const std::string& type_name,
            std::function<void(const std::string& name, const fastrtps::types::DynamicType_ptr type)>& callback);

private:

    DomainParticipant();

    virtual ~DomainParticipant();

    DomainParticipantImpl* impl_;

    friend class DomainParticipantFactory;

    friend class DomainParticipantImpl;
};

} // namespace dds
} // namespace fastdds
} /* namespace eprosima */

#endif /* _FASTDDS_DOMAIN_PARTICIPANT_HPP_ */
