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

/**
 * @file PersistenceService.h
 */

#ifndef PERSISTENCESERVICE_H_
#define PERSISTENCESERVICE_H_

#include <fastdds/rtps/common/Guid.h>
#include <fastdds/rtps/common/CacheChange.h>
#include <fastdds/rtps/attributes/PropertyPolicy.h>
#include <fastdds/rtps/history/IChangePool.h>
#include <fastdds/rtps/history/IPayloadPool.h>

#include <foonathan/memory/container.hpp>
#include <foonathan/memory/memory_pool.hpp>

#include <map>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class WriterHistory;

/**
 * Abstract interface representing a persistence service implementaion
 * @ingroup RTPS_PERSISTENCE_MODULE
 */
class IPersistenceService
{
public:

    using map_allocator_t =
            foonathan::memory::memory_pool<foonathan::memory::node_pool, foonathan::memory::heap_allocator>;

    virtual ~IPersistenceService() = default;

    /**
     * Get all data stored for a writer.
     * @param persistence_guid   GUID of the writer used to store samples.
     * @param writer_guid        GUID of the writer to load.
     * @param changes            History of the writer to load.
     * @param change_pool        Pool where new changes should be obtained from.
     * @param payload_pool       Pool where payloads should be obtained from.
     * @param next_sequence      Sequence that should be applied to the next created sample.
     * @return True if operation was successful.
     */
    virtual bool load_writer_from_storage(
            const std::string& persistence_guid,
            const GUID_t& writer_guid,
            WriterHistory* history,
            const std::shared_ptr<IChangePool>& change_pool,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            SequenceNumber_t& next_sequence) = 0;

    /**
     * Add a change to storage.
     * @param persistence_guid   GUID of the writer used to store samples.
     * @param change             The cache change to add.
     * @return True if operation was successful.
     */
    virtual bool add_writer_change_to_storage(
            const std::string& persistence_guid,
            const CacheChange_t& change) = 0;

    /**
     * Remove a change from storage.
     * @param persistence_guid   GUID of the writer used to store samples.
     * @param change             The cache change to remove.
     * @return True if operation was successful.
     */
    virtual bool remove_writer_change_from_storage(
            const std::string& persistence_guid,
            const CacheChange_t& change) = 0;

    /**
     * Get all data stored for a reader.
     * @param reader_guid   GUID of the reader to load.
     * @param seq_map       History record (map of low marks) to be loaded.
     * @return True if operation was successful.
     */
    virtual bool load_reader_from_storage(
            const std::string& reader_guid,
            foonathan::memory::map<GUID_t, SequenceNumber_t, map_allocator_t>& seq_map) = 0;

    /**
     * Update the sequence number associated to a writer on a reader.
     * @param reader_guid GUID of the reader to update.
     * @param writer_guid GUID of the associated writer to update.
     * @param seq_number New sequence number value to set for the associated writer.
     * @return True if operation was successful.
     */
    virtual bool update_writer_seq_on_storage(
            const std::string& reader_guid,
            const GUID_t& writer_guid,
            const SequenceNumber_t& seq_number) = 0;

    static std::vector<CacheChange_t*>& get_changes(
            WriterHistory* history);

    static void set_fragments(
            WriterHistory* history,
            CacheChange_t* change);

};

/**
 * Abstract factory to create a persistence service from participant or endpoint properties
 * @ingroup RTPS_PERSISTENCE_MODULE
 */
class PersistenceFactory
{
public:

    /**
     * Create a persistence service implementation
     * @param property_policy PropertyPolicy where the persistence configuration will be searched
     * @return A pointer to a persistence service implementation. nullptr when policy does not contain the necessary properties or if persistence service could not be created
     */
    static IPersistenceService* create_persistence_service(
            const PropertyPolicy& property_policy);
};


} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif /* PERSISTENCESERVICE_H_ */
