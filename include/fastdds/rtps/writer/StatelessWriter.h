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
 * @file StatelessWriter.h
 */
#ifndef _FASTDDS_RTPS_STATELESSWRITER_H_
#define _FASTDDS_RTPS_STATELESSWRITER_H_

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/common/Time_t.h>
#include <fastdds/rtps/history/IChangePool.h>
#include <fastdds/rtps/history/IPayloadPool.h>
#include <fastdds/rtps/writer/ChangeForReader.h>
#include <fastdds/rtps/writer/ReaderLocator.h>
#include <fastdds/rtps/writer/RTPSWriter.h>
#include <fastrtps/utils/collections/ResourceLimitedVector.hpp>

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

namespace eprosima {
namespace fastrtps {
namespace rtps {


/**
 * Class StatelessWriter, specialization of RTPSWriter that manages writers that don't keep state of the matched readers.
 * @ingroup WRITER_MODULE
 */
class StatelessWriter : public RTPSWriter
{
    friend class RTPSParticipantImpl;

protected:

    StatelessWriter(
            RTPSParticipantImpl* participant,
            const GUID_t& guid,
            const WriterAttributes& attributes,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* history,
            WriterListener* listener = nullptr);

    StatelessWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

    StatelessWriter(
            RTPSParticipantImpl* impl,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            const std::shared_ptr<IChangePool>& change_pool,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr);

public:

    virtual ~StatelessWriter();

    /**
     * Add a specific change to all ReaderLocators.
     * @param change Pointer to the change.
     * @param max_blocking_time
     */
    void unsent_change_added_to_history(
            CacheChange_t* change,
            const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) override;

    /**
     * Indicate the writer that a change has been removed by the history due to some HistoryQos requirement.
     * @param change Pointer to the change that is going to be removed.
     * @return True if removed correctly.
     */
    bool change_removed_by_history(
            CacheChange_t* change) override;

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

    /**
     * Update the Attributes of the Writer.
     * @param att New attributes
     */
    void updateAttributes(
            const WriterAttributes& att) override
    {
        (void)att;
        //FOR NOW THERE IS NOTHING TO UPDATE.
    }

    bool set_fixed_locators(
            const LocatorList_t& locator_list);

    //!Reset the unsent changes.
    void unsent_changes_reset();

    bool is_acked_by_all(
            const CacheChange_t* change) const override;

    bool try_remove_change(
            const std::chrono::steady_clock::time_point&,
            std::unique_lock<RecursiveTimedMutex>&) override;

    bool wait_for_acknowledgement(
            const SequenceNumber_t& seq,
            const std::chrono::steady_clock::time_point& max_blocking_time_point,
            std::unique_lock<RecursiveTimedMutex>& lock) override;

    /**
     * Send a message through this interface.
     *
     * @param message Pointer to the buffer with the message already serialized.
     * @param max_blocking_time_point Future timepoint where blocking send should end.
     */
    bool send_nts(
            CDRMessage_t* message,
            const LocatorSelectorSender& locator_selector,
            std::chrono::steady_clock::time_point& max_blocking_time_point) const override;

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
        return locator_selector_;
    }

    LocatorSelectorSender& get_async_locator_selector() override
    {
        return locator_selector_;
    }

private:

    void init(
            RTPSParticipantImpl* participant,
            const WriterAttributes& attributes);

    void get_builtin_guid();

    bool has_builtin_guid();

    void update_reader_info(
            bool create_sender_resources);

    bool datasharing_delivery(
            CacheChange_t* change);

    bool intraprocess_delivery(
            CacheChange_t* change,
            ReaderLocator& reader_locator);

    bool is_inline_qos_expected_ = false;
    LocatorList_t fixed_locators_;
    ResourceLimitedVector<std::unique_ptr<ReaderLocator>> matched_remote_readers_;

    std::condition_variable_any unsent_changes_cond_;

    uint64_t current_sequence_number_sent_ = 0;

    FragmentNumber_t current_fragment_sent_ = 0;

    uint64_t last_sequence_number_sent_ = 0;

    ResourceLimitedVector<std::unique_ptr<ReaderLocator>> matched_local_readers_;
    ResourceLimitedVector<std::unique_ptr<ReaderLocator>> matched_datasharing_readers_;
    ResourceLimitedVector<std::unique_ptr<ReaderLocator>> matched_readers_pool_;

    LocatorSelectorSender locator_selector_;
};

}        /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif /* _FASTDDS_RTPS_STATELESSWRITER_H_ */
