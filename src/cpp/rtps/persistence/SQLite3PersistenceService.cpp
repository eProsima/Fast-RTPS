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
 * @file SQLite3PersistenceService.cpp
 *
 */

#include <rtps/persistence/SQLite3PersistenceService.h>
#include <rtps/persistence/SQLite3PersistenceServiceStatements.h>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/rtps/history/WriterHistory.h>
#include <fastrtps/utils/TimeConversion.h>

#include <rtps/persistence/sqlite3.h>

#include <sstream>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
 * @brief Retrieve the schema version of the database
 * @param db [IN] Database of which we want to get the schema version
 * @return Integer representing the schema version. Zero if an error occurs during the processing.
 */
static unsigned int database_version(
        sqlite3* db)
{
    sqlite3_stmt* version_stmt;
    unsigned int version = 1;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &version_stmt, NULL) != SQLITE_OK)
    {
        return 0;
    }

    if (SQLITE_ROW == sqlite3_step(version_stmt))
    {
        version = sqlite3_column_int(version_stmt, 0);
        if (version == 0)
        {
            //No version information. It really means version 1
            version = 1;
        }
    }
    sqlite3_finalize(version_stmt);
    return version;
}

static int upgrade(
        sqlite3* db,
        int from,
        int to)
{
    if (from == to)
    {
        return SQLITE_OK;
    }

    if (from == 1 && to == 2)
    {
        return sqlite3_exec(db, SQLite3PersistenceServiceSchemaV2::update_from_v1_statement().c_str(), 0, 0, 0);
    }

    if (from == 2 && to == 3
            && SQLite3PersistenceServiceSchemaV3::database_create_temporary_defaults_table(db))
    {
        return sqlite3_exec(db, SQLite3PersistenceServiceSchemaV3::update_from_v2_statement().c_str(), 0, 0, 0);
    }

    // iterate if not direct upgrade
    if (from < to)
    {
        if (SQLITE_ERROR != upgrade(db, from, to - 1))
        {
            return upgrade(db, to - 1, to);
        }
    }

    // unsupported upgrade path
    logError(RTPS_PERSISTENCE, "Unsupported database upgrade from version " << from << " to version " << to);
    return SQLITE_ERROR;
}

static sqlite3* open_or_create_database(
        const char* filename,
        bool update_schema)
{
    sqlite3* db = NULL;
    int rc;
    int version = 3;

    // Open database
    int flags = SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_SHAREDCACHE;
    rc = sqlite3_open_v2(filename, &db, flags, 0); // malloc that is not erased
    if (rc != SQLITE_OK)
    {
        // In case file cantopen, memory is reserved in db, so it must be free (for valgrind sake)
        if (rc == SQLITE_CANTOPEN)
        {
            sqlite3_close(db);
        }

        //probably the database does not exists. Create new and no need to upgrade schema
        flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_SHAREDCACHE;
        rc = sqlite3_open_v2(filename, &db, flags, 0);
        if (rc != SQLITE_OK)
        {
            logError(RTPS_PERSISTENCE, "Unable to create persistence database " << filename);
            sqlite3_close(db);
            return NULL;
        }
    }
    else
    {
        // Find the database version and handle upgrades
        int db_version = database_version(db);
        if (db_version == 0)
        {
            logError(RTPS_PERSISTENCE, "Error retrieving version on database " << filename);
            sqlite3_close(db);
            return NULL;
        }
        if (db_version != version)
        {
            if (update_schema)
            {
                rc = upgrade(db, db_version, version);
                if (rc != SQLITE_OK)
                {
                    sqlite3_close(db);
                    return NULL;
                }
            }
            else
            {
                logError(RTPS_PERSISTENCE, "Old schema version " << db_version << " on database " << filename
                                                                 << ". Set property dds.persistence.update_schema to force automatic schema upgrade");
                sqlite3_close(db);
                return NULL;
            }

        }
    }

    // Create tables if they don't exist
    rc = sqlite3_exec(db, SQLite3PersistenceServiceSchemaV3::database_create_statement().c_str(), 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        return NULL;
    }

    return db;
}

static void finalize_statement(
        sqlite3_stmt*& stmt)
{
    if (stmt != NULL)
    {
        int res = sqlite3_finalize(stmt);
        if (res != SQLITE_OK)
        {
            logWarning(RTPS_PERSISTENCE, "Statement could not be finalized. sqlite3_finalize code: " << res);
        }
        stmt = NULL;
    }
}

IPersistenceService* create_SQLite3_persistence_service(
        const char* filename,
        bool update_schema)
{
    sqlite3* db = open_or_create_database(filename, update_schema);
    return (db == NULL) ? nullptr : new SQLite3PersistenceService(db);
}

SQLite3PersistenceService::SQLite3PersistenceService(
        sqlite3* db)
    : db_(db)
    , load_writer_stmt_(NULL)
    , add_writer_change_stmt_(NULL)
    , remove_writer_change_stmt_(NULL)
    , load_writer_last_seq_num_stmt_(NULL)
    , update_writer_last_seq_num_stmt_(NULL)
    , load_reader_stmt_(NULL)
    , update_reader_stmt_(NULL)
{
    // Prepare writer statements
    sqlite3_prepare_v3(db_, "SELECT seq_num, instance, payload, related_sample_guid, related_sample_seq_num, source_timestamp "
            "FROM writers_histories WHERE guid=?;", -1,
            SQLITE_PREPARE_PERSISTENT,
            &load_writer_stmt_,
            NULL);
    sqlite3_prepare_v3(db_, "INSERT INTO writers_histories VALUES(?,?,?,?,?,?,?);", -1, SQLITE_PREPARE_PERSISTENT,
            &add_writer_change_stmt_, NULL);
    sqlite3_prepare_v3(db_, "DELETE FROM writers_histories WHERE guid=? AND seq_num=?;", -1, SQLITE_PREPARE_PERSISTENT,
            &remove_writer_change_stmt_, NULL);

    sqlite3_prepare_v3(db_, "SELECT last_seq_num FROM writers_states WHERE guid=?;", -1, SQLITE_PREPARE_PERSISTENT,
            &load_writer_last_seq_num_stmt_, NULL);
    sqlite3_prepare_v3(db_, "INSERT OR REPLACE INTO writers_states VALUES(?,?);", -1, SQLITE_PREPARE_PERSISTENT,
            &update_writer_last_seq_num_stmt_, NULL);

    // Prepare reader statements
    sqlite3_prepare_v3(db_, "SELECT writer_guid_prefix,writer_guid_entity,seq_num FROM readers WHERE guid=?;", -1,
            SQLITE_PREPARE_PERSISTENT, &load_reader_stmt_, NULL);
    sqlite3_prepare_v3(db_, "INSERT OR REPLACE INTO readers VALUES(?,?,?,?);", -1, SQLITE_PREPARE_PERSISTENT,
            &update_reader_stmt_, NULL);
}

SQLite3PersistenceService::~SQLite3PersistenceService()
{
    // Finalize writer statements
    finalize_statement(load_writer_stmt_);
    finalize_statement(add_writer_change_stmt_);
    finalize_statement(remove_writer_change_stmt_);

    // Finalize reader statements
    finalize_statement(load_reader_stmt_);
    finalize_statement(update_reader_stmt_);

    // Finalize writer seq_num statements
    finalize_statement(load_writer_last_seq_num_stmt_);
    finalize_statement(update_writer_last_seq_num_stmt_);

    int res = sqlite3_close(db_);
    if (res != SQLITE_OK) // (0) SQLITE_OK
    {
        logError(RTPS_PERSISTENCE, "Database could not be closed. sqlite3_close code: " << res);
    }
    db_ = NULL;
}

/**
 * Get all data stored for a writer.
 * @param persistence_guid GUID of persistence service that holds the data.
 * @param writer_guid GUID of the writer to load.
 * @param changes History of CacheChanges of the writer. It will be filled.
 * @param pool Pool of CacheChanges from which new ones are reserved to add to the history.
 * @param next_sequence Buffer to fill with the last sequence number on the history.
 * @return True if operation was successful.
 */
bool SQLite3PersistenceService::load_writer_from_storage(
        const std::string& persistence_guid,
        const GUID_t& writer_guid,
        WriterHistory* history,
        const std::shared_ptr<IChangePool>& change_pool,
        const std::shared_ptr<IPayloadPool>& payload_pool,
        SequenceNumber_t& next_sequence)
{
    logInfo(RTPS_PERSISTENCE, "Loading writer " << writer_guid);

    if (load_writer_stmt_ != NULL)
    {
        sqlite3_reset(load_writer_stmt_);
        sqlite3_bind_text(load_writer_stmt_, 1, persistence_guid.c_str(), -1, SQLITE_STATIC);

        std::vector<CacheChange_t*>& changes = get_changes(history);

        while (SQLITE_ROW == sqlite3_step(load_writer_stmt_))
        {
            SequenceNumber_t sn(sqlite3_column_int64(load_writer_stmt_, 0));
            CacheChange_t* change = nullptr;
            int size = sqlite3_column_bytes(load_writer_stmt_, 2);

            if (!change_pool->reserve_cache(change))
            {
                continue;
            }

            SampleIdentity identity;
            identity.writer_guid(writer_guid);
            identity.sequence_number(sn);
            if (!payload_pool->get_payload(size, *change))
            {
                change_pool->release_cache(change);
                continue;
            }

            int instance_size = sqlite3_column_bytes(load_writer_stmt_, 1);
            instance_size = (instance_size > 16) ? 16 : instance_size;
            change->kind = ALIVE;
            change->writerGUID = writer_guid;
            memcpy(change->instanceHandle.value, sqlite3_column_blob(load_writer_stmt_, 1), instance_size);
            change->sequenceNumber = identity.sequence_number();
            change->serializedPayload.length = size;
            memcpy(change->serializedPayload.data, sqlite3_column_blob(load_writer_stmt_, 2), size);
            change->writer_info.previous = nullptr;
            change->writer_info.next = nullptr;
            change->writer_info.num_sent_submessages = 0;

            // related sample identity
            {
                using namespace std;
                // GUID_t
                istringstream is(string(reinterpret_cast<const char*>(sqlite3_column_text(load_writer_stmt_, 3))));
                auto& si = change->write_params.related_sample_identity();
                is >> si.writer_guid();
                // Sequence Number
                SequenceNumber_t rsn(sqlite3_column_int64(load_writer_stmt_, 4));
                si.sequence_number(rsn);
            }

            // timestamp
            change->sourceTimestamp.from_ns(sqlite3_column_int64(load_writer_stmt_, 5));

            set_fragments(history, change);

            changes.insert(changes.begin(), change);
        }

        sqlite3_reset(load_writer_last_seq_num_stmt_);
        sqlite3_bind_text(load_writer_last_seq_num_stmt_, 1, persistence_guid.c_str(), -1, SQLITE_STATIC);

        while (SQLITE_ROW == sqlite3_step(load_writer_last_seq_num_stmt_))
        {
            next_sequence = SequenceNumber_t(sqlite3_column_int64(load_writer_last_seq_num_stmt_, 0));
        }
    }

    return true;
}

/**
 * Add a change to storage.
 * @param change The cache change to add.
 * @return True if operation was successful.
 */
bool SQLite3PersistenceService::add_writer_change_to_storage(
        const std::string& persistence_guid,
        const CacheChange_t& change)
{
    logInfo(RTPS_PERSISTENCE, "Writer " << change.writerGUID << " storing change for seq " << change.sequenceNumber);

    if (add_writer_change_stmt_ != NULL)
    {
        //First add the last seq number, it is needed for the foreign key on writers_histories
        sqlite3_reset(update_writer_last_seq_num_stmt_);
        sqlite3_bind_text(update_writer_last_seq_num_stmt_, 1, persistence_guid.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(update_writer_last_seq_num_stmt_, 2, change.sequenceNumber.to64long());

        if (sqlite3_step(update_writer_last_seq_num_stmt_) == SQLITE_DONE)
        {
            sqlite3_reset(add_writer_change_stmt_);
            sqlite3_bind_text(add_writer_change_stmt_, 1, persistence_guid.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int64(add_writer_change_stmt_, 2, change.sequenceNumber.to64long());
            if (change.instanceHandle.isDefined())
            {
                sqlite3_bind_blob(add_writer_change_stmt_, 3, change.instanceHandle.value, 16, SQLITE_STATIC);
            }
            else
            {
                sqlite3_bind_zeroblob(add_writer_change_stmt_, 3, 16);
            }
            sqlite3_bind_blob(add_writer_change_stmt_, 4, change.serializedPayload.data,
                    change.serializedPayload.length, SQLITE_STATIC);

            // related sample identity
            {
                using namespace std;
                ostringstream os;
                auto& si = change.write_params.related_sample_identity();
                os << si.writer_guid();
                auto guids = os.str();

                sqlite3_bind_text(add_writer_change_stmt_, 5, guids.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int64(add_writer_change_stmt_, 6, si.sequence_number().to64long());
            }

            // source time stamp
            sqlite3_bind_int64(add_writer_change_stmt_, 7, change.sourceTimestamp.to_ns());

            return sqlite3_step(add_writer_change_stmt_) == SQLITE_DONE;
        }
    }

    return false;
}

/**
 * Remove a change from storage.
 * @param change The cache change to remove.
 * @return True if operation was successful.
 */
bool SQLite3PersistenceService::remove_writer_change_from_storage(
        const std::string& persistence_guid,
        const CacheChange_t& change)
{
    logInfo(RTPS_PERSISTENCE, "Writer " << change.writerGUID << " removing change for seq " << change.sequenceNumber);

    if (remove_writer_change_stmt_ != NULL)
    {
        sqlite3_reset(remove_writer_change_stmt_);
        sqlite3_bind_text(remove_writer_change_stmt_, 1, persistence_guid.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(remove_writer_change_stmt_, 2, change.sequenceNumber.to64long());
        return sqlite3_step(remove_writer_change_stmt_) == SQLITE_DONE;
    }

    return false;
}

/**
 * Get all data stored for a reader.
 * @param reader_guid GUID of the reader to load.
 * @return True if operation was successful.
 */
bool SQLite3PersistenceService::load_reader_from_storage(
        const std::string& reader_guid,
        foonathan::memory::map<GUID_t, SequenceNumber_t, IPersistenceService::map_allocator_t>& seq_map)
{
    logInfo(RTPS_PERSISTENCE, "Loading reader " << reader_guid);

    if (load_reader_stmt_ != NULL)
    {
        sqlite3_reset(load_reader_stmt_);
        sqlite3_bind_text(load_reader_stmt_, 1, reader_guid.c_str(), -1, SQLITE_STATIC);

        while (SQLITE_ROW == sqlite3_step(load_reader_stmt_))
        {
            GUID_t guid;
            memcpy(guid.guidPrefix.value, sqlite3_column_blob(load_reader_stmt_, 0), GuidPrefix_t::size);
            memcpy(guid.entityId.value, sqlite3_column_blob(load_reader_stmt_, 1), EntityId_t::size);
            sqlite3_int64 sn = sqlite3_column_int64(load_reader_stmt_, 2);
            SequenceNumber_t seq((int32_t)((sn >> 32) & 0xFFFFFFFF), (uint32_t)(sn & 0xFFFFFFFF));
            seq_map[guid] = seq;
        }
    }

    return true;
}

/**
 * Update the sequence number associated to a writer on a reader.
 * @param reader_guid GUID of the reader to update.
 * @param writer_guid GUID of the associated writer to update.
 * @param seq_number New sequence number value to set for the associated writer.
 * @return True if operation was successful.
 */
bool SQLite3PersistenceService::update_writer_seq_on_storage(
        const std::string& reader_guid,
        const GUID_t& writer_guid,
        const SequenceNumber_t& seq_number)
{
    logInfo(RTPS_PERSISTENCE,
            "Reader " << reader_guid << " setting seq for writer " << writer_guid << " to " << seq_number);

    if (update_reader_stmt_ != NULL)
    {
        sqlite3_reset(update_reader_stmt_);
        sqlite3_bind_text(update_reader_stmt_, 1, reader_guid.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_blob(update_reader_stmt_, 2, writer_guid.guidPrefix.value, GuidPrefix_t::size, SQLITE_STATIC);
        sqlite3_bind_blob(update_reader_stmt_, 3, writer_guid.entityId.value, EntityId_t::size, SQLITE_STATIC);
        sqlite3_bind_int64(update_reader_stmt_, 4, seq_number.to64long());
        return sqlite3_step(update_reader_stmt_) == SQLITE_DONE;
    }

    return false;
}

bool SQLite3PersistenceServiceSchemaV3::database_create_temporary_defaults_table(
        sqlite3* db)
{
    using namespace std;

    sqlite3_stmt* insert_default_stmt;

    // create temporary table
    int rc = sqlite3_exec(
        db,
        "CREATE TEMP TABLE IF NOT EXISTS Defaults (Name TEST PRIMARY KEY, Value TEST);",
        0, 0, 0);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    // Insert default values
    sqlite3_prepare_v3(
        db,
        "INSERT OR REPLACE INTO TEMP.Defaults VALUES (?, ?);",
        -1, SQLITE_PREPARE_PERSISTENT,
        &insert_default_stmt, NULL);

    // Default GUID_t value
    sqlite3_reset(insert_default_stmt);
    sqlite3_bind_text(insert_default_stmt, 1, "GUID_t", -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_default_stmt, 2, SQLite3PersistenceServiceSchemaV3::default_guid(), -1, SQLITE_STATIC);
    rc = sqlite3_step(insert_default_stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    // Default SequenceNumber_t
    sqlite3_reset(insert_default_stmt);
    sqlite3_bind_text(insert_default_stmt, 1, "SequenceNumber_t", -1, SQLITE_STATIC);
    sqlite3_bind_int64(insert_default_stmt, 2, SQLite3PersistenceServiceSchemaV3::default_seqnum());
    rc = sqlite3_step(insert_default_stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    // Default rtps::Time_t
    sqlite3_reset(insert_default_stmt);
    sqlite3_bind_text(insert_default_stmt, 1, "rtps::Time_t", -1, SQLITE_STATIC);

    sqlite3_bind_int64(insert_default_stmt, 2, SQLite3PersistenceServiceSchemaV3::now());
    rc = sqlite3_step(insert_default_stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    // free resources
    finalize_statement(insert_default_stmt);

    return true;
}

const char* SQLite3PersistenceServiceSchemaV3::default_guid()
{
    using namespace std;

    static string def_guid;

    if (def_guid.empty())
    {
        ostringstream ss;
        auto def = GUID_t::unknown();
        ss << def;
        def_guid = ss.str();
    }

    return def_guid.c_str();
}

uint64_t SQLite3PersistenceServiceSchemaV3::default_seqnum()
{
    return SequenceNumber_t::unknown().to64long();
}

int64_t SQLite3PersistenceServiceSchemaV3::now()
{
    Time_t ts;
    Time_t::now(ts);
    return ts.to_ns();
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
