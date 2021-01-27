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
 * @file PubSubWriter.hpp
 *
 */

#ifndef _TEST_BLACKBOX_PUBSUBWRITER_HPP_
#define _TEST_BLACKBOX_PUBSUBWRITER_HPP_

#include <fastrtps/fastrtps_fwd.h>
#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/participant/ParticipantListener.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/publisher/PublisherListener.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/rtps/common/Locator.h>
#include <fastrtps/rtps/builtin/data/ReaderProxyData.h>
#include <fastrtps/rtps/builtin/data/WriterProxyData.h>
#include <fastrtps/xmlparser/XMLParser.h>
#include <fastrtps/xmlparser/XMLTree.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <string>
#include <list>
#include <map>
#include <condition_variable>
#include <asio.hpp>
#include <gtest/gtest.h>
#include <thread>

using eprosima::fastrtps::rtps::IPLocator;
using eprosima::fastrtps::rtps::UDPv4TransportDescriptor;

template<class TypeSupport>
class PubSubWriter
{
    class ParticipantListener : public eprosima::fastrtps::ParticipantListener
    {
    public:

        ParticipantListener(
                PubSubWriter& writer)
            : writer_(writer)
        {
        }

        ~ParticipantListener()
        {
        }

        void onParticipantDiscovery(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& info) override
        {
            if (writer_.onDiscovery_ != nullptr)
            {
                writer_.discovery_result_ = writer_.onDiscovery_(info);
            }

            if (info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT)
            {
                writer_.participant_matched();
            }
            else if (info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT ||
                    info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT)
            {
                writer_.participant_unmatched();
            }
        }

#if HAVE_SECURITY
        void onParticipantAuthentication(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::ParticipantAuthenticationInfo&& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::ParticipantAuthenticationInfo::AUTHORIZED_PARTICIPANT)
            {
                writer_.authorized();
            }
            else if (info.status == eprosima::fastrtps::rtps::ParticipantAuthenticationInfo::UNAUTHORIZED_PARTICIPANT)
            {
                writer_.unauthorized();
            }
        }

#endif // if HAVE_SECURITY

        void onSubscriberDiscovery(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::ReaderDiscoveryInfo&& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER)
            {
                writer_.add_reader_info(info.info);

            }
            else if (info.status == eprosima::fastrtps::rtps::ReaderDiscoveryInfo::CHANGED_QOS_READER)
            {
                writer_.change_reader_info(info.info);
            }
            else if (info.status == eprosima::fastrtps::rtps::ReaderDiscoveryInfo::REMOVED_READER)
            {
                writer_.remove_reader_info(info.info);
            }
        }

        void onPublisherDiscovery(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::WriterDiscoveryInfo&& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER)
            {
                writer_.add_writer_info(info.info);
            }
            else if (info.status == eprosima::fastrtps::rtps::WriterDiscoveryInfo::CHANGED_QOS_WRITER)
            {
                writer_.change_writer_info(info.info);
            }
            else if (info.status == eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER)
            {
                writer_.remove_writer_info(info.info);
            }
        }

    private:

        ParticipantListener& operator =(
                const ParticipantListener&) = delete;

        PubSubWriter& writer_;

    }
    participant_listener_;

    class Listener : public eprosima::fastrtps::PublisherListener
    {
    public:

        Listener(
                PubSubWriter& writer)
            : writer_(writer)
            , times_deadline_missed_(0)
            , times_liveliness_lost_(0)
        {
        }

        ~Listener()
        {
        }

        void onPublicationMatched(
                eprosima::fastrtps::Publisher* /*pub*/,
                eprosima::fastrtps::rtps::MatchingInfo& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::MATCHED_MATCHING)
            {
                std::cout << "Publisher matched subscriber " << info.remoteEndpointGuid << std::endl;
                writer_.matched();
            }
            else
            {
                std::cout << "Publisher unmatched subscriber " << info.remoteEndpointGuid << std::endl;
                writer_.unmatched();
            }
        }

        void on_offered_deadline_missed(
                eprosima::fastrtps::Publisher* pub,
                const eprosima::fastrtps::OfferedDeadlineMissedStatus& status) override
        {
            (void)pub;
            times_deadline_missed_ = status.total_count;
        }

        void on_liveliness_lost(
                eprosima::fastrtps::Publisher* pub,
                const eprosima::fastrtps::LivelinessLostStatus& status) override
        {
            (void)pub;
            times_liveliness_lost_ = status.total_count;
            writer_.liveliness_lost();
        }

        unsigned int missed_deadlines() const
        {
            return times_deadline_missed_;
        }

        unsigned int times_liveliness_lost() const
        {
            return times_liveliness_lost_;
        }

    private:

        Listener& operator =(
                const Listener&) = delete;

        PubSubWriter& writer_;

        //! The number of times deadline was missed
        unsigned int times_deadline_missed_;
        //! The number of times liveliness was lost
        unsigned int times_liveliness_lost_;

    }
    listener_;

public:

    typedef TypeSupport type_support;
    typedef typename type_support::type type;

    PubSubWriter(
            const std::string& topic_name)
        : participant_listener_(*this)
        , listener_(*this)
        , participant_(nullptr)
        , publisher_(nullptr)
        , initialized_(false)
        , matched_(0)
        , participant_matched_(0)
        , discovery_result_(false)
        , onDiscovery_(nullptr)
        , times_liveliness_lost_(0)
#if HAVE_SECURITY
        , authorized_(0)
        , unauthorized_(0)
#endif // if HAVE_SECURITY
    {
        publisher_attr_.topic.topicDataType = type_.getName();
        // Generate topic name
        std::ostringstream t;
        t << topic_name << "_" << asio::ip::host_name() << "_" << GET_PID();
        publisher_attr_.topic.topicName = t.str();
        topic_name_ = t.str();
        publisher_attr_.topic.topicKind =
                type_.m_isGetKeyDefined ? ::eprosima::fastrtps::rtps::WITH_KEY : ::eprosima::fastrtps::rtps::NO_KEY;

        // By default, only allow participants on the same host
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags =
                eprosima::fastrtps::rtps::FILTER_DIFFERENT_HOST;

        // By default, memory mode is preallocated (the most restritive)
        publisher_attr_.historyMemoryPolicy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;

        // By default, heartbeat period and nack response delay are 100 milliseconds.
        publisher_attr_.times.heartbeatPeriod.seconds = 0;
        publisher_attr_.times.heartbeatPeriod.nanosec = 100000000;
        publisher_attr_.times.nackResponseDelay.seconds = 0;
        publisher_attr_.times.nackResponseDelay.nanosec = 100000000;

        // Increase default max_blocking_time to 1 second, as our CI infrastructure shows some
        // big CPU overhead sometimes
        publisher_attr_.qos.m_reliability.max_blocking_time.seconds = 1;
        publisher_attr_.qos.m_reliability.max_blocking_time.nanosec = 0;
    }

    ~PubSubWriter()
    {
        if (participant_ != nullptr)
        {
            eprosima::fastrtps::Domain::removeParticipant(participant_);
        }
    }

    void init()
    {
        //Create participant
        participant_attr_.domainId = (uint32_t)GET_PID() % 230;

        // Use local copies of attributes to catch #6507 issues with valgrind
        eprosima::fastrtps::ParticipantAttributes participant_attr;
        eprosima::fastrtps::PublisherAttributes publisher_attr;
        participant_attr = participant_attr_;
        publisher_attr = publisher_attr_;

        participant_ = eprosima::fastrtps::Domain::createParticipant(participant_attr, &participant_listener_);

        if (participant_ != nullptr)
        {
            participant_guid_ = participant_->getGuid();

            // Register type
            eprosima::fastrtps::Domain::registerType(participant_, &type_);

            //Create publisher
            publisher_ = eprosima::fastrtps::Domain::createPublisher(participant_, publisher_attr, &listener_);

            if (publisher_ != nullptr)
            {
                publisher_guid_ = publisher_->getGuid();
                std::cout << "Created publisher " << publisher_guid_ << " for topic " <<
                    publisher_attr_.topic.topicName << std::endl;
                initialized_ = true;
                return;
            }

            eprosima::fastrtps::Domain::removeParticipant(participant_);
        }
    }

    void createPublisher()
    {
        if (participant_ != nullptr)
        {
            //Create publisher
            publisher_ = eprosima::fastrtps::Domain::createPublisher(participant_, publisher_attr_, &listener_);

            if (publisher_ != nullptr)
            {
                publisher_guid_ = publisher_->getGuid();
                std::cout << "Created publisher " << publisher_guid_ << " for topic " <<
                    publisher_attr_.topic.topicName << std::endl;
                initialized_ = true;
                return;
            }
        }
        return;
    }

    void removePublisher()
    {
        initialized_ = false;
        eprosima::fastrtps::Domain::removePublisher(publisher_);
        return;
    }

    bool isInitialized() const
    {
        return initialized_;
    }

    eprosima::fastrtps::Participant* getParticipant()
    {
        return participant_;
    }

    void destroy()
    {
        if (participant_ != nullptr)
        {
            eprosima::fastrtps::Domain::removeParticipant(participant_);
            participant_ = nullptr;
        }
    }

    void send(
            std::list<type>& msgs,
            uint32_t milliseconds = 0)
    {
        auto it = msgs.begin();

        while (it != msgs.end())
        {
            if (publisher_->write((void*)&(*it)))
            {
                default_send_print<type>(*it);
                it = msgs.erase(it);
                if (milliseconds > 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
                }
            }
            else
            {
                break;
            }
        }
    }

    eprosima::fastrtps::rtps::InstanceHandle_t register_instance(
            type& msg)
    {
        return publisher_->register_instance((void*)&msg);
    }

    bool unregister_instance(
            type& msg,
            const eprosima::fastrtps::rtps::InstanceHandle_t& instance_handle)
    {
        return publisher_->unregister_instance((void*)&msg, instance_handle);
    }

    bool dispose(
            type& msg,
            const eprosima::fastrtps::rtps::InstanceHandle_t& instance_handle)
    {
        return publisher_->dispose((void*)&msg, instance_handle);
    }

    bool send_sample(
            type& msg)
    {
        return publisher_->write((void*)&msg);
    }

    void assert_liveliness()
    {
        publisher_->assert_liveliness();
    }

    void wait_discovery(
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Writer is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            cv_.wait(lock, [&]()
                    {
                        return matched_ != 0;
                    });
        }
        else
        {
            cv_.wait_for(lock, timeout, [&]()
                    {
                        return matched_ != 0;
                    });
        }

        std::cout << "Writer discovery finished..." << std::endl;
    }

    void wait_discovery(
            unsigned int expected_match,
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Writer is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            cv_.wait(lock, [&]()
                    {
                        return matched_ == expected_match;
                    });
        }
        else
        {
            cv_.wait_for(lock, timeout, [&]()
                    {
                        return matched_ == expected_match;
                    });
        }

        std::cout << "Writer discovery finished..." << std::endl;
    }

    bool wait_participant_undiscovery(
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        bool ret_value = true;
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Writer is waiting undiscovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            cv_.wait(lock, [&]()
                    {
                        return participant_matched_ == 0;
                    });
        }
        else
        {
            if (!cv_.wait_for(lock, timeout, [&]()
                    {
                        return participant_matched_ == 0;
                    }))
            {
                ret_value = false;
            }
        }

        if (ret_value)
        {
            std::cout << "Writer undiscovery finished successfully..." << std::endl;
        }
        else
        {
            std::cout << "Writer undiscovery finished unsuccessfully..." << std::endl;
        }

        return ret_value;
    }

    void wait_reader_undiscovery()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Writer is waiting removal..." << std::endl;

        cv_.wait(lock, [&]()
                {
                    return matched_ == 0;
                });

        std::cout << "Writer removal finished..." << std::endl;
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

    void liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(liveliness_mutex_);
        times_liveliness_lost_++;
        liveliness_cv_.notify_one();
    }

#if HAVE_SECURITY
    void waitAuthorized()
    {
        std::unique_lock<std::mutex> lock(mutexAuthentication_);

        std::cout << "Writer is waiting authorization..." << std::endl;

        cvAuthentication_.wait(lock, [&]() -> bool
                {
                    return authorized_ > 0;
                });

        std::cout << "Writer authorization finished..." << std::endl;
    }

    void waitUnauthorized()
    {
        std::unique_lock<std::mutex> lock(mutexAuthentication_);

        std::cout << "Writer is waiting unauthorization..." << std::endl;

        cvAuthentication_.wait(lock, [&]() -> bool
                {
                    return unauthorized_ > 0;
                });

        std::cout << "Writer unauthorization finished..." << std::endl;
    }

#endif // if HAVE_SECURITY

    template<class _Rep,
            class _Period
            >
    bool waitForAllAcked(
            const std::chrono::duration<_Rep, _Period>& max_wait)
    {
        return publisher_->wait_for_all_acked(eprosima::fastrtps::Time_t((int32_t)max_wait.count(), 0));
    }

    void block_until_discover_topic(
            const std::string& topicName,
            int repeatedTimes)
    {
        std::unique_lock<std::mutex> lock(mutexEntitiesInfoList_);

        cvEntitiesInfoList_.wait(lock, [&]()
                {
                    int times = mapTopicCountList_.count(topicName) == 0 ? 0 : mapTopicCountList_[topicName];
                    return times == repeatedTimes;
                });
    }

    void block_until_discover_partition(
            const std::string& partition,
            int repeatedTimes)
    {
        std::unique_lock<std::mutex> lock(mutexEntitiesInfoList_);

        cvEntitiesInfoList_.wait(lock, [&]()
                {
                    int times = mapPartitionCountList_.count(partition) == 0 ? 0 : mapPartitionCountList_[partition];
                    return times == repeatedTimes;
                });
    }

    /*** Function to change QoS ***/
    PubSubWriter& reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        publisher_attr_.qos.m_reliability.kind = kind;
        return *this;
    }

    PubSubWriter& deadline_period(
            const eprosima::fastrtps::Duration_t deadline_period)
    {
        publisher_attr_.qos.m_deadline.period = deadline_period;
        return *this;
    }

    PubSubWriter& liveliness_kind(
            const eprosima::fastrtps::LivelinessQosPolicyKind kind)
    {
        publisher_attr_.qos.m_liveliness.kind = kind;
        return *this;
    }

    PubSubWriter& liveliness_lease_duration(
            const eprosima::fastrtps::Duration_t lease_duration)
    {
        publisher_attr_.qos.m_liveliness.lease_duration = lease_duration;
        return *this;
    }

    PubSubWriter& latency_budget_duration(
            const eprosima::fastrtps::Duration_t& latency_duration)
    {
        publisher_attr_.qos.m_latencyBudget.duration = latency_duration;
        return *this;
    }

    eprosima::fastrtps::Duration_t get_latency_budget_duration()
    {
        return publisher_attr_.qos.m_latencyBudget.duration;
    }

    PubSubWriter& liveliness_announcement_period(
            const eprosima::fastrtps::Duration_t announcement_period)
    {
        publisher_attr_.qos.m_liveliness.announcement_period = announcement_period;
        return *this;
    }

    PubSubWriter& lifespan_period(
            const eprosima::fastrtps::Duration_t lifespan_period)
    {
        publisher_attr_.qos.m_lifespan.duration = lifespan_period;
        return *this;
    }

    PubSubWriter& keep_duration(
            const eprosima::fastrtps::Duration_t duration)
    {
        publisher_attr_.qos.m_disablePositiveACKs.enabled = true;
        publisher_attr_.qos.m_disablePositiveACKs.duration = duration;
        return *this;
    }

    PubSubWriter& max_blocking_time(
            const eprosima::fastrtps::Duration_t time)
    {
        publisher_attr_.qos.m_reliability.max_blocking_time = time;
        return *this;
    }

    PubSubWriter& add_throughput_controller_descriptor_to_pparams(
            uint32_t bytesPerPeriod,
            uint32_t periodInMs)
    {
        eprosima::fastrtps::rtps::ThroughputControllerDescriptor descriptor {bytesPerPeriod, periodInMs};
        publisher_attr_.throughputController = descriptor;

        return *this;
    }

    PubSubWriter& asynchronously(
            const eprosima::fastrtps::PublishModeQosPolicyKind kind)
    {
        publisher_attr_.qos.m_publishMode.kind = kind;
        return *this;
    }

    PubSubWriter& history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        publisher_attr_.topic.historyQos.kind = kind;
        return *this;
    }

    PubSubWriter& history_depth(
            const int32_t depth)
    {
        publisher_attr_.topic.historyQos.depth = depth;
        return *this;
    }

    PubSubWriter& disable_builtin_transport()
    {
        participant_attr_.rtps.useBuiltinTransports = false;
        return *this;
    }

    PubSubWriter& add_user_transport_to_pparams(
            std::shared_ptr<eprosima::fastrtps::rtps::TransportDescriptorInterface> userTransportDescriptor)
    {
        participant_attr_.rtps.userTransports.push_back(userTransportDescriptor);
        return *this;
    }

    PubSubWriter& durability_kind(
            const eprosima::fastrtps::DurabilityQosPolicyKind kind)
    {
        publisher_attr_.qos.m_durability.kind = kind;
        return *this;
    }

    PubSubWriter& resource_limits_allocated_samples(
            const int32_t initial)
    {
        publisher_attr_.topic.resourceLimitsQos.allocated_samples = initial;
        return *this;
    }

    PubSubWriter& resource_limits_max_samples(
            const int32_t max)
    {
        publisher_attr_.topic.resourceLimitsQos.max_samples = max;
        return *this;
    }

    PubSubWriter& resource_limits_max_instances(
            const int32_t max)
    {
        publisher_attr_.topic.resourceLimitsQos.max_instances = max;
        return *this;
    }

    PubSubWriter& resource_limits_max_samples_per_instance(
            const int32_t max)
    {
        publisher_attr_.topic.resourceLimitsQos.max_samples_per_instance = max;
        return *this;
    }

    PubSubWriter& matched_readers_allocation(
            size_t initial,
            size_t maximum)
    {
        publisher_attr_.matched_subscriber_allocation.initial = initial;
        publisher_attr_.matched_subscriber_allocation.maximum = maximum;
        return *this;
    }

    PubSubWriter& expect_no_allocs()
    {
        // TODO(Mcc): Add no allocations check code when feature is completely ready
        return *this;
    }

    PubSubWriter& heartbeat_period_seconds(
            int32_t sec)
    {
        publisher_attr_.times.heartbeatPeriod.seconds = sec;
        return *this;
    }

    PubSubWriter& heartbeat_period_nanosec(
            uint32_t nanosec)
    {
        publisher_attr_.times.heartbeatPeriod.nanosec = nanosec;
        return *this;
    }

    PubSubWriter& unicastLocatorList(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        publisher_attr_.unicastLocatorList = unicastLocators;
        return *this;
    }

    PubSubWriter& add_to_unicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        publisher_attr_.unicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubWriter& multicastLocatorList(
            eprosima::fastrtps::rtps::LocatorList_t multicastLocators)
    {
        publisher_attr_.multicastLocatorList = multicastLocators;
        return *this;
    }

    PubSubWriter& add_to_multicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        publisher_attr_.multicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubWriter& metatraffic_unicast_locator_list(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        participant_attr_.rtps.builtin.metatrafficUnicastLocatorList = unicastLocators;
        return *this;
    }

    PubSubWriter& add_to_metatraffic_unicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        participant_attr_.rtps.builtin.metatrafficUnicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubWriter& metatraffic_multicast_locator_list(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        participant_attr_.rtps.builtin.metatrafficMulticastLocatorList = unicastLocators;
        return *this;
    }

    PubSubWriter& add_to_metatraffic_multicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        participant_attr_.rtps.builtin.metatrafficMulticastLocatorList.push_back(loc);

        return *this;
    }

    PubSubWriter& initial_peers(
            eprosima::fastrtps::rtps::LocatorList_t initial_peers)
    {
        participant_attr_.rtps.builtin.initialPeersList = initial_peers;
        return *this;
    }

    PubSubWriter& static_discovery(
            const char* filename)
    {
        participant_attr_.rtps.builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = false;
        participant_attr_.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol = true;
        participant_attr_.rtps.builtin.discovery_config.setStaticEndpointXMLFilename(filename);
        return *this;
    }

    PubSubWriter& property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        participant_attr_.rtps.properties = property_policy;
        return *this;
    }

    PubSubWriter& entity_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        publisher_attr_.properties = property_policy;
        return *this;
    }

    PubSubWriter& setPublisherIDs(
            uint8_t UserID,
            uint8_t EntityID)
    {
        publisher_attr_.setUserDefinedID(UserID);
        publisher_attr_.setEntityID(EntityID);
        return *this;
    }

    PubSubWriter& setManualTopicName(
            std::string topicName)
    {
        publisher_attr_.topic.topicName = topicName;
        return *this;
    }

    PubSubWriter& disable_multicast(
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

    PubSubWriter& partition(
            const std::string& partition)
    {
        publisher_attr_.qos.m_partition.push_back(partition.c_str());
        return *this;
    }

    PubSubWriter& userData(
            std::vector<eprosima::fastrtps::rtps::octet> user_data)
    {
        participant_attr_.rtps.userData = user_data;
        return *this;
    }

    PubSubWriter& endpoint_userData(
            std::vector<eprosima::fastrtps::rtps::octet> user_data)
    {
        publisher_attr_.qos.m_userData = user_data;
        return *this;
    }

    PubSubWriter& lease_duration(
            eprosima::fastrtps::Duration_t lease_duration,
            eprosima::fastrtps::Duration_t announce_period)
    {
        participant_attr_.rtps.builtin.discovery_config.leaseDuration = lease_duration;
        participant_attr_.rtps.builtin.discovery_config.leaseDuration_announcementperiod = announce_period;
        return *this;
    }

    PubSubWriter& load_publisher_attr(
            const std::string& xml)
    {
        std::unique_ptr<eprosima::fastrtps::xmlparser::BaseNode> root;
        if (eprosima::fastrtps::xmlparser::XMLParser::loadXML(xml.data(), xml.size(),
                root) == eprosima::fastrtps::xmlparser::XMLP_ret::XML_OK)
        {
            for (const auto& profile : root->getChildren())
            {
                if (profile->getType() == eprosima::fastrtps::xmlparser::NodeType::PUBLISHER)
                {
                    publisher_attr_ =
                            *(dynamic_cast<eprosima::fastrtps::xmlparser::DataNode<eprosima::fastrtps::PublisherAttributes>
                            *>(
                                profile.get())->get());
                }
            }
        }
        return *this;
    }

    PubSubWriter& max_initial_peers_range(
            uint32_t maxInitialPeerRange)
    {
        participant_attr_.rtps.useBuiltinTransports = false;
        std::shared_ptr<UDPv4TransportDescriptor> descriptor = std::make_shared<UDPv4TransportDescriptor>();
        descriptor->maxInitialPeersRange = maxInitialPeerRange;
        participant_attr_.rtps.userTransports.push_back(descriptor);
        return *this;
    }

    PubSubWriter& socket_buffer_size(
            uint32_t sockerBufferSize)
    {
        participant_attr_.rtps.listenSocketBufferSize = sockerBufferSize;
        return *this;
    }

    PubSubWriter& participant_id(
            int32_t participantId)
    {
        participant_attr_.rtps.participantID = participantId;
        return *this;
    }

    PubSubWriter& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags = flags;
        return *this;
    }

    const std::string& topic_name() const
    {
        return topic_name_;
    }

    eprosima::fastrtps::rtps::GUID_t participant_guid()
    {
        return participant_guid_;
    }

    eprosima::fastrtps::rtps::GUID_t datawriter_guid()
    {
        return publisher_guid_;
    }

    bool update_partition(
            const std::string& partition)
    {
        publisher_attr_.qos.m_partition.clear();
        publisher_attr_.qos.m_partition.push_back(partition.c_str());
        return publisher_->updateAttributes(publisher_attr_);
    }

    bool remove_all_changes(
            size_t* number_of_changes_removed)
    {
        return publisher_->removeAllChange(number_of_changes_removed);
    }

    bool is_matched() const
    {
        return matched_ > 0;
    }

    unsigned int missed_deadlines() const
    {
        return listener_.missed_deadlines();
    }

    unsigned int times_liveliness_lost() const
    {
        return listener_.times_liveliness_lost();
    }

private:

    void participant_matched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        ++participant_matched_;
        cv_.notify_one();
    }

    void participant_unmatched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        --participant_matched_;
        cv_.notify_one();
    }

    void matched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        ++matched_;
        cv_.notify_one();
    }

    void unmatched()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);
        --matched_;
        cv_.notify_one();
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

    void add_writer_info(
            const eprosima::fastrtps::rtps::WriterProxyData& writer_data)
    {
        mutexEntitiesInfoList_.lock();
        auto ret = mapWriterInfoList_.insert(std::make_pair(writer_data.guid(), writer_data));

        if (!ret.second)
        {
            ret.first->second = writer_data;
        }

        auto ret_topic = mapTopicCountList_.insert(std::make_pair(writer_data.topicName().to_string(), 1));

        if (!ret_topic.second)
        {
            ++ret_topic.first->second;
        }

        for (auto partition : writer_data.m_qos.m_partition.names())
        {
            auto ret_partition = mapPartitionCountList_.insert(std::make_pair(partition, 1));

            if (!ret_partition.second)
            {
                ++ret_partition.first->second;
            }
        }

        mutexEntitiesInfoList_.unlock();
        cvEntitiesInfoList_.notify_all();
    }

    void change_writer_info(
            const eprosima::fastrtps::rtps::WriterProxyData& writer_data)
    {
        mutexEntitiesInfoList_.lock();
        auto ret = mapWriterInfoList_.insert(std::make_pair(writer_data.guid(), writer_data));

        ASSERT_FALSE(ret.second);
        eprosima::fastrtps::rtps::WriterProxyData old_writer_data = ret.first->second;
        ret.first->second = writer_data;

        ASSERT_GT(mapTopicCountList_.count(writer_data.topicName().to_string()), 0ul);

        // Remove previous partitions
        for (auto partition : old_writer_data.m_qos.m_partition.names())
        {
            auto partition_it = mapPartitionCountList_.find(partition);
            ASSERT_TRUE(partition_it != mapPartitionCountList_.end());
            --(*partition_it).second;
            if ((*partition_it).second == 0)
            {
                mapPartitionCountList_.erase(partition);
            }
        }

        // Add new partitions
        for (auto partition : writer_data.m_qos.m_partition.names())
        {
            auto ret_partition = mapPartitionCountList_.insert(std::make_pair(partition, 1));

            if (!ret_partition.second)
            {
                ++ret_partition.first->second;
            }
        }

        mutexEntitiesInfoList_.unlock();
        cvEntitiesInfoList_.notify_all();
    }

    void add_reader_info(
            const eprosima::fastrtps::rtps::ReaderProxyData& reader_data)
    {
        mutexEntitiesInfoList_.lock();
        auto ret = mapReaderInfoList_.insert(std::make_pair(reader_data.guid(), reader_data));

        if (!ret.second)
        {
            ret.first->second = reader_data;
        }

        auto ret_topic = mapTopicCountList_.insert(std::make_pair(reader_data.topicName().to_string(), 1));

        if (!ret_topic.second)
        {
            ++ret_topic.first->second;
        }

        for (auto partition : reader_data.m_qos.m_partition.names())
        {
            auto ret_partition = mapPartitionCountList_.insert(std::make_pair(partition, 1));

            if (!ret_partition.second)
            {
                ++ret_partition.first->second;
            }
        }

        mutexEntitiesInfoList_.unlock();
        cvEntitiesInfoList_.notify_all();
    }

    void change_reader_info(
            const eprosima::fastrtps::rtps::ReaderProxyData& reader_data)
    {
        mutexEntitiesInfoList_.lock();
        auto ret = mapReaderInfoList_.insert(std::make_pair(reader_data.guid(), reader_data));

        ASSERT_FALSE(ret.second);
        eprosima::fastrtps::rtps::ReaderProxyData old_reader_data = ret.first->second;
        ret.first->second = reader_data;

        ASSERT_GT(mapTopicCountList_.count(reader_data.topicName().to_string()), 0ul);

        // Remove previous partitions
        for (auto partition : old_reader_data.m_qos.m_partition.names())
        {
            auto partition_it = mapPartitionCountList_.find(partition);
            ASSERT_TRUE(partition_it != mapPartitionCountList_.end());
            --(*partition_it).second;
            if ((*partition_it).second == 0)
            {
                mapPartitionCountList_.erase(partition);
            }
        }

        for (auto partition : reader_data.m_qos.m_partition.names())
        {
            auto ret_partition = mapPartitionCountList_.insert(std::make_pair(partition, 1));

            if (!ret_partition.second)
            {
                ++ret_partition.first->second;
            }
        }

        mutexEntitiesInfoList_.unlock();
        cvEntitiesInfoList_.notify_all();
    }

    void remove_writer_info(
            const eprosima::fastrtps::rtps::WriterProxyData& writer_data)
    {
        std::unique_lock<std::mutex> lock(mutexEntitiesInfoList_);

        ASSERT_GT(mapWriterInfoList_.count(writer_data.guid()), 0ul);

        mapWriterInfoList_.erase(writer_data.guid());

        ASSERT_GT(mapTopicCountList_.count(writer_data.topicName().to_string()), 0ul);

        --mapTopicCountList_[writer_data.topicName().to_string()];

        for (auto partition : writer_data.m_qos.m_partition.names())
        {
            auto partition_it = mapPartitionCountList_.find(partition);
            ASSERT_TRUE(partition_it != mapPartitionCountList_.end());
            --(*partition_it).second;
            if ((*partition_it).second == 0)
            {
                mapPartitionCountList_.erase(partition);
            }
        }

        lock.unlock();
        cvEntitiesInfoList_.notify_all();
    }

    void remove_reader_info(
            const eprosima::fastrtps::rtps::ReaderProxyData& reader_data)
    {
        std::unique_lock<std::mutex> lock(mutexEntitiesInfoList_);

        ASSERT_GT(mapReaderInfoList_.count(reader_data.guid()), 0ul);

        mapReaderInfoList_.erase(reader_data.guid());

        ASSERT_GT(mapTopicCountList_.count(reader_data.topicName().to_string()), 0ul);

        --mapTopicCountList_[reader_data.topicName().to_string()];

        for (auto partition : reader_data.m_qos.m_partition.names())
        {
            auto partition_it = mapPartitionCountList_.find(partition);
            ASSERT_TRUE(partition_it != mapPartitionCountList_.end());
            --(*partition_it).second;
            if ((*partition_it).second == 0)
            {
                mapPartitionCountList_.erase(partition);
            }
        }

        lock.unlock();
        cvEntitiesInfoList_.notify_all();
    }

    PubSubWriter& operator =(
            const PubSubWriter&) = delete;

    eprosima::fastrtps::Participant* participant_;
    eprosima::fastrtps::ParticipantAttributes participant_attr_;
    eprosima::fastrtps::Publisher* publisher_;
    eprosima::fastrtps::PublisherAttributes publisher_attr_;
    std::string topic_name_;
    eprosima::fastrtps::rtps::GUID_t participant_guid_;
    eprosima::fastrtps::rtps::GUID_t publisher_guid_;
    bool initialized_;
    std::mutex mutexDiscovery_;
    std::condition_variable cv_;
    std::atomic<unsigned int> matched_;
    unsigned int participant_matched_;
    type_support type_;
    std::mutex mutexEntitiesInfoList_;
    std::condition_variable cvEntitiesInfoList_;
    std::map<eprosima::fastrtps::rtps::GUID_t, eprosima::fastrtps::rtps::WriterProxyData> mapWriterInfoList_;
    std::map<eprosima::fastrtps::rtps::GUID_t, eprosima::fastrtps::rtps::ReaderProxyData> mapReaderInfoList_;
    std::map<std::string,  int> mapTopicCountList_;
    std::map<std::string,  int> mapPartitionCountList_;
    bool discovery_result_;

    std::function<bool(const eprosima::fastrtps::rtps::ParticipantDiscoveryInfo& info)> onDiscovery_;

    //! A mutex for liveliness
    std::mutex liveliness_mutex_;
    //! A condition variable for liveliness
    std::condition_variable liveliness_cv_;
    //! The number of times liveliness was lost
    unsigned int times_liveliness_lost_;

#if HAVE_SECURITY
    std::mutex mutexAuthentication_;
    std::condition_variable cvAuthentication_;
    unsigned int authorized_;
    unsigned int unauthorized_;
#endif // if HAVE_SECURITY
};

#endif // _TEST_BLACKBOX_PUBSUBWRITER_HPP_
