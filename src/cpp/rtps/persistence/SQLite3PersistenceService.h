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
 * @file SQLite3PersistenceService.h
 */

#ifndef SQLITE3PERSISTENCESERVICE_H_
#define SQLITE3PERSISTENCESERVICE_H_

#include <rtps/persistence/PersistenceService.h>
#include <rtps/persistence/sqlite3.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
 * Create a new SQLite3 implementation of persistence service
 * @ingroup RTPS_PERSISTENCE_MODULE
 */
IPersistenceService* create_SQLite3_persistence_service(
        const char* filename,
        bool update_schema);


/**
 * Persistence service implementation over SQLite3
 * @ingroup RTPS_PERSISTENCE_MODULE
 */
class SQLite3PersistenceService : public IPersistenceService
{
public:

    SQLite3PersistenceService(
            sqlite3* db);
    virtual ~SQLite3PersistenceService() override;

    /**
     * Get all data stored for a writer.
     * @param writer_guid GUID of the writer to load.
     * @return True if operation was successful.
     */
    bool load_writer_from_storage(
            const std::string& persistence_guid,
            const GUID_t& writer_guid,
            WriterHistory* history,
            const std::shared_ptr<IChangePool>& change_pool,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            SequenceNumber_t& next_sequence) final;

    /**
     * Add a change to storage.
     * @param change The cache change to add.
     * @return True if operation was successful.
     */
    virtual bool add_writer_change_to_storage(
            const std::string& persistence_guid,
            const CacheChange_t& change) final;

    /**
     * Remove a change from storage.
     * @param change The cache change to remove.
     * @return True if operation was successful.
     */
    virtual bool remove_writer_change_from_storage(
            const std::string& persistence_guid,
            const CacheChange_t& change) final;

    /**
     * Get all data stored for a reader.
     * @param reader_guid GUID of the reader to load.
     * @return True if operation was successful.
     */
    virtual bool load_reader_from_storage(
            const std::string& reader_guid,
            foonathan::memory::map<GUID_t, SequenceNumber_t, map_allocator_t>& seq_map) final;

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
            const SequenceNumber_t& seq_number) final;

private:

    sqlite3* db_;

    sqlite3_stmt* load_writer_stmt_;
    sqlite3_stmt* add_writer_change_stmt_;
    sqlite3_stmt* remove_writer_change_stmt_;

    sqlite3_stmt* load_writer_last_seq_num_stmt_;
    sqlite3_stmt* update_writer_last_seq_num_stmt_;

    sqlite3_stmt* load_reader_stmt_;
    sqlite3_stmt* update_reader_stmt_;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif /*  */
