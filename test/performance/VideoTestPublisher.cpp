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
 * @file VideoPublisher.cpp
 *
 */

#include "VideoTestPublisher.h"
#include "fastrtps/log/Log.h"
#include "fastrtps/log/Colors.h"
#include <fastrtps/xmlparser/XMLProfileManager.h>

#include <fstream>
#include <inttypes.h>

#define TIME_LIMIT_US 10000

using namespace eprosima;
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

VideoTestPublisher::VideoTestPublisher()
    : mp_participant(nullptr)
    , mp_datapub(nullptr)
    , mp_commandpub(nullptr)
    , mp_commandsub(nullptr)
    , mp_video_out(nullptr)
    , n_subscribers(0)
    , n_samples(0)
    , disc_count_(0)
    , comm_count_(0)
    , timer_on_(false)
    , m_status(0)
    , n_received(0)
    , m_datapublistener(nullptr)
    , m_commandpublistener(nullptr)
    , m_commandsublistener(nullptr)
    , m_dropRate(0)
    , m_sendSleepTime(0)
    , m_forcedDomain(-1)
    , m_videoWidth(1024)
    , m_videoHeight(720)
    , m_videoFrameRate(30)
{
    m_datapublistener.mp_up = this;
    m_commandpublistener.mp_up = this;
    m_commandsublistener.mp_up = this;
}

VideoTestPublisher::~VideoTestPublisher()
{
    if (sink)
    {
        gst_object_unref(GST_OBJECT(sink));
        sink = nullptr;
    }

    if (videorate)
    {
        gst_object_unref(GST_OBJECT(videorate));
        videorate = nullptr;
    }

    if (filesrc)
    {
        gst_object_unref(GST_OBJECT(filesrc));
        filesrc = nullptr;
    }

    if (pipeline)
    {
        gst_object_unref(GST_OBJECT(pipeline));
        pipeline = nullptr;
    }

    Domain::removeParticipant(mp_participant);
}


bool VideoTestPublisher::init(int n_sub, int n_sam, bool reliable, uint32_t pid, bool hostname,
        const PropertyPolicy& part_property_policy, const PropertyPolicy& property_policy, bool large_data,
        const std::string& sXMLConfigFile, int test_time, int drop_rate, int max_sleep_time,
        int forced_domain, int videoWidth, int videoHeight, int videoFrameRate)
{
    large_data = true;
    m_testTime = test_time;
    m_dropRate = drop_rate;
    m_sendSleepTime = max_sleep_time;
    m_sXMLConfigFile = sXMLConfigFile;
    n_samples = n_sam;
    n_subscribers = n_sub;
    reliable_ = reliable;
    m_forcedDomain = forced_domain;
    m_videoWidth = videoWidth;
    m_videoHeight = videoHeight;
    m_videoFrameRate = videoFrameRate;

    // GSTREAMER PIPELINE INITIALIZATION.
    InitGStreamer();

    // Create RTPSParticipant
    std::string participant_profile_name = "participant_profile";
    ParticipantAttributes PParam;
    if (m_forcedDomain >= 0)
    {
        PParam.rtps.builtin.domainId = m_forcedDomain;
    }
    else
    {
        PParam.rtps.builtin.domainId = pid % 230;
    }
    PParam.rtps.properties = part_property_policy;
    PParam.rtps.setName("Participant_pub");

    if(m_sXMLConfigFile.length() > 0)
    {
        if (m_forcedDomain >= 0)
        {
            ParticipantAttributes participant_att;
            if (eprosima::fastrtps::xmlparser::XMLP_ret::XML_OK ==
                eprosima::fastrtps::xmlparser::XMLProfileManager::fillParticipantAttributes(participant_profile_name,
                    participant_att))
            {
                participant_att.rtps.builtin.domainId = m_forcedDomain;
                mp_participant = Domain::createParticipant(participant_att);
            }
        }
        else
        {
            mp_participant = Domain::createParticipant(participant_profile_name);
        }
    }
    else
    {
        mp_participant = Domain::createParticipant(PParam);
    }

    if(mp_participant == nullptr)
    {
        return false;
    }

    // Register the type
    Domain::registerType(mp_participant, (TopicDataType*)&video_t);
    Domain::registerType(mp_participant, (TopicDataType*)&command_t);


    // Create Data Publisher
    std::string profile_name = "publisher_profile";
    PublisherAttributes PubDataparam;

    if (!reliable)
    {
        PubDataparam.qos.m_reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
    }
    PubDataparam.properties = property_policy;
    if (large_data)
    {
        PubDataparam.historyMemoryPolicy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
        PubDataparam.qos.m_publishMode.kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    }

    if(m_sXMLConfigFile.length() > 0)
    {
        eprosima::fastrtps::xmlparser::XMLProfileManager::fillPublisherAttributes(profile_name, PubDataparam);
    }

    PubDataparam.topic.topicDataType = "VideoType";
    PubDataparam.topic.topicKind = NO_KEY;
    std::ostringstream pt;
    pt << "VideoTest_";
    if (hostname)
    {
        pt << asio::ip::host_name() << "_";
    }
    pt << pid << "_PUB2SUB";
    PubDataparam.topic.topicName = pt.str();
    PubDataparam.times.heartbeatPeriod.seconds = 0;
    PubDataparam.times.heartbeatPeriod.nanosec = 100000000;

    mp_datapub = Domain::createPublisher(mp_participant, PubDataparam, (PublisherListener*)&this->m_datapublistener);
    if (mp_datapub == nullptr)
    {
        return false;
    }

    // Create Command Publisher
    PublisherAttributes PubCommandParam;
    PubCommandParam.topic.topicDataType = "TestCommandType";
    PubCommandParam.topic.topicKind = NO_KEY;
    std::ostringstream pct;
    pct << "VideoTest_Command_";
    if (hostname)
    {
        pct << asio::ip::host_name() << "_";
    }
    pct << pid << "_PUB2SUB";
    PubCommandParam.topic.topicName = pct.str();
    PubCommandParam.topic.historyQos.kind = KEEP_ALL_HISTORY_QOS;
    PubCommandParam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    PubCommandParam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    PubCommandParam.qos.m_publishMode.kind = eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE;
    mp_commandpub = Domain::createPublisher(mp_participant, PubCommandParam, &this->m_commandpublistener);

    if (mp_commandpub == nullptr)
    {
        return false;
    }

    SubscriberAttributes SubCommandParam;
    SubCommandParam.topic.topicDataType = "TestCommandType";
    SubCommandParam.topic.topicKind = NO_KEY;
    std::ostringstream sct;
    sct << "VideoTest_Command_";
    if (hostname)
    {
        sct << asio::ip::host_name() << "_";
    }
    sct << pid << "_SUB2PUB";
    SubCommandParam.topic.topicName = sct.str();
    SubCommandParam.topic.historyQos.kind = KEEP_ALL_HISTORY_QOS;
    SubCommandParam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    SubCommandParam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    mp_commandsub = Domain::createSubscriber(mp_participant, SubCommandParam, &this->m_commandsublistener);
    if (mp_commandsub == nullptr)
    {
        return false;
    }

    return true;
}

void VideoTestPublisher::DataPubListener::onPublicationMatched(Publisher* /*pub*/, MatchingInfo& info)
{
    std::unique_lock<std::mutex> lock(mp_up->mutex_);

    if(info.status == MATCHED_MATCHING)
    {
        cout << C_MAGENTA << "Data Pub Matched "<<C_DEF<<endl;

        n_matched++;
        if(n_matched > mp_up->n_subscribers)
        {
            std::cout << "More matched subscribers than expected" << std::endl;
            mp_up->m_status = -1;
        }

        ++mp_up->disc_count_;
    }
    else
    {
        --mp_up->disc_count_;
    }

    lock.unlock();
    mp_up->disc_cond_.notify_one();
}

void VideoTestPublisher::CommandPubListener::onPublicationMatched(Publisher* /*pub*/, MatchingInfo& info)
{
    std::unique_lock<std::mutex> lock(mp_up->mutex_);

    if(info.status == MATCHED_MATCHING)
    {
        cout << C_MAGENTA << "Command Pub Matched "<<C_DEF<<endl;

        n_matched++;
        if(n_matched > mp_up->n_subscribers)
        {
            std::cout << "More matched subscribers than expected" << std::endl;
            mp_up->m_status = -1;
        }

        ++mp_up->disc_count_;
    }
    else
    {
        --mp_up->disc_count_;
    }

    lock.unlock();
    mp_up->disc_cond_.notify_one();
}

void VideoTestPublisher::CommandSubListener::onSubscriptionMatched(Subscriber* /*sub*/,MatchingInfo& info)
{
    std::unique_lock<std::mutex> lock(mp_up->mutex_);
    if(info.status == MATCHED_MATCHING)
    {
        cout << C_MAGENTA << "Command Sub Matched "<<C_DEF<<endl;

        n_matched++;
        if(n_matched > mp_up->n_subscribers)
        {
            std::cout << "More matched subscribers than expected" << std::endl;
            mp_up->m_status = -1;
        }

        ++mp_up->disc_count_;
    }
    else
    {
        --mp_up->disc_count_;
    }

    lock.unlock();
    mp_up->disc_cond_.notify_one();
}

void VideoTestPublisher::CommandSubListener::onNewDataMessage(Subscriber* subscriber)
{
    TestCommandType command;
    SampleInfo_t info;
    //	cout << "COMMAND RECEIVED"<<endl;
    if(subscriber->takeNextData((void*)&command,&info))
    {
        if(info.sampleKind == ALIVE)
        {
            //cout << "ALIVE "<<command.m_command<<endl;
            if(command.m_command == BEGIN)
            {
                //	cout << "POSTING"<<endl;
                mp_up->mutex_.lock();
                ++mp_up->comm_count_;
                mp_up->mutex_.unlock();
                mp_up->comm_cond_.notify_one();
            }
        }
    }
    else
        cout<< "Problem reading"<<endl;
}

void VideoTestPublisher::run()
{
    //WAIT FOR THE DISCOVERY PROCESS FO FINISH:
    //EACH SUBSCRIBER NEEDS 4 Matchings (2 publishers and 2 subscribers)
    std::unique_lock<std::mutex> disc_lock(mutex_);
    disc_cond_.wait(disc_lock, [&]() { return disc_count_ >= (n_subscribers * 3); });
    disc_lock.unlock();

    cout << C_B_MAGENTA << "DISCOVERY COMPLETE "<<C_DEF<<endl;

    this->test(0);

    cout << "REMOVING PUBLISHER"<<endl;
    Domain::removePublisher(this->mp_commandpub);
    cout << "REMOVING SUBSCRIBER"<<endl;
    Domain::removeSubscriber(mp_commandsub);

    std::string str_reliable = "besteffort";
    if (reliable_)
    {
        str_reliable = "reliable";
    }
}

bool VideoTestPublisher::test(uint32_t datasize)
{
    m_status = 0;
    n_received = 0;
    // Create video sample with size <datasize>
    mp_video_out = new VideoType(datasize);

    // Send READY command
    TestCommandType command;
    command.m_command = READY;
    if (mp_commandpub->write(&command))
    {
        std::cout << "Start command sent" << std::endl;
    }
    else
    {
        std::cout << "Error: Start command NOT sent" << std::endl;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    // Wait for all the subscribers
    comm_cond_.wait(lock, [&]() {
        return comm_count_ >= n_subscribers;
    });
    --comm_count_;

    // BEGIN THE TEST
    // --------------------------------------------------------------
    // Get new sample
    mp_video_out = new VideoType();
    // Start "playing" on the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Get start time
    send_start_ = std::chrono::steady_clock::now();
    // Wait until timer returns "finished"
    timer_cond_.wait(lock, [&]() {
        return timer_on_;
    });

    // Paused the sending
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

    // Send STOP command to subscriber
    command.m_command = STOP;
    if (mp_commandpub->write(&command))
    {
        std::cout << "Stop command sent" << std::endl;
    }
    else
    {
        std::cout << "Error: Stop command NOT sent" << std::endl;
    }

    // Wait until all subscribers unmatch
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int>> secs(std::chrono::duration<int>(5));
    disc_cond_.wait_until(lock, secs, [&]() {
        return disc_count_ == 0;
    });

    if(m_status !=0)
    {
        cout << "Error in test "<<endl;
        return false;
    }
    // --------------------------------------------------------------
    //TEST FINISHED

    // Clean up
    size_t removed = 0;
    mp_datapub->removeAllChange(&removed);
    delete(mp_video_out);

    return true;
}

void VideoTestPublisher::InitGStreamer()
{
    bool ok = true;
    std::string id_ = "tst";
    std::stringstream default_name;
    default_name << "pipeline_" << id_.c_str();
    pipeline = gst_element_factory_make("pipeline", default_name.str().c_str());
    ok = (pipeline != nullptr);
    if (ok)
    {
        filesrc = gst_element_factory_make("videotestsrc", "source");
        ok = (filesrc != nullptr);
        if (ok)
        {
            g_object_set(filesrc, "is-live", TRUE, NULL);

            videorate = gst_element_factory_make("videorate", "rate"); //autovideosink
            ok = (videorate != nullptr);
            if (ok)
            {
                sink = gst_element_factory_make("appsink", "sink"); //autovideosink appsink fpsdisplaysink
                ok = (sink != nullptr);
                if (ok)
                {
                    g_object_set(sink, "emit-signals", TRUE, NULL);
                    g_signal_connect(sink, "new-sample", G_CALLBACK(new_sample), this);

                    /*std::string sFramerate = std::to_string(m_videoFrameRate) + "/1";*/
                    GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420",
                        "width", G_TYPE_INT, m_videoWidth, "height", G_TYPE_INT, m_videoHeight,
                        "framerate", GST_TYPE_FRACTION, m_videoFrameRate, 1, NULL);

                    // Link the camera source and colorspace filter using capabilities specified
                    gst_bin_add_many(GST_BIN(pipeline), filesrc, videorate, sink, NULL);
                    //ok = gst_element_link_filtered(filesrc, sink, caps) == TRUE;
                    ok = gst_element_link_filtered(filesrc, videorate, caps) == TRUE;
                    ok = gst_element_link_filtered(videorate, sink, caps) == TRUE;
                    gst_caps_unref(caps);
                }
            }
        }
    }
}

/* The appsink has received a buffer */
GstFlowReturn VideoTestPublisher::new_sample(GstElement *sink, VideoTestPublisher *sub)
{
    GstFlowReturn returned_value = GST_FLOW_ERROR;
    if (sub->mp_video_out != nullptr)
    {
        if (sub->m_sendSleepTime != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % sub->m_sendSleepTime));
        }

        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        if (sample)
        {
            GstBuffer* buffer = gst_sample_get_buffer(sample);
            if (buffer != nullptr)
            {
                GstClockTime timestamp = GST_BUFFER_PTS(buffer);
                GstClockTime duration = GST_BUFFER_DURATION(buffer);
                GstMapInfo map;
                if (gst_buffer_map(buffer, &map, GST_MAP_READ))
                {
                    sub->mp_video_out->seqnum++;
                    sub->mp_video_out->duration = duration;
                    sub->mp_video_out->timestamp = timestamp;

                    //std::cout << "NEW SAMPLE " << timestamp << std::endl;

                    sub->mp_video_out->data.assign(map.data, map.data + map.size);
                    sub->t_start_ = std::chrono::steady_clock::now();

                    if (rand() % 100 > sub->m_dropRate)
                    {
                        if (!sub->mp_datapub->write((void*)sub->mp_video_out))
                        {
                            std::cout << "VideoPublication::run -> Cannot write video" << std::endl;
                        }
                    }
                    gst_buffer_unmap(buffer, &map);
                }
            }
            else
            {
                std::cout << "VideoPublication::run -> Buffer is nullptr" << std::endl;
            }

            gst_sample_unref(sample);
            returned_value =  GST_FLOW_OK;
        }
        else
        {
            std::cout << "VideoPublication::run -> Sample is nullptr" << std::endl;
        }

        returned_value =  GST_FLOW_ERROR;
    }
    else
    {
        std::cout << "VideoPublication::run -> Sample is nullptr" << std::endl;
    }

    returned_value =  GST_FLOW_OK;

    std::chrono::steady_clock::time_point send_end = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lock(sub->mutex_);
    if(std::chrono::duration<double, std::ratio<1, 1>>(send_end - sub->send_start_).count() >= sub->m_testTime)
    {
        sub->timer_on_ = true;
        sub->timer_cond_.notify_one();
    }

    return returned_value;
}
