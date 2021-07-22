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
 * @file StatefulWriter.h
 */

#ifndef _FASTDDS_RTPS_STATEFULWRITER_H_
#define _FASTDDS_RTPS_STATEFULWRITER_H_

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/writer/RTPSWriter.h>
#include <fastdds/rtps/writer/IReaderDataFilter.hpp>
#include <fastdds/rtps/history/IChangePool.h>
#include <fastdds/rtps/history/IPayloadPool.h>
#include <fastrtps/utils/collections/ResourceLimitedVector.hpp>
#include <condition_variable>
#include <mutex>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class ReaderProxy;
class TimedEvent;

/**
 * Class StatefulWriter, specialization of RTPSWriter that maintains information of each matched Reader.
 * @ingroup WRITER_MODULE
 */
class StatefulWriter : public RTPSWriter
{
    friend class RTPSParticipantImpl;
    friend class ReaderProxy;

public:

    //!Destructor
    virtual ~StatefulWriter();

protected:

    //!Constructor
    StatefulWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    StatefulWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    StatefulWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            const std::shared_ptr<IChangePool>& change_pool,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    void rebuild_status_after_load();

    virtual void print_inconsistent_acknack(
            const GUID_t& writer_guid,
            const GUID_t& reader_guid,
            const SequenceNumber_t& min_requested_sequence_number,
            const SequenceNumber_t& max_requested_sequence_number,
            const SequenceNumber_t& next_sequence_number);

private:

    void init(
            RTPSParticipantImpl* pimpl,
            const WriterAttributes& att);

    //!Timed Event to manage the periodic HB to the Reader.
    TimedEvent* periodic_hb_event_;

    //! Timed Event to manage the Acknack response delay.
    TimedEvent* nack_response_event_;

    //! A timed event to mark samples as acknowledget (used only if disable positive ACKs QoS is enabled)
    TimedEvent* ack_event_;

    //!Count of the sent heartbeats.
    Count_t m_heartbeatCount;
    //!WriterTimes
    WriterTimes m_times;

    //! Vector containing all the remote ReaderProxies.
    ResourceLimitedVector<ReaderProxy*> matched_remote_readers_;
    //! Vector containing all the inactive, ready for reuse, ReaderProxies.
    ResourceLimitedVector<ReaderProxy*> matched_readers_pool_;

    using ReaderProxyIterator = ResourceLimitedVector<ReaderProxy*>::iterator;
    using ReaderProxyConstIterator = ResourceLimitedVector<ReaderProxy*>::const_iterator;

    //!To avoid notifying twice of the same sequence number
    SequenceNumber_t next_all_acked_notify_sequence_;
    SequenceNumber_t min_readers_low_mark_;

    // TODO Join this mutex when main mutex would not be recursive.
    std::mutex all_acked_mutex_;
    std::condition_variable all_acked_cond_;
    // TODO Also remove when main mutex not recursive.
    bool all_acked_;
    std::condition_variable_any may_remove_change_cond_;
    unsigned int may_remove_change_;

public:

    /**
     * Add a specific change to all ReaderLocators.
     * @param p Pointer to the change.
     * @param max_blocking_time
     */
    void unsent_change_added_to_history(
            CacheChange_t* p,
            const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) override;

    /**
     * Indicate the writer that a change has been removed by the history due to some HistoryQos requirement.
     * @param a_change Pointer to the change that is going to be removed.
     * @return True if removed correctly.
     */
    bool change_removed_by_history(
            CacheChange_t* a_change) override;

    /**
     * Sends a change directly to a intraprocess reader.
     */
    bool intraprocess_delivery(
            CacheChange_t* change,
            ReaderProxy* reader_proxy);

    bool intraprocess_gap(
            ReaderProxy* reader_proxy,
            const SequenceNumber_t& seq_num)
    {
        SequenceNumber_t last_seq = seq_num + 1;
        return intraprocess_gap(reader_proxy, seq_num, last_seq);
    }

    bool intraprocess_gap(
            ReaderProxy* reader_proxy,
            const SequenceNumber_t& first_seq,
            const SequenceNumber_t& last_seq);

    bool intraprocess_heartbeat(
            ReaderProxy* reader_proxy,
            bool liveliness = false);

    //!Increment the HB count.
    inline void incrementHBCount()
    {
        on_heartbeat(++m_heartbeatCount);
    }

    /**
     * Add a matched reader.
     * @param data Pointer to the ReaderProxyData object added.
     * @return True if added.
     */
    bool matched_reader_add(
            const ReaderProxyData& data) override;

    /**
     * Remove a matched reader.
     * @param reader_guid GUID of the reader to remove.
     * @return True if removed.
     */
    bool matched_reader_remove(
            const GUID_t& reader_guid) override;

    /**
     * Tells us if a specific Reader is matched against this writer
     * @param reader_guid GUID of the reader to check.
     * @return True if it was matched.
     */
    bool matched_reader_is_matched(
            const GUID_t& reader_guid) override;

    bool is_acked_by_all(
            const CacheChange_t* a_change) const override;

    template <typename Function>
    Function for_each_reader_proxy(
            Function f) const
    {
        // we cannot directly pass iterators neither const_iterators to matched_readers_ because then the functor would
        // be able to modify ReaderProxy elements
        for ( const ReaderProxy* rp : matched_local_readers_ )
        {
            f(rp);
        }
        for ( const ReaderProxy* rp : matched_datasharing_readers_ )
        {
            f(rp);
        }
        for ( const ReaderProxy* rp : matched_remote_readers_ )
        {
            f(rp);
        }

        return f;
    }

    bool wait_for_all_acked(
            const Duration_t& max_wait) override;

    bool all_readers_updated();

    /**
     * Remove the change with the minimum SequenceNumber
     * @return True if removed.
     */
    bool try_remove_change(
            const std::chrono::steady_clock::time_point& max_blocking_time_point,
            std::unique_lock<RecursiveTimedMutex>& lock) override;

    bool wait_for_acknowledgement(
            const SequenceNumber_t& seq,
            const std::chrono::steady_clock::time_point& max_blocking_time_point,
            std::unique_lock<RecursiveTimedMutex>& lock) override;

    /**
     * Update the Attributes of the Writer.
     * @param att New attributes
     */
    void updateAttributes(
            const WriterAttributes& att) override;

    /**
     * Find a Reader Proxy in this writer.
     * @param[in] readerGuid The GUID_t of the reader.
     * @param[out] RP Pointer to pointer to return the ReaderProxy.
     * @return True if correct.
     */
    bool matched_reader_lookup(
            GUID_t& readerGuid,
            ReaderProxy** RP);

    /** Get count of heartbeats
     * @return count of heartbeats
     */
    inline Count_t getHeartbeatCount() const
    {
        return this->m_heartbeatCount;
    }

    /**
     * Get the RTPS participant
     * @return RTPS participant
     */
    inline RTPSParticipantImpl* getRTPSParticipant() const
    {
        return mp_RTPSParticipant;
    }

    /**
     * Get the number of matched readers
     * @return Number of the matched readers
     */
    inline size_t getMatchedReadersSize() const
    {
        std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
        return matched_remote_readers_.size()
               + matched_local_readers_.size()
               + matched_datasharing_readers_.size();
    }

    /**
     * @brief Returns true if disable positive ACKs QoS is enabled
     * @return True if positive acks are disabled, false otherwise
     */
    inline bool get_disable_positive_acks() const
    {
        return disable_positive_acks_;
    }

    /**
     * Update the WriterTimes attributes of all associated ReaderProxy.
     * @param times WriterTimes parameter.
     */
    void updateTimes(
            const WriterTimes& times);

    SequenceNumber_t next_sequence_number() const;

    /**
     * @brief Sends a periodic heartbeat
     * @param final Final flag
     * @param liveliness Liveliness flag
     * @return True on success
     */
    bool send_periodic_heartbeat(
            bool final = false,
            bool liveliness = false);

    /*!
     * @brief Sends a heartbeat to a remote reader.
     * @remarks This function is non thread-safe.
     */
    void send_heartbeat_to_nts(
            ReaderProxy& remoteReaderProxy,
            bool liveliness = false,
            bool force = false);

    void perform_nack_response();

    void perform_nack_supression(
            const GUID_t& reader_guid);

    /**
     * Process an incoming ACKNACK submessage.
     * @param[in] writer_guid      GUID of the writer the submessage is directed to.
     * @param[in] reader_guid      GUID of the reader originating the submessage.
     * @param[in] ack_count        Count field of the submessage.
     * @param[in] sn_set           Sequence number bitmap field of the submessage.
     * @param[in] final_flag       Final flag field of the submessage.
     * @param[out] result          true if the writer could process the submessage.
     *                             Only valid when returned value is true.
     * @return true when the submessage was destinated to this writer, false otherwise.
     */
    bool process_acknack(
            const GUID_t& writer_guid,
            const GUID_t& reader_guid,
            uint32_t ack_count,
            const SequenceNumberSet_t& sn_set,
            bool final_flag,
            bool& result) override;

    /**
     * Process an incoming NACKFRAG submessage.
     * @param[in] writer_guid      GUID of the writer the submessage is directed to.
     * @param[in] reader_guid      GUID of the reader originating the submessage.
     * @param[in] ack_count        Count field of the submessage.
     * @param[in] seq_num          Sequence number field of the submessage.
     * @param[in] fragments_state  Sequence number field of the submessage.
     * @param[out] result          true if the writer could process the submessage.
     *                             Only valid when returned value is true.
     * @return true when the submessage was destinated to this writer, false otherwise.
     */
    virtual bool process_nack_frag(
            const GUID_t& writer_guid,
            const GUID_t& reader_guid,
            uint32_t ack_count,
            const SequenceNumber_t& seq_num,
            const FragmentNumberSet_t fragments_state,
            bool& result) override;

    /**
     * @brief Set a reader data filter to filter data in ReaderProxies
     * @param reader_data_filter The reader data filter
     */
    void reader_data_filter(
            fastdds::rtps::IReaderDataFilter* reader_data_filter);

    /**
     * @brief Get the reader data filter used to filter data in ReaderProxies
     */
    const fastdds::rtps::IReaderDataFilter* reader_data_filter() const;

    /*!
     * Tells writer the sample can be sent to the network.
     * This function should be used by a fastdds::rtps::FlowController.
     *
     * @param cache_change Pointer to the CacheChange_t that represents the sample which can be sent.
     * @param group RTPSMessageGroup reference uses for generating the RTPS message.
     * @param locator_selector RTPSMessageSenderInterface reference uses for selecting locators. The reference has to
     * be a member of this RTPSWriter object.
     * @param max_blocking_time_point Future timepoint where blocking send should end.
     * @return Return code.
     * @note Must be non-thread safe.
     */
    DeliveryRetCode deliver_sample_nts(
            CacheChange_t* cache_change,
            RTPSMessageGroup& group,
            LocatorSelectorSender& locator_selector,
            const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) override;

    LocatorSelectorSender& get_general_locator_selector() override
    {
        return locator_selector_general_;
    }

    LocatorSelectorSender& get_async_locator_selector() override
    {
        return locator_selector_async_;
    }

private:

    bool is_acked_by_all(
            const SequenceNumber_t seq) const;

    void update_reader_info(
            LocatorSelectorSender& locator_selector,
            bool create_sender_resources);

    void send_heartbeat_piggyback_nts_(
            ReaderProxy* reader,
            RTPSMessageGroup& message_group,
            LocatorSelectorSender& locator_selector,
            uint32_t& last_bytes_processed);

    void send_heartbeat_nts_(
            size_t number_of_readers,
            RTPSMessageGroup& message_group,
            bool final,
            bool liveliness = false);

    void check_acked_status();

    /**
     * @brief A method called when the ack timer expires
     * @details Only used if disable positive ACKs QoS is enabled
     */
    bool ack_timer_expired();

    void send_heartbeat_to_all_readers();

    void deliver_sample_to_intraprocesses(
            CacheChange_t* change);

    void deliver_sample_to_datasharing(
            CacheChange_t* change);

    DeliveryRetCode deliver_sample_to_network(
            CacheChange_t* change,
            RTPSMessageGroup& group,
            LocatorSelectorSender& locator_selector,
            const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time);

    void prepare_datasharing_delivery(
            CacheChange_t* change);

    //! True to disable piggyback heartbeats
    bool disable_heartbeat_piggyback_;
    //! True to disable positive ACKs
    bool disable_positive_acks_;
    //! Keep duration for disable positive ACKs QoS, in microseconds
    std::chrono::duration<double, std::ratio<1, 1000000>> keep_duration_us_;
    //! Last acknowledged cache change (only used if using disable positive ACKs QoS)
    SequenceNumber_t last_sequence_number_;
    //! Biggest sequence number removed from history
    SequenceNumber_t biggest_removed_sequence_number_;

    const uint32_t sendBufferSize_;

    int32_t currentUsageSendBufferSize_;

    bool there_are_remote_readers_ = false;
    bool there_are_local_readers_ = false;

    StatefulWriter& operator =(
            const StatefulWriter&) = delete;

    //! The filter for the reader
    fastdds::rtps::IReaderDataFilter* reader_data_filter_ = nullptr;
    //! Vector containing all the active ReaderProxies for intraprocess delivery.
    ResourceLimitedVector<ReaderProxy*> matched_local_readers_;
    //! Vector containing all the active ReaderProxies for datasharing delivery.
    ResourceLimitedVector<ReaderProxy*> matched_datasharing_readers_;
    bool there_are_datasharing_readers_ = false;

    LocatorSelectorSender locator_selector_general_;

    LocatorSelectorSender locator_selector_async_;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif /* _FASTDDS_RTPS_STATEFULWRITER_H_ */
