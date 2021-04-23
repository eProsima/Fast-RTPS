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
 * @file RTPSWriter.h
 */

#ifndef _FASTDDS_RTPS_RTPSWRITER_H_
#define _FASTDDS_RTPS_RTPSWRITER_H_

#include <fastdds/rtps/Endpoint.h>
#include <fastdds/rtps/messages/RTPSMessageGroup.h>
#include <fastdds/rtps/attributes/HistoryAttributes.h>
#include <fastdds/rtps/attributes/WriterAttributes.h>
#include <fastrtps/qos/LivelinessLostStatus.h>
#include <fastrtps/utils/collections/ResourceLimitedVector.hpp>
#include <fastdds/rtps/common/LocatorSelector.hpp>
#include <fastdds/rtps/messages/RTPSMessageSenderInterface.hpp>
#include <fastdds/statistics/IListeners.hpp>

#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class WriterListener;
class WriterHistory;
class FlowController;
class DataSharingNotifier;
struct CacheChange_t;

/**
 * Class RTPSWriter, manages the sending of data to the readers. Is always associated with a HistoryCache.
 * @ingroup WRITER_MODULE
 */
class RTPSWriter : public Endpoint, public RTPSMessageSenderInterface
{
    friend class WriterHistory;
    friend class RTPSParticipantImpl;
    friend class RTPSMessageGroup;
    friend class AsyncInterestTree;

protected:

    RTPSWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    RTPSWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    RTPSWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            const std::shared_ptr<IChangePool>& change_pool,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    virtual ~RTPSWriter();

public:

    /**
     * Create a new change based with the provided changeKind.
     * @param data Data of the change.
     * @param changeKind The type of change.
     * @param handle InstanceHandle to assign.
     * @return Pointer to the CacheChange or nullptr if incorrect.
     */
    template<typename T>
    CacheChange_t* new_change(
            T& data,
            ChangeKind_t changeKind,
            InstanceHandle_t handle = c_InstanceHandle_Unknown)
    {
        return new_change([data]() -> uint32_t
                       {
                           return (uint32_t)T::getCdrSerializedSize(data);
                       }, changeKind, handle);
    }

    RTPS_DllAPI CacheChange_t* new_change(
            const std::function<uint32_t()>& dataCdrSerializedSize,
            ChangeKind_t changeKind,
            InstanceHandle_t handle = c_InstanceHandle_Unknown);

    RTPS_DllAPI CacheChange_t* new_change(
            ChangeKind_t changeKind,
            InstanceHandle_t handle = c_InstanceHandle_Unknown);

    /**
     * Release a change when it is not being used anymore.
     *
     * @param change Pointer to the cache change to be released.
     *
     * @returns whether the operation succeeded or not
     *
     * @pre
     *     @li @c change is not @c nullptr
     *     @li @c change points to a cache change obtained from a call to @c this->new_change
     *
     * @post memory pointed to by @c change is not accessed
     */
    RTPS_DllAPI bool release_change(
            CacheChange_t* change);

    /**
     * Add a matched reader.
     * @param data Pointer to the ReaderProxyData object added.
     * @return True if added.
     */
    RTPS_DllAPI virtual bool matched_reader_add(
            const ReaderProxyData& data) = 0;

    /**
     * Remove a matched reader.
     * @param reader_guid GUID of the reader to remove.
     * @return True if removed.
     */
    RTPS_DllAPI virtual bool matched_reader_remove(
            const GUID_t& reader_guid) = 0;

    /**
     * Tells us if a specific Reader is matched against this writer.
     * @param reader_guid GUID of the reader to check.
     * @return True if it was matched.
     */
    RTPS_DllAPI virtual bool matched_reader_is_matched(
            const GUID_t& reader_guid) = 0;

    /**
     * Check if a specific change has been acknowledged by all Readers.
     * Is only useful in reliable Writer. In BE Writers returns false when pending to be sent.
     * @return True if acknowledged by all.
     */
    RTPS_DllAPI virtual bool is_acked_by_all(
            const CacheChange_t* /*a_change*/) const
    {
        return false;
    }

    /**
     * Waits until all changes were acknowledged or max_wait.
     * @return True if all were acknowledged.
     */
    RTPS_DllAPI virtual bool wait_for_all_acked(
            const Duration_t& /*max_wait*/)
    {
        return true;
    }

    /**
     * Update the Attributes of the Writer.
     * @param att New attributes
     */
    RTPS_DllAPI virtual void updateAttributes(
            const WriterAttributes& att) = 0;

    /**
     * This method triggers the send operation for unsent changes.
     * @return number of messages sent
     */
    RTPS_DllAPI virtual void send_any_unsent_changes() = 0;

    /**
     * Get Min Seq Num in History.
     * @return Minimum sequence number in history
     */
    RTPS_DllAPI SequenceNumber_t get_seq_num_min();

    /**
     * Get Max Seq Num in History.
     * @return Maximum sequence number in history
     */
    RTPS_DllAPI SequenceNumber_t get_seq_num_max();

    /**
     * Get maximum size of the serialized type
     * @return Maximum size of the serialized type
     */
    RTPS_DllAPI uint32_t getTypeMaxSerialized();

    //!Get maximum size of the data
    uint32_t getMaxDataSize();

    //! Calculates the maximum size of the data
    uint32_t calculateMaxDataSize(
            uint32_t length);

    /**
     * Get listener
     * @return Listener
     */
    RTPS_DllAPI inline WriterListener* getListener()
    {
        return mp_listener;
    }

    RTPS_DllAPI inline bool set_listener(
            WriterListener* listener)
    {
        mp_listener = listener;
        return true;
    }

    /**
     * Get the publication mode
     * @return publication mode
     */
    RTPS_DllAPI inline bool isAsync() const
    {
        return is_async_;
    }

    /**
     * Remove an specified max number of changes
     * @param max Maximum number of changes to remove.
     * @return at least one change has been removed
     */
    RTPS_DllAPI bool remove_older_changes(
            unsigned int max = 0);

    /**
     * Tries to remove a change waiting a maximum of the provided microseconds.
     * @param max_blocking_time_point Maximum time to wait for.
     * @param lock Lock of the Change list.
     * @return at least one change has been removed
     */
    virtual bool try_remove_change(
            const std::chrono::steady_clock::time_point& max_blocking_time_point,
            std::unique_lock<RecursiveTimedMutex>& lock) = 0;

    /**
     * Waits till a change has been acknowledged.
     * @param seq Sequence number to wait for acknowledgement.
     * @param max_blocking_time_point Maximum time to wait for.
     * @param lock Lock of the Change list.
     * @return true when change was acknowledged, false when timeout is reached.
     */
    virtual bool wait_for_acknowledgement(
            const SequenceNumber_t& seq,
            const std::chrono::steady_clock::time_point& max_blocking_time_point,
            std::unique_lock<RecursiveTimedMutex>& lock) = 0;

    /*
     * Add a flow controller that will apply to this writer exclusively.
     * @param controller
     */
    virtual void add_flow_controller(
            std::unique_ptr<FlowController> controller) = 0;

#ifdef FASTDDS_STATISTICS

    /*
     * Add a listener to receive statistics backend callbacks
     * @param listener
     * @return true if successfully added
     */
    RTPS_DllAPI bool add_statistics_listener(
            std::shared_ptr<fastdds::statistics::IListener> listener);

    /*
     * Remove a listener from receiving statistics backend callbacks
     * @param listener
     * @return true if successfully removed
     */
    RTPS_DllAPI bool remove_statistics_listener(
            std::shared_ptr<fastdds::statistics::IListener> listener);

#endif // FASTDDS_STATISTICS

    /**
     * Get RTPS participant
     * @return RTPS participant
     */
    inline RTPSParticipantImpl* getRTPSParticipant() const
    {
        return mp_RTPSParticipant;
    }

    /**
     * Enable or disable sending data to readers separately
     * NOTE: This will only work for synchronous writers
     * @param enable If separate sending should be enabled
     */
    void set_separate_sending (
            bool enable)
    {
        m_separateSendingEnabled = enable;
    }

    /**
     * Inform if data is sent to readers separately
     * @return true if separate sending is enabled
     */
    bool get_separate_sending () const
    {
        return m_separateSendingEnabled;
    }

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
    virtual bool process_acknack(
            const GUID_t& writer_guid,
            const GUID_t& reader_guid,
            uint32_t ack_count,
            const SequenceNumberSet_t& sn_set,
            bool final_flag,
            bool& result)
    {
        (void)reader_guid; (void)ack_count; (void)sn_set; (void)final_flag;

        result = false;
        return writer_guid == m_guid;
    }

    /**
     * Process an incoming NACKFRAG submessage.
     * @param[in] writer_guid      GUID of the writer the submessage is directed to.
     * @param[in] reader_guid      GUID of the reader originating the submessage.
     * @param[in] ack_count        Count field of the submessage.
     * @param[in] seq_num          Sequence number field of the submessage.
     * @param[in] fragments_state  Fragment number bitmap field of the submessage.
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
            bool& result)
    {
        (void)reader_guid; (void)ack_count; (void)seq_num; (void)fragments_state;

        result = false;
        return writer_guid == m_guid;
    }

    /**
     * @brief A method to retrieve the liveliness kind
     * @return Liveliness kind
     */
    const LivelinessQosPolicyKind& get_liveliness_kind() const;

    /**
     * @brief A method to retrieve the liveliness lease duration
     * @return Lease duration
     */
    const Duration_t& get_liveliness_lease_duration() const;

    /**
     * @brief A method to return the liveliness announcement period
     * @return The announcement period
     */
    const Duration_t& get_liveliness_announcement_period() const;

    //! Liveliness lost status of this writer
    LivelinessLostStatus liveliness_lost_status_;

    /**
     * Check if the destinations managed by this sender interface have changed.
     *
     * @return true if destinations have changed, false otherwise.
     */
    bool destinations_have_changed() const override;

    /**
     * Get a GUID prefix representing all destinations.
     *
     * @return When all the destinations share the same prefix (i.e. belong to the same participant)
     * that prefix is returned. When there are no destinations, or they belong to different
     * participants, c_GuidPrefix_Unknown is returned.
     */
    GuidPrefix_t destination_guid_prefix() const override;

    /**
     * Get the GUID prefix of all the destination participants.
     *
     * @return a const reference to a vector with the GUID prefix of all destination participants.
     */
    const std::vector<GuidPrefix_t>& remote_participants() const override;

    /**
     * Get the GUID of all destinations.
     *
     * @return a const reference to a vector with the GUID of all destinations.
     */
    const std::vector<GUID_t>& remote_guids() const override;

    /**
     * Send a message through this interface.
     *
     * @param buffers Array of buffers to gather.
     * @param num_buffers Number of elements on @c buffers.
     * @param total_bytes Total size of the raw data. Should be equal to the sum of the @c length field of all buffers.
     * @param max_blocking_time_point Future timepoint where blocking send should end.
     */
    bool send(
            const RTPSMessageSenderInterface::NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            std::chrono::steady_clock::time_point& max_blocking_time_point) const override;

    /**
     * @return Whether the writer is data sharing compatible or not
     */
    bool is_datasharing_compatible() const;

protected:

    //!Is the data sent directly or announced by HB and THEN sent to the ones who ask for it?.
    bool m_pushMode = true;
    //!WriterHistory
    WriterHistory* mp_history = nullptr;
    //!Listener
    WriterListener* mp_listener = nullptr;
    //!Asynchronous publication activated
    bool is_async_ = false;
    //!Separate sending activated
    bool m_separateSendingEnabled = false;

    LocatorSelector locator_selector_;

    ResourceLimitedVector<GUID_t> all_remote_readers_;
    ResourceLimitedVector<GuidPrefix_t> all_remote_participants_;

    //! The liveliness kind of this writer
    LivelinessQosPolicyKind liveliness_kind_;
    //! The liveliness lease duration of this writer
    Duration_t liveliness_lease_duration_;
    //! The liveliness announcement period
    Duration_t liveliness_announcement_period_;

    void add_guid(
            const GUID_t& remote_guid);

    void compute_selected_guids();

    void update_cached_info_nts();

    /**
     * Add a change to the unsent list.
     * @param change Pointer to the change to add.
     * @param max_blocking_time
     */
    virtual void unsent_change_added_to_history(
            CacheChange_t* change,
            const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) = 0;

    /**
     * Indicate the writer that a change has been removed by the history due to some HistoryQos requirement.
     * @param a_change Pointer to the change that is going to be removed.
     * @return True if removed correctly.
     */
    virtual bool change_removed_by_history(
            CacheChange_t* a_change) = 0;

    bool is_datasharing_compatible_with(
            const ReaderProxyData& rdata) const;

    bool is_pool_initialized() const;

private:

    RTPSWriter& operator =(
            const RTPSWriter&) = delete;

    void init(
            const std::shared_ptr<IPayloadPool>& payload_pool,
            const std::shared_ptr<IChangePool>& change_pool,
            const WriterAttributes& att);


    RTPSWriter* next_[2] = { nullptr, nullptr };

};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif /* _FASTDDS_RTPS_RTPSWRITER_H_ */
