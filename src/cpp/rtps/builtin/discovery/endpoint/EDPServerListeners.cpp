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
 * @file EDPServerListener.cpp
 *
 */

#include <fastdds/rtps/writer/StatefulWriter.h>
#include <fastdds/rtps/history/WriterHistory.h>
#include <fastdds/rtps/history/ReaderHistory.h>
#include <fastdds/rtps/reader/RTPSReader.h>

#include <fastrtps/utils/fixed_size_string.hpp>

#include <fastdds/dds/log/Log.hpp>

#include <rtps/builtin/discovery/endpoint/EDPServerListeners.hpp>
#include <rtps/builtin/discovery/endpoint/EDPServer.hpp>
#include <rtps/builtin/discovery/participant/PDPServer.hpp>

namespace eprosima {
namespace fastdds {
namespace rtps {

using namespace eprosima::fastrtps::rtps;

PDPServer* EDPServerPUBListener::get_pdp()
{
    return sedp_->get_pdp();
}

EDPServerPUBListener::EDPServerPUBListener(
        EDPServer* sedp)
    : EDPBasePUBListener(sedp->mp_RTPSParticipant->getAttributes().allocation.locators,
            sedp->mp_RTPSParticipant->getAttributes().allocation.data_limits)
    , sedp_(sedp)
{
}

void EDPServerPUBListener::onNewCacheChangeAdded(
        RTPSReader* reader,
        const CacheChange_t* const change_in)
{
    logInfo(RTPS_EDP_LISTENER, "");
    logInfo(RTPS_EDP_LISTENER, "------------------ EDP PUB SERVER LISTENER START ------------------");
    logInfo(RTPS_EDP_LISTENER,
            "-------------------- " << sedp_->mp_RTPSParticipant->getGuid() << " --------------------");

    // Create a new change from the one received
    CacheChange_t* change = (CacheChange_t*)change_in;
    logInfo(RTPS_EDP_LISTENER, "EDP Server PUB Message received: " << change_in->instanceHandle);

    // DATA(w)s should have key
    if (!computeKey(change))
    {
        logWarning(RTPS_EDP_LISTENER, "Received change with no Key");
    }

    // Get writer's GUID and EDP publications' reader history
    GUID_t auxGUID = iHandle2GUID(change->instanceHandle);
    ReaderHistory* reader_history = sedp_->publications_reader_.second;

    // Related_sample_identity could be lost in message delivered, so we set as sample_identity
    // An empty related_sample_identity could lead into an empty sample_identity when resending this msg
    if (change->write_params.related_sample_identity() == SampleIdentity::unknown())
    {
        change->write_params.related_sample_identity(change->write_params.sample_identity());
    }

    // Reset the internal CacheChange_t union.
    change->writer_info.next = nullptr;
    change->writer_info.previous = nullptr;
    change->writer_info.num_sent_submessages = 0;

    // String to store the topic of the writer
    std::string topic_name = "";

    // DATA(w) case: new writer or updated information about an existing writer
    if (change->kind == ALIVE)
    {
        // Note: add_writer_from_change() removes the change from the EDP publications' reader history, but it does not
        // return it to the pool
        add_writer_from_change(reader, reader_history, change, sedp_, false);

        // Retrieve the topic after creating the WriterProxyData (in add_writer_from_change()). This way, not matter
        // whether the DATA(w) is a new one or an update, the WriterProxyData exists, and so the topic can be retrieved
        if (get_pdp()->lookupWriterProxyData(auxGUID, temp_writer_data_))
        {
            topic_name = temp_writer_data_.topicName().to_string();
        }
    }
    // DATA(Uw) case
    else
    {
        logInfo(RTPS_EDP_LISTENER, "Disposed Remote Writer, removing...");

        // Retrieve the topic before removing the WriterProxyData. We need it to add the DATA(Uw) to the database
        if (get_pdp()->lookupWriterProxyData(auxGUID, temp_writer_data_))
        {
            topic_name = temp_writer_data_.topicName().to_string();
        }
        else
        {
            logWarning(RTPS_EDP_LISTENER, "Writer Proxy Data missing for change " << auxGUID);
        }


        // Remove WriterProxy data information
        get_pdp()->removeWriterProxyData(auxGUID);

        // Removing change from history, not returning the change to the pool, since the ownership will be yielded to
        // the database
        reader_history->remove_change(reader_history->find_change(change), false);
    }

    // Notify the DiscoveryDataBase if it is enabled already
    // In case it is not enable, the change should not be updated or released because it is been
    // updated from a backup
    if (!get_pdp()->discovery_db().backup_in_progress())
    {
        if (topic_name.size() > 0 &&
                get_pdp()->discovery_db().update(change, topic_name))
        {
            // From here on, the discovery database takes ownership of the CacheChange_t. Henceforth there are no
            // references to the CacheChange_t.
            // Ensure processing time for the cache by triggering the Server thread (which process the updates
            get_pdp()->awake_routine_thread();
        }
        else
        {
            // If the database doesn't take the ownership, then return the CacheChante_t to the pool.
            reader->releaseCache(change);
        }
    }
    logInfo(RTPS_EDP_LISTENER,
            "-------------------- " << sedp_->mp_RTPSParticipant->getGuid() << " --------------------");
    logInfo(RTPS_EDP_LISTENER, "------------------ EDP PUB SERVER LISTENER END ------------------");
    logInfo(RTPS_EDP_LISTENER, "");
}

PDPServer* EDPServerSUBListener::get_pdp()
{
    return sedp_->get_pdp();
}

EDPServerSUBListener::EDPServerSUBListener(
        EDPServer* sedp)
    : EDPBaseSUBListener(sedp->mp_RTPSParticipant->getAttributes().allocation.locators,
            sedp->mp_RTPSParticipant->getAttributes().allocation.data_limits)
    , sedp_(sedp)
{
}

void EDPServerSUBListener::onNewCacheChangeAdded(
        RTPSReader* reader,
        const CacheChange_t* const change_in)
{
    logInfo(RTPS_EDP_LISTENER, "");
    logInfo(RTPS_EDP_LISTENER, "------------------ EDP SUB SERVER LISTENER START ------------------");
    logInfo(RTPS_EDP_LISTENER,
            "-------------------- " << sedp_->mp_RTPSParticipant->getGuid() << " --------------------");

    // Create a new change from the one received
    CacheChange_t* change = (CacheChange_t*)change_in;
    logInfo(RTPS_EDP_LISTENER, "EDP Server SUB Message received: " << change_in->instanceHandle);

    // DATA(r)s should have key
    if (!computeKey(change))
    {
        logWarning(RTPS_EDP_LISTENER, "Received change with no Key");
    }

    // Related_sample_identity could be lost in message delivered, so we set as sample_identity
    // An empty related_sample_identity could lead into an empty sample_identity when resending this msg
    if (change->write_params.related_sample_identity() == SampleIdentity::unknown())
    {
        change->write_params.related_sample_identity(change->write_params.sample_identity());
    }

    // Reset the internal CacheChange_t union.
    change->writer_info.next = nullptr;
    change->writer_info.previous = nullptr;
    change->writer_info.num_sent_submessages = 0;

    // Get readers's GUID and EDP subscriptions' reader history
    GUID_t auxGUID = iHandle2GUID(change->instanceHandle);
    ReaderHistory* reader_history = sedp_->subscriptions_reader_.second;

    // String to store the topic of the reader
    std::string topic_name = "";

    // DATA(r) case: new reader or updated information about an existing reader
    if (change->kind == ALIVE)
    {
        // Note: add_reader_from_change() removes the change from the EDP subscriptions' reader history, but it does not
        // return it to the pool
        add_reader_from_change(reader, reader_history, change, sedp_, false);

        // Retrieve the topic after creating the ReaderProxyData (in add_reader_from_change()). This way, not matter
        // whether the DATA(r) is a new one or an update, the ReaderProxyData exists, and so the topic can be retrieved
        if (get_pdp()->lookupReaderProxyData(auxGUID, temp_reader_data_))
        {
            topic_name = temp_reader_data_.topicName().to_string();
        }
        else
        {
            logWarning(RTPS_EDP_LISTENER, "Reader Proxy Data missing for change " << auxGUID);
        }
    }
    // DATA(Ur) case
    else
    {
        //REMOVE WRITER FROM OUR READERS:
        logInfo(RTPS_EDP_LISTENER, "Disposed Remote Reader, removing...");

        // Retrieve the topic before removing the ReaderProxyData. We need it to add the DATA(Ur) to the database
        if (get_pdp()->lookupReaderProxyData(auxGUID, temp_reader_data_))
        {
            topic_name = temp_reader_data_.topicName().to_string();
        }

        // Remove ReaderProxy data information
        get_pdp()->removeReaderProxyData(auxGUID);

        // Removing change from history, not returning the change to the pool, since the ownership will be yielded to
        // the database
        reader_history->remove_change(reader_history->find_change(change), false);
    }

    // Notify the DiscoveryDataBase if it is enabled already
    // In case it is not enable, the change should not be updated or released because it is been
    // updated from a backup
    if (!get_pdp()->discovery_db().backup_in_progress())
    {
        if (topic_name.size() > 0 &&
                get_pdp()->discovery_db().update(change, topic_name))
        {
            // From here on, the discovery database takes ownership of the CacheChange_t. Henceforth there are no
            // references to the CacheChange_t.
            // Ensure processing time for the cache by triggering the Server thread (which process the updates
            get_pdp()->awake_routine_thread();
        }
        else
        {
            // If the database doesn't take the ownership, then return the CacheChante_t to the pool.
            reader->releaseCache(change);
        }
    }

    logInfo(RTPS_EDP_LISTENER,
            "-------------------- " << sedp_->mp_RTPSParticipant->getGuid() << " --------------------");
    logInfo(RTPS_EDP_LISTENER, "------------------ EDP SUB SERVER LISTENER END ------------------");
    logInfo(RTPS_EDP_LISTENER, "");
}

} /* namespace rtps */
} // namespace fastdds
} /* namespace eprosima */
