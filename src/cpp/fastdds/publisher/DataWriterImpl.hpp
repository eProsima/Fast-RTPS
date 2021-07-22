// Copyright 2019, 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file DataWriterImpl.hpp
 */

#ifndef _FASTRTPS_DATAWRITERIMPL_HPP_
#define _FASTRTPS_DATAWRITERIMPL_HPP_

#include <fastdds/dds/core/status/BaseStatus.hpp>
#include <fastdds/dds/core/status/IncompatibleQosStatus.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <fastdds/rtps/attributes/WriterAttributes.h>
#include <fastdds/rtps/common/LocatorList.hpp>
#include <fastdds/rtps/common/Guid.h>
#include <fastdds/rtps/common/WriteParams.h>
#include <fastdds/rtps/history/IPayloadPool.h>
#include <fastdds/rtps/writer/WriterListener.h>

#include <fastrtps/publisher/PublisherHistory.h>
#include <fastrtps/qos/DeadlineMissedStatus.h>
#include <fastrtps/qos/LivelinessLostStatus.h>

#include <fastrtps/types/TypesBase.h>

#include <rtps/common/PayloadInfo_t.hpp>
#include <rtps/history/ITopicPayloadPool.h>
#include <rtps/DataSharing/DataSharingPayloadPool.hpp>

using eprosima::fastrtps::types::ReturnCode_t;

namespace eprosima {
namespace fastrtps {
namespace rtps {

class RTPSWriter;
class RTPSParticipant;
class TimedEvent;

} // namespace rtps

} // namespace fastrtps

namespace fastdds {

#ifdef FASTDDS_STATISTICS
namespace statistics {
namespace dds {
class DomainParticipantImpl;
} // namespace dds
} // namespace statistics
#endif // FASTDDS_STATISTICS

namespace dds {

class PublisherListener;
class PublisherImpl;
class Publisher;

/**
 * Class DataWriterImpl, contains the actual implementation of the behaviour of the DataWriter.
 * @ingroup FASTDDS_MODULE
 */
class DataWriterImpl
{
    using LoanInitializationKind = DataWriter::LoanInitializationKind;
    using PayloadInfo_t = eprosima::fastrtps::rtps::detail::PayloadInfo_t;
    using CacheChange_t = eprosima::fastrtps::rtps::CacheChange_t;
    class LoanCollection;

protected:

    friend class PublisherImpl;

#ifdef FASTDDS_STATISTICS
    friend class eprosima::fastdds::statistics::dds::DomainParticipantImpl;
#endif // FASTDDS_STATISTICS

    /**
     * Create a data writer, assigning its pointer to the associated writer.
     * Don't use directly, create Publisher using DomainRTPSParticipant static function.
     */
    DataWriterImpl(
            PublisherImpl* p,
            TypeSupport type,
            Topic* topic,
            const DataWriterQos& qos,
            DataWriterListener* listener = nullptr);

public:

    virtual ~DataWriterImpl();

    virtual ReturnCode_t enable();

    ReturnCode_t check_delete_preconditions();

    /**
     * Get a pointer to the internal pool where the user could directly write.
     *
     * @param [out] sample          Pointer to the sample on the internal pool.
     * @param [in]  initialization  How to initialize the loaned sample.
     *
     * @return ReturnCode_t::RETCODE_ILLEGAL_OPERATION when the type does not support loans.
     * @return ReturnCode_t::RETCODE_OUT_OF_RESOURCES if the pool has been exhausted.
     * @return ReturnCode_t::RETCODE_OK if a pointer to a sample is successfully obtained.
     */
    ReturnCode_t loan_sample(
            void*& sample,
            LoanInitializationKind initialization);

    /**
     * Discards a loaned sample pointer.
     *
     * @param [in,out] sample  Pointer to the previously loaned sample.
     *
     * @return ReturnCode_t::RETCODE_ILLEGAL_OPERATION when the type does not support loans.
     * @return ReturnCode_t::RETCODE_BAD_PARAMETER if the pointer does not correspond to a loaned sample.
     * @return ReturnCode_t::RETCODE_OK if the loan is successfully discarded.
     */
    ReturnCode_t discard_loan(
            void*& sample);

    /**
     * Write data to the topic.
     * @param data Pointer to the data
     * @return True if correct
     * @par Calling example:
     * @snippet fastrtps_example.cpp ex_PublisherWrite
     */
    bool write(
            void* data);

    /**
     * Write data with params to the topic.
     * @param data Pointer to the data
     * @param params Extra write parameters.
     * @return True if correct
     * @par Calling example:
     * @snippet fastrtps_example.cpp ex_PublisherWrite
     */
    bool write(
            void* data,
            fastrtps::rtps::WriteParams& params);

    /**
     * Write data with handle.
     * @param data Pointer to the data
     * @param handle InstanceHandle_t.
     * @return True if correct
     * @par Calling example:
     * @snippet fastrtps_example.cpp ex_PublisherWrite
     */
    ReturnCode_t write(
            void* data,
            const InstanceHandle_t& handle);

    /*!
     * @brief Implementation of the DDS `register_instance` operation.
     * It deduces the instance's key and tries to get resources in the PublisherHistory.
     * @param[in] instance Sample used to get the instance's key.
     * @return Handle containing the instance's key.
     * This handle could be used in successive `write` or `dispose` operations.
     * In case of error, HANDLE_NIL will be returned.
     */
    InstanceHandle_t register_instance(
            void* instance);

    /*!
     * @brief Implementation of the DDS `unregister_instance` and `dispose` operations.
     * It sends a CacheChange_t with a kind that depends on the `dispose` parameter and
     * `writer_data_lifecycle` QoS.
     * @param[in] instance Sample used to deduce instance's key in case of `handle` parameter is HANDLE_NIL.
     * @param[in] handle Instance's key to be unregistered or disposed.
     * @param[in] dispose If it is `false`, a CacheChange_t with kind set to NOT_ALIVE_UNREGISTERED is sent, or if
     * `writer_data_lifecycle.autodispose_unregistered_instances` is `true` then it is sent with kind set to
     * NOT_ALIVE_DISPOSED_UNREGISTERED.
     * If `dispose` is `true`, a CacheChange_t with kind set to NOT_ALIVE_DISPOSED is sent.
     * @return Returns the operation's result.
     * If the operation finishes successfully, ReturnCode_t::RETCODE_OK is returned.
     */
    ReturnCode_t unregister_instance(
            void* instance,
            const InstanceHandle_t& handle,
            bool dispose = false);

    /**
     *
     * @return
     */
    const fastrtps::rtps::GUID_t& guid() const;

    InstanceHandle_t get_instance_handle() const;

    /**
     * Get topic data type
     * @return Topic data type
     */
    TypeSupport get_type() const
    {
        return type_;
    }

    ReturnCode_t wait_for_acknowledgments(
            const fastrtps::Duration_t& max_wait);

    ReturnCode_t get_offered_deadline_missed_status(
            fastrtps::OfferedDeadlineMissedStatus& status);

    ReturnCode_t get_offered_incompatible_qos_status(
            OfferedIncompatibleQosStatus& status);

    ReturnCode_t set_qos(
            const DataWriterQos& qos);

    const DataWriterQos& get_qos() const;

    Topic* get_topic() const;

    const DataWriterListener* get_listener() const;

    ReturnCode_t set_listener(
            DataWriterListener* listener);

    /* TODO
       bool get_key_value(
            void* key_holder,
            const InstanceHandle_t& handle);
     */

    ReturnCode_t get_liveliness_lost_status(
            LivelinessLostStatus& status);

    const Publisher* get_publisher() const;

    ReturnCode_t assert_liveliness();

    //! Remove all listeners in the hierarchy to allow a quiet destruction
    virtual void disable();

    /**
     * Removes all changes from the History.
     * @param[out] removed Number of removed elements
     * @return ReturnCode_t::RETCODE_OK if correct, ReturnCode_t::RETCODE_ERROR if not.
     */
    ReturnCode_t clear_history(
            size_t* removed);

    /**
     * @brief Get the list of locators from which this DataWriter may send data.
     *
     * @param [out] locators  LocatorList where the list of locators will be stored.
     *
     * @return NOT_ENABLED if the reader has not been enabled.
     * @return OK if a list of locators is returned.
     */
    ReturnCode_t get_sending_locators(
            rtps::LocatorList& locators) const;

protected:

    using IPayloadPool = eprosima::fastrtps::rtps::IPayloadPool;
    using ITopicPayloadPool = eprosima::fastrtps::rtps::ITopicPayloadPool;

    PublisherImpl* publisher_ = nullptr;

    //! Pointer to the associated Data Writer.
    fastrtps::rtps::RTPSWriter* writer_ = nullptr;

    //! Pointer to the TopicDataType object.
    TypeSupport type_;

    Topic* topic_ = nullptr;

    DataWriterQos qos_;

    //!Publisher History
    fastrtps::PublisherHistory history_;

    //! DataWriterListener
    DataWriterListener* listener_ = nullptr;

    //!Listener to capture the events of the Writer
    class InnerDataWriterListener : public fastrtps::rtps::WriterListener
    {
    public:

        InnerDataWriterListener(
                DataWriterImpl* w)
            : data_writer_(w)
        {
        }

        virtual ~InnerDataWriterListener() override
        {
        }

        void onWriterMatched(
                fastrtps::rtps::RTPSWriter* writer,
                const fastdds::dds::PublicationMatchedStatus& info) override;

        void on_offered_incompatible_qos(
                fastrtps::rtps::RTPSWriter* writer,
                fastdds::dds::PolicyMask qos) override;

        void onWriterChangeReceivedByAll(
                fastrtps::rtps::RTPSWriter* writer,
                fastrtps::rtps::CacheChange_t* change) override;

        void on_liveliness_lost(
                fastrtps::rtps::RTPSWriter* writer,
                const fastrtps::LivelinessLostStatus& status) override;

        DataWriterImpl* data_writer_;
    }
    writer_listener_;

    //! A timer used to check for deadlines
    fastrtps::rtps::TimedEvent* deadline_timer_ = nullptr;

    //! Deadline duration in microseconds
    std::chrono::duration<double, std::ratio<1, 1000000>> deadline_duration_us_;

    //! The current timer owner, i.e. the instance which started the deadline timer
    InstanceHandle_t timer_owner_;

    //! The offered deadline missed status
    fastrtps::OfferedDeadlineMissedStatus deadline_missed_status_;

    //! The offered incompatible qos status
    OfferedIncompatibleQosStatus offered_incompatible_qos_status_;

    //! A timed callback to remove expired samples for lifespan QoS
    fastrtps::rtps::TimedEvent* lifespan_timer_ = nullptr;

    //! The lifespan duration, in microseconds
    std::chrono::duration<double, std::ratio<1, 1000000>> lifespan_duration_us_;

    DataWriter* user_datawriter_ = nullptr;

    bool is_data_sharing_compatible_ = false;

    uint32_t fixed_payload_size_ = 0u;

    std::shared_ptr<IPayloadPool> payload_pool_;

    std::unique_ptr<LoanCollection> loans_;

    virtual fastrtps::rtps::RTPSWriter* create_rtps_writer(
            fastrtps::rtps::RTPSParticipant* p,
            fastrtps::rtps::WriterAttributes& watt,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            fastrtps::rtps::WriterHistory* hist,
            fastrtps::rtps::WriterListener* listen);

    /**
     *
     * @param kind
     * @param  data
     * @return
     */
    ReturnCode_t create_new_change(
            fastrtps::rtps::ChangeKind_t kind,
            void* data);

    /**
     *
     * @param kind
     * @param  data
     * @param wparams
     * @return
     */
    ReturnCode_t create_new_change_with_params(
            fastrtps::rtps::ChangeKind_t kind,
            void* data,
            fastrtps::rtps::WriteParams& wparams);

    /**
     *
     * @param kind
     * @param  data
     * @param wparams
     * @param handle
     * @return
     */
    ReturnCode_t create_new_change_with_params(
            fastrtps::rtps::ChangeKind_t kind,
            void* data,
            fastrtps::rtps::WriteParams& wparams,
            const InstanceHandle_t& handle);

    /**
     * Removes the cache change with the minimum sequence number
     * @return True if correct.
     */
    bool remove_min_seq_change();

    /**
     * @brief A method called when an instance misses the deadline
     */
    bool deadline_missed();

    /**
     * @brief A method to reschedule the deadline timer
     */
    bool deadline_timer_reschedule();

    /**
     * @brief A method to remove expired samples, invoked when the lifespan timer expires
     */
    bool lifespan_expired();

    ReturnCode_t check_new_change_preconditions(
            fastrtps::rtps::ChangeKind_t change_kind,
            void* data);

    ReturnCode_t perform_create_new_change(
            fastrtps::rtps::ChangeKind_t change_kind,
            void* data,
            fastrtps::rtps::WriteParams& wparams,
            const InstanceHandle_t& handle);

    static fastrtps::TopicAttributes get_topic_attributes(
            const DataWriterQos& qos,
            const Topic& topic,
            const TypeSupport& type);

    static void set_qos(
            DataWriterQos& to,
            const DataWriterQos& from,
            bool is_default);

    static ReturnCode_t check_qos(
            const DataWriterQos& qos);

    static bool can_qos_be_updated(
            const DataWriterQos& to,
            const DataWriterQos& from);

    void publisher_qos_updated();

    OfferedIncompatibleQosStatus& update_offered_incompatible_qos(
            PolicyMask incompatible_policies);

    /**
     * Returns the most appropriate listener to handle the callback for the given status,
     * or nullptr if there is no appropriate listener.
     */
    DataWriterListener* get_listener_for(
            const StatusMask& status);

    void set_fragment_size_on_change(
            fastrtps::rtps::WriteParams& wparams,
            fastrtps::rtps::CacheChange_t* ch,
            const uint32_t& high_mark_for_frag);

    std::shared_ptr<IPayloadPool> get_payload_pool();

    bool release_payload_pool();

    ReturnCode_t check_datasharing_compatible(
            const fastrtps::rtps::WriterAttributes& writer_attributes,
            bool& is_datasharing_compatible) const;

    template<typename SizeFunctor>
    bool get_free_payload_from_pool(
            const SizeFunctor& size_getter,
            PayloadInfo_t& payload)
    {
        CacheChange_t change;
        if (!payload_pool_)
        {
            return false;
        }

        uint32_t size = fixed_payload_size_ ? fixed_payload_size_ : size_getter();
        if (!payload_pool_->get_payload(size, change))
        {
            return false;
        }

        payload.move_from_change(change);
        return true;
    }

    void return_payload_to_pool(
            PayloadInfo_t& payload)
    {
        CacheChange_t change;
        payload.move_into_change(change);
        payload_pool_->release_payload(change);
    }

    bool add_loan(
            void* data,
            PayloadInfo_t& payload);

    bool check_and_remove_loan(
            void* data,
            PayloadInfo_t& payload);

};

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */

#endif //_FASTRTPS_DATAWRITERIMPL_HPP_
