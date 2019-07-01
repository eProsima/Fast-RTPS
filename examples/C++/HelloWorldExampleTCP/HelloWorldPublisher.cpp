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
 * @file HelloWorldPublisher.cpp
 *
 */

#include "HelloWorldPublisher.h"
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/Domain.h>
#include <fastrtps/utils/eClock.h>
#include <fastrtps/utils/IPLocator.h>

#include <thread>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

HelloWorldPublisher::HelloWorldPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
{
}

bool HelloWorldPublisher::init(
        const std::string &wan_ip,
        unsigned short port,
        bool use_tls,
        const std::vector<std::string>& whitelist)
{
    stop_ = false;
    hello_.index(0);
    hello_.message("HelloWorld");
    ParticipantAttributes pparam;

    pparam.rtps.builtin.domainId = 0;
    pparam.rtps.builtin.leaseDuration = c_TimeInfinite;
    pparam.rtps.builtin.leaseDuration_announcementperiod = Duration_t(5, 0);
    pparam.rtps.setName("Participant_pub");

    pparam.rtps.useBuiltinTransports = false;

    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();

    for (std::string ip : whitelist)
    {
        descriptor->interfaceWhiteList.push_back(ip);
        std::cout << "Whitelisted " << ip << std::endl;
    }

    if (use_tls)
    {
        using TLSOptions = TCPTransportDescriptor::TLSConfig::TLSOptions;
        descriptor->apply_security = true;
        descriptor->tls_config.password = "test";
        descriptor->tls_config.cert_chain_file = "server.pem";
        descriptor->tls_config.private_key_file = "server.pem";
        descriptor->tls_config.tmp_dh_file = "dh2048.pem";
        descriptor->tls_config.add_option(TLSOptions::DEFAULT_WORKAROUNDS);
        descriptor->tls_config.add_option(TLSOptions::SINGLE_DH_USE);
        descriptor->tls_config.add_option(TLSOptions::NO_SSLV2);
    }

    descriptor->wait_for_tcp_negotiation = false;
    descriptor->sendBufferSize = 0;
    descriptor->receiveBufferSize = 0;

    if (!wan_ip.empty())
    {
        descriptor->set_WAN_address(wan_ip);
        std::cout << wan_ip << ":" << port << std::endl;
    }
    descriptor->add_listener_port(port);
    pparam.rtps.userTransports.push_back(descriptor);

    participant_ = Domain::createParticipant(pparam);

    if (participant_ == nullptr)
    {
        return false;
    }
    //REGISTER THE TYPE

    Domain::registerType(participant_, &type_);

    //CREATE THE PUBLISHER
    PublisherAttributes wparam;
    wparam.topic.topicKind = NO_KEY;
    wparam.topic.topicDataType = "HelloWorld";
    wparam.topic.topicName = "HelloWorldTopicTCP";
    wparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
    wparam.topic.historyQos.depth = 30;
    wparam.topic.resourceLimitsQos.max_samples = 50;
    wparam.topic.resourceLimitsQos.allocated_samples = 20;
    wparam.times.heartbeatPeriod.seconds = 2;
    wparam.times.heartbeatPeriod.nanosec = 200 * 1000 * 1000;
    wparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    publisher_ = Domain::createPublisher(participant_, wparam, (PublisherListener*)&listener_);
    if (publisher_ == nullptr)
    {
        return false;
    }

    return true;
}

HelloWorldPublisher::~HelloWorldPublisher()
{
    Domain::removeParticipant(participant_);
}

void HelloWorldPublisher::PubListener::onPublicationMatched(
        Publisher*,
        MatchingInfo& info)
{
    if (info.status == MATCHED_MATCHING)
    {
        n_matched++;
        firstConnected = true;
        //logError(HW, "Matched");
        std::cout << "[RTCP] Publisher matched" << std::endl;
    }
    else
    {
        n_matched--;
        std::cout << "[RTCP] Publisher unmatched" << std::endl;
    }
}

void HelloWorldPublisher::runThread(
        uint32_t samples,
        long sleep_ms)
{
    if (samples == 0)
    {
        while (!stop_)
        {
            if (publish(false))
            {
                //logError(HW, "SENT " <<  hello_.index());
                std::cout << "[RTCP] Message: " << hello_.message() << " with index: "
                    << hello_.index() << " SENT" << std::endl;
            }
            eClock::my_sleep(sleep_ms);
        }
    }
    else
    {
        for (uint32_t i = 0; i < samples; ++i)
        {
            if (!publish())
            {
                --i;
            }
            else
            {
                std::cout << "[RTCP] Message: " << hello_.message() << " with index: "
                    << hello_.index() << " SENT" << std::endl;
            }
            eClock::my_sleep(sleep_ms);
        }
    }
}

void HelloWorldPublisher::run(
        uint32_t samples,
        long sleep_ms)
{
    std::thread thread(&HelloWorldPublisher::runThread, this, samples, sleep_ms);
    if (samples == 0)
    {
        std::cout << "Publisher running. Please press enter to stop the Publisher at any time." << std::endl;
        std::cin.ignore();
        stop_ = true;
    }
    else
    {
        std::cout << "Publisher running " << samples << " samples." << std::endl;
    }
    thread.join();
}

bool HelloWorldPublisher::publish(bool waitForListener)
{
    if (listener_.firstConnected || !waitForListener || listener_.n_matched > 0)
    {
        hello_.index(hello_.index() + 1);
        publisher_->write((void*)&hello_);
        return true;
    }
    return false;
}
