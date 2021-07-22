// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/*
 * @file StatefulPersistentWriter.cpp
 *
 */

#include <fastdds/rtps/writer/StatefulPersistentWriter.h>
#include <fastdds/rtps/history/WriterHistory.h>
#include <rtps/persistence/PersistenceService.h>
#include <fastrtps_deprecated/participant/ParticipantImpl.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {


StatefulPersistentWriter::StatefulPersistentWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen,
        IPersistenceService* persistence)
    : StatefulWriter(pimpl, guid, att, flow_controller, hist, listen)
    , PersistentWriter(guid, att, payload_pool_, change_pool_, hist, persistence)
{
    rebuild_status_after_load();
}

StatefulPersistentWriter::StatefulPersistentWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen,
        IPersistenceService* persistence)
    : StatefulWriter(pimpl, guid, att, payload_pool, flow_controller, hist, listen)
    , PersistentWriter(guid, att, payload_pool_, change_pool_, hist, persistence)
{
}

StatefulPersistentWriter::StatefulPersistentWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen,
        IPersistenceService* persistence)
    : StatefulWriter(pimpl, guid, att, payload_pool, change_pool, flow_controller, hist, listen)
    , PersistentWriter(guid, att, payload_pool_, change_pool_, hist, persistence)
{
}

StatefulPersistentWriter::~StatefulPersistentWriter()
{
    deinit();
}

/*
 *	CHANGE-RELATED METHODS
 */

void StatefulPersistentWriter::unsent_change_added_to_history(
        CacheChange_t* cptr,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    add_persistent_change(cptr);
    StatefulWriter::unsent_change_added_to_history(cptr, max_blocking_time);
}

bool StatefulPersistentWriter::change_removed_by_history(
        CacheChange_t* change)
{
    remove_persistent_change(change);
    return StatefulWriter::change_removed_by_history(change);
}

void StatefulPersistentWriter::print_inconsistent_acknack(
        const GUID_t& writer_guid,
        const GUID_t& reader_guid,
        const SequenceNumber_t& min_requested_sequence_number,
        const SequenceNumber_t& max_requested_sequence_number,
        const SequenceNumber_t& next_sequence_number)
{
    if (!log_error_printed_)
    {
        log_error_printed_ = true;
        logError(RTPS_WRITER, "Inconsistent acknack received in Local Writer "
                << writer_guid << ". Maybe the persistent database has been erased locally.");
    }
    StatefulWriter::print_inconsistent_acknack(writer_guid, reader_guid, min_requested_sequence_number,
            max_requested_sequence_number, next_sequence_number);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
