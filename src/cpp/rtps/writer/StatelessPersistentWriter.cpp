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
 * @file StatelessPersistentWriter.cpp
 *
 */

#include <fastdds/rtps/writer/StatelessPersistentWriter.h>
#include <fastdds/rtps/history/WriterHistory.h>
#include <rtps/persistence/PersistenceService.h>
#include <fastrtps_deprecated/participant/ParticipantImpl.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

StatelessPersistentWriter::StatelessPersistentWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen,
        IPersistenceService* persistence)
    : StatelessWriter(pimpl, guid, att, flow_controller, hist, listen)
    , PersistentWriter(guid, att, payload_pool_, change_pool_, hist, persistence)
{
}

StatelessPersistentWriter::StatelessPersistentWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen,
        IPersistenceService* persistence)
    : StatelessWriter(pimpl, guid, att, payload_pool, flow_controller, hist, listen)
    , PersistentWriter(guid, att, payload_pool_, change_pool_, hist, persistence)
{
}

StatelessPersistentWriter::StatelessPersistentWriter(
        RTPSParticipantImpl* pimpl,
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        fastdds::rtps::FlowController* flow_controller,
        WriterHistory* hist,
        WriterListener* listen,
        IPersistenceService* persistence)
    : StatelessWriter(pimpl, guid, att, payload_pool, change_pool, flow_controller, hist, listen)
    , PersistentWriter(guid, att, payload_pool_, change_pool_, hist, persistence)
{
}

StatelessPersistentWriter::~StatelessPersistentWriter()
{
    deinit();
}

/*
 *	CHANGE-RELATED METHODS
 */

void StatelessPersistentWriter::unsent_change_added_to_history(
        CacheChange_t* cptr,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time)
{
    add_persistent_change(cptr);
    StatelessWriter::unsent_change_added_to_history(cptr, max_blocking_time);
}

bool StatelessPersistentWriter::change_removed_by_history(
        CacheChange_t* change)
{
    remove_persistent_change(change);
    return StatelessWriter::change_removed_by_history(change);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
