// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file DataReader.cpp
 *
 */

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/subscriber/DataReaderImpl.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>



namespace eprosima {

using namespace fastrtps;
using namespace fastrtps::rtps;

namespace fastdds {
namespace dds {

DataReader::DataReader(
        DataReaderImpl* impl,
        const StatusMask& mask)
    : DomainEntity(mask)
    , impl_(impl)
{
}

DataReader::DataReader(
        Subscriber* s,
        TopicDescription* topic,
        const DataReaderQos& qos,
        DataReaderListener* listener,
        const StatusMask& mask)
    : DomainEntity(mask)
    , impl_(s->create_datareader(topic, qos, listener, mask)->impl_)
{
}

DataReader::~DataReader()
{
}

ReturnCode_t DataReader::enable()
{
    if (enable_)
    {
        return ReturnCode_t::RETCODE_OK;
    }

    if (false == impl_->get_subscriber()->is_enabled())
    {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    ReturnCode_t ret_code = impl_->enable();
    enable_ = ReturnCode_t::RETCODE_OK == ret_code;
    return ret_code;
}

bool DataReader::wait_for_unread_message(
        const fastrtps::Duration_t& timeout)
{
    return impl_->wait_for_unread_message(timeout);
}

ReturnCode_t DataReader::read(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos,
        int32_t max_samples,
        SampleStateMask sample_states,
        ViewStateMask view_states,
        InstanceStateMask instance_states)
{
    return impl_->read(data_values, sample_infos, max_samples, sample_states, view_states, instance_states);
}

ReturnCode_t DataReader::read_instance(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos,
        int32_t max_samples,
        const InstanceHandle_t& a_handle,
        SampleStateMask sample_states,
        ViewStateMask view_states,
        InstanceStateMask instance_states)
{
    return impl_->read_instance(data_values, sample_infos, max_samples, a_handle, sample_states, view_states,
                   instance_states);
}

ReturnCode_t DataReader::read_next_instance(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos,
        int32_t max_samples,
        const InstanceHandle_t& previous_handle,
        SampleStateMask sample_states,
        ViewStateMask view_states,
        InstanceStateMask instance_states)
{
    return impl_->read_next_instance(data_values, sample_infos, max_samples, previous_handle, sample_states,
                   view_states, instance_states);
}

ReturnCode_t DataReader::take(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos,
        int32_t max_samples,
        SampleStateMask sample_states,
        ViewStateMask view_states,
        InstanceStateMask instance_states)
{
    return impl_->take(data_values, sample_infos, max_samples, sample_states, view_states, instance_states);
}

ReturnCode_t DataReader::take_instance(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos,
        int32_t max_samples,
        const InstanceHandle_t& a_handle,
        SampleStateMask sample_states,
        ViewStateMask view_states,
        InstanceStateMask instance_states)
{
    return impl_->take_instance(data_values, sample_infos, max_samples, a_handle, sample_states, view_states,
                   instance_states);
}

ReturnCode_t DataReader::take_next_instance(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos,
        int32_t max_samples,
        const InstanceHandle_t& previous_handle,
        SampleStateMask sample_states,
        ViewStateMask view_states,
        InstanceStateMask instance_states)
{
    return impl_->take_next_instance(data_values, sample_infos, max_samples, previous_handle, sample_states,
                   view_states, instance_states);
}

ReturnCode_t DataReader::return_loan(
        LoanableCollection& data_values,
        SampleInfoSeq& sample_infos)
{
    return impl_->return_loan(data_values, sample_infos);
}

ReturnCode_t DataReader::read_next_sample(
        void* data,
        SampleInfo* info)
{
    return impl_->read_next_sample(data, info);
}

ReturnCode_t DataReader::take_next_sample(
        void* data,
        SampleInfo* info)
{
    return impl_->take_next_sample(data, info);
}

ReturnCode_t DataReader::get_first_untaken_info(
        SampleInfo* info)
{
    return impl_->get_first_untaken_info(info);
}

const GUID_t& DataReader::guid()
{
    return impl_->guid();
}

InstanceHandle_t DataReader::get_instance_handle() const
{
    return impl_->get_instance_handle();
}

ReturnCode_t DataReader::set_qos(
        const DataReaderQos& qos)
{
    return impl_->set_qos(qos);
}

const DataReaderQos& DataReader::get_qos() const
{
    return impl_->get_qos();
}

ReturnCode_t DataReader::get_qos(
        DataReaderQos& qos) const
{
    qos = impl_->get_qos();
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReader::get_requested_deadline_missed_status(
        RequestedDeadlineMissedStatus& status)
{
    return impl_->get_requested_deadline_missed_status(status);
}

ReturnCode_t DataReader::get_requested_incompatible_qos_status(
        RequestedIncompatibleQosStatus& status)
{
    return impl_->get_requested_incompatible_qos_status(status);
}

ReturnCode_t DataReader::set_listener(
        DataReaderListener* listener)
{
    return set_listener(listener, StatusMask::all());
}

ReturnCode_t DataReader::set_listener(
        DataReaderListener* listener,
        const StatusMask& mask)
{
    ReturnCode_t ret_val = impl_->set_listener(listener);
    if (ret_val == ReturnCode_t::RETCODE_OK)
    {
        status_mask_ = mask;
    }

    return ret_val;
}

const DataReaderListener* DataReader::get_listener() const
{
    return impl_->get_listener();
}

/* TODO
   bool DataReader::get_key_value(
        void* data,
        const InstanceHandle_t& handle)
   {
    return impl->get_key_value(...);
   }
 */

ReturnCode_t DataReader::get_liveliness_changed_status(
        LivelinessChangedStatus& status) const
{
    return impl_->get_liveliness_changed_status(status);
}

ReturnCode_t DataReader::get_sample_lost_status(
        SampleLostStatus& status) const
{
    static_cast<void> (status);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
    /*
       return impl_->get_sample_lost_status(status);
     */
}

ReturnCode_t DataReader::get_sample_rejected_status(
        SampleRejectedStatus& status) const
{
    static_cast<void> (status);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
    /*
       return impl_->get_sample_rejected_status(status);
     */
}

ReturnCode_t DataReader::get_subscription_matched_status(
        SubscriptionMatchedStatus& status) const
{
    static_cast<void> (status);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
    /*
       return impl_->get_subscription_matched_status(status);
     */
}

ReturnCode_t DataReader::get_matched_publication_data(
        PublicationBuiltinTopicData& publication_data,
        fastrtps::rtps::InstanceHandle_t publication_handle) const
{
    static_cast<void> (publication_data);
    static_cast<void> (publication_handle);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
    /*
       return impl_->get_matched_publication_data(publication_data, publication_handle);
     */
}

ReturnCode_t DataReader::get_matched_publications(
        std::vector<fastrtps::rtps::InstanceHandle_t*>& publication_handles) const
{
    static_cast<void> (publication_handles);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
    /*
       return impl_->get_matched_publication_data(publication_handles);
     */
}

ReadCondition* DataReader::create_readcondition(
        std::vector<SampleStateKind> sample_states,
        std::vector<ViewStateKind> view_states,
        std::vector<InstanceStateKind> instance_states)
{
    logWarning(DataReader, "create_readcondition method not implemented");
    static_cast<void> (sample_states);
    static_cast<void> (view_states);
    static_cast<void> (instance_states);
    return nullptr;
    /*
       return impl_->create_readcondition(sample_states, view_states, instance_states);
     */
}

QueryCondition* DataReader::create_querycondition(
        std::vector<SampleStateKind> sample_states,
        std::vector<ViewStateKind> view_states,
        std::vector<InstanceStateKind> instance_states,
        std::string& query_expression,
        std::vector<std::string> query_parameters)
{
    logWarning(DataReader, "create_querycondition method not implemented");
    static_cast<void> (sample_states);
    static_cast<void> (view_states);
    static_cast<void> (instance_states);
    static_cast<void> (query_expression);
    static_cast<void> (query_parameters);
    return nullptr;
    /*
       return impl_->create_querycondition(sample_states, view_states, instance_states, query_expression, query_parameters);
     */
}

ReturnCode_t DataReader::delete_readcondition(
        ReadCondition* a_condition)
{
    static_cast<void> (a_condition);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
    /*
       return impl_->delete_readcondition(a_condition);
     */
}

const Subscriber* DataReader::get_subscriber() const
{
    return impl_->get_subscriber();
}

ReturnCode_t DataReader::wait_for_historical_data(
        const Duration_t& max_wait)
{
    bool success = impl_->wait_for_unread_message(max_wait);
    return (success) ? ReturnCode_t::RETCODE_OK : ReturnCode_t::RETCODE_TIMEOUT;
}

TypeSupport DataReader::type()
{
    return impl_->type();
}

const TopicDescription* DataReader::get_topicdescription() const
{
    return impl_->get_topicdescription();
}

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */
