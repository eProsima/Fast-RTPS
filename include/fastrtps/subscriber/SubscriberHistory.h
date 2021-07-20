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
 * @file SubscriberHistory.h
 *
 */

#ifndef SUBSCRIBERHISTORY_H_
#define SUBSCRIBERHISTORY_H_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastdds/rtps/resources/ResourceManagement.h>
#include <fastrtps/qos/ReaderQos.h>
#include <fastdds/rtps/history/ReaderHistory.h>
#include <fastrtps/qos/QosPolicies.h>
#include <fastrtps/common/KeyedChanges.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/attributes/TopicAttributes.h>

#include <chrono>
#include <functional>

namespace eprosima {
namespace fastrtps {

/**
 * Class SubscriberHistory, container of the different CacheChanges of a subscriber
 *  @ingroup FASTRTPS_MODULE
 */
class SubscriberHistory : public rtps::ReaderHistory
{
public:

    using instance_info = std::pair<rtps::InstanceHandle_t, std::vector<rtps::CacheChange_t*>*>;

    /**
     * Constructor. Requires information about the subscriber.
     * @param topic_att TopicAttributes.
     * @param type TopicDataType.
     * @param qos ReaderQoS policy.
     * @param payloadMax Maximum payload size per change.
     * @param mempolicy Set whether the payloads ccan dynamically resized or not.
     */
    SubscriberHistory(
            const TopicAttributes& topic_att,
            fastdds::dds::TopicDataType* type,
            const fastrtps::ReaderQos& qos,
            uint32_t payloadMax,
            rtps::MemoryManagementPolicy_t mempolicy);

    ~SubscriberHistory() override;

    /**
     * Called when a change is received by the Subscriber. Will add the change to the history.
     * @pre Change should not be already present in the history.
     * @param[in] change The received change
     * @param unknown_missing_changes_up_to Number of missing changes before this one
     * @return
     */
    bool received_change(
            rtps::CacheChange_t* change,
            size_t unknown_missing_changes_up_to) override;

    /** @name Read or take data methods.
     * Methods to read or take data from the History.
     * @param data Pointer to the object where you want to read or take the information.
     * @param info Pointer to a SampleInfo_t object where you want
     * @param max_blocking_time Maximum time the function can be blocked.
     * to store the information about the retrieved data
     */
    ///@{
    bool readNextData(
            void* data,
            SampleInfo_t* info,
            std::chrono::steady_clock::time_point& max_blocking_time);

    bool takeNextData(
            void* data,
            SampleInfo_t* info,
            std::chrono::steady_clock::time_point& max_blocking_time);
    ///@}

    /**
     * @brief Returns information about the first untaken sample.
     * @param [out] info Pointer to a SampleInfo_t structure to store first untaken sample information.
     * @return true if sample info was returned. false if there is no sample to take.
     */
    bool get_first_untaken_info(
            SampleInfo_t* info);

    /**
     * Remove all changes from the History that have a certain guid.
     * @param a_guid Pointer to the target guid to search for.
     * @return True if successful, even if no changes have been removed.
     * */
    bool remove_changes_with_guid(
            const fastrtps::rtps::GUID_t& a_guid) override;

    /**
     * This method is called to remove a change from the SubscriberHistory.
     * @param change Pointer to the CacheChange_t.
     * @return True if removed.
     */
    bool remove_change_sub(
            rtps::CacheChange_t* change);

    /**
     * This method is called to remove a change from the SubscriberHistory.
     * @param [in]     change Pointer to the CacheChange_t.
     * @param [in,out] it     Iterator pointing to change on input. Will point to next valid change on output.
     * @return True if removed.
     */
    bool remove_change_sub(
            rtps::CacheChange_t* change,
            iterator& it);

    /**
     * @brief A method to set the next deadline for the given instance
     * @param handle The handle to the instance
     * @param next_deadline_us The time point when the deadline will occur
     * @return True if the deadline was set correctly
     */
    bool set_next_deadline(
            const rtps::InstanceHandle_t& handle,
            const std::chrono::steady_clock::time_point& next_deadline_us);

    /**
     * @brief A method to get the next instance handle that will miss the deadline and the time when the deadline will occur
     * @param handle The handle to the instance
     * @param next_deadline_us The time point when the instance will miss the deadline
     * @return True if the deadline was retrieved successfully
     */
    bool get_next_deadline(
            rtps::InstanceHandle_t& handle,
            std::chrono::steady_clock::time_point& next_deadline_us);

    /**
     * @brief Get the list of changes corresponding to an instance handle.
     * @param handle The handle to the instance.
     * @param exact  Indicates if the handle should match exactly (true) or if the first instance greater than the
     *               input handle should be returned.
     * @return A pair where:
     *         - @c first is a boolean indicating if an instance was found
     *         - @c second is a pair where:
     *           - @c first is the handle of the returned instance
     *           - @c second is a pointer to a std::vector<rtps::CacheChange_t*> with the list of changes for the
     *             returned instance
     *
     * @remarks When used on a NO_KEY topic, an instance will only be returned when called with
     *          `handle = HANDLE_NIL` and `exact = false`.
     */
    std::pair<bool, instance_info> lookup_instance(
            const rtps::InstanceHandle_t& handle,
            bool exact);

private:

    using t_m_Inst_Caches = std::map<rtps::InstanceHandle_t, KeyedChanges>;

    //!Map where keys are instance handles and values vectors of cache changes
    t_m_Inst_Caches keyed_changes_;
    //!Time point when the next deadline will occur (only used for topics with no key)
    std::chrono::steady_clock::time_point next_deadline_us_;
    //!HistoryQosPolicy values.
    HistoryQosPolicy history_qos_;
    //!ResourceLimitsQosPolicy values.
    ResourceLimitsQosPolicy resource_limited_qos_;
    //!Topic Attributes
    TopicAttributes topic_att_;
    //!TopicDataType
    fastdds::dds::TopicDataType* type_;
    //!ReaderQos
    fastrtps::ReaderQos qos_;

    //!Type object to deserialize Key
    void* get_key_object_;

    /// Function processing a received change
    std::function<bool(rtps::CacheChange_t*, size_t)> receive_fn_;

    /**
     * @brief Method that finds a key in m_keyedChanges or tries to add it if not found
     * @param a_change The change to get the key from
     * @param map_it A map iterator to the given key
     * @return True if it was found or could be added to the map
     */
    bool find_key(
            rtps::CacheChange_t* a_change,
            t_m_Inst_Caches::iterator* map_it);

    /**
     * @brief Method that finds a key in m_keyedChanges or tries to add it if not found
     * @param a_change The change to get the key from
     * @param map_it A map iterator to the given key
     * @return True if it was found or could be added to the map
     */
    bool find_key_for_change(
            rtps::CacheChange_t* a_change,
            t_m_Inst_Caches::iterator& map_it);

    /**
     * @name Variants of incoming change processing.
     *       Will be called with the history mutex taken.
     * @param[in] change The received change
     * @param unknown_missing_changes_up_to Number of missing changes before this one
     * @return
     */
    ///@{
    bool received_change_keep_all_no_key(
            rtps::CacheChange_t* change,
            size_t unknown_missing_changes_up_to);

    bool received_change_keep_last_no_key(
            rtps::CacheChange_t* change,
            size_t unknown_missing_changes_up_to);

    bool received_change_keep_all_with_key(
            rtps::CacheChange_t* change,
            size_t unknown_missing_changes_up_to);

    bool received_change_keep_last_with_key(
            rtps::CacheChange_t* change,
            size_t unknown_missing_changes_up_to);
    ///@}

    bool add_received_change(
            rtps::CacheChange_t* a_change);

    bool add_received_change_with_key(
            rtps::CacheChange_t* a_change,
            std::vector<rtps::CacheChange_t*>& instance_changes);

    bool deserialize_change(
            rtps::CacheChange_t* change,
            uint32_t ownership_strength,
            void* data,
            SampleInfo_t* info);
};

} // namespace fastrtps
} // namespace eprosima

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#endif /* SUBSCRIBERHISTORY_H_ */
