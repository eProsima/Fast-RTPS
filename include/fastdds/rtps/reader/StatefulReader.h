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
 * @file StatefulReader.h
 */

#ifndef _FASTDDS_RTPS_READER_STATEFULREADER_H_
#define _FASTDDS_RTPS_READER_STATEFULREADER_H_

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/reader/RTPSReader.h>
#include <fastrtps/utils/collections/ResourceLimitedVector.hpp>
#include <fastdds/rtps/common/CDRMessage_t.h>
#include <fastdds/rtps/messages/RTPSMessageGroup.h>

#include <mutex>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class WriterProxy;
class RTPSMessageSenderInterface;

/**
 * Class StatefulReader, specialization of RTPSReader than stores the state of the matched writers.
 * @ingroup READER_MODULE
 */
class StatefulReader : public RTPSReader
{
public:

    friend class RTPSParticipantImpl;

    virtual ~StatefulReader();

protected:

    StatefulReader(
            RTPSParticipantImpl* pimpl,
            const GUID_t& guid,
            const ReaderAttributes& att,
            ReaderHistory* hist,
            ReaderListener* listen = nullptr);

    StatefulReader(
            RTPSParticipantImpl* pimpl,
            const GUID_t& guid,
            const ReaderAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            ReaderHistory* hist,
            ReaderListener* listen = nullptr);

    StatefulReader(
            RTPSParticipantImpl* pimpl,
            const GUID_t& guid,
            const ReaderAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            const std::shared_ptr<IChangePool>& change_pool,
            ReaderHistory* hist,
            ReaderListener* listen = nullptr);

public:

    /**
     * Add a matched writer represented by its attributes.
     * @param wdata Attributes of the writer to add.
     * @return True if correctly added.
     */
    bool matched_writer_add(
            const WriterProxyData& wdata) override;

    /**
     * Remove a WriterProxyData from the matached writers.
     * @param writer_guid GUID of the writer to remove.
     * @param removed_by_lease true it the writer was removed due to lease duration.
     * @return True if correct.
     */
    bool matched_writer_remove(
            const GUID_t& writer_guid,
            bool removed_by_lease = false) override;

    /**
     * Tells us if a specific Writer is matched against this reader.
     * @param writer_guid GUID of the writer to check.
     * @return True if it is matched.
     */
    bool matched_writer_is_matched(
            const GUID_t& writer_guid) override;

    /**
     * Look for a specific WriterProxy.
     * @param writerGUID GUID_t of the writer we are looking for.
     * @param WP Pointer to pointer to a WriterProxy.
     * @return True if found.
     */
    bool matched_writer_lookup(
            const GUID_t& writerGUID,
            WriterProxy** WP);

    /**
     * Processes a new DATA message.
     * @param change Pointer to the CacheChange_t.
     * @return true if the reader accepts messages.
     */
    bool processDataMsg(
            CacheChange_t* change) override;

    /**
     * Processes a new DATA FRAG message.
     *
     * @param change Pointer to the CacheChange_t.
     * @param sampleSize Size of the complete, assembled message.
     * @param fragmentStartingNum Starting number of this particular message.
     * @param fragmentsInSubmessage Number of fragments on this particular message.
     * @return true if the reader accepts message.
     */
    bool processDataFragMsg(
            CacheChange_t* change,
            uint32_t sampleSize,
            uint32_t fragmentStartingNum,
            uint16_t fragmentsInSubmessage) override;

    /**
     * Processes a new HEARTBEAT message.
     *
     * @return true if the reader accepts messages.
     */
    bool processHeartbeatMsg(
            const GUID_t& writerGUID,
            uint32_t hbCount,
            const SequenceNumber_t& firstSN,
            const SequenceNumber_t& lastSN,
            bool finalFlag,
            bool livelinessFlag) override;

    bool processGapMsg(
            const GUID_t& writerGUID,
            const SequenceNumber_t& gapStart,
            const SequenceNumberSet_t& gapList) override;

    /**
     * Method to indicate the reader that some change has been removed due to HistoryQos requirements.
     * @param change Pointer to the CacheChange_t.
     * @param prox Pointer to the WriterProxy.
     * @return True if correctly removed.
     */
    bool change_removed_by_history(
            CacheChange_t* change,
            WriterProxy* prox = nullptr) override;

    /**
     * This method is called when a new change is received. This method calls the received_change of the History
     * and depending on the implementation performs different actions.
     * @param a_change Pointer of the change to add.
     * @param prox Pointer to the WriterProxy that adds the Change.
     * @return True if added.
     */
    bool change_received(
            CacheChange_t* a_change,
            WriterProxy* prox);

    /**
     * Get the RTPS participant
     * @return Associated RTPS participant
     */
    inline RTPSParticipantImpl* getRTPSParticipant() const
    {
        return mp_RTPSParticipant;
    }

    /**
     * Read the next unread CacheChange_t from the history
     * @param change Pointer to pointer of CacheChange_t
     * @param wpout Pointer to pointer the matched writer proxy
     * @return True if read.
     */
    bool nextUnreadCache(
            CacheChange_t** change,
            WriterProxy** wpout = nullptr) override;

    /**
     * Take the next CacheChange_t from the history;
     * @param change Pointer to pointer of CacheChange_t
     * @param wpout Pointer to pointer the matched writer proxy
     * @return True if read.
     */
    bool nextUntakenCache(
            CacheChange_t** change,
            WriterProxy** wpout = nullptr) override;

    /**
     * Update the times parameters of the Reader.
     * @param times ReaderTimes reference.
     * @return True if correctly updated.
     */
    bool updateTimes(
            const ReaderTimes& times);

    /**
     *
     * @return Reference to the ReaderTimes.
     */
    inline ReaderTimes& getTimes()
    {
        return times_;
    }

    /**
     * Get the number of matched writers
     * @return Number of matched writers
     */
    inline size_t getMatchedWritersSize() const
    {
        return matched_writers_.size();
    }

    /*!
     * @brief Returns there is a clean state with all Writers.
     * It occurs when the Reader received all samples sent by Writers. In other words,
     * its WriterProxies are up to date.
     * @return There is a clean state with all Writers.
     */
    bool isInCleanState() override;

    /**
     * Sends an acknack message from this reader.
     * @param writer Pointer to the info of the remote writer.
     * @param sns Sequence number bitmap with the acknack information.
     * @param sender Message sender interface.
     * @param is_final Value for final flag.
     */
    void send_acknack(
            const WriterProxy* writer,
            const SequenceNumberSet_t& sns,
            const RTPSMessageSenderInterface& sender,
            bool is_final);

    /**
     * Sends an acknack message from this reader in response to a heartbeat.
     * @param writer Pointer to the proxy representing the writer to send the acknack to.
     * @param sender Message sender interface.
     * @param heartbeat_was_final Final flag of the last received heartbeat.
     */
    void send_acknack(
            const WriterProxy* writer,
            const RTPSMessageSenderInterface& sender,
            bool heartbeat_was_final);

    /**
     * Use the participant of this reader to send a message to certain locator.
     * @param buffers Array of buffers to gather.
     * @param num_buffers Number of elements on @c buffers.
     * @param total_bytes Total size of the raw data. Should be equal to the sum of the @c length field of all buffers.
     * @param locators_begin Destination locators iterator begin.
     * @param locators_end Destination locators iterator end.
     * @param max_blocking_time_point Future time point where any blocking should end.
     */
    bool send_sync_nts(
            const RTPSMessageSenderInterface::NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            const Locators& locators_begin,
            const Locators& locators_end,
            std::chrono::steady_clock::time_point& max_blocking_time_point);

    /**
     * Assert the livelines of a matched writer.
     * @param writer GUID of the writer to assert.
     */
    void assert_writer_liveliness(
            const GUID_t& writer) override;

    /**
     * Called just before a change is going to be deserialized.
     * @param [in]  change            Pointer to the change being accessed.
     * @param [out] wp                Writer proxy the @c change belongs to.
     * @param [out] is_future_change  Whether the change is in the future (i.e. there are
     *                                earlier unreceived changes from the same writer).
     *
     * @return Whether the change is still valid or not.
     */
    bool begin_sample_access_nts(
            CacheChange_t* change,
            WriterProxy*& wp,
            bool& is_future_change) override;

    /**
     * Called after the change has been deserialized.
     * @param [in] change        Pointer to the change being accessed.
     * @param [in] wp            Writer proxy the @c change belongs to.
     * @param [in] mark_as_read  Whether the @c change should be marked as read or not.
     */
    void end_sample_access_nts(
            CacheChange_t* change,
            WriterProxy*& wp,
            bool mark_as_read) override;

    /**
     * Called when the user has retrieved a change from the history.
     * @param change Pointer to the change to ACK
     * @param writer Writer proxy of the \c change.
     * @param mark_as_read Whether the \c change should be marked as read or not
     */
    void change_read_by_user(
            CacheChange_t* change,
            const WriterProxy* writer,
            bool mark_as_read = true) override;

private:

    void init(
            RTPSParticipantImpl* pimpl,
            const ReaderAttributes& att);

    bool acceptMsgFrom(
            const GUID_t& entityGUID,
            WriterProxy** wp) const;

    /*!
     * @remarks Non thread-safe.
     */
    bool findWriterProxy(
            const GUID_t& writerGUID,
            WriterProxy** wp) const;

    void NotifyChanges(
            WriterProxy* wp);

    void remove_changes_from(
            const GUID_t& writerGUID,
            bool is_payload_pool_lost = false);

    //! Acknack Count
    uint32_t acknack_count_;
    //! NACKFRAG Count
    uint32_t nackfrag_count_;
    //!ReaderTimes of the StatefulReader.
    ReaderTimes times_;
    //! Vector containing pointers to all the active WriterProxies.
    ResourceLimitedVector<WriterProxy*> matched_writers_;
    //! Vector containing pointers to all the inactive, ready for reuse, WriterProxies.
    ResourceLimitedVector<WriterProxy*> matched_writers_pool_;
    //!
    ResourceLimitedContainerConfig proxy_changes_config_;
    //! True to disable positive ACKs
    bool disable_positive_acks_;
    //! False when being destroyed
    bool is_alive_;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#endif // _FASTDDS_RTPS_READER_STATEFULREADER_H_
