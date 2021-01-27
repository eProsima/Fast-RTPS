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
 * @file PubSubParticipant.hpp
 *
 */

#ifndef _TEST_BLACKBOX_PUBSUBPARTICIPANT_HPP_
#define _TEST_BLACKBOX_PUBSUBPARTICIPANT_HPP_

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

#include <asio.hpp>
#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <tuple>

/**
 * @brief A class with one participant that can have multiple publishers and subscribers
 */
template<class TypeSupport>
class PubSubParticipant
{
    class PubListener : public eprosima::fastdds::dds::DataWriterListener
    {
        friend class PubSubParticipant;

    public:

        PubListener(
                PubSubParticipant* participant)
            : participant_(participant)
        {
        }

        ~PubListener()
        {
        }

        void on_publication_matched(
                eprosima::fastdds::dds::DataWriter*,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info) override
        {
            (0 < info.current_count_change) ? participant_->pub_matched() : participant_->pub_unmatched();
        }

        void on_liveliness_lost(
                eprosima::fastdds::dds::DataWriter*,
                const eprosima::fastdds::dds::LivelinessLostStatus& status) override
        {
            (void)status;
            participant_->pub_liveliness_lost();
        }

    private:

        PubListener& operator =(
                const PubListener&) = delete;
        //! A pointer to the participant
        PubSubParticipant* participant_;
    };

    class SubListener : public eprosima::fastdds::dds::DataReaderListener
    {
        friend class PubSubParticipant;

    public:

        SubListener(
                PubSubParticipant* participant)
            : participant_(participant)
        {
        }

        ~SubListener()
        {
        }

        void on_subscription_matched(
                eprosima::fastdds::dds::DataReader*,
                const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override
        {
            (0 < info.current_count_change) ? participant_->sub_matched() : participant_->sub_unmatched();
        }

        void on_liveliness_changed(
                eprosima::fastdds::dds::DataReader*,
                const eprosima::fastdds::dds::LivelinessChangedStatus& status) override
        {
            (status.alive_count_change ==
            1) ? participant_->sub_liveliness_recovered() : participant_->sub_liveliness_lost();

        }

    private:

        SubListener& operator =(
                const SubListener&) = delete;
        //! A pointer to the participant
        PubSubParticipant* participant_;
    };

public:

    typedef TypeSupport type_support;
    typedef typename type_support::type type;

    PubSubParticipant(
            unsigned int num_publishers,
            unsigned int num_subscribers,
            unsigned int num_expected_publishers,
            unsigned int num_expected_subscribers)
        : participant_(nullptr)
        , num_publishers_(num_publishers)
        , num_subscribers_(num_subscribers)
        , num_expected_subscribers_(num_expected_subscribers)
        , num_expected_publishers_(num_expected_publishers)
        , publishers_(num_publishers)
        , subscribers_(num_subscribers)
        , pub_listener_(this)
        , sub_listener_(this)
        , pub_matched_(0)
        , sub_matched_(0)
        , pub_times_liveliness_lost_(0)
        , sub_times_liveliness_lost_(0)
        , sub_times_liveliness_recovered_(0)
    {
        // By default, only allow participants on the same host
        participant_qos_.wire_protocol().builtin.discovery_config.ignoreParticipantFlags =
                eprosima::fastrtps::rtps::FILTER_DIFFERENT_HOST;

#if defined(PREALLOCATED_WITH_REALLOC_MEMORY_MODE_TEST)
        datawriter_qos_.historyMemoryPolicy = rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
#elif defined(DYNAMIC_RESERVE_MEMORY_MODE_TEST)
        datawriter_qos_.historyMemoryPolicy = rtps::DYNAMIC_RESERVE_MEMORY_MODE;
#else
        datawriter_qos_.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;
#endif // if defined(PREALLOCATED_WITH_REALLOC_MEMORY_MODE_TEST)

        // By default, heartbeat period and nack response delay are 100 milliseconds.
        datawriter_qos_.reliable_writer_qos().times.heartbeatPeriod.seconds = 0;
        datawriter_qos_.reliable_writer_qos().times.heartbeatPeriod.nanosec = 100000000;
        datawriter_qos_.reliable_writer_qos().times.nackResponseDelay.seconds = 0;
        datawriter_qos_.reliable_writer_qos().times.nackResponseDelay.nanosec = 100000000;

        // Increase default max_blocking_time to 1 second, as our CI infrastructure shows some
        // big CPU overhead sometimes
        datawriter_qos_.reliability().max_blocking_time.seconds = 1;
        datawriter_qos_.reliability().max_blocking_time.nanosec = 0;

        // By default, heartbeat period delay is 100 milliseconds.
        datareader_qos_.reliable_reader_qos().times.heartbeatResponseDelay = 0.1;
    }

    ~PubSubParticipant()
    {
        if (participant_ != nullptr)
        {
            for (auto p : publishers_)
            {
                if (std::get<1>(p))
                {
                    if (std::get<2>(p))
                    {
                        std::get<1>(p)->delete_datawriter(std::get<2>(p));
                    }
                    participant_->delete_publisher(std::get<1>(p));
                    participant_->delete_topic(std::get<0>(p));
                }
            }
            publishers_.clear();
            for (auto s : subscribers_)
            {
                if (std::get<1>(s))
                {
                    if (std::get<2>(s))
                    {
                        std::get<1>(s)->delete_datareader(std::get<2>(s));
                    }
                    participant_->delete_subscriber(std::get<1>(s));
                    participant_->delete_topic(std::get<0>(s));
                }
            }
            subscribers_.clear();

            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    bool init_participant()
    {
        participant_ = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(
            (uint32_t)GET_PID() % 230,
            participant_qos_);

        if (participant_ != nullptr)
        {
            type_.reset(new type_support());
            participant_->register_type(type_);
            return true;
        }

        return false;
    }

    bool init_publisher(
            unsigned int index)
    {
        if (participant_ == nullptr)
        {
            return false;
        }
        if (index >= num_publishers_)
        {
            return false;
        }

        eprosima::fastdds::dds::Topic* topic =
                dynamic_cast<eprosima::fastdds::dds::Topic*>(participant_->lookup_topicdescription(
                    publisher_topicname_));

        if (topic == nullptr)
        {
            topic = participant_->create_topic(publisher_topicname_,
                            type_->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        }
        if (topic)
        {
            auto publisher = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT);
            if (publisher)
            {
                auto datawriter = publisher->create_datawriter(topic, datawriter_qos_, &pub_listener_);
                if (datawriter && datawriter->is_enabled())
                {
                    publishers_[index] = {topic, publisher, datawriter};
                    return true;
                }
            }
        }
        return false;
    }

    bool init_subscriber(
            unsigned int index)
    {
        if (participant_ == nullptr)
        {
            return false;
        }
        if (index >= num_subscribers_)
        {
            return false;
        }

        eprosima::fastdds::dds::Topic* topic =
                dynamic_cast<eprosima::fastdds::dds::Topic*>(participant_->lookup_topicdescription(
                    subscriber_topicname_));

        if (topic == nullptr)
        {
            topic = participant_->create_topic(subscriber_topicname_,
                            type_->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        }
        if (topic)
        {
            auto subscriber = participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT);
            if (subscriber)
            {
                auto datareader = subscriber->create_datareader(topic, datareader_qos_, &sub_listener_);
                if (datareader && datareader->is_enabled())
                {
                    subscribers_[index] = {topic, subscriber, datareader};
                    return true;
                }
            }
        }
        return false;
    }

    bool send_sample(
            type& msg,
            unsigned int index = 0)
    {
        return std::get<2>(publishers_[index])->write((void*)&msg);
    }

    void assert_liveliness_participant()
    {
        participant_->assert_liveliness();
    }

    void assert_liveliness(
            unsigned int index = 0)
    {
        std::get<2>(publishers_[index])->assert_liveliness();
    }

    void pub_wait_discovery(
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(pub_mutex_);

        std::cout << "Publisher is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            pub_cv_.wait(lock, [&]()
                    {
                        return pub_matched_ == num_expected_publishers_;
                    });
        }
        else
        {
            pub_cv_.wait_for(lock, timeout, [&]()
                    {
                        return pub_matched_ == num_expected_publishers_;
                    });
        }

        std::cout << "Publisher discovery finished " << std::endl;
    }

    void pub_wait_discovery(
            unsigned int expected_match,
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(pub_mutex_);

        std::cout << "Publisher is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            pub_cv_.wait(lock, [&]()
                    {
                        return pub_matched_ == expected_match;
                    });
        }
        else
        {
            pub_cv_.wait_for(lock, timeout, [&]()
                    {
                        return pub_matched_ == expected_match;
                    });
        }

        std::cout << "Publisher discovery finished " << std::endl;
    }

    void sub_wait_discovery(
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(sub_mutex_);

        std::cout << "Subscriber is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            sub_cv_.wait(lock, [&]()
                    {
                        return sub_matched_ == num_expected_subscribers_;
                    });
        }
        else
        {
            sub_cv_.wait_for(lock, timeout, [&]()
                    {
                        return sub_matched_ == num_expected_subscribers_;
                    });
        }

        std::cout << "Subscriber discovery finished " << std::endl;
    }

    void sub_wait_discovery(
            unsigned int expected_match,
            std::chrono::seconds timeout = std::chrono::seconds::zero())
    {
        std::unique_lock<std::mutex> lock(sub_mutex_);

        std::cout << "Subscriber is waiting discovery..." << std::endl;

        if (timeout == std::chrono::seconds::zero())
        {
            sub_cv_.wait(lock, [&]()
                    {
                        return sub_matched_ == expected_match;
                    });
        }
        else
        {
            sub_cv_.wait_for(lock, timeout, [&]()
                    {
                        return sub_matched_ == expected_match;
                    });
        }

        std::cout << "Subscriber discovery finished " << std::endl;
    }

    void pub_wait_liveliness_lost(
            unsigned int times = 1)
    {
        std::unique_lock<std::mutex> lock(pub_liveliness_mutex_);
        pub_liveliness_cv_.wait(lock, [&]()
                {
                    return pub_times_liveliness_lost_ >= times;
                });
    }

    void sub_wait_liveliness_recovered(
            unsigned int num_recovered)
    {
        std::unique_lock<std::mutex> lock(sub_liveliness_mutex_);
        sub_liveliness_cv_.wait(lock, [&]()
                {
                    return sub_times_liveliness_recovered_ >= num_recovered;
                });
    }

    void sub_wait_liveliness_lost(
            unsigned int num_lost)
    {
        std::unique_lock<std::mutex> lock(sub_liveliness_mutex_);
        sub_liveliness_cv_.wait(lock, [&]()
                {
                    return sub_times_liveliness_lost_ >= num_lost;
                });
    }

    PubSubParticipant& property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        participant_qos_.properties() = property_policy;
        return *this;
    }

    PubSubParticipant& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_qos_.wire_protocol().builtin.discovery_config.ignoreParticipantFlags = flags;
        return *this;
    }

    PubSubParticipant& pub_topic_name(
            std::string topicName)
    {
        // Generate topic name
        std::ostringstream t;
        t << topicName << "_" << asio::ip::host_name() << "_" << GET_PID();
        publisher_topicname_ = t.str();
        return *this;
    }

    PubSubParticipant& sub_topic_name(
            std::string topicName)
    {
        // Generate topic name
        std::ostringstream t;
        t << topicName << "_" << asio::ip::host_name() << "_" << GET_PID();
        subscriber_topicname_ = t.str();
        return *this;
    }

    PubSubParticipant& reliability(
            const eprosima::fastdds::dds::ReliabilityQosPolicyKind kind)
    {
        datawriter_qos_.reliability().kind = kind;
        datareader_qos_.reliability().kind = kind;
        return *this;
    }

    PubSubParticipant& pub_liveliness_kind(
            const eprosima::fastdds::dds::LivelinessQosPolicyKind kind)
    {
        datawriter_qos_.liveliness().kind = kind;
        return *this;
    }

    PubSubParticipant& pub_liveliness_lease_duration(
            const Duration_t lease_duration)
    {
        datawriter_qos_.liveliness().lease_duration = lease_duration;
        return *this;
    }

    PubSubParticipant& pub_liveliness_announcement_period(
            const Duration_t announcement_period)
    {
        datawriter_qos_.liveliness().announcement_period = announcement_period;
        return *this;
    }

    PubSubParticipant& sub_liveliness_kind(
            const eprosima::fastdds::dds::LivelinessQosPolicyKind& kind)
    {
        datareader_qos_.liveliness().kind = kind;
        return *this;
    }

    PubSubParticipant& sub_liveliness_lease_duration(
            const Duration_t lease_duration)
    {
        datareader_qos_.liveliness().lease_duration = lease_duration;
        return *this;
    }

    PubSubParticipant& pub_deadline_period(
            const Duration_t& deadline_period)
    {
        datawriter_qos_.deadline().period = deadline_period;
        return *this;
    }

    PubSubParticipant& sub_deadline_period(
            const Duration_t& deadline_period)
    {
        datareader_qos_.deadline().period = deadline_period;
        return *this;
    }

    bool sub_update_deadline_period(
            const Duration_t& deadline_period,
            unsigned int index)
    {
        if (index >= num_subscribers_)
        {
            return false;
        }
        if (std::get<2>(subscribers_[index]) == nullptr)
        {
            return false;
        }

        eprosima::fastdds::dds::DataReaderQos qos;
        qos = std::get<2>(subscribers_[index])->get_qos();
        qos.deadline().period = deadline_period;

        return ReturnCode_t::RETCODE_OK == std::get<2>(subscribers_[index])->set_qos(qos);
    }

    void pub_liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(pub_liveliness_mutex_);
        pub_times_liveliness_lost_++;
        pub_liveliness_cv_.notify_one();
    }

    void sub_liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(sub_liveliness_mutex_);
        sub_times_liveliness_lost_++;
        sub_liveliness_cv_.notify_one();
    }

    void sub_liveliness_recovered()
    {
        std::unique_lock<std::mutex> lock(sub_liveliness_mutex_);
        sub_times_liveliness_recovered_++;
        sub_liveliness_cv_.notify_one();
    }

    unsigned int pub_times_liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(pub_liveliness_mutex_);
        return pub_times_liveliness_lost_;
    }

    unsigned int sub_times_liveliness_lost()
    {
        std::unique_lock<std::mutex> lock(sub_liveliness_mutex_);
        return sub_times_liveliness_lost_;
    }

    unsigned int sub_times_liveliness_recovered()
    {
        std::unique_lock<std::mutex> lock(sub_liveliness_mutex_);
        return sub_times_liveliness_recovered_;
    }

private:

    PubSubParticipant& operator =(
            const PubSubParticipant&) = delete;

    void pub_matched()
    {
        std::unique_lock<std::mutex> lock(pub_mutex_);
        ++pub_matched_;
        pub_cv_.notify_one();
    }

    void pub_unmatched()
    {
        std::unique_lock<std::mutex> lock(pub_mutex_);
        --pub_matched_;
        pub_cv_.notify_one();
    }

    void sub_matched()
    {
        std::unique_lock<std::mutex> lock(sub_mutex_);
        ++sub_matched_;
        sub_cv_.notify_one();
    }

    void sub_unmatched()
    {
        std::unique_lock<std::mutex> lock(sub_mutex_);
        --sub_matched_;
        sub_cv_.notify_one();
    }

    //! The participant
    eprosima::fastdds::dds::DomainParticipant* participant_;
    //! Participant attributes
    eprosima::fastdds::dds::DomainParticipantQos participant_qos_;
    //! Number of publishers in this participant
    unsigned int num_publishers_;
    //! Number of subscribers in this participant
    unsigned int num_subscribers_;
    //! Number of expected subscribers to match
    unsigned int num_expected_subscribers_;
    //! Number of expected subscribers to match
    unsigned int num_expected_publishers_;
    //! A vector of publishers
    std::vector<std::tuple<
                eprosima::fastdds::dds::Topic*,
                eprosima::fastdds::dds::Publisher*,
                eprosima::fastdds::dds::DataWriter*>> publishers_;
    //! A vector of subscribers
    std::vector<std::tuple<
                eprosima::fastdds::dds::Topic*,
                eprosima::fastdds::dds::Subscriber*,
                eprosima::fastdds::dds::DataReader*>> subscribers_;
    //! Publisher attributes
    eprosima::fastdds::dds::DataWriterQos datawriter_qos_;
    //! Subscriber attributes
    eprosima::fastdds::dds::DataReaderQos datareader_qos_;
    //! A listener for publishers
    PubListener pub_listener_;
    //! A listener for subscribers
    SubListener sub_listener_;
    std::string publisher_topicname_;
    std::string subscriber_topicname_;

    std::mutex pub_mutex_;
    std::mutex sub_mutex_;
    std::condition_variable pub_cv_;
    std::condition_variable sub_cv_;
    std::atomic<unsigned int> pub_matched_;
    std::atomic<unsigned int> sub_matched_;

    //! Number of times liveliness was lost on the publishing side
    unsigned int pub_times_liveliness_lost_;
    //! The number of times liveliness was lost on the subscribing side
    unsigned int sub_times_liveliness_lost_;
    //! The number of times liveliness was recovered on the subscribing side
    unsigned int sub_times_liveliness_recovered_;
    //! A mutex protecting liveliness data
    std::mutex sub_liveliness_mutex_;
    //! A condition variable for liveliness data
    std::condition_variable sub_liveliness_cv_;
    //! A mutex protecting liveliness of publisher
    std::mutex pub_liveliness_mutex_;
    //! A condition variable for liveliness of publisher
    std::condition_variable pub_liveliness_cv_;

    eprosima::fastdds::dds::TypeSupport type_;
};

#endif // _TEST_BLACKBOX_PUBSUBPARTICIPANT_HPP_
