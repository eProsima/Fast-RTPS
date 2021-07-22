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
 * @file TestWriterSocket.cpp
 *
 */

#include "TestWriterSocket.h"

#include "fastrtps/rtps/writer/RTPSWriter.h"
#include "fastrtps/rtps/participant/RTPSParticipant.h"
#include "fastrtps/rtps/RTPSDomain.h"

#include "fastrtps/rtps/attributes/RTPSParticipantAttributes.h"
#include "fastrtps/rtps/attributes/WriterAttributes.h"
#include "fastrtps/rtps/attributes/HistoryAttributes.h"

#include "fastrtps/rtps/builtin/data/ReaderProxyData.h"

#include "fastrtps/rtps/history/WriterHistory.h"
#include "fastrtps/utils/IPLocator.h"

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

TestWriterSocket::TestWriterSocket()
    : mp_participant(nullptr)
    , mp_writer(nullptr)
    , mp_history(nullptr)
{
}

TestWriterSocket::~TestWriterSocket()
{
    RTPSDomain::removeRTPSParticipant(mp_participant);
    delete(mp_history);
}

bool TestWriterSocket::init(
        std::string ip,
        uint32_t port)
{
    //CREATE PARTICIPANT
    RTPSParticipantAttributes PParam;
    PParam.builtin.discovery_config.discoveryProtocol = eprosima::fastrtps::rtps::DiscoveryProtocol::NONE;
    PParam.builtin.use_WriterLivelinessProtocol = false;
    mp_participant = RTPSDomain::createParticipant(0, PParam);
    if (mp_participant == nullptr)
    {
        return false;
    }

    //CREATE WRITERHISTORY
    HistoryAttributes hatt;
    hatt.payloadMaxSize = 255;
    mp_history = new WriterHistory(hatt);

    //CREATE WRITER
    WriterAttributes watt;
    watt.endpoint.reliabilityKind = BEST_EFFORT;
    mp_writer = RTPSDomain::createRTPSWriter(mp_participant, watt, mp_history);
    if (mp_writer == nullptr)
    {
        return false;
    }

    //ADD REMOTE READER (IN THIS CASE A READER IN THE SAME MACHINE)
    ReaderProxyData ratt(4u, 1u);
    ratt.guid({c_GuidPrefix_Unknown, 0x304});
    Locator_t loc;
    IPLocator::setIPv4(loc, ip);
    loc.port = static_cast<uint16_t>(port);
    ratt.add_unicast_locator(loc);
    mp_writer->matched_reader_add(ratt);
    return true;
}

void TestWriterSocket::run(
        uint16_t nmsgs)
{
    for (int i = 0; i < nmsgs; ++i )
    {
        CacheChange_t* ch = mp_writer->new_change([]() -> uint32_t
                        {
                            return 255;
                        }, ALIVE);
#if defined(_WIN32)
        ch->serializedPayload.length =
                sprintf_s((char*)ch->serializedPayload.data, 255, "My example string %d", i) + 1;
#else
        ch->serializedPayload.length =
                sprintf((char*)ch->serializedPayload.data, "My example string %d", i) + 1;
#endif // if defined(_WIN32)
        printf("Sending: %s\n", (char*)ch->serializedPayload.data);
        mp_history->add_change(ch);
    }
}
