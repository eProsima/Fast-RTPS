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
 * @file PubSubWriterReader.hpp
 *
 */

#ifndef _TEST_BLACKBOX_PUBSUBWRITERREADER_HPP_
#define _TEST_BLACKBOX_PUBSUBWRITERREADER_HPP_

#include <fastrtps/fastrtps_fwd.h>
#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/participant/ParticipantListener.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/publisher/PublisherListener.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/subscriber/SubscriberListener.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/transport/TransportDescriptorInterface.h>

#include <string>
#include <list>
#include <map>
#include <condition_variable>
#include <asio.hpp>
#include <gtest/gtest.h>

template<class TypeSupport>
class PubSubWriterReader
{
    class ParticipantListener : public eprosima::fastrtps::ParticipantListener
    {
    public:

        ParticipantListener(
                PubSubWriterReader& wreader)
            : wreader_(wreader)
        {
        }

        ~ParticipantListener()
        {
        }

#if HAVE_SECURITY
        void onParticipantAuthentication(
                eprosima::fastrtps::Participant*,
                eprosima::fastrtps::rtps::ParticipantAuthenticationInfo&& info) override
        {
            if (info.status == eprosima::fastrtps::rtps::ParticipantAuthenticationInfo::AUTHORIZED_PARTICIPANT)
            {
                wreader_.authorized();
            }
            else if (info.status == eprosima::fastrtps::rtps::ParticipantAuthenticationInfo::UNAUTHORIZED_PARTICIPANT)
            {
                wreader_.unauthorized();
            }
        }

#endif // if HAVE_SECURITY
        void onParticipantDiscovery(
                eprosima::fastrtps::Participant* participant,
                eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& info) override
        {
            (void)participant;

            switch (info.status)
            {
                case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
                    info_add(discovered_participants_, info.info.m_guid);
                    break;

                case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
                    info_remove(discovered_participants_, info.info.m_guid);
                    break;

                case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
                    std::cout << "Participant " << info.info.m_guid << " has been dropped";
                    info_remove(discovered_participants_, info.info.m_guid);
                    break;

                default:
                    break;
            }
        }

        void onSubscriberDiscovery(
                eprosima::fastrtps::Participant* participant,
                eprosima::fastrtps::rtps::ReaderDiscoveryInfo&& info) override
        {
            (void)participant;

            switch (info.status)
            {
                case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER:
                    info_add(discovered_subscribers_, info.info.guid());
                    break;

                case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::REMOVED_READER:
                    info_remove(discovered_subscribers_, info.info.guid());
                    break;

                default:
                    break;
            }
        }

        void onPublisherDiscovery(
                eprosima::fastrtps::Participant* participant,
                eprosima::fastrtps::rtps::WriterDiscoveryInfo&& info) override
        {
            (void)participant;

            switch (info.status)
            {
                case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
                    info_add(discovered_publishers_, info.info.guid());
                    break;

                case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
                    info_remove(discovered_publishers_, info.info.guid());
                    break;

                default:
                    break;
            }
        }

        size_t get_num_discovered_participants() const
        {
            std::lock_guard<std::mutex> guard(info_mutex_);
            return discovered_participants_.size();
        }

        size_t get_num_discovered_publishers() const
        {
            std::lock_guard<std::mutex> guard(info_mutex_);
            return discovered_publishers_.size();
        }

        size_t get_num_discovered_subscribers() const
        {
            std::lock_guard<std::mutex> guard(info_mutex_);
            return discovered_subscribers_.size();
        }

    private:

        //! Mutex guarding all info collections
        mutable std::mutex info_mutex_;
        //! The discovered participants excluding the participant this listener is listening to
        std::set<eprosima::fastrtps::rtps::GUID_t> discovered_participants_;
        //! Number of subscribers discovered
        std::set<eprosima::fastrtps::rtps::GUID_t> discovered_subscribers_;
        //! Number of publishers discovered
        std::set<eprosima::fastrtps::rtps::GUID_t> discovered_publishers_;

        void info_add(
                std::set<eprosima::fastrtps::rtps::GUID_t>& collection,
                const eprosima::fastrtps::rtps::GUID_t& item)
        {
            std::lock_guard<std::mutex> guard(info_mutex_);
            collection.insert(item);
        }

        void info_remove(
                std::set<eprosima::fastrtps::rtps::GUID_t>& collection,
                const eprosima::fastrtps::rtps::GUID_t& item)
        {
            std::lock_guard<std::mutex> guard(info_mutex_);
            collection.erase(item);
        }

        //! Deleted assignment operator
        ParticipantListener& operator =(
                const ParticipantListener&) = delete;
        //! Pointer to the pub sub writer reader
        PubSubWriterReader& wreader_;

    }
    participant_listener_;

    class PubListener : public eprosima::fastrtps::PublisherListener
    {
    public:

        PubListener(
                PubSubWriterReader& wreader)
            : wreader_(wreader)
        {
        }

        ~PubListener()
        {
        }

        void onPublicationMatched(
                eprosima::fastrtps::Publisher* /*pub*/,
                eprosima::fastrtps::rtps::MatchingInfo& info)
        {
            if (info.status == eprosima::fastrtps::rtps::MATCHED_MATCHING)
            {
                wreader_.publication_matched(info);
            }
            else
            {
                wreader_.publication_unmatched(info);
            }
        }

    private:

        PubListener& operator =(
                const PubListener&) = delete;

        PubSubWriterReader& wreader_;

    }
    pub_listener_;

    class SubListener : public eprosima::fastrtps::SubscriberListener
    {
    public:

        SubListener(
                PubSubWriterReader& wreader)
            : wreader_(wreader)
        {
        }

        ~SubListener()
        {
        }

        void onNewDataMessage(
                eprosima::fastrtps::Subscriber* sub)
        {
            ASSERT_NE(sub, nullptr);

            if (wreader_.receiving_.load())
            {
                bool ret = false;
                do
                {
                    wreader_.receive_one(sub, ret);
                } while (ret);
            }
        }

        void onSubscriptionMatched(
                eprosima::fastrtps::Subscriber* /*sub*/,
                eprosima::fastrtps::rtps::MatchingInfo& info)
        {
            if (info.status == eprosima::fastrtps::rtps::MATCHED_MATCHING)
            {
                wreader_.subscription_matched(info);
            }
            else
            {
                wreader_.subscription_unmatched(info);
            }
        }

    private:

        SubListener& operator =(
                const SubListener&) = delete;

        PubSubWriterReader& wreader_;
    }
    sub_listener_;

    friend class PubListener;
    friend class SubListener;

public:

    typedef TypeSupport type_support;
    typedef typename type_support::type type;

    PubSubWriterReader(
            const std::string& topic_name)
        : participant_listener_(*this)
        , pub_listener_(*this)
        , sub_listener_(*this)
        , participant_(nullptr)
        , publisher_(nullptr)
        , subscriber_(nullptr)
        , initialized_(false)
        , receiving_(false)
        , current_received_count_(0)
        , number_samples_expected_(0)
#if HAVE_SECURITY
        , authorized_(0)
        , unauthorized_(0)
#endif // if HAVE_SECURITY
    {
        publisher_attr_.topic.topicDataType = type_.getName();
        subscriber_attr_.topic.topicDataType = type_.getName();
        // Generate topic name
        std::ostringstream t;
        t << topic_name << "_" << asio::ip::host_name() << "_" << GET_PID();

        publisher_attr_.topic.topicName = t.str();
        subscriber_attr_.topic.topicName = t.str();
        topic_name_ = t.str();
        publisher_attr_.topic.topicKind =
                type_.m_isGetKeyDefined ? ::eprosima::fastrtps::rtps::WITH_KEY : ::eprosima::fastrtps::rtps::NO_KEY;
        subscriber_attr_.topic.topicKind =
                type_.m_isGetKeyDefined ? ::eprosima::fastrtps::rtps::WITH_KEY : ::eprosima::fastrtps::rtps::NO_KEY;

        // By default, only allow participants on the same host
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags =
                eprosima::fastrtps::rtps::FILTER_DIFFERENT_HOST;

        // By default, memory mode is preallocated (the most restritive)
        publisher_attr_.historyMemoryPolicy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;
        subscriber_attr_.historyMemoryPolicy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;

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
        subscriber_attr_.times.heartbeatResponseDelay.seconds = 0;
        subscriber_attr_.times.heartbeatResponseDelay.nanosec = 100000000;
    }

    ~PubSubWriterReader()
    {
        if (participant_ != nullptr)
        {
            eprosima::fastrtps::Domain::removeParticipant(participant_);
        }
    }

    void init(
            bool avoid_multicast = true,
            uint32_t initial_pdp_count = 5)
    {
        //Create participant
        participant_attr_.rtps.builtin.avoid_builtin_multicast = avoid_multicast;
        participant_attr_.rtps.builtin.discovery_config.initial_announcements.count = initial_pdp_count;
        participant_attr_.domainId = (uint32_t)GET_PID() % 230;
        participant_ = eprosima::fastrtps::Domain::createParticipant(participant_attr_, &participant_listener_);

        if (participant_ != nullptr)
        {
            // Register type
            eprosima::fastrtps::Domain::registerType(participant_, &type_);

            //Create publisher
            publisher_ = eprosima::fastrtps::Domain::createPublisher(participant_, publisher_attr_, &pub_listener_);

            if (publisher_ != nullptr)
            {
                //Create subscribe r
                subscriber_ = eprosima::fastrtps::Domain::createSubscriber(participant_, subscriber_attr_,
                                &sub_listener_);

                if (subscriber_ != nullptr)
                {
                    initialized_ = true;
                    return;
                }
            }

            eprosima::fastrtps::Domain::removeParticipant(participant_);
        }
    }

    bool create_additional_topics(
            size_t num_topics)
    {
        bool ret_val = initialized_;
        if (ret_val)
        {
            std::string topic_name = publisher_attr_.topic.topicName.c_str();

            for (size_t i = 0; ret_val && (i < num_topics); i++)
            {
                topic_name += "/";
                publisher_attr_.topic.topicName = topic_name;
                ret_val &=
                        nullptr != eprosima::fastrtps::Domain::createPublisher(participant_, publisher_attr_,
                                &pub_listener_);
            }

            topic_name = subscriber_attr_.topic.topicName.c_str();

            for (size_t i = 0; ret_val && (i < num_topics); i++)
            {
                topic_name += "/";
                subscriber_attr_.topic.topicName = topic_name;
                ret_val &=
                        nullptr != eprosima::fastrtps::Domain::createSubscriber(participant_, subscriber_attr_,
                                &sub_listener_);
            }
        }

        return ret_val;
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

    void send(
            std::list<type>& msgs)
    {
        auto it = msgs.begin();

        while (it != msgs.end())
        {
            if (publisher_->write((void*)&(*it)))
            {
                default_send_print<type>(*it);
                it = msgs.erase(it);
            }
            else
            {
                break;
            }
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

    void block(
            std::function<bool()> checker)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, checker);
    }

    void wait_discovery()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Waiting discovery..." << std::endl;

        if (matched_readers_.size() < 1 || matched_writers_.size() < 1)
        {
            cvDiscovery_.wait(lock);
        }

        ASSERT_GE(matched_readers_.size() + matched_writers_.size(), 2u);
        std::cout << "Discovery finished..." << std::endl;
    }

    void waitRemoval()
    {
        std::unique_lock<std::mutex> lock(mutexDiscovery_);

        std::cout << "Waiting removal..." << std::endl;

        if (matched_writers_.size() != 0 || matched_readers_.size() != 0)
        {
            cvDiscovery_.wait(lock);
        }

        ASSERT_EQ(matched_readers_.size() + matched_writers_.size(), 0u);
        std::cout << "Removal finished..." << std::endl;
    }

#if HAVE_SECURITY
    void waitAuthorized(
            unsigned int how_many = 1)
    {
        std::unique_lock<std::mutex> lock(mutexAuthentication_);

        std::cout << "WReader is waiting authorization..." << std::endl;

        while (authorized_ != how_many)
        {
            cvAuthentication_.wait(lock);
        }

        ASSERT_EQ(authorized_, how_many);
        std::cout << "WReader authorization finished..." << std::endl;
    }

    void waitUnauthorized(
            unsigned int how_many = 1)
    {
        std::unique_lock<std::mutex> lock(mutexAuthentication_);

        std::cout << "WReader is waiting unauthorization..." << std::endl;

        while (unauthorized_ != how_many)
        {
            cvAuthentication_.wait(lock);
        }

        ASSERT_EQ(unauthorized_, how_many);
        std::cout << "WReader unauthorization finished..." << std::endl;
    }

#endif // if HAVE_SECURITY

    PubSubWriterReader& pub_durability_kind(
            const eprosima::fastrtps::DurabilityQosPolicyKind kind)
    {
        publisher_attr_.qos.m_durability.kind = kind;
        return *this;
    }

    PubSubWriterReader& sub_durability_kind(
            const eprosima::fastrtps::DurabilityQosPolicyKind kind)
    {
        subscriber_attr_.qos.m_durability.kind = kind;
        return *this;
    }

    PubSubWriterReader& pub_reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        publisher_attr_.qos.m_reliability.kind = kind;
        return *this;
    }

    PubSubWriterReader& sub_reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        subscriber_attr_.qos.m_reliability.kind = kind;
        return *this;
    }

    PubSubWriterReader& pub_history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        publisher_attr_.topic.historyQos.kind = kind;
        return *this;
    }

    PubSubWriterReader& sub_history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        subscriber_attr_.topic.historyQos.kind = kind;
        return *this;
    }

    PubSubWriterReader& pub_history_depth(
            const int32_t depth)
    {
        publisher_attr_.topic.historyQos.depth = depth;
        return *this;
    }

    PubSubWriterReader& sub_history_depth(
            const int32_t depth)
    {
        subscriber_attr_.topic.historyQos.depth = depth;
        return *this;
    }

    PubSubWriterReader& disable_builtin_transport()
    {
        participant_attr_.rtps.useBuiltinTransports = false;
        return *this;
    }

    PubSubWriterReader& add_user_transport_to_pparams(
            std::shared_ptr<eprosima::fastrtps::rtps::TransportDescriptorInterface> userTransportDescriptor)
    {
        participant_attr_.rtps.userTransports.push_back(userTransportDescriptor);
        return *this;
    }

    PubSubWriterReader& property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        participant_attr_.rtps.properties = property_policy;
        return *this;
    }

    PubSubWriterReader& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_attr_.rtps.builtin.discovery_config.ignoreParticipantFlags = flags;
        return *this;
    }

    PubSubWriterReader& pub_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        publisher_attr_.properties = property_policy;
        return *this;
    }

    PubSubWriterReader& sub_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        subscriber_attr_.properties = property_policy;
        return *this;
    }

    size_t get_num_discovered_participants() const
    {
        return participant_listener_.get_num_discovered_participants();
    }

    size_t get_num_discovered_publishers() const
    {
        return participant_listener_.get_num_discovered_publishers();
    }

    size_t get_num_discovered_subscribers() const
    {
        return participant_listener_.get_num_discovered_subscribers();
    }

    size_t get_publication_matched()
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        return matched_writers_.size();
    }

    size_t get_subscription_matched()
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        return matched_readers_.size();
    }

private:

    void receive_one(
            eprosima::fastrtps::Subscriber* subscriber,
            bool& returnedValue)
    {
        returnedValue = false;
        type data;
        eprosima::fastrtps::SampleInfo_t info;

        if (subscriber->takeNextData((void*)&data, &info))
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

    void publication_matched(
            eprosima::fastrtps::rtps::MatchingInfo& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_writers_.insert(info.remoteEndpointGuid);
        cvDiscovery_.notify_one();
    }

    void publication_unmatched(
            eprosima::fastrtps::rtps::MatchingInfo& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_writers_.erase(info.remoteEndpointGuid);
        cvDiscovery_.notify_one();
    }

    void subscription_matched(
            eprosima::fastrtps::rtps::MatchingInfo& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_readers_.insert(info.remoteEndpointGuid);
        cvDiscovery_.notify_one();
    }

    void subscription_unmatched(
            eprosima::fastrtps::rtps::MatchingInfo& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_readers_.erase(info.remoteEndpointGuid);
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

    PubSubWriterReader& operator =(
            const PubSubWriterReader&) = delete;

    eprosima::fastrtps::Participant* participant_;
    eprosima::fastrtps::ParticipantAttributes participant_attr_;

    eprosima::fastrtps::Publisher* publisher_;
    eprosima::fastrtps::PublisherAttributes publisher_attr_;

    eprosima::fastrtps::Subscriber* subscriber_;
    eprosima::fastrtps::SubscriberAttributes subscriber_attr_;

    std::string topic_name_;
    bool initialized_;
    std::list<type> total_msgs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::mutex mutexDiscovery_;
    std::condition_variable cvDiscovery_;
    std::set<eprosima::fastrtps::rtps::GUID_t> matched_writers_;
    std::set<eprosima::fastrtps::rtps::GUID_t> matched_readers_;
    std::atomic<bool> receiving_;
    type_support type_;
    eprosima::fastrtps::rtps::SequenceNumber_t last_seq;
    size_t current_received_count_;
    size_t number_samples_expected_;
#if HAVE_SECURITY
    std::mutex mutexAuthentication_;
    std::condition_variable cvAuthentication_;
    unsigned int authorized_;
    unsigned int unauthorized_;
#endif // if HAVE_SECURITY
};

#endif // _TEST_BLACKBOX_PUBSUBWRITER_HPP_
