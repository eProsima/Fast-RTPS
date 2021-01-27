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

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastrtps/transport/TransportDescriptorInterface.h>

#include <string>
#include <list>
#include <map>
#include <vector>
#include <tuple>
#include <condition_variable>
#include <asio.hpp>
#include <gtest/gtest.h>

using DomainParticipantFactory = eprosima::fastdds::dds::DomainParticipantFactory;

template<class TypeSupport>
class PubSubWriterReader
{
    class ParticipantListener : public eprosima::fastdds::dds::DomainParticipantListener
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
                eprosima::fastdds::dds::DomainParticipant*,
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
        void on_participant_discovery(
                eprosima::fastdds::dds::DomainParticipant* participant,
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

        void on_subscriber_discovery(
                eprosima::fastdds::dds::DomainParticipant* participant,
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

        void on_publisher_discovery(
                eprosima::fastdds::dds::DomainParticipant* participant,
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

    class PubListener : public eprosima::fastdds::dds::DataWriterListener
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

        void on_publication_matched(
                eprosima::fastdds::dds::DataWriter* /*datawriter*/,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info) override
        {
            if (0 < info.current_count_change)
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

    class SubListener : public eprosima::fastdds::dds::DataReaderListener
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

        void on_data_available(
                eprosima::fastdds::dds::DataReader* datareader) override
        {
            ASSERT_NE(datareader, nullptr);

            if (wreader_.receiving_.load())
            {
                bool ret = false;
                do
                {
                    wreader_.receive_one(datareader, ret);
                } while (ret);
            }
        }

        void on_subscription_matched(
                eprosima::fastdds::dds::DataReader* /*datareader*/,
                const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override
        {
            if (0 < info.current_count_change)
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
        , topic_(nullptr)
        , publisher_(nullptr)
        , datawriter_(nullptr)
        , subscriber_(nullptr)
        , datareader_(nullptr)
        , initialized_(false)
        , receiving_(false)
        , current_received_count_(0)
        , number_samples_expected_(0)
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
        datareader_qos_.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;

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
        datareader_qos_.reliable_reader_qos().times.heartbeatResponseDelay.seconds = 0;
        datareader_qos_.reliable_reader_qos().times.heartbeatResponseDelay.nanosec = 100000000;
    }

    ~PubSubWriterReader()
    {
        destroy();
    }

    void init(
            bool avoid_multicast = true,
            uint32_t initial_pdp_count = 5)
    {
        //Create participant
        participant_qos_.wire_protocol().builtin.avoid_builtin_multicast = avoid_multicast;
        participant_qos_.wire_protocol().builtin.discovery_config.initial_announcements.count = initial_pdp_count;

        participant_ = DomainParticipantFactory::get_instance()->create_participant(
            (uint32_t)GET_PID() % 230, participant_qos_, &participant_listener_,
            eprosima::fastdds::dds::StatusMask::none());
        ASSERT_NE(participant_, nullptr);
        ASSERT_TRUE(participant_->is_enabled());

        type_.reset(new type_support());

        // Register type
        ASSERT_EQ(participant_->register_type(type_), ReturnCode_t::RETCODE_OK);

        //Create publisher
        publisher_ = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT);
        ASSERT_NE(publisher_, nullptr);
        ASSERT_TRUE(publisher_->is_enabled());

        //Create subscriber
        subscriber_ = participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT);
        ASSERT_NE(subscriber_, nullptr);
        ASSERT_TRUE(subscriber_->is_enabled());

        // Create topic
        topic_ = participant_->create_topic(topic_name_, type_->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        ASSERT_NE(topic_, nullptr);
        ASSERT_TRUE(topic_->is_enabled());

        datawriter_ = publisher_->create_datawriter(topic_, datawriter_qos_, &pub_listener_);
        ASSERT_NE(datawriter_, nullptr);
        ASSERT_TRUE(datawriter_->is_enabled());

        datareader_ = subscriber_->create_datareader(topic_, datareader_qos_, &sub_listener_);
        ASSERT_NE(datareader_, nullptr);
        ASSERT_TRUE(datareader_->is_enabled());

        initialized_ = true;
    }

    bool create_additional_topics(
            size_t num_topics)
    {
        bool ret_val = initialized_;
        if (ret_val)
        {
            std::string topic_name = topic_name_;

            for (size_t i = 0; i < entities_extra_.size(); i++)
            {
                topic_name += "/";
            }

            for (size_t i = 0; ret_val && (i < num_topics); i++)
            {
                topic_name += "/";
                eprosima::fastdds::dds::Topic* topic = participant_->create_topic(topic_name,
                                type_->getName(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
                ret_val &= (nullptr != topic);
                if (!ret_val)
                {
                    break;
                }

                eprosima::fastdds::dds::DataWriter* datawriter = publisher_->create_datawriter(topic, datawriter_qos_,
                                &pub_listener_);
                ret_val &= (nullptr != datawriter);
                if (!ret_val)
                {
                    break;
                }

                eprosima::fastdds::dds::DataReader* datareader = subscriber_->create_datareader(topic, datareader_qos_,
                                &sub_listener_);
                ret_val &= (nullptr != datareader);
                if (!ret_val)
                {
                    break;
                }

                entities_extra_.push_back({topic, datawriter, datareader});
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
        for (auto& tuple : entities_extra_)
        {
            if (subscriber_)
            {
                subscriber_->delete_datareader(std::get<2>(tuple));
            }
            if (publisher_)
            {
                publisher_->delete_datawriter(std::get<1>(tuple));
            }
            if (participant_)
            {
                participant_->delete_topic(std::get<0>(tuple));
            }
        }
        entities_extra_.clear();

        if (participant_)
        {
            if (subscriber_)
            {
                if (datareader_)
                {
                    subscriber_->delete_datareader(datareader_);
                    datareader_ = nullptr;
                }
                participant_->delete_subscriber(subscriber_);
                subscriber_ = nullptr;
            }
            if (publisher_)
            {
                if (datawriter_)
                {
                    publisher_->delete_datawriter(datawriter_);
                    datawriter_ = nullptr;
                }
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
            std::list<type>& msgs)
    {
        auto it = msgs.begin();

        while (it != msgs.end())
        {
            if (datawriter_->write((void*)&(*it)))
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
            receive_one(datareader_, ret);
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
        datawriter_qos_.durability().kind = kind;
        return *this;
    }

    PubSubWriterReader& sub_durability_kind(
            const eprosima::fastrtps::DurabilityQosPolicyKind kind)
    {
        datareader_qos_.durability().kind = kind;
        return *this;
    }

    PubSubWriterReader& pub_reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        datawriter_qos_.reliability().kind = kind;
        return *this;
    }

    PubSubWriterReader& sub_reliability(
            const eprosima::fastrtps::ReliabilityQosPolicyKind kind)
    {
        datareader_qos_.reliability().kind = kind;
        return *this;
    }

    PubSubWriterReader& pub_history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        datawriter_qos_.history().kind = kind;
        return *this;
    }

    PubSubWriterReader& sub_history_kind(
            const eprosima::fastrtps::HistoryQosPolicyKind kind)
    {
        datareader_qos_.history().kind = kind;
        return *this;
    }

    PubSubWriterReader& pub_history_depth(
            const int32_t depth)
    {
        datawriter_qos_.history().depth = depth;
        return *this;
    }

    PubSubWriterReader& sub_history_depth(
            const int32_t depth)
    {
        datareader_qos_.history().depth = depth;
        return *this;
    }

    PubSubWriterReader& disable_builtin_transport()
    {
        participant_qos_.transport().use_builtin_transports = false;
        return *this;
    }

    PubSubWriterReader& add_user_transport_to_pparams(
            std::shared_ptr<eprosima::fastrtps::rtps::TransportDescriptorInterface> userTransportDescriptor)
    {
        participant_qos_.transport().user_transports.push_back(userTransportDescriptor);
        return *this;
    }

    PubSubWriterReader& property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        participant_qos_.properties() = property_policy;
        return *this;
    }

    PubSubWriterReader& ignore_participant_flags(
            eprosima::fastrtps::rtps::ParticipantFilteringFlags_t flags)
    {
        participant_qos_.wire_protocol().builtin.discovery_config.ignoreParticipantFlags = flags;
        return *this;
    }

    PubSubWriterReader& pub_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        datawriter_qos_.properties() = property_policy;
        return *this;
    }

    PubSubWriterReader& sub_property_policy(
            const eprosima::fastrtps::rtps::PropertyPolicy property_policy)
    {
        datareader_qos_.properties() = property_policy;
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
            eprosima::fastdds::dds::DataReader* datareader,
            bool& returnedValue)
    {
        returnedValue = false;
        type data;
        eprosima::fastdds::dds::SampleInfo info;

        if ((ReturnCode_t::RETCODE_OK == datareader->take_next_sample((void*)&data, &info)))
        {
            returnedValue = true;

            std::unique_lock<std::mutex> lock(mutex_);

            // Check order of changes.
            ASSERT_LT(last_seq, info.sample_identity.sequence_number());
            last_seq = info.sample_identity.sequence_number();

            if (info.instance_state == eprosima::fastdds::dds::ALIVE_INSTANCE_STATE)
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
            const eprosima::fastdds::dds::PublicationMatchedStatus& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_writers_.insert(info.last_subscription_handle);
        cvDiscovery_.notify_one();
    }

    void publication_unmatched(
            const eprosima::fastdds::dds::PublicationMatchedStatus& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_writers_.erase(info.last_subscription_handle);
        cvDiscovery_.notify_one();
    }

    void subscription_matched(
            const eprosima::fastdds::dds::SubscriptionMatchedStatus& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_readers_.insert(info.last_publication_handle);
        cvDiscovery_.notify_one();
    }

    void subscription_unmatched(
            const eprosima::fastdds::dds::SubscriptionMatchedStatus& info)
    {
        std::lock_guard<std::mutex> guard(mutexDiscovery_);
        matched_readers_.erase(info.last_publication_handle);
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

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::DomainParticipantQos participant_qos_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::DataWriter* datawriter_;
    eprosima::fastdds::dds::DataWriterQos datawriter_qos_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::DataReader* datareader_;
    eprosima::fastdds::dds::DataReaderQos datareader_qos_;

    std::vector<std::tuple<
                eprosima::fastdds::dds::Topic*,
                eprosima::fastdds::dds::DataWriter*,
                eprosima::fastdds::dds::DataReader*
                >> entities_extra_;

    std::string topic_name_;
    bool initialized_;
    std::list<type> total_msgs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::mutex mutexDiscovery_;
    std::condition_variable cvDiscovery_;
    std::set<eprosima::fastrtps::rtps::InstanceHandle_t> matched_writers_;
    std::set<eprosima::fastrtps::rtps::InstanceHandle_t> matched_readers_;
    std::atomic<bool> receiving_;
    eprosima::fastdds::dds::TypeSupport type_;
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
