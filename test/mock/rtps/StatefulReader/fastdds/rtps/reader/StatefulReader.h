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

#ifndef _FASTDDS_RTPS_READER_STATEFULREADER_H_
#define _FASTDDS_RTPS_READER_STATEFULREADER_H_

#include <fastrtps/rtps/reader/RTPSReader.h>
#include <fastrtps/rtps/attributes/ReaderAttributes.h>
#include <fastrtps/rtps/common/Guid.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class RTPSMessageGroup_t;
class WriterProxy;
class RTPSMessageSenderInterface;
class RTPSParticipantImpl;
struct CDRMessage_t;

using NetworkBuffer = eprosima::fastdds::rtps::NetworkBuffer;

class StatefulReader : public RTPSReader
{
public:

    StatefulReader()
    {
    }

    StatefulReader(
            ReaderHistory* history,
            RecursiveTimedMutex* mutex)
        : RTPSReader(history, mutex)
    {
    }

    virtual ~StatefulReader()
    {
    }

    // *INDENT-OFF* Uncrustify makes a mess with MOCK_METHOD macros
    MOCK_METHOD1(matched_writer_add, bool(const WriterProxyData&));

    MOCK_METHOD2(matched_writer_remove, bool(const GUID_t&, bool));

    MOCK_METHOD1(liveliness_expired, bool(const GUID_t&));

    MOCK_METHOD2(change_received, bool(CacheChange_t* a_change, WriterProxy* prox));

    MOCK_METHOD1 (matched_writer_is_matched, bool(const GUID_t& writer_guid));
    // *INDENT-ON*


    // In real class, inherited from Endpoint base class.
    inline const GUID_t& getGuid() const
    {
        return guid_;
    }

    ReaderTimes& getTimes()
    {
        return times_;
    }

    void send_acknack(
            const WriterProxy* /*writer*/,
            const SequenceNumberSet_t& sns,
            const RTPSMessageSenderInterface& /*sender*/,
            bool /*is_final*/)
    {
        // only insterested in SequenceNumberSet_t.
        simp_send_acknack( sns );
    }

    // See gmock cookbook #SimplerInterfaces
    MOCK_METHOD1(simp_send_acknack, void(const SequenceNumberSet_t& seq));

    void send_acknack(
            const WriterProxy* /*writer*/,
            const RTPSMessageSenderInterface& /*sender*/,
            bool /*heartbeat_was_final*/)
    {
    }

    RTPSParticipantImpl* getRTPSParticipant() const
    {
        return nullptr;
    }

    bool send_sync_nts(
            const NetworkBuffer* /*buffers*/,
            size_t /*num_buffers*/,
            uint32_t /*total_bytes*/,
            const LocatorsIterator& /*destination_locators_begin*/,
            const LocatorsIterator& /*destination_locators_end*/,
            std::chrono::steady_clock::time_point& /*max_blocking_time_point*/)
    {
        return true;
    }

private:

    GUID_t guid_;

    ReaderTimes times_;
};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // _FASTDDS_RTPS_READER_STATEFULREADER_H_
