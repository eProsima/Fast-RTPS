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
 * @file PersistentWriter.cpp
 *
 */

#include <fastdds/rtps/writer/PersistentWriter.h>
#include <fastdds/rtps/history/WriterHistory.h>
#include <rtps/persistence/PersistenceService.h>
#include <fastrtps_deprecated/participant/ParticipantImpl.h>
#include <rtps/DataSharing/WriterPool.hpp>

namespace eprosima {
namespace fastrtps {
namespace rtps {


PersistentWriter::PersistentWriter(
        const GUID_t& guid,
        const WriterAttributes& att,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        const std::shared_ptr<IChangePool>& change_pool,
        WriterHistory* hist,
        IPersistenceService* persistence)
    : persistence_(persistence)
    , persistence_guid_()
{
    // When persistence GUID is unknown, create from rtps GUID
    GUID_t p_guid = att.endpoint.persistence_guid == c_Guid_Unknown ? guid : att.endpoint.persistence_guid;
    std::ostringstream ss;
    ss << p_guid;
    persistence_guid_ = ss.str();

    persistence_->load_writer_from_storage(persistence_guid_, guid, hist,
            change_pool, payload_pool, hist->m_lastCacheChangeSeqNum);

    // Update history state after loading from DB
    hist->m_isHistoryFull =
            hist->m_att.maximumReservedCaches > 0 &&
            static_cast<int32_t>(hist->m_changes.size()) == hist->m_att.maximumReservedCaches;

    // Prepare the changes for datasharing if compatible
    if (att.endpoint.data_sharing_configuration().kind() != OFF)
    {
        auto pool = std::dynamic_pointer_cast<WriterPool>(payload_pool);
        assert(pool != nullptr);
        for (auto change : hist->m_changes)
        {
            pool->add_to_shared_history(change);
        }
    }
}

PersistentWriter::~PersistentWriter()
{
    delete persistence_;
}

/*
 * CHANGE-RELATED METHODS
 */

void PersistentWriter::add_persistent_change(
        CacheChange_t* cptr)
{
    persistence_->add_writer_change_to_storage(persistence_guid_, *cptr);
}

void PersistentWriter::remove_persistent_change(
        CacheChange_t* change)
{
    persistence_->remove_writer_change_from_storage(persistence_guid_, *change);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
