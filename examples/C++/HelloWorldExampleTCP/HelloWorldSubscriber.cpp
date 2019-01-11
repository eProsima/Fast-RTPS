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
 * @file HelloWorldSubscriber.cpp
 *
 */

#include "HelloWorldSubscriber.h"
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/Domain.h>
#include <fastrtps/utils/eClock.h>
#include <fastrtps/utils/IPLocator.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

HelloWorldSubscriber::HelloWorldSubscriber():mp_participant(nullptr),
mp_subscriber(nullptr)
{
}

bool HelloWorldSubscriber::init(const std::string &wan_ip, unsigned short port)
{
    ParticipantAttributes PParam;
    int32_t kind = LOCATOR_KIND_TCPv4;

    Locator_t initial_peer_locator;
    initial_peer_locator.kind = kind;
	if (!wan_ip.empty())
	{
		IPLocator::setIPv4(initial_peer_locator, wan_ip);
		std::cout << wan_ip << ":" << port << std::endl;
	}
	else
	{
		IPLocator::setIPv4(initial_peer_locator, "127.0.0.1");
	}
    initial_peer_locator.port = port;
    PParam.rtps.builtin.initialPeersList.push_back(initial_peer_locator); // Publisher's meta channel

    PParam.rtps.builtin.domainId = 0;
    PParam.rtps.builtin.leaseDuration = c_TimeInfinite;
    PParam.rtps.builtin.leaseDuration_announcementperiod = Duration_t(5, 0);
    PParam.rtps.setName("Participant_sub");

    PParam.rtps.useBuiltinTransports = false;
    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();
    descriptor->wait_for_tcp_negotiation = false;
    PParam.rtps.userTransports.push_back(descriptor);

    mp_participant = Domain::createParticipant(PParam);
    if(mp_participant==nullptr)
        return false;

    //REGISTER THE TYPE
    Domain::registerType(mp_participant,&m_type);

    //CREATE THE SUBSCRIBER
    SubscriberAttributes Rparam;
    Rparam.topic.topicKind = NO_KEY;
    Rparam.topic.topicDataType = "HelloWorld";
    Rparam.topic.topicName = "HelloWorldTopicTCP";
    Rparam.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
    Rparam.topic.historyQos.depth = 30;
    Rparam.topic.resourceLimitsQos.max_samples = 50;
    Rparam.topic.resourceLimitsQos.allocated_samples = 20;
    Rparam.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    Rparam.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    mp_subscriber = Domain::createSubscriber(mp_participant,Rparam,(SubscriberListener*)&m_listener);

    if(mp_subscriber == nullptr)
        return false;


    return true;
}

HelloWorldSubscriber::~HelloWorldSubscriber() {
    Domain::removeParticipant(mp_participant);
}

void HelloWorldSubscriber::SubListener::onSubscriptionMatched(Subscriber* ,MatchingInfo& info)
{
    if(info.status == MATCHED_MATCHING)
    {
        n_matched++;
        //logError(HW, "Matched");
        std::cout << "[RTCP] Subscriber matched"<<std::endl;
    }
    else
    {
        n_matched--;
        std::cout << "[RTCP] Subscriber unmatched"<<std::endl;
    }
}

void HelloWorldSubscriber::SubListener::onNewDataMessage(Subscriber* sub)
{
    if(sub->takeNextData((void*)&m_Hello, &m_info))
    {
        if(m_info.sampleKind == ALIVE)
        {
            this->n_samples++;
            // Print your structure data here.
            //logError(HW, "RECEIVED " <<  m_Hello.index());
            std::cout << "[RTCP] Message "<<m_Hello.message()<< " "<< m_Hello.index()<< " RECEIVED"<<std::endl;
        }
    }

}


void HelloWorldSubscriber::run()
{
    std::cout << "[RTCP] Subscriber running. Please press enter to stop the Subscriber" << std::endl;
    std::cin.ignore();
}

void HelloWorldSubscriber::run(uint32_t number)
{
    std::cout << "[RTCP] Subscriber running until "<< number << "samples have been received"<<std::endl;
    while(number < this->m_listener.n_samples)
        eClock::my_sleep(500);
}
