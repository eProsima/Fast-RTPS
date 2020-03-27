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
 * @file Subscriber.cpp
 *
 */

#include <asio.hpp>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/SubscriberListener.hpp>
#include <fastdds/dds/topic/DataReader.hpp>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/types/DynamicData.h>
#include <fastrtps/types/TypeObjectFactory.h>

#include <mutex>
#include <condition_variable>
#include <fstream>
#include <string>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

static bool g_run = true;
static types::DynamicType_ptr g_type;
static SubscriberAttributes g_subscriber_attributes;
static Subscriber* g_subscriber = nullptr;

class ParListener : public DomainParticipantListener
{
public:

    ParListener()
    {}

    virtual ~ParListener() override
    {}

    /**
     * This method is called when a new Participant is discovered, or a previously discovered participant changes its QOS or is removed.
     * @param p Pointer to the Participant
     * @param info DiscoveryInfo.
     */
    void on_participant_discovery(
            DomainParticipant* /*participant*/,
            rtps::ParticipantDiscoveryInfo&& info) override
    {
        if (info.status == rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT)
        {
            std::cout << "Subscriber participant " << //participant->getGuid() <<
                " discovered participant " << info.info.m_guid << std::endl;
        }
        else if (info.status == rtps::ParticipantDiscoveryInfo::CHANGED_QOS_PARTICIPANT)
        {
            std::cout << "Subscriber participant " << //participant->getGuid() <<
                " detected changes on participant " << info.info.m_guid << std::endl;
        }
        else if (info.status == rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT)
        {
            std::cout << "Subscriber participant " << //participant->getGuid() <<
                " removed participant " << info.info.m_guid << std::endl;
        }
        else if (info.status == rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT)
        {
            std::cout << "Subscriber participant " << //participant->getGuid() <<
                " dropped participant " << info.info.m_guid << std::endl;
        }
    }

    void on_type_information_received(
            eprosima::fastdds::dds::DomainParticipant* participant,
            const eprosima::fastrtps::string_255 topic_name,
            const eprosima::fastrtps::string_255 type_name,
            const eprosima::fastrtps::types::TypeInformation& type_information) override
    {
        std::function<void(const std::string&, const types::DynamicType_ptr)> callback =
            [topic_name, type_name](const std::string& name, const types::DynamicType_ptr type)
                {
                    if (nullptr != g_subscriber)
                    {
                        std::cout << "Discovered type: " << name << " from topic " << topic_name << std::endl;
                        g_subscriber_attributes.topic.topicDataType = type_name;
                        g_subscriber->create_datareader(
                            g_subscriber_attributes.topic,
                            g_subscriber_attributes.qos,
                            nullptr);

                        if (type == nullptr)
                        {
                            const types::TypeIdentifier* ident =
                                types::TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(name);

                            if (nullptr != ident)
                            {
                                const types::TypeObject* obj =
                                    types::TypeObjectFactory::get_instance()->get_type_object(ident);

                                types::DynamicType_ptr dyn_type =
                                    types::TypeObjectFactory::get_instance()->build_dynamic_type(name, ident, obj);

                                if (nullptr != dyn_type)
                                {
                                    g_type = dyn_type;
                                }
                                else
                                {
                                    std::cout << "ERROR: DynamicType cannot be created for type: " << name << std::endl;
                                }
                            }
                            else
                            {
                                std::cout << "ERROR: TypeIdentifier cannot be retrieved for type: "
                                          << name << std::endl;
                            }
                        }
                        else
                        {
                            g_type = type;
                        }
                    }
                };

        participant->register_remote_type(
            type_information,
            type_name.to_string(),
            callback);
    }

#if HAVE_SECURITY
    void onParticipantAuthentication(
            DomainParticipant* /*participant*/,
            rtps::ParticipantAuthenticationInfo&& info) override
    {
        if (rtps::ParticipantAuthenticationInfo::AUTHORIZED_PARTICIPANT == info.status)
        {
            std::cout << "Subscriber participant " << //participant->guid() <<
                " authorized participant " << info.guid << std::endl;
        }
        else
        {
            std::cout << "Subscriber participant " << //participant->guid() <<
                " unauthorized participant " << info.guid << std::endl;
        }
    }
#endif
};

class SubListener : public SubscriberListener
{
public:

    SubListener()
        : number_samples_(0) {}

    ~SubListener() override {}

    void on_subscription_matched(
            Subscriber* /*subscriber*/,
            const SubscriptionMatchedStatus& info) override
    {
        if (info.current_count_change == 1)
        {
            std::cout << "Subscriber matched with publisher " << info.last_publication_handle << std::endl;
        }
        else if (info.current_count_change == -1)
        {
            std::cout << "Subscriber unmatched with publisher " << info.last_publication_handle << std::endl;
        }
        else
        {
            std::cout << info.current_count_change
                      << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
        }
    }

    void on_new_data_message(
            Subscriber* subscriber) override
    {
        if (nullptr != g_type)
        {
            types::DynamicPubSubType pst(g_type);
            types::DynamicData_ptr sample(static_cast<types::DynamicData*>(pst.createData()));
            SampleInfo_t info;
            DataReader* reader = subscriber->lookup_datareader(g_subscriber_attributes.topic.topicName.to_string());

            if (nullptr != reader && !!reader->take_next_sample(sample.get(), &info))
            {
                if (info.sampleKind == ALIVE)
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    ++number_samples_;
                    std::string message;
                    uint32_t index;
                    octet count;
                    sample->get_string_value(message, 0);
                    sample->get_uint32_value(index, 1);
                    types::DynamicData* inner = sample->loan_value(2);
                    inner->get_byte_value(count, 0);
                    sample->return_loaned_value(inner);
                    std::cout << "Received sample: index(" << index << "), message("
                              << message << "), inner_count(" << std::hex << (uint32_t)count << ")" << std::endl;
                    cv_.notify_all();
                }
            }
        }
    }

    void on_liveliness_changed(
            Subscriber* sub,
            const eprosima::fastdds::dds::LivelinessChangedStatus& status) override
    {
        (void)sub;
        if (status.alive_count_change == 1)
        {
            std::cout << "Publisher recovered liveliness" << std::endl;
        }
        else if (status.not_alive_count_change == 1)
        {
            std::cout << "Publisher lost liveliness" << std::endl;
            g_run = false;
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    unsigned int number_samples_;
};

int main(
        int argc,
        char** argv)
{
    int arg_count = 1;
    bool notexit = false;
    uint32_t seed = 7800;
    uint32_t samples = 4;
    //char* xml_file = nullptr;
    std::string magic;

    while (arg_count < argc)
    {
        if (strcmp(argv[arg_count], "--notexit") == 0)
        {
            notexit = true;
        }
        else if (strcmp(argv[arg_count], "--seed") == 0)
        {
            if (++arg_count >= argc)
            {
                std::cout << "--seed expects a parameter" << std::endl;
                return -1;
            }

            seed = strtol(argv[arg_count], nullptr, 10);
        }
        else if (strcmp(argv[arg_count], "--samples") == 0)
        {
            if (++arg_count >= argc)
            {
                std::cout << "--samples expects a parameter" << std::endl;
                return -1;
            }

            samples = strtol(argv[arg_count], nullptr, 10);
        }
        else if (strcmp(argv[arg_count], "--magic") == 0)
        {
            if (++arg_count >= argc)
            {
                std::cout << "--magic expects a parameter" << std::endl;
                return -1;
            }

            magic = argv[arg_count];
        }
        else if (strcmp(argv[arg_count], "--xmlfile") == 0)
        {
            std::cout << "--xmlfile option isn't implemented yet." << std::endl;
            if (++arg_count >= argc)
            {
                std::cout << "--xmlfile expects a parameter" << std::endl;
                return -1;
            }

            //xml_file = argv[arg_count];
        }

        ++arg_count;
    }

    /* TODO - XMLProfileManager doesn't support DDS yet
    if(xml_file)
    {
        DomainParticipantFactory::get_instance()->load_XML_profiles_file(xml_file);
    }
    */

    ParticipantAttributes participant_attributes;
    DomainParticipantFactory::get_instance()->get_default_participant_qos(participant_attributes);
    participant_attributes.rtps.builtin.typelookup_config.use_client = true;
    participant_attributes.rtps.builtin.domainId = seed % 230;
    ParListener participant_listener;
    DomainParticipant* participant =
            DomainParticipantFactory::get_instance()->create_participant(participant_attributes, false, &participant_listener);

    if (participant == nullptr)
    {
        std::cout << "Error creating subscriber participant" << std::endl;
        return 1;
    }

    // Generate topic name
    std::ostringstream topic;
    topic << "HelloWorldTopic_" << ((magic.empty()) ? asio::ip::host_name() : magic) << "_" << seed;

    SubListener listener;

    //CREATE THE SUBSCRIBER
    //Domain::getDefaultSubscriberAttributes(subscriber_attributes);
    g_subscriber_attributes.topic.topicKind = NO_KEY;
    //g_subscriber_attributes.topic.topicDataType = type.getName();
    g_subscriber_attributes.topic.topicName = topic.str();
    g_subscriber_attributes.qos.m_liveliness.lease_duration = 3;
    g_subscriber_attributes.qos.m_liveliness.kind = eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS;
    g_subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, g_subscriber_attributes, &listener);

    if (g_subscriber == nullptr)
    {
        std::cout << "Error creating subscriber" << std::endl;
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 1;
    }

    //Now that everything is created we can enable the protocols
    participant->enable();

    while (notexit && g_run)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (g_run)
    {
        std::unique_lock<std::mutex> lock(listener.mutex_);
        listener.cv_.wait(lock, [&] {
            return listener.number_samples_ >= samples;
        });
    }

    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}
