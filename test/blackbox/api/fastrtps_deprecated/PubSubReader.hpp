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
 * @file PubSubReader.hpp
 *
 */

#ifndef _TEST_BLACKBOX_PUBSUBREADER_HPP_
#define _TEST_BLACKBOX_PUBSUBREADER_HPP_

#include <fastrtps/fastrtps_fwd.h>
#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/participant/ParticipantListener.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/subscriber/SubscriberListener.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/xmlparser/XMLParser.h>
#include <fastrtps/xmlparser/XMLTree.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>

#include <string>
#include <list>
#include <atomic>
#include <condition_variable>
#include <asio.hpp>
#include <gtest/gtest.h>

using eprosima::fastrtps::rtps::IPLocator;
using eprosima::fastrtps::rtps::UDPv4TransportDescriptor;

template<class TypeSupport>
class PubSubReader
{
public:

    typedef TypeSupport type_support;
    typedef typename type_support::type type;

private:

    class ParticipantListener : public eprosima::fastrtps::ParticipantListener
    {
    public:

        ParticipantListener(
                PubSubReader& reader)
            : reader_(reader)
        {
        }

        ~ParticipantListener()
        {
        }

        void onParticipantDiscovery(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& info) override
        {
            if (reader_.onDiscovery_ != nullptr)
            {
                std::unique_lock<std::mutex> lock(reader_.mutexDiscovery_);
                reader_.discovery_result_ |= reader_.onDiscovery_(info);
                reader_.cvDiscovery_.notify_one();
            }

            if (info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT)
            {
                reader_.participant_matched();

            }
            else if (info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT ||
                    info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT)
            {
                reader_.participant_unmatched();
            }
        }

        void onPublisherDiscovery(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::WriterDiscoveryInfo&& info) override
        {
            if (reader_.onEndpointDiscovery_ != nullptr)
            {
                std::unique_lock<std::mutex> lock(reader_.mutexDiscovery_);
                reader_.discovery_result_ |= reader_.onEndpointDiscovery_(info);
                reader_.cvDiscovery_.notify_one();
            }
        }

#if HAVE_SECURITY
        void onParticipantAuthentication(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::ParticipantAuthenticationInfo&& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::ParticipantAuthenticationInfo::AUTHORIZED_PARTICIPANT)
            {
                reader_.authorized();
            }
            else if (info.status == eprosima::fastrtps::rtps::ParticipantAuthenticationInfo::UNAUTHORIZED_PARTICIPANT)
            {
                reader_.unauthorized();
            }
        }

#endif // if HAVE_SECURITY

    private:

        ParticipantListener& operator =(
                const ParticipantListener&) = delete;
        PubSubReader& reader_;

    }
    participant_listener_;

    class Listener : public eprosima::fastrtps::SubscriberListener
    {
    public:

        Listener(
                PubSubReader& reader)
            : reader_(reader)
            , times_deadline_missed_(0)
        {
        }

        ~Listener()
        {
        }

        void onNewDataMessage(
                eprosima::fastrtps::Subscriber* sub) override
        {
            ASSERT_NE(sub, nullptr);

            if (reader_.receiving_.load())
            {
                bool ret = false;
                do
                {
                    reader_.receive_one(sub, ret);
                } while (ret);
            }
        }

        void onSubscriptionMatched(
                eprosima::fastrtps::Subscriber* /*sub*/,
                eprosima::fastrtps::rtps::MatchingInfo& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::MATCHED_MATCHING)
            {
                std::cout << "Subscriber matched publisher " << info.remoteEndpointGuid << std::endl;
                reader_.matched();
            }
            else
            {
                std::cout << "Subscriber unmatched publisher " << info.remoteEndpointGuid << std::endl;
                reader_.unmatched();
            }
        }

        void on_requested_deadline_missed(
                eprosima::fastrtps::Subscriber* sub,
                const eprosima::fastrtps::RequestedDeadlineMissedStatus& status) override
        {
            (void)sub;

            times_deadline_missed_ = status.total_count;
        }

        void on_liveliness_changed(
                eprosima::fastrtps::Subscriber* sub,
                const eprosima::fastrtps::LivelinessChangedStatus& status) override
        {
            (void)sub;

            reader_.set_liveliness_changed_status(status);

            if (status.alive_count_change == 1)
            {
                reader_.liveliness_recovered();

            }
            else if (status.not_alive_count_change == 1)
            {
                reader_.liveliness_lost();

            }
        }

        unsigned int missed_deadlines() const
        {
            return times_deadline_missed_;
        }

    private:

        Listener& operator =(
                const Listener&) = delete;

        PubSubReader& reader_;

        //! Number of times deadline was missed
        unsigned int times_deadline_missed_;

    }
    listener_;

    friend class Listener;

public:

    PubSubReader(
            const std::string& topic_name,
            bool take = true)
        : participant_listener_(*this)
        , listener_(*this)
        , participant_(nullptr)
        , subscriber_(nullptr)
        , topic_name_(topic_name)
        , initialized_(false)
        , matched_(0)
        , participant_matched_(0)
        , receiving_(false)
        , current_received_count_(0)
        , number_samples_expected_(0)
        , discovery_result_(false)
        , onDiscovery_(nullptr)
        , onEndpointDiscovery_(nullptr)
        , take_(take)
#if HAVE_SECURITY
        , authorized_(0)
        , unauthorized_(0)
#endif // if HAVE_SECURITY
        , liveliness_mutex_()
        , liveliness_cv_()
        , times_liveliness_lost_(0)
        , times_liveliness_recovered_(0)
    {
        subscriber_attr_.topic.topicDataType = type_.getName();
        // Generate topic name
        std::ostringstream t;
        t << topic_name_ << "_" << asio::ip::host_name() << "_" << GET_PID();
        subscriber_attr_.topic.topicName = t.str();
        subscriber_attr_.topic.topicKind =
                type_.m_isGetKeyDefined ? ::eprosima::fastrtps::rtps::WITH_KEY : ::eprosima::fastrtps::rtps::NO_KEY;

        // By default, only allow participants on the same host
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags =
                eprosima::fastrtps::rtps::FILTER_DIFFERENT_HOST;

        // By default, memory mode is preallocated (the most restritive)
        subscriber_attr_.historyMemoryPolicy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;

        // By default, heartbeat period delay is 100 milliseconds.
        subscriber_attr_.times.heartbeatResponseDelay.seconds = 0;
        subscriber_attr_.times.heartbeatResponseDelay.nanosec = 100000000;
    }

    ~PubSubReader()
    {
        if (participant_ != nullptr)
        {
            eprosima::fastrtps::Domain::removeParticipant(participant_);
        }
    }

    void init()
    {
        participant_attr_.domainId = (uint32_t)GET_PID() % 230;

        // Use local copies of attributes to catch #6507 issues with valgrind
        eprosima::fastrtps::ParticipantAttributes participant_attr;
        eprosima::fastrtps::SubscriberAttributes subscriber_attr;
        participant_attr = participant_attr_;
        subscriber_attr = subscriber_attr_;

        participant_ = eprosima::fastrtps::Domain::createParticipant(participant_attr, &participant_listener_);

        ASSERT_NE(participant_, nullptr);

        participant_guid_ = participant_->getGuid();

        // Register type
        ASSERT_EQ(eprosima::fastrtps::Domain::registerType(participant_, &type_), true);

        //Create subscribe r
        subscriber_ = eprosima::fastrtps::Domain::createSubscriber(participant_, subscriber_attr, &listener_);

        if (subscriber_ != nullptr)
        {
            std::cout << "Created subscriber " << subscriber_->getGuid() << " for topic " <<
                subscriber_attr_.topic.topicName << std::endl;

            initialized_ = true;
        }
    }

    bool isInitialized() const
    {
        return initialized_;
    }

    void destroy()
    {
        if (participant_ != nullptr)
        {
            eprosima::fastrtps::Domain::removeParticipant(participant_);
            participant_ = nullptr;
        }
    }

    std::list<type> data_not_received()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return total_msgs_;
    }

    void startReception(
            std::list<type>& msgs)
    {
        mutex_.lock();
        total_msgs_ = msgs;
        number_samples_expected_ = total_msgs_.size();
        current_received_count_ = 0;
        last_seq = eprosima::fastrtps::rtps::SequenceNumber_t();
        mutex_.unlock();

        bool ret = false;
        do
        {
            receive_one(subscriber_, ret);
        }
        while (ret);

        receiving_.store(true);
    }

    void stopReception()
    {
        receiving_.store(false);
    }

    void block_for_all()
    {
        block([this]() -> bool
                {
                    return number_samples_expected_ == current_received_count_;
                });
    }

    void block_for_seq(
            eprosima::fastrtps::rtps::SequenceNumber_t seq)
    {
        block([this, seq]() -> bool
                {
                    return last_seq == seq;
                });
    }

    size_t block_for_at_least(
            size_t at_least)
    {
        block([this, at_least]() -> bool
                {
                    return current_received_count_ >= at_least;
                });
        return current_received_count_;
    }

    void block(
            std::function<bool()> checker)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, checker);
    }

    template<class _Rep,
            class _Period
            >
    size_t block_for_all(
            const std::chrono::duration<_Rep, _Period>& max_wait)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, max_wait, [this]() -> bool
                {
                    return number_samples_expected_ == current_received_count_;
                });

        return current_received_count_;
    }

    void wait_discovery(
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Reader is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            cvDiscovery_.wait(lock, [&]()
                    {
                        return matched_ != 0;
                    });
        }
        else
        {
            cvDiscovery_.wait_for(lock, timeout, [&]()
                    {
                        return matched_ != 0;
                    });
        }

        std::cout << "Reader discovery finished..." << std::endl;
    }

    bool wait_participant_undiscovery(
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        bool ret_value = true;
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Reader is waiting undiscovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            cvDiscovery_.wait(lock, [&]()
                    {
                        return participant_matched_ == 0;
                    });
        }
        else
        {
            if (!cvDiscovery_.wait_for(lock, timeout, [&]()
                    {
                        return participant_matched_ == 0;
                    }))
            {
                ret_value = false;
            }
        }

        if (ret_value)
        {
            std::cout << "Reader undiscovery finished successfully..." << std::endl;
        }
        else
        {
            std::cout << "Reader undiscovery finished unsuccessfully..." << std::endl;
        }

        return ret_value;
    }

    void wait_writer_undiscovery()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Reader is waiting removal..." << std::endl;

        cvDiscovery_.wait(lock, [&]()
                {
                    return matched_ == 0;
                });

        std::cout << "Reader removal finished..." << std::endl;
    }

    void wait_liveliness_recovered(
            unsigned int times = 1)
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);

        liveliness_cv_.wait(lock, [&]()
                {
                    return times_liveliness_recovered_ >= times;
                });
    }

    void wait_liveliness_lost(
            unsigned int times = 1)
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);

        liveliness_cv_.wait(lock, [&]()
                {
                    return times_liveliness_lost_ >= times;
                });
    }

#if HAVE_SECURITY
    void waitAuthorized()
    {
        std::unique_lock<std::mutex> lock(mutexAuthentication_);

        std::cout << "Reader is waiting authorization..." << std::endl;

        cvAuthentication_.wait(lock, [&]() -> bool
                {
                    return authorized_ > 0;
                });

        std::cout << "Reader authorization finished..." << std::endl;
    }

    void waitUnauthorized()
    {
        std::unique_lock<std::mutex> lock(mutexAuthentication_);

        std::cout << "Reader is waiting unauthorization..." << std::endl;

        cvAuthentication_.wait(lock, [&]() -> bool
                {
                    return unauthorized_ > 0;
                });

        std::cout << "Reader unauthorization finished..." << std::endl;
    }

#endif // if HAVE_SECURITY

    size_t getReceivedCount() const
    {
        return current_received_count_;
    }

    eprosima::fastrtps::rtps::SequenceNumber_t get_last_sequence_received()
    {
        return last_seq;
    }

    /*** Function to change QoS ***/
    PubSubReader& reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        subscriber_attr_.qos.m_reliability.kind = kind;
        return *this;
    }

    PubSubReader& mem_policy(
            const eprosima::fastrtps::rtps::MemoryManagementPolicy mem_policy)
    {
        subscriber_attr_.historyMemoryPolicy = mem_policy;
        return *this;
    }

    PubSubReader& deadline_period(
            const eprosima::fastrtps::Duration_t deadline_period)
    {
        subscriber_attr_.qos.m_deadline.period = deadline_period;
        return *this;
    }

    bool update_deadline_period(
            const eprosima::fastrtps::Duration_t& deadline_period)
    {
        eprosima::fastrtps::SubscriberAttributes attr;
        attr = subscriber_attr_;
        attr.qos.m_deadline.period = deadline_period;

        return subscriber_->updateAttributes(attr);
    }

    PubSubReader& liveliness_kind(
            const eprosima::fastrtps::LivelinessQosPolicyKind& kind)
    {
        subscriber_attr_.qos.m_liveliness.kind = kind;
        return *this;
    }

    PubSubReader& liveliness_lease_duration(
            const eprosima::fastrtps::Duration_t lease_duration)
    {
        subscriber_attr_.qos.m_liveliness.lease_duration = lease_duration;
        return *this;
    }

    PubSubReader& latency_budget_duration(
            const eprosima::fastrtps::Duration_t& latency_duration)
    {
        subscriber_attr_.qos.m_latencyBudget.duration = latency_duration;
        return *this;
    }

    eprosima::fastrtps::Duration_t get_latency_budget_duration()
    {
        return subscriber_attr_.qos.m_latencyBudget.duration;
    }

    PubSubReader& lifespan_period(
            const eprosima::fastrtps::Duration_t lifespan_period)
    {
        subscriber_attr_.qos.m_lifespan.duration = lifespan_period;
        return *this;
    }

    PubSubReader& keep_duration(
            const eprosima::fastrtps::Duration_t duration)
    {
        subscriber_attr_.qos.m_disablePositiveACKs.enabled = true;
        subscriber_attr_.qos.m_disablePositiveACKs.duration = duration;
        return *this;
    }

    PubSubReader& history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        subscriber_attr_.topic.historyQos.kind = kind;
        return *this;
    }

    PubSubReader& history_depth(
            const int32_t depth)
    {
        subscriber_attr_.topic.historyQos.depth = depth;
        return *this;
    }

    PubSubReader& disable_builtin_transport()
    {
        participant_attr_.rtps.useBuiltinTransports = false;
        return *this;
    }

    PubSubReader& add_user_transport_to_pparams(
            std::shared_ptr<eprosima::fastrtps::rtps::TransportDescriptorInterface> userTransportDescriptor)
    {
        participant_attr_.rtps.userTransports.push_back(userTransportDescriptor);
        return *this;
    }

    PubSubReader& resource_limits_allocated_samples(
            const int32_t initial)
    {
        subscriber_attr_.topic.resourceLimitsQos.allocated_samples = initial;
        return *this;
    }

    PubSubReader& resource_limits_max_samples(
            const int32_t max)
    {
        subscriber_attr_.topic.resourceLimitsQos.max_samples = max;
        return *this;
    }

    PubSubReader& resource_limits_max_instances(
            const int32_t max)
    {
        subscriber_attr_.topic.resourceLimitsQos.max_instances = max;
        return *this;
    }

    PubSubReader& resource_limits_max_samples_per_instance(
            const int32_t max)
    {
        subscriber_attr_.topic.resourceLimitsQos.max_samples_per_instance = max;
        return *this;
    }

    PubSubReader& matched_writers_allocation(
            size_t initial,
            size_t maximum)
    {
        subscriber_attr_.matched_publisher_allocation.initial = initial;
        subscriber_attr_.matched_publisher_allocation.maximum = maximum;
        return *this;
    }

    PubSubReader& expect_no_allocs()
    {
        // TODO(Mcc): Add no allocations check code when feature is completely ready
        return *this;
    }

    PubSubReader& heartbeatResponseDelay(
            const int32_t secs,
            const int32_t frac)
    {
        subscriber_attr_.times.heartbeatResponseDelay.seconds = secs;
        subscriber_attr_.times.heartbeatResponseDelay.fraction(frac);
        return *this;
    }

    PubSubReader& unicastLocatorList(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        subscriber_attr_.unicastLocatorList = unicastLocators;
        return *this;
    }

    PubSubReader& add_to_unicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        subscriber_attr_.unicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubReader& multicastLocatorList(
            eprosima::fastrtps::rtps::LocatorList_t multicastLocators)
    {
        subscriber_attr_.multicastLocatorList = multicastLocators;
        return *this;
    }

    PubSubReader& add_to_multicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        subscriber_attr_.multicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubReader& metatraffic_unicast_locator_list(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        participant_attr_.rtps.builtin.metatrafficUnicastLocatorList = unicastLocators;
        return *this;
    }

    PubSubReader& add_to_metatraffic_unicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        participant_attr_.rtps.builtin.metatrafficUnicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubReader& metatraffic_multicast_locator_list(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        participant_attr_.rtps.builtin.metatrafficMulticastLocatorList = unicastLocators;
        return *this;
    }

    PubSubReader& add_to_metatraffic_multicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        participant_attr_.rtps.builtin.metatrafficMulticastLocatorList.push_back(loc);

        return *this;
    }

    PubSubReader& initial_peers(
            eprosima::fastrtps::rtps::LocatorList_t initial_peers)
    {
        participant_attr_.rtps.builtin.initialPeersList = initial_peers;
        return *this;
    }

    PubSubReader& socket_buffer_size(
            uint32_t sockerBufferSize)
    {
        participant_attr_.rtps.listenSocketBufferSize = sockerBufferSize;
        return *this;
    }

    PubSubReader& durability_kind(
            const eprosima::fastrtps::DurabilityQosPolicyKind kind)
    {
        subscriber_attr_.qos.m_durability.kind = kind;
        return *this;
    }

    PubSubReader& static_discovery(
            const char* filename)
    {
        participant_attr_.rtps.builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = false;
        participant_attr_.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol = true;
        participant_attr_.rtps.builtin.discovery_config.setStaticEndpointXMLFilename(filename);
        return *this;
    }

    PubSubReader& setSubscriberIDs(
            uint8_t UserID,
            uint8_t EntityID)
    {
        subscriber_attr_.setUserDefinedID(UserID);
        subscriber_attr_.setEntityID(EntityID);
        return *this;

    }

    PubSubReader& setManualTopicName(
            std::string topicName)
    {
        subscriber_attr_.topic.topicName = topicName;
        return *this;
    }

    PubSubReader& disable_multicast(
            int32_t participantId)
    {
        participant_attr_.rtps.participantID = participantId;

        eprosima::fastrtps::rtps::LocatorList_t default_unicast_locators;
        eprosima::fastrtps::rtps::Locator_t default_unicast_locator;

        default_unicast_locators.push_back(default_unicast_locator);
        participant_attr_.rtps.builtin.metatrafficUnicastLocatorList = default_unicast_locators;

        eprosima::fastrtps::rtps::Locator_t loopback_locator;
        IPLocator::setIPv4(loopback_locator, 127, 0, 0, 1);
        participant_attr_.rtps.builtin.initialPeersList.push_back(loopback_locator);
        return *this;
    }

    PubSubReader& property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        participant_attr_.rtps.properties = property_policy;
        return *this;
    }

    PubSubReader& entity_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        subscriber_attr_.properties = property_policy;
        return *this;
    }

    PubSubReader& partition(
            const std::string& partition)
    {
        subscriber_attr_.qos.m_partition.push_back(partition.c_str());
        return *this;
    }

    PubSubReader& userData(
            std::vector<eprosima::fastrtps::rtps::octet> user_data)
    {
        participant_attr_.rtps.userData = user_data;
        return *this;
    }

    PubSubReader& lease_duration(
            eprosima::fastrtps::Duration_t lease_duration,
            eprosima::fastrtps::Duration_t announce_period)
    {
        participant_attr_.rtps.builtin.discovery_config.leaseDuration = lease_duration;
        participant_attr_.rtps.builtin.discovery_config.leaseDuration_announcementperiod = announce_period;
        return *this;
    }

    PubSubReader& load_participant_attr(
            const std::string& xml)
    {
        std::unique_ptr<eprosima::fastrtps::xmlparser::BaseNode> root;
        if (eprosima::fastrtps::xmlparser::XMLParser::loadXML(xml.data(), xml.size(),
                root) == eprosima::fastrtps::xmlparser::XMLP_ret::XML_OK)
        {
            for (const auto& profile : root->getChildren())
            {
                if (profile->getType() == eprosima::fastrtps::xmlparser::NodeType::PARTICIPANT)
                {
                    participant_attr_ =
                            *(dynamic_cast<eprosima::fastrtps::xmlparser::DataNode<eprosima::fastrtps::ParticipantAttributes>
                            *>(
                                profile.get())->get());
                }
            }
        }
        return *this;
    }

    PubSubReader& load_subscriber_attr(
            const std::string& xml)
    {
        std::unique_ptr<eprosima::fastrtps::xmlparser::BaseNode> root;
        if (eprosima::fastrtps::xmlparser::XMLParser::loadXML(xml.data(), xml.size(),
                root) == eprosima::fastrtps::xmlparser::XMLP_ret::XML_OK)
        {
            for (const auto& profile : root->getChildren())
            {
                if (profile->getType() == eprosima::fastrtps::xmlparser::NodeType::SUBSCRIBER)
                {
                    subscriber_attr_ =
                            *(dynamic_cast<eprosima::fastrtps::xmlparser::DataNode<eprosima::fastrtps::SubscriberAttributes>
                            *>(
                                profile.get())->get());
                }
            }
        }
        return *this;
    }

    PubSubReader& max_initial_peers_range(
            uint32_t maxInitialPeerRange)
    {
        participant_attr_.rtps.useBuiltinTransports = false;
        std::shared_ptr<UDPv4TransportDescriptor> descriptor = std::make_shared<UDPv4TransportDescriptor>();
        descriptor->maxInitialPeersRange = maxInitialPeerRange;
        participant_attr_.rtps.userTransports.push_back(descriptor);
        return *this;
    }

    PubSubReader& participant_id(
            int32_t participantId)
    {
        participant_attr_.rtps.participantID = participantId;
        return *this;
    }

    PubSubReader& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags = flags;
        return *this;
    }

    PubSubReader& user_data_max_size(
            size_t size)
    {
        participant_attr_.rtps.allocation.data_limits.max_user_data = size;
        return *this;
    }

    PubSubReader& properties_max_size(
            size_t max_properties)
    {
        participant_attr_.rtps.allocation.data_limits.max_properties = max_properties;
        return *this;
    }

    PubSubReader& partitions_max_size(
            size_t max_partitions)
    {
        participant_attr_.rtps.allocation.data_limits.max_partitions = max_partitions;
        return *this;
    }

    bool update_partition(
            const std::string& partition)
    {
        subscriber_attr_.qos.m_partition.clear();
        subscriber_attr_.qos.m_partition.push_back(partition.c_str());
        return subscriber_->updateAttributes(subscriber_attr_);
    }

    /*** Function for discovery callback ***/

    void wait_discovery_result()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Reader is waiting discovery result..." << std::endl;

        cvDiscovery_.wait(lock, [&]()
                {
                    return discovery_result_;
                });

        std::cout << "Reader gets discovery result..." << std::endl;
    }

    void setOnDiscoveryFunction(
            std::function<bool(const eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&)> f)
    {
        onDiscovery_ = f;
    }

    void setOnEndpointDiscoveryFunction(
            std::function<bool(const eprosima::fastrtps::rtps::WriterDiscoveryInfo&)> f)
    {
        onEndpointDiscovery_ = f;
    }

    bool takeNextData(
            void* data)
    {
        eprosima::fastrtps::SampleInfo_t info;
        if (subscriber_->takeNextData(data, &info))
        {
            current_received_count_++;
            return true;
        }
        return false;
    }

    unsigned int missed_deadlines() const
    {
        return listener_.missed_deadlines();
    }

    void liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);
        times_liveliness_lost_++;
        liveliness_cv_.notify_one();
    }

    void liveliness_recovered()
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);
        times_liveliness_recovered_++;
        liveliness_cv_.notify_one();
    }

    void set_liveliness_changed_status(
            const eprosima::fastrtps::LivelinessChangedStatus& status)
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);

        liveliness_changed_status_ = status;
    }

    unsigned int times_liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);

        return times_liveliness_lost_;
    }

    unsigned int times_liveliness_recovered()
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);

        return times_liveliness_recovered_;
    }

    const eprosima::fastrtps::LivelinessChangedStatus& liveliness_changed_status()
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);

        return liveliness_changed_status_;
    }

    bool is_matched() const
    {
        return matched_ > 0;
    }

private:

    const eprosima::fastrtps::rtps::GUID_t& participant_guid() const
    {
        return participant_guid_;
    }

private:

    void receive_one(
            eprosima::fastrtps::Subscriber* subscriber,
            bool& returnedValue)
    {
        returnedValue = false;
        type data;
        eprosima::fastrtps::SampleInfo_t info;

        bool success = take_ ?
                subscriber->takeNextData((void*)&data, &info) :
                subscriber->readNextData((void*)&data, &info);
        if (success)
        {
            returnedValue = true;

            std::unique_lock<std::mutex> lock(mutex_);

            // Check order of changes.
            ASSERT_LT(last_seq, info.sample_identity.sequence_number());
            last_seq = info.sample_identity.sequence_number();

            if (info.sampleKind == eprosima::fastrtps::rtps::ALIVE)
            {
                auto it = std::find(total_msgs_.begin(), total_msgs_.end(), data);
                ASSERT_NE(it, total_msgs_.end());
                total_msgs_.erase(it);
                ++current_received_count_;
                default_receive_print<type>(data);
                cv_.notify_one();
            }
        }
    }

    void participant_matched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        ++participant_matched_;
        cvDiscovery_.notify_one();
    }

    void participant_unmatched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        --participant_matched_;
        cvDiscovery_.notify_one();
    }

    void matched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        ++matched_;
        cvDiscovery_.notify_one();
    }

    void unmatched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        --matched_;
        cvDiscovery_.notify_one();
    }

#if HAVE_SECURITY
    void authorized()
    {
        mutexAuthentication_.lock();
        ++authorized_;
        mutexAuthentication_.unlock();
        cvAuthentication_.notify_all();
    }

    void unauthorized()
    {
        mutexAuthentication_.lock();
        ++unauthorized_;
        mutexAuthentication_.unlock();
        cvAuthentication_.notify_all();
    }

#endif // if HAVE_SECURITY

    PubSubReader& operator =(
            const PubSubReader&) = delete;

    eprosima::fastrtps::Participant* participant_;
    eprosima::fastrtps::ParticipantAttributes participant_attr_;
    eprosima::fastrtps::Subscriber* subscriber_;
    eprosima::fastrtps::SubscriberAttributes subscriber_attr_;
    std::string topic_name_;
    eprosima::fastrtps::rtps::GUID_t participant_guid_;
    bool initialized_;
    std::list<type> total_msgs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::mutex mutexDiscovery_;
    std::condition_variable cvDiscovery_;
    std::atomic<unsigned int> matched_;
    unsigned int participant_matched_;
    std::atomic<bool> receiving_;
    type_support type_;
    eprosima::fastrtps::rtps::SequenceNumber_t last_seq;
    size_t current_received_count_;
    size_t number_samples_expected_;
    bool discovery_result_;

    std::function<bool(const eprosima::fastrtps::rtps::ParticipantDiscoveryInfo& info)> onDiscovery_;
    std::function<bool(const eprosima::fastrtps::rtps::WriterDiscoveryInfo& info)> onEndpointDiscovery_;

    //! True to take data from history. False to read
    bool take_;

#if HAVE_SECURITY
    std::mutex mutexAuthentication_;
    std::condition_variable cvAuthentication_;
    unsigned int authorized_;
    unsigned int unauthorized_;
#endif // if HAVE_SECURITY

    //! A mutex for liveliness status
    std::mutex liveliness_mutex_;
    //! A condition variable to notify when liveliness was recovered
    std::condition_variable liveliness_cv_;
    //! Number of times liveliness was lost
    unsigned int times_liveliness_lost_;
    //! Number of times liveliness was recovered
    unsigned int times_liveliness_recovered_;
    //! The liveliness changed status
    eprosima::fastrtps::LivelinessChangedStatus liveliness_changed_status_;
};

#endif // _TEST_BLACKBOX_PUBSUBREADER_HPP_
