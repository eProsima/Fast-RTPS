// Copyright 2016, 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
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

using DomainParticipantFactory = eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastrtps::rtps::IPLocator;
using eprosima::fastrtps::rtps::UDPv4TransportDescriptor;

template<class TypeSupport>
class PubSubWriter
{
    class ParticipantListener : public eprosima::fastdds::dds::DomainParticipantListener
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

        void on_participant_discovery(
                eprosima::fastdds::dds::DomainParticipant*,
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
                eprosima::fastdds::dds::DomainParticipant*,
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

        void on_subscriber_discovery(
                eprosima::fastdds::dds::DomainParticipant*,
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

        void on_publisher_discovery(
                eprosima::fastdds::dds::DomainParticipant*,
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

    class Listener : public eprosima::fastdds::dds::DataWriterListener
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

        void on_publication_matched(
                eprosima::fastdds::dds::DataWriter* /*datawriter*/,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info) override
        {
            if (0 < info.current_count_change)
            {
                std::cout << "Publisher matched subscriber " << info.last_subscription_handle << std::endl;
                writer_.matched();
            }
            else
            {
                std::cout << "Publisher unmatched subscriber " << info.last_subscription_handle << std::endl;
                writer_.unmatched();
            }
        }

        void on_offered_deadline_missed(
                eprosima::fastdds::dds::DataWriter* datawriter,
                const eprosima::fastrtps::OfferedDeadlineMissedStatus& status) override
        {
            (void)datawriter;
            times_deadline_missed_ = status.total_count;
        }

        void on_offered_incompatible_qos(
                eprosima::fastdds::dds::DataWriter* datawriter,
                const eprosima::fastdds::dds::OfferedIncompatibleQosStatus& status) override
        {
            (void)datawriter;
            writer_.incompatible_qos(status);
        }

        void on_liveliness_lost(
                eprosima::fastdds::dds::DataWriter* datawriter,
                const eprosima::fastrtps::LivelinessLostStatus& status) override
        {
            (void)datawriter;
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
        , topic_(nullptr)
        , publisher_(nullptr)
        , datawriter_(nullptr)
        , status_mask_(eprosima::fastdds::dds::StatusMask::all())
        , initialized_(false)
        , matched_(0)
        , participant_matched_(0)
        , discovery_result_(false)
        , onDiscovery_(nullptr)
        , times_liveliness_lost_(0)
        , times_incompatible_qos_(0)
        , last_incompatible_qos_(eprosima::fastdds::dds::INVALID_QOS_POLICY_ID)

#if HAVE_SECURITY
        , authorized_(0)
        , unauthorized_(0)
#endif // if HAVE_SECURITY
    {
        // Generate topic name
        std::ostringstream t;
        t << topic_name << "_" << asio::ip::host_name() << "_" << GET_PID();
        topic_name_ = t.str();

        // By default, only allow participants on the same host
        participant_qos_.wire_protocol().builtin.discovery_config.ignoreParticipantFlags =
                eprosima::fastrtps::rtps::FILTER_DIFFERENT_HOST;

        // By default, memory mode is preallocated (the most restritive)
        datawriter_qos_.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;

        // By default, heartbeat period and nack response delay are 100 milliseconds.
        datawriter_qos_.reliable_writer_qos().times.heartbeatPeriod.seconds = 0;
        datawriter_qos_.reliable_writer_qos().times.heartbeatPeriod.nanosec = 100000000;
        datawriter_qos_.reliable_writer_qos().times.nackResponseDelay.seconds = 0;
        datawriter_qos_.reliable_writer_qos().times.nackResponseDelay.nanosec = 100000000;

        // Increase default max_blocking_time to 1 second, as our CI infrastructure shows some
        // big CPU overhead sometimes
        datawriter_qos_.reliability().max_blocking_time.seconds = 1;
        datawriter_qos_.reliability().max_blocking_time.nanosec = 0;
    }

    ~PubSubWriter()
    {
        destroy();
    }

    void init()
    {
        if (!xml_file_.empty())
        {
            DomainParticipantFactory::get_instance()->load_XML_profiles_file(xml_file_);
            if (!participant_profile_.empty())
            {
                participant_ = DomainParticipantFactory::get_instance()->create_participant_with_profile(
                    (uint32_t)GET_PID() % 230,
                    participant_profile_,
                    &participant_listener_,
                    eprosima::fastdds::dds::StatusMask::none());
                ASSERT_NE(participant_, nullptr);
                ASSERT_TRUE(participant_->is_enabled());
            }
        }
        if (participant_ == nullptr)
        {
            participant_ = DomainParticipantFactory::get_instance()->create_participant(
                (uint32_t)GET_PID() % 230,
                participant_qos_,
                &participant_listener_,
                eprosima::fastdds::dds::StatusMask::none());
        }

        if (participant_ != nullptr)
        {
            participant_guid_ = participant_->guid();

            type_.reset(new type_support());

            // Register type
            ASSERT_EQ(participant_->register_type(type_), ReturnCode_t::RETCODE_OK);

            // Create topic
            topic_ = participant_->create_topic(topic_name_, type_->getName(),
                            eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
            ASSERT_NE(topic_, nullptr);
            ASSERT_TRUE(topic_->is_enabled());

            // Create publisher
            createPublisher();
        }
        return;
    }

    void createPublisher()
    {
        if (participant_ != nullptr)
        {
            // Create publisher
            publisher_ = participant_->create_publisher(publisher_qos_);
            ASSERT_NE(publisher_, nullptr);
            ASSERT_TRUE(publisher_->is_enabled());

            if (!xml_file_.empty())
            {
                if (!datawriter_profile_.empty())
                {
                    datawriter_ = publisher_->create_datawriter_with_profile(topic_, datawriter_profile_, &listener_,
                                    status_mask_);
                    ASSERT_NE(datawriter_, nullptr);
                    ASSERT_TRUE(datawriter_->is_enabled());
                }
            }
            if (datawriter_ == nullptr)
            {
                datawriter_ = publisher_->create_datawriter(topic_, datawriter_qos_, &listener_, status_mask_);
            }

            if (datawriter_ != nullptr)
            {
                datawriter_guid_ = datawriter_->guid();
                std::cout << "Created datawriter " << datawriter_guid_ << " for topic " <<
                    topic_name_ << std::endl;

                initialized_ = datawriter_->is_enabled();
            }
        }
        return;
    }

    void removePublisher()
    {
        initialized_ = false;
        if (datawriter_ != nullptr)
        {
            publisher_->delete_datawriter(datawriter_);
        }
        datawriter_ = nullptr;
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        publisher_ = nullptr;
        return;
    }

    bool isInitialized() const
    {
        return initialized_;
    }

    eprosima::fastdds::dds::DomainParticipant* getParticipant()
    {
        return participant_;
    }

    void destroy()
    {
        if (participant_)
        {
            if (datawriter_)
            {
                publisher_->delete_datawriter(datawriter_);
                datawriter_ = nullptr;
            }
            if (publisher_)
            {
                participant_->delete_publisher(publisher_);
                publisher_ = nullptr;
            }
            if (topic_)
            {
                participant_->delete_topic(topic_);
                topic_ = nullptr;
            }
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
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
            if (datawriter_->write((void*)&(*it)))
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
        return datawriter_->register_instance((void*)&msg);
    }

    bool unregister_instance(
            type& msg,
            const eprosima::fastrtps::rtps::InstanceHandle_t& instance_handle)
    {
        return ReturnCode_t::RETCODE_OK == datawriter_->unregister_instance((void*)&msg, instance_handle);
    }

    bool dispose(
            type& msg,
            const eprosima::fastrtps::rtps::InstanceHandle_t& instance_handle)
    {
        return ReturnCode_t::RETCODE_OK == datawriter_->dispose((void*)&msg, instance_handle);
    }

    bool send_sample(
            type& msg)
    {
        return datawriter_->write((void*)&msg);
    }

    void assert_liveliness()
    {
        datawriter_->assert_liveliness();
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

    void wait_incompatible_qos(
            unsigned int times = 1)
    {
        std::unique_lock<std::mutex> lock(incompatible_qos_mutex_);
        incompatible_qos_cv_.wait(lock, [&]()
                {
                    return times_incompatible_qos_ >= times;
                });
    }

    void incompatible_qos(
            eprosima::fastdds::dds::OfferedIncompatibleQosStatus status)
    {
        std::unique_lock<std::mutex> lock(incompatible_qos_mutex_);
        times_incompatible_qos_ = status.total_count;
        last_incompatible_qos_ = status.last_policy_id;
        incompatible_qos_cv_.notify_one();
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
        return (ReturnCode_t::RETCODE_OK ==
               datawriter_->wait_for_acknowledgments(eprosima::fastrtps::Time_t((int32_t)max_wait.count(), 0)));
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

    PubSubWriter& deactivate_status_listener(
            eprosima::fastdds::dds::StatusMask mask)
    {
        status_mask_ &= ~mask;
        return *this;
    }

    PubSubWriter& activate_status_listener(
            eprosima::fastdds::dds::StatusMask mask)
    {
        status_mask_ |= mask;
        return *this;
    }

    PubSubWriter& reset_status_listener()
    {
        status_mask_ = eprosima::fastdds::dds::StatusMask::all();
        return *this;
    }

    /*** Function to change QoS ***/
    PubSubWriter& reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        datawriter_qos_.reliability().kind = kind;
        return *this;
    }

    PubSubWriter& deadline_period(
            const eprosima::fastrtps::Duration_t deadline_period)
    {
        datawriter_qos_.deadline().period = deadline_period;
        return *this;
    }

    PubSubWriter& liveliness_kind(
            const eprosima::fastrtps::LivelinessQosPolicyKind kind)
    {
        datawriter_qos_.liveliness().kind = kind;
        return *this;
    }

    PubSubWriter& liveliness_lease_duration(
            const eprosima::fastrtps::Duration_t lease_duration)
    {
        datawriter_qos_.liveliness().lease_duration = lease_duration;
        return *this;
    }

    PubSubWriter& latency_budget_duration(
            const eprosima::fastrtps::Duration_t& latency_duration)
    {
        datawriter_qos_.latency_budget().duration = latency_duration;
        return *this;
    }

    eprosima::fastrtps::Duration_t get_latency_budget_duration()
    {
        return datawriter_qos_.latency_budget().duration;
    }

    PubSubWriter& liveliness_announcement_period(
            const eprosima::fastrtps::Duration_t announcement_period)
    {
        datawriter_qos_.liveliness().announcement_period = announcement_period;
        return *this;
    }

    PubSubWriter& lifespan_period(
            const eprosima::fastrtps::Duration_t lifespan_period)
    {
        datawriter_qos_.lifespan().duration = lifespan_period;
        return *this;
    }

    PubSubWriter& keep_duration(
            const eprosima::fastrtps::Duration_t duration)
    {
        datawriter_qos_.reliable_writer_qos().disable_positive_acks.enabled = true;
        datawriter_qos_.reliable_writer_qos().disable_positive_acks.duration = duration;
        return *this;
    }

    PubSubWriter& max_blocking_time(
            const eprosima::fastrtps::Duration_t time)
    {
        datawriter_qos_.reliability().max_blocking_time = time;
        return *this;
    }

    PubSubWriter& add_throughput_controller_descriptor_to_pparams(
            uint32_t bytesPerPeriod,
            uint32_t periodInMs)
    {
        eprosima::fastrtps::rtps::ThroughputControllerDescriptor descriptor {bytesPerPeriod, periodInMs};
        datawriter_qos_.throughput_controller() = descriptor;

        return *this;
    }

    PubSubWriter& asynchronously(
            const eprosima::fastrtps::PublishModeQosPolicyKind kind)
    {
        datawriter_qos_.publish_mode().kind = kind;
        return *this;
    }

    PubSubWriter& history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        datawriter_qos_.history().kind = kind;
        return *this;
    }

    PubSubWriter& history_depth(
            const int32_t depth)
    {
        datawriter_qos_.history().depth = depth;
        return *this;
    }

    PubSubWriter& disable_builtin_transport()
    {
        participant_qos_.transport().use_builtin_transports = false;
        return *this;
    }

    PubSubWriter& add_user_transport_to_pparams(
            std::shared_ptr<eprosima::fastrtps::rtps::TransportDescriptorInterface> userTransportDescriptor)
    {
        participant_qos_.transport().user_transports.push_back(userTransportDescriptor);
        return *this;
    }

    PubSubWriter& durability_kind(
            const eprosima::fastrtps::DurabilityQosPolicyKind kind)
    {
        datawriter_qos_.durability().kind = kind;
        return *this;
    }

    PubSubWriter& resource_limits_allocated_samples(
            const int32_t initial)
    {
        datawriter_qos_.resource_limits().allocated_samples = initial;
        return *this;
    }

    PubSubWriter& resource_limits_max_samples(
            const int32_t max)
    {
        datawriter_qos_.resource_limits().max_samples = max;
        return *this;
    }

    PubSubWriter& resource_limits_max_instances(
            const int32_t max)
    {
        datawriter_qos_.resource_limits().max_instances = max;
        return *this;
    }

    PubSubWriter& resource_limits_max_samples_per_instance(
            const int32_t max)
    {
        datawriter_qos_.resource_limits().max_samples_per_instance = max;
        return *this;
    }

    PubSubWriter& matched_readers_allocation(
            size_t initial,
            size_t maximum)
    {
        datawriter_qos_.writer_resource_limits().matched_subscriber_allocation.initial = initial;
        datawriter_qos_.writer_resource_limits().matched_subscriber_allocation.maximum = maximum;
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
        datawriter_qos_.reliable_writer_qos().times.heartbeatPeriod.seconds = sec;
        return *this;
    }

    PubSubWriter& heartbeat_period_nanosec(
            uint32_t nanosec)
    {
        datawriter_qos_.reliable_writer_qos().times.heartbeatPeriod.nanosec = nanosec;
        return *this;
    }

    PubSubWriter& unicastLocatorList(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        datawriter_qos_.endpoint().unicast_locator_list = unicastLocators;
        return *this;
    }

    PubSubWriter& add_to_unicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        datawriter_qos_.endpoint().unicast_locator_list.push_back(loc);

        return *this;
    }

    PubSubWriter& multicastLocatorList(
            eprosima::fastrtps::rtps::LocatorList_t multicastLocators)
    {
        datawriter_qos_.endpoint().multicast_locator_list = multicastLocators;
        return *this;
    }

    PubSubWriter& add_to_multicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        datawriter_qos_.endpoint().multicast_locator_list.push_back(loc);

        return *this;
    }

    PubSubWriter& metatraffic_unicast_locator_list(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        participant_qos_.wire_protocol().builtin.metatrafficUnicastLocatorList = unicastLocators;
        return *this;
    }

    PubSubWriter& add_to_metatraffic_unicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        participant_qos_.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(loc);

        return *this;
    }

    PubSubWriter& metatraffic_multicast_locator_list(
            eprosima::fastrtps::rtps::LocatorList_t unicastLocators)
    {
        participant_qos_.wire_protocol().builtin.metatrafficMulticastLocatorList = unicastLocators;
        return *this;
    }

    PubSubWriter& add_to_metatraffic_multicast_locator_list(
            const std::string& ip,
            uint32_t port)
    {
        eprosima::fastrtps::rtps::Locator_t loc;
        IPLocator::setIPv4(loc, ip);
        loc.port = port;
        participant_qos_.wire_protocol().builtin.metatrafficMulticastLocatorList.push_back(loc);

        return *this;
    }

    PubSubWriter& initial_peers(
            eprosima::fastrtps::rtps::LocatorList_t initial_peers)
    {
        participant_qos_.wire_protocol().builtin.initialPeersList = initial_peers;
        return *this;
    }

    PubSubWriter& static_discovery(
            const char* filename)
    {
        participant_qos_.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = false;
        participant_qos_.wire_protocol().builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol = true;
        participant_qos_.wire_protocol().builtin.discovery_config.setStaticEndpointXMLFilename(filename);
        return *this;
    }

    PubSubWriter& property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        participant_qos_.properties() = property_policy;
        return *this;
    }

    PubSubWriter& entity_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        datawriter_qos_.properties() = property_policy;
        return *this;
    }

    PubSubWriter& setPublisherIDs(
            uint8_t UserID,
            uint8_t EntityID)
    {
        datawriter_qos_.endpoint().user_defined_id = UserID;
        datawriter_qos_.endpoint().entity_id = EntityID;
        return *this;
    }

    PubSubWriter& setManualTopicName(
            std::string topicName)
    {
        topic_name_ = topicName;
        return *this;
    }

    PubSubWriter& disable_multicast(
            int32_t participantId)
    {
        participant_qos_.wire_protocol().participant_id = participantId;

        eprosima::fastrtps::rtps::LocatorList_t default_unicast_locators;
        eprosima::fastrtps::rtps::Locator_t default_unicast_locator;

        default_unicast_locators.push_back(default_unicast_locator);
        participant_qos_.wire_protocol().builtin.metatrafficUnicastLocatorList = default_unicast_locators;

        eprosima::fastrtps::rtps::Locator_t loopback_locator;
        IPLocator::setIPv4(loopback_locator, 127, 0, 0, 1);
        participant_qos_.wire_protocol().builtin.initialPeersList.push_back(loopback_locator);
        return *this;
    }

    PubSubWriter& partition(
            const std::string& partition)
    {
        publisher_qos_.partition().push_back(partition.c_str());
        return *this;
    }

    PubSubWriter& userData(
            std::vector<eprosima::fastrtps::rtps::octet> user_data)
    {
        participant_qos_.user_data() = user_data;
        return *this;
    }

    PubSubWriter& endpoint_userData(
            std::vector<eprosima::fastrtps::rtps::octet> user_data)
    {
        datawriter_qos_.user_data() = user_data;
        return *this;
    }

    PubSubWriter& user_data_max_size(
            uint32_t max_user_data)
    {
        participant_qos_.allocation().data_limits.max_user_data = max_user_data;
        return *this;
    }

    PubSubWriter& properties_max_size(
            uint32_t max_properties)
    {
        participant_qos_.allocation().data_limits.max_properties = max_properties;
        return *this;
    }

    PubSubWriter& partitions_max_size(
            uint32_t max_partitions)
    {
        participant_qos_.allocation().data_limits.max_partitions = max_partitions;
        return *this;
    }

    PubSubWriter& lease_duration(
            eprosima::fastrtps::Duration_t lease_duration,
            eprosima::fastrtps::Duration_t announce_period)
    {
        participant_qos_.wire_protocol().builtin.discovery_config.leaseDuration = lease_duration;
        participant_qos_.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = announce_period;
        return *this;
    }

    PubSubWriter& load_publisher_attr(
            const std::string& /*xml*/)
    {
        /*TODO
           std::unique_ptr<eprosima::fastrtps::xmlparser::BaseNode> root;
           if (eprosima::fastrtps::xmlparser::XMLParser::loadXML(xml.data(), xml.size(),
                root) == eprosima::fastrtps::xmlparser::XMLP_ret::XML_OK)
           {
            for (const auto& profile : root->getChildren())
            {
                if (profile->getType() == eprosima::fastrtps::xmlparser::NodeType::PUBLISHER)
                {
                    datawriter_qos_ =
         *(dynamic_cast<eprosima::fastrtps::xmlparser::DataNode<eprosima::fastrtps::PublisherAttributes>
         *>(
                                profile.get())->get());
                }
            }
           }
         */
        publisher_qos_.partition().push_back("A");
        return *this;
    }

    PubSubWriter& max_initial_peers_range(
            uint32_t maxInitialPeerRange)
    {
        participant_qos_.transport().use_builtin_transports = false;
        std::shared_ptr<UDPv4TransportDescriptor> descriptor = std::make_shared<UDPv4TransportDescriptor>();
        descriptor->maxInitialPeersRange = maxInitialPeerRange;
        participant_qos_.transport().user_transports.push_back(descriptor);
        return *this;
    }

    PubSubWriter& socket_buffer_size(
            uint32_t sockerBufferSize)
    {
        participant_qos_.transport().listen_socket_buffer_size = sockerBufferSize;
        return *this;
    }

    PubSubWriter& participant_id(
            int32_t participantId)
    {
        participant_qos_.wire_protocol().participant_id = participantId;
        return *this;
    }

    PubSubWriter& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_qos_.wire_protocol().builtin.discovery_config.ignoreParticipantFlags = flags;
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
        return datawriter_guid_;
    }

    bool update_partition(
            const std::string& partition)
    {
        publisher_qos_.partition().clear();
        publisher_qos_.partition().push_back(partition.c_str());
        return (ReturnCode_t::RETCODE_OK == publisher_->set_qos(publisher_qos_));
    }

    bool remove_all_changes(
            size_t* number_of_changes_removed)
    {
        return (ReturnCode_t::RETCODE_OK == datawriter_->clear_history(number_of_changes_removed));
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

    unsigned int times_incompatible_qos() const
    {
        return times_incompatible_qos_;
    }

    eprosima::fastdds::dds::QosPolicyId_t last_incompatible_qos() const
    {
        return last_incompatible_qos_;
    }

    eprosima::fastdds::dds::OfferedIncompatibleQosStatus get_incompatible_qos_status() const
    {
        eprosima::fastdds::dds::OfferedIncompatibleQosStatus status;
        datawriter_->get_offered_incompatible_qos_status(status);
        return status;
    }

    void set_xml_filename(
            const std::string& name)
    {
        xml_file_ = name;
    }

    void set_participant_profile(
            const std::string& profile)
    {
        participant_profile_ = profile;
    }

    void set_datawriter_profile(
            const std::string& profile)
    {
        datawriter_profile_ = profile;
    }

#if HAVE_SQLITE3
    PubSubWriter& make_persistent(
            const std::string& filename,
            const std::string& persistence_guid)
    {
        participant_qos_.properties().properties().emplace_back("dds.persistence.plugin", "builtin.SQLITE3");
        participant_qos_.properties().properties().emplace_back("dds.persistence.sqlite3.filename", filename);
        datawriter_qos_.durability().kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
        datawriter_qos_.properties().properties().emplace_back("dds.persistence.guid", persistence_guid);

        return *this;
    }

#endif // if HAVE_SQLITE3

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

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::DomainParticipantQos participant_qos_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::PublisherQos publisher_qos_;
    eprosima::fastdds::dds::DataWriter* datawriter_;
    eprosima::fastdds::dds::DataWriterQos datawriter_qos_;
    eprosima::fastdds::dds::StatusMask status_mask_;
    std::string topic_name_;
    eprosima::fastrtps::rtps::GUID_t participant_guid_;
    eprosima::fastrtps::rtps::GUID_t datawriter_guid_;
    bool initialized_;
    std::mutex mutexDiscovery_;
    std::condition_variable cv_;
    std::atomic<unsigned int> matched_;
    unsigned int participant_matched_;
    eprosima::fastdds::dds::TypeSupport type_;
    std::mutex mutexEntitiesInfoList_;
    std::condition_variable cvEntitiesInfoList_;
    std::map<eprosima::fastrtps::rtps::GUID_t, eprosima::fastrtps::rtps::WriterProxyData> mapWriterInfoList_;
    std::map<eprosima::fastrtps::rtps::GUID_t, eprosima::fastrtps::rtps::ReaderProxyData> mapReaderInfoList_;
    std::map<std::string,  int> mapTopicCountList_;
    std::map<std::string,  int> mapPartitionCountList_;
    bool discovery_result_;

    std::string xml_file_ = "";
    std::string participant_profile_ = "";
    std::string datawriter_profile_ = "";

    std::function<bool(const eprosima::fastrtps::rtps::ParticipantDiscoveryInfo& info)> onDiscovery_;

    //! A mutex for liveliness
    std::mutex liveliness_mutex_;
    //! A condition variable for liveliness
    std::condition_variable liveliness_cv_;
    //! The number of times liveliness was lost
    unsigned int times_liveliness_lost_;

    //! A mutex for incompatible qos
    std::mutex incompatible_qos_mutex_;
    //! A condition variable for incompatible qos
    std::condition_variable incompatible_qos_cv_;
    //! Number of times incompatible_qos was received
    unsigned int times_incompatible_qos_;
    //! Latest conflicting PolicyId
    eprosima::fastdds::dds::QosPolicyId_t last_incompatible_qos_;

#if HAVE_SECURITY
    std::mutex mutexAuthentication_;
    std::condition_variable cvAuthentication_;
    unsigned int authorized_;
    unsigned int unauthorized_;
#endif // if HAVE_SECURITY
};

#endif // _TEST_BLACKBOX_PUBSUBWRITER_HPP_
