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

#include <fastrtps/fastrtps_fwd.h>
#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/participant/ParticipantListener.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/publisher/PublisherListener.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/subscriber/SubscriberListener.h>
#include <fastrtps/attributes/PublisherAttributes.h>

#include <asio.hpp>
#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace eprosima {
namespace fastrtps {

/**
 * @brief A class with one participant that can have multiple publishers and subscribers
 */
template<class TypeSupport>
class PubSubParticipant
{
    class PubListener : public PublisherListener
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

        void onPublicationMatched(
                Publisher* pub,
                rtps::MatchingInfo& info) override
        {
            (void)pub;
            (info.status == rtps::MATCHED_MATCHING) ? participant_->pub_matched() : participant_->pub_unmatched();
        }

        void on_liveliness_lost(
                Publisher* pub,
                const LivelinessLostStatus& status) override
        {
            (void)pub;
            (void)status;
            participant_->pub_liveliness_lost();
        }

    private:

        PubListener& operator =(
                const PubListener&) = delete;
        //! A pointer to the participant
        PubSubParticipant* participant_;
    };

    class SubListener : public SubscriberListener
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

        void onSubscriptionMatched(
                Subscriber* sub,
                rtps::MatchingInfo& info) override
        {
            (void)sub;
            (info.status == rtps::MATCHED_MATCHING) ? participant_->sub_matched() : participant_->sub_unmatched();
        }

        void on_liveliness_changed(
                Subscriber* sub,
                const LivelinessChangedStatus& status) override
        {
            (void)sub;
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
        , participant_attr_()
        , num_publishers_(num_publishers)
        , num_subscribers_(num_subscribers)
        , num_expected_subscribers_(num_expected_subscribers)
        , num_expected_publishers_(num_expected_publishers)
        , publishers_(num_publishers)
        , subscribers_(num_subscribers)
        , publisher_attr_()
        , pub_listener_(this)
        , sub_listener_(this)
        , pub_matched_(0)
        , sub_matched_(0)
        , pub_times_liveliness_lost_(0)
        , sub_times_liveliness_lost_(0)
        , sub_times_liveliness_recovered_(0)
    {

#if defined(PREALLOCATED_WITH_REALLOC_MEMORY_MODE_TEST)
        publisher_attr_.historyMemoryPolicy = rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
#elif defined(DYNAMIC_RESERVE_MEMORY_MODE_TEST)
        publisher_attr_.historyMemoryPolicy = rtps::DYNAMIC_RESERVE_MEMORY_MODE;
#else
        publisher_attr_.historyMemoryPolicy = rtps::PREALLOCATED_MEMORY_MODE;
#endif // if defined(PREALLOCATED_WITH_REALLOC_MEMORY_MODE_TEST)

        // By default, only allow participants on the same host
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags =
                eprosima::fastrtps::rtps::FILTER_DIFFERENT_HOST;

        // By default, heartbeat period and nack response delay are 100 milliseconds.
        publisher_attr_.times.heartbeatPeriod.seconds = 0;
        publisher_attr_.times.heartbeatPeriod.nanosec = 100000000;
        publisher_attr_.times.nackResponseDelay.seconds = 0;
        publisher_attr_.times.nackResponseDelay.nanosec = 100000000;

        // Increase default max_blocking_time to 1 second, as our CI infrastructure shows some
        // big CPU overhead sometimes
        publisher_attr_.qos.m_reliability.max_blocking_time.seconds = 1;
        publisher_attr_.qos.m_reliability.max_blocking_time.nanosec = 0;

        // By default, heartbeat period delay is 100 milliseconds.
        subscriber_attr_.times.heartbeatResponseDelay = 0.1;
    }

    ~PubSubParticipant()
    {
        if (participant_ != nullptr)
        {
            Domain::removeParticipant(participant_);
            participant_ = nullptr;
        }
    }

    bool init_participant()
    {
        participant_attr_.domainId = (uint32_t)GET_PID() % 230;
        participant_ = Domain::createParticipant(participant_attr_);
        if (participant_ != nullptr)
        {
            Domain::registerType(participant_, &type_);
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

        auto pub = Domain::createPublisher(participant_, publisher_attr_, &pub_listener_);
        if (pub != nullptr)
        {
            publishers_[index] = pub;
            return true;
        }
        return false;
    }

    bool init_subscriber(
            unsigned int index)
    {
        if (index >= num_subscribers_)
        {
            return false;
        }
        auto subscriber = Domain::createSubscriber(participant_, subscriber_attr_, &sub_listener_);
        if (subscriber != nullptr)
        {
            subscribers_[index] = subscriber;
            return true;
        }
        return false;
    }

    bool send_sample(
            type& msg,
            unsigned int index = 0)
    {
        return publishers_[index]->write((void*)&msg);
    }

    void assert_liveliness_participant()
    {
        participant_->assert_liveliness();
    }

    void assert_liveliness(
            unsigned int index = 0)
    {
        publishers_[index]->assert_liveliness();
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
        participant_attr_.rtps.properties = property_policy;
        return *this;
    }

    PubSubParticipant& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags = flags;
        return *this;
    }

    PubSubParticipant& pub_topic_name(
            std::string topicName)
    {
        // Generate topic name
        std::ostringstream t;
        t << topicName << "_" << asio::ip::host_name() << "_" << GET_PID();
        publisher_attr_.topic.topicName = t.str();
        publisher_attr_.topic.topicDataType = type_.getName();
        return *this;
    }

    PubSubParticipant& sub_topic_name(
            std::string topicName)
    {
        // Generate topic name
        std::ostringstream t;
        t << topicName << "_" << asio::ip::host_name() << "_" << GET_PID();
        subscriber_attr_.topic.topicName = t.str();
        subscriber_attr_.topic.topicDataType = type_.getName();
        return *this;
    }

    PubSubParticipant& reliability(
            const ReliabilityQosPolicyKind kind)
    {
        publisher_attr_.qos.m_reliability.kind = kind;
        subscriber_attr_.qos.m_reliability.kind = kind;
        return *this;
    }

    PubSubParticipant& pub_liveliness_kind(
            const LivelinessQosPolicyKind kind)
    {
        publisher_attr_.qos.m_liveliness.kind = kind;
        return *this;
    }

    PubSubParticipant& pub_liveliness_lease_duration(
            const Duration_t lease_duration)
    {
        publisher_attr_.qos.m_liveliness.lease_duration = lease_duration;
        return *this;
    }

    PubSubParticipant& pub_liveliness_announcement_period(
            const Duration_t announcement_period)
    {
        publisher_attr_.qos.m_liveliness.announcement_period = announcement_period;
        return *this;
    }

    PubSubParticipant& sub_liveliness_kind(
            const LivelinessQosPolicyKind& kind)
    {
        subscriber_attr_.qos.m_liveliness.kind = kind;
        return *this;
    }

    PubSubParticipant& sub_liveliness_lease_duration(
            const Duration_t lease_duration)
    {
        subscriber_attr_.qos.m_liveliness.lease_duration = lease_duration;
        return *this;
    }

    PubSubParticipant& pub_deadline_period(
            const Duration_t& deadline_period)
    {
        publisher_attr_.qos.m_deadline.period = deadline_period;
        return *this;
    }

    PubSubParticipant& sub_deadline_period(
            const Duration_t& deadline_period)
    {
        subscriber_attr_.qos.m_deadline.period = deadline_period;
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
        if (subscribers_[index] == nullptr)
        {
            return false;
        }

        SubscriberAttributes attr;
        attr = subscriber_attr_;
        attr.qos.m_deadline.period = deadline_period;

        return subscribers_[index]->updateAttributes(attr);
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
    Participant* participant_;
    //! Participant attributes
    ParticipantAttributes participant_attr_;
    //! Number of publishers in this participant
    unsigned int num_publishers_;
    //! Number of subscribers in this participant
    unsigned int num_subscribers_;
    //! Number of expected subscribers to match
    unsigned int num_expected_subscribers_;
    //! Number of expected subscribers to match
    unsigned int num_expected_publishers_;
    //! A vector of publishers
    std::vector<Publisher*> publishers_;
    //! A vector of subscribers
    std::vector<Subscriber*> subscribers_;
    //! Publisher attributes
    PublisherAttributes publisher_attr_;
    //! Subscriber attributes
    SubscriberAttributes subscriber_attr_;
    //! A listener for publishers
    PubListener pub_listener_;
    //! A listener for subscribers
    SubListener sub_listener_;

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

    type_support type_;
};

} // namespace fastrtps
} // namespace eprosima

#endif // _TEST_BLACKBOX_PUBSUBPARTICIPANT_HPP_
