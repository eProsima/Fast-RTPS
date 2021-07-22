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
 * @file ParticipantImpl.cpp
 *
 */

#include <fastrtps_deprecated/participant/ParticipantImpl.h>

#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastdds/rtps/RTPSDomain.h>
#include <fastdds/rtps/builtin/liveliness/WLP.h>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.h>
#include <fastdds/rtps/participant/RTPSParticipant.h>
#include <fastdds/rtps/reader/ReaderDiscoveryInfo.h>
#include <fastdds/rtps/writer/WriterDiscoveryInfo.h>
#include <fastdds/rtps/resources/TimedEvent.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/participant/ParticipantListener.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps_deprecated/publisher/PublisherImpl.h>
#include <fastrtps_deprecated/subscriber/SubscriberImpl.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

using eprosima::fastdds::dds::TopicDataType;

ParticipantImpl::ParticipantImpl(
        const ParticipantAttributes& patt,
        Participant* pspart,
        ParticipantListener* listen)
    : m_att(patt)
    , mp_rtpsParticipant(nullptr)
    , mp_participant(pspart)
    , mp_listener(listen)
#pragma warning (disable : 4355 )
    , m_rtps_listener(this)
{
    mp_participant->mp_impl = this;
}

ParticipantImpl::~ParticipantImpl()
{
    while (m_publishers.size() > 0)
    {
        this->removePublisher(m_publishers.begin()->first);
    }
    while (m_subscribers.size() > 0)
    {
        this->removeSubscriber(m_subscribers.begin()->first);
    }

    if (this->mp_rtpsParticipant != nullptr)
    {
        RTPSDomain::removeRTPSParticipant(this->mp_rtpsParticipant);
    }

    delete(mp_participant);
}

bool ParticipantImpl::removePublisher(
        Publisher* pub)
{
    for (auto pit = this->m_publishers.begin(); pit != m_publishers.end(); ++pit)
    {
        if (pit->second->getGuid() == pub->getGuid())
        {
            delete(pit->second);
            m_publishers.erase(pit);
            return true;
        }
    }
    return false;
}

bool ParticipantImpl::removeSubscriber(
        Subscriber* sub)
{
    for (auto sit = m_subscribers.begin(); sit != m_subscribers.end(); ++sit)
    {
        if (sit->second->getGuid() == sub->getGuid())
        {
            delete(sit->second);
            m_subscribers.erase(sit);
            return true;
        }
    }
    return false;
}

const GUID_t& ParticipantImpl::getGuid() const
{
    return this->mp_rtpsParticipant->getGuid();
}

Publisher* ParticipantImpl::createPublisher(
        const PublisherAttributes& att,
        PublisherListener* listen)
{
    logInfo(PARTICIPANT, "CREATING PUBLISHER IN TOPIC: " << att.topic.getTopicName());
    //Look for the correct type registration

    TopicDataType* p_type = nullptr;

    /// Preconditions
    // Check the type was registered.
    if (!getRegisteredType(att.topic.getTopicDataType().c_str(), &p_type))
    {
        logError(PARTICIPANT, "Type : " << att.topic.getTopicDataType() << " Not Registered");
        return nullptr;
    }
    // Check the type supports keys.
    if (att.topic.topicKind == WITH_KEY && !p_type->m_isGetKeyDefined)
    {
        logError(PARTICIPANT, "Keyed Topic needs getKey function");
        return nullptr;
    }

    if (m_att.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol)
    {
        if (att.getUserDefinedID() <= 0)
        {
            logError(PARTICIPANT, "Static EDP requires user defined Id");
            return nullptr;
        }
    }
    if (!att.unicastLocatorList.isValid())
    {
        logError(PARTICIPANT, "Unicast Locator List for Publisher contains invalid Locator");
        return nullptr;
    }
    if (!att.multicastLocatorList.isValid())
    {
        logError(PARTICIPANT, " Multicast Locator List for Publisher contains invalid Locator");
        return nullptr;
    }
    if (!att.remoteLocatorList.isValid())
    {
        logError(PARTICIPANT, "Remote Locator List for Publisher contains invalid Locator");
        return nullptr;
    }
    if (!att.qos.checkQos() || !att.topic.checkQos())
    {
        return nullptr;
    }

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    PublisherImpl* pubimpl = new PublisherImpl(this, p_type, att, listen);
    Publisher* pub = new Publisher(pubimpl);
    pubimpl->mp_userPublisher = pub;
    pubimpl->mp_rtpsParticipant = this->mp_rtpsParticipant;

    WriterAttributes watt;
    watt.throughputController = att.throughputController;
    watt.endpoint.durabilityKind = att.qos.m_durability.durabilityKind();
    watt.endpoint.endpointKind = WRITER;
    watt.endpoint.multicastLocatorList = att.multicastLocatorList;
    watt.endpoint.reliabilityKind = att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS ? RELIABLE : BEST_EFFORT;
    watt.endpoint.topicKind = att.topic.topicKind;
    watt.endpoint.unicastLocatorList = att.unicastLocatorList;
    watt.endpoint.remoteLocatorList = att.remoteLocatorList;
    watt.mode = att.qos.m_publishMode.kind ==
            eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE ? SYNCHRONOUS_WRITER : ASYNCHRONOUS_WRITER;
    watt.endpoint.properties = att.properties;
    watt.flow_controller_name = att.qos.m_publishMode.flow_controller_name;
    if (att.getEntityID() > 0)
    {
        watt.endpoint.setEntityID((uint8_t)att.getEntityID());
    }
    if (att.getUserDefinedID() > 0)
    {
        watt.endpoint.setUserDefinedID((uint8_t)att.getUserDefinedID());
    }
    watt.times = att.times;
    watt.liveliness_kind = att.qos.m_liveliness.kind;
    watt.liveliness_lease_duration = att.qos.m_liveliness.lease_duration;
    watt.liveliness_announcement_period = att.qos.m_liveliness.announcement_period;
    watt.matched_readers_allocation = att.matched_subscriber_allocation;

    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(att.topic.getTopicName().c_str());
    watt.endpoint.properties.properties().push_back(std::move(property));
    if (att.qos.m_partition.names().size() > 0)
    {
        property.name("partitions");
        std::string partitions;
        bool is_first_partition = true;
        for (auto partition : att.qos.m_partition.names())
        {
            partitions += (is_first_partition ? "" : ";") + partition;
            is_first_partition = false;
        }
        property.value(std::move(partitions));
        watt.endpoint.properties.properties().push_back(std::move(property));
    }
    if (att.qos.m_disablePositiveACKs.enabled &&
            att.qos.m_disablePositiveACKs.duration != c_TimeInfinite)
    {
        watt.disable_positive_acks = true;
        watt.keep_duration = att.qos.m_disablePositiveACKs.duration;
    }

    RTPSWriter* writer = RTPSDomain::createRTPSWriter(
        this->mp_rtpsParticipant,
        watt, pubimpl->payload_pool(),
        (WriterHistory*)&pubimpl->m_history,
        (WriterListener*)&pubimpl->m_writerListener);
    if (writer == nullptr)
    {
        logError(PARTICIPANT, "Problem creating associated Writer");
        delete(pubimpl);
        return nullptr;
    }
    pubimpl->mp_writer = writer;

    // In case it has been loaded from the persistence DB, rebuild instances on history
    pubimpl->m_history.rebuild_instances();
    if (att.qos.m_lifespan.duration != c_TimeInfinite)
    {
        if (pubimpl->lifespan_expired())
        {
            pubimpl->lifespan_timer_->restart_timer();
        }
    }

    //SAVE THE PUBLISHER PAIR
    t_p_PublisherPair pubpair;
    pubpair.first = pub;
    pubpair.second = pubimpl;
    m_publishers.push_back(pubpair);

    //REGISTER THE WRITER
    this->mp_rtpsParticipant->registerWriter(writer, att.topic, att.qos);

    return pub;
}

std::vector<std::string> ParticipantImpl::getParticipantNames() const
{
    return mp_rtpsParticipant->getParticipantNames();
}

Subscriber* ParticipantImpl::createSubscriber(
        const SubscriberAttributes& att,
        SubscriberListener* listen)
{
    logInfo(PARTICIPANT, "CREATING SUBSCRIBER IN TOPIC: " << att.topic.getTopicName());
    //Look for the correct type registration

    TopicDataType* p_type = nullptr;

    if (!getRegisteredType(att.topic.getTopicDataType().c_str(), &p_type))
    {
        logError(PARTICIPANT, "Type : " << att.topic.getTopicDataType() << " Not Registered");
        return nullptr;
    }
    if (att.topic.topicKind == WITH_KEY && !p_type->m_isGetKeyDefined)
    {
        logError(PARTICIPANT, "Keyed Topic needs getKey function");
        return nullptr;
    }
    if (m_att.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol)
    {
        if (att.getUserDefinedID() <= 0)
        {
            logError(PARTICIPANT, "Static EDP requires user defined Id");
            return nullptr;
        }
    }
    if (!att.unicastLocatorList.isValid())
    {
        logError(PARTICIPANT, "Unicast Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }
    if (!att.multicastLocatorList.isValid())
    {
        logError(PARTICIPANT, " Multicast Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }
    if (!att.remoteLocatorList.isValid())
    {
        logError(PARTICIPANT, "Output Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }
    if (!att.qos.checkQos() || !att.topic.checkQos())
    {
        return nullptr;
    }

    SubscriberImpl* subimpl = new SubscriberImpl(this, p_type, att, listen);
    Subscriber* sub = new Subscriber(subimpl);
    subimpl->mp_userSubscriber = sub;
    subimpl->mp_rtpsParticipant = this->mp_rtpsParticipant;

    ReaderAttributes ratt;
    ratt.endpoint.durabilityKind = att.qos.m_durability.durabilityKind();
    ratt.endpoint.endpointKind = READER;
    ratt.endpoint.multicastLocatorList = att.multicastLocatorList;
    ratt.endpoint.reliabilityKind = att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS ? RELIABLE : BEST_EFFORT;
    ratt.endpoint.topicKind = att.topic.topicKind;
    ratt.endpoint.unicastLocatorList = att.unicastLocatorList;
    ratt.endpoint.remoteLocatorList = att.remoteLocatorList;
    ratt.expectsInlineQos = att.expectsInlineQos;
    ratt.endpoint.properties = att.properties;
    if (att.getEntityID() > 0)
    {
        ratt.endpoint.setEntityID((uint8_t)att.getEntityID());
    }
    if (att.getUserDefinedID() > 0)
    {
        ratt.endpoint.setUserDefinedID((uint8_t)att.getUserDefinedID());
    }
    ratt.times = att.times;
    ratt.matched_writers_allocation = att.matched_publisher_allocation;
    ratt.liveliness_kind_ = att.qos.m_liveliness.kind;
    ratt.liveliness_lease_duration = att.qos.m_liveliness.lease_duration;

    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(att.topic.getTopicName().c_str());
    ratt.endpoint.properties.properties().push_back(std::move(property));
    if (att.qos.m_partition.names().size() > 0)
    {
        property.name("partitions");
        std::string partitions;
        bool is_first_partition = true;
        for (auto partition : att.qos.m_partition.names())
        {
            partitions += (is_first_partition ? "" : ";") + partition;
            is_first_partition = false;
        }
        property.value(std::move(partitions));
        ratt.endpoint.properties.properties().push_back(std::move(property));
    }
    if (att.qos.m_disablePositiveACKs.enabled)
    {
        ratt.disable_positive_acks = true;
    }

    RTPSReader* reader = RTPSDomain::createRTPSReader(this->mp_rtpsParticipant,
                    ratt, subimpl->payload_pool(),
                    (ReaderHistory*)&subimpl->m_history,
                    (ReaderListener*)&subimpl->m_readerListener);
    if (reader == nullptr)
    {
        logError(PARTICIPANT, "Problem creating associated Reader");
        delete(subimpl);
        return nullptr;
    }
    subimpl->mp_reader = reader;
    //SAVE THE PUBLICHER PAIR
    t_p_SubscriberPair subpair;
    subpair.first = sub;
    subpair.second = subimpl;
    m_subscribers.push_back(subpair);

    //REGISTER THE READER
    this->mp_rtpsParticipant->registerReader(reader, att.topic, att.qos);

    return sub;
}

bool ParticipantImpl::getRegisteredType(
        const char* typeName,
        TopicDataType** type)
{
    for (std::vector<TopicDataType*>::iterator it = m_types.begin();
            it != m_types.end(); ++it)
    {
        if (strcmp((*it)->getName(), typeName) == 0)
        {
            *type = *it;
            return true;
        }
    }
    return false;
}

bool ParticipantImpl::registerType(
        TopicDataType* type)
{
    if (type->m_typeSize <= 0)
    {
        logError(PARTICIPANT, "Registered Type must have maximum byte size > 0");
        return false;
    }
    const char* name = type->getName();
    if (strlen(name) <= 0)
    {
        logError(PARTICIPANT, "Registered Type must have a name");
        return false;
    }
    for (auto ty = m_types.begin(); ty != m_types.end(); ++ty)
    {
        if (strcmp((*ty)->getName(), type->getName()) == 0)
        {
            logError(PARTICIPANT, "Type with the same name already exists:" << type->getName());
            return false;
        }
    }
    m_types.push_back(type);
    logInfo(PARTICIPANT, "Type " << type->getName() << " registered.");
    return true;
}

bool ParticipantImpl::unregisterType(
        const char* typeName)
{
    bool retValue = false;
    std::vector<TopicDataType*>::iterator typeit;

    for (typeit = m_types.begin(); typeit != m_types.end(); ++typeit)
    {
        if (strcmp((*typeit)->getName(), typeName) == 0)
        {
            break;
        }
    }

    if (typeit != m_types.end())
    {
        bool inUse = false;

        for (auto sit = m_subscribers.begin(); !inUse && sit != m_subscribers.end(); ++sit)
        {
            if (strcmp(sit->second->getType()->getName(), typeName) == 0)
            {
                inUse = true;
            }
        }

        for (auto pit = m_publishers.begin(); pit != m_publishers.end(); ++pit)
        {
            if (strcmp(pit->second->getType()->getName(), typeName) == 0)
            {
                inUse = true;
            }
        }

        if (!inUse)
        {
            m_types.erase(typeit);
            retValue = true;
        }
    }

    return retValue;
}

void ParticipantImpl::MyRTPSParticipantListener::onParticipantDiscovery(
        RTPSParticipant*,
        rtps::ParticipantDiscoveryInfo&& info)
{
    if (this->mp_participantimpl->mp_listener != nullptr)
    {
        this->mp_participantimpl->mp_listener->onParticipantDiscovery(mp_participantimpl->mp_participant, std::move(
                    info));
    }
}

#if HAVE_SECURITY
void ParticipantImpl::MyRTPSParticipantListener::onParticipantAuthentication(
        RTPSParticipant*,
        ParticipantAuthenticationInfo&& info)
{
    if (this->mp_participantimpl->mp_listener != nullptr)
    {
        this->mp_participantimpl->mp_listener->onParticipantAuthentication(mp_participantimpl->mp_participant, std::move(
                    info));
    }
}

#endif // if HAVE_SECURITY

void ParticipantImpl::MyRTPSParticipantListener::onReaderDiscovery(
        RTPSParticipant*,
        rtps::ReaderDiscoveryInfo&& info)
{
    if (this->mp_participantimpl->mp_listener != nullptr)
    {
        this->mp_participantimpl->mp_listener->onSubscriberDiscovery(mp_participantimpl->mp_participant,
                std::move(info));
    }
}

void ParticipantImpl::MyRTPSParticipantListener::onWriterDiscovery(
        RTPSParticipant*,
        rtps::WriterDiscoveryInfo&& info)
{
    if (this->mp_participantimpl->mp_listener != nullptr)
    {
        this->mp_participantimpl->mp_listener->onPublisherDiscovery(mp_participantimpl->mp_participant,
                std::move(info));
    }
}

bool ParticipantImpl::newRemoteEndpointDiscovered(
        const GUID_t& partguid,
        uint16_t endpointId,
        EndpointKind_t kind)
{
    if (kind == WRITER)
    {
        return this->mp_rtpsParticipant->newRemoteWriterDiscovered(partguid, endpointId);
    }
    else
    {
        return this->mp_rtpsParticipant->newRemoteReaderDiscovered(partguid, endpointId);
    }
}

ResourceEvent& ParticipantImpl::get_resource_event() const
{
    return mp_rtpsParticipant->get_resource_event();
}

void ParticipantImpl::assert_liveliness()
{
    if (mp_rtpsParticipant->wlp() != nullptr)
    {
        mp_rtpsParticipant->wlp()->assert_liveliness_manual_by_participant();
    }
    else
    {
        logError(PARTICIPANT, "Invalid WLP, cannot assert liveliness of participant");
    }
}

const Participant* ParticipantImpl::get_participant() const
{
    return mp_participant;
}
