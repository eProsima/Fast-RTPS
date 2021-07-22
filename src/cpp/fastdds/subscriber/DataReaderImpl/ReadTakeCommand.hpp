// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file ReadTakeCommand.hpp
 */

#ifndef _FASTDDS_SUBSCRIBER_DATAREADERIMPL_READTAKECOMMAND_HPP_
#define _FASTDDS_SUBSCRIBER_DATAREADERIMPL_READTAKECOMMAND_HPP_

#include <cassert>
#include <cstdint>

#include <fastdds/dds/core/LoanableCollection.hpp>
#include <fastdds/dds/core/LoanableTypedCollection.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <fastrtps/subscriber/SubscriberHistory.h>
#include <fastrtps/types/TypesBase.h>

#include <fastdds/subscriber/DataReaderImpl.hpp>
#include <fastdds/subscriber/DataReaderImpl/DataReaderLoanManager.hpp>
#include <fastdds/subscriber/DataReaderImpl/StateFilter.hpp>
#include <fastdds/subscriber/DataReaderImpl/SampleInfoPool.hpp>
#include <fastdds/subscriber/DataReaderImpl/SampleLoanManager.hpp>

#include <fastdds/rtps/common/CacheChange.h>
#include <fastdds/rtps/reader/RTPSReader.h>

#include <rtps/reader/WriterProxy.h>
#include <rtps/DataSharing/DataSharingPayloadPool.hpp>


namespace eprosima {
namespace fastdds {
namespace dds {
namespace detail {

struct ReadTakeCommand
{
    using ReturnCode_t = eprosima::fastrtps::types::ReturnCode_t;
    using history_type = eprosima::fastrtps::SubscriberHistory;
    using CacheChange_t = eprosima::fastrtps::rtps::CacheChange_t;
    using RTPSReader = eprosima::fastrtps::rtps::RTPSReader;
    using WriterProxy = eprosima::fastrtps::rtps::WriterProxy;
    using SampleInfoSeq = LoanableTypedCollection<SampleInfo>;
    using DataSharingPayloadPool = eprosima::fastrtps::rtps::DataSharingPayloadPool;

    ReadTakeCommand(
            DataReaderImpl& reader,
            LoanableCollection& data_values,
            SampleInfoSeq& sample_infos,
            int32_t max_samples,
            const StateFilter& states,
            history_type::instance_info instance,
            bool single_instance = false)
        : type_(reader.type_)
        , loan_manager_(reader.loan_manager_)
        , history_(reader.history_)
        , reader_(reader.reader_)
        , info_pool_(reader.sample_info_pool_)
        , sample_pool_(reader.sample_pool_)
        , data_values_(data_values)
        , sample_infos_(sample_infos)
        , remaining_samples_(max_samples)
        , states_(states)
        , instance_(instance)
        , handle_(instance.first)
        , single_instance_(single_instance)
    {
        assert(0 <= remaining_samples_);

        current_slot_ = data_values_.length();
        finished_ = nullptr == instance.second;
    }

    ~ReadTakeCommand()
    {
        if (!data_values_.has_ownership() && ReturnCode_t::RETCODE_NO_DATA == return_value_)
        {
            loan_manager_.return_loan(data_values_, sample_infos_);
            data_values_.unloan();
            sample_infos_.unloan();
        }
    }

    bool add_instance(
            bool take_samples)
    {
        // Advance to the first instance with a valid state
        if (!go_to_first_valid_instance())
        {
            return false;
        }

        // Traverse changes on current instance
        bool ret_val = false;
        LoanableCollection::size_type first_slot = current_slot_;
        auto it = instance_.second->begin();
        while (!finished_ && it != instance_.second->end())
        {
            CacheChange_t* change = *it;
            SampleStateKind check;
            check = change->isRead ? SampleStateKind::READ_SAMPLE_STATE : SampleStateKind::NOT_READ_SAMPLE_STATE;
            if ((check & states_.sample_states) != 0)
            {
                WriterProxy* wp = nullptr;
                bool is_future_change = false;
                bool remove_change = false;
                if (reader_->begin_sample_access_nts(change, wp, is_future_change))
                {
                    //Check if the payload is dirty
                    remove_change = !check_datasharing_validity(change, data_values_.has_ownership(), wp);
                }
                else
                {
                    remove_change = true;
                }

                if (remove_change)
                {
                    // Remove from history
                    history_.remove_change_sub(change, it);

                    // Current iterator will point to change next to the one removed. Avoid incrementing.
                    continue;
                }

                // If the change is in the future we can skip the remaining changes in the history, as they will be
                // in the future also
                if (!is_future_change)
                {

                    // Add sample and info to collections
                    ReturnCode_t previous_return_value = return_value_;
                    bool added = add_sample(change, remove_change);
                    reader_->end_sample_access_nts(change, wp, added);

                    // Check if the payload is dirty
                    if (added && !check_datasharing_validity(change, data_values_.has_ownership(), wp))
                    {
                        // Decrement length of collections
                        --current_slot_;
                        ++remaining_samples_;
                        data_values_.length(current_slot_);
                        sample_infos_.length(current_slot_);

                        return_value_ = previous_return_value;
                        finished_ = false;

                        remove_change = true;
                        added = false;
                    }

                    if (remove_change || (added && take_samples))
                    {
                        // Remove from history
                        history_.remove_change_sub(change, it);

                        // Current iterator will point to change next to the one removed. Avoid incrementing.
                        continue;
                    }
                }
            }

            // Go to next sample on instance
            ++it;
        }

        if (current_slot_ > first_slot)
        {
            ret_val = true;

            // complete sample infos
            LoanableCollection::size_type slot = current_slot_;
            LoanableCollection::size_type n = 0;
            while (slot > first_slot)
            {
                --slot;
                sample_infos_[slot].sample_rank = n;
                ++n;
            }
        }

        next_instance();
        return ret_val;
    }

    inline bool is_finished() const
    {
        return finished_;
    }

    inline ReturnCode_t return_value() const
    {
        return return_value_;
    }

private:

    const TypeSupport& type_;
    DataReaderLoanManager& loan_manager_;
    history_type& history_;
    RTPSReader* reader_;
    SampleInfoPool& info_pool_;
    std::shared_ptr<detail::SampleLoanManager> sample_pool_;
    LoanableCollection& data_values_;
    SampleInfoSeq& sample_infos_;
    int32_t remaining_samples_;
    StateFilter states_;
    history_type::instance_info instance_;
    InstanceHandle_t handle_;
    bool single_instance_;

    bool finished_ = false;
    ReturnCode_t return_value_ = ReturnCode_t::RETCODE_NO_DATA;

    LoanableCollection::size_type current_slot_ = 0;

    bool go_to_first_valid_instance()
    {
        while (!is_current_instance_valid())
        {
            if (!next_instance())
            {
                return false;
            }
        }
        return true;
    }

    bool is_current_instance_valid()
    {
        // We are not implementing instance_state or view_state yet, so all instances will be considered to have
        // a valid state. In the future this should check instance_state against states_.instance_states and
        // view_state against states_.view_states
        return true;
    }

    bool next_instance()
    {
        if (single_instance_)
        {
            finished_ = true;
            return false;
        }

        auto result = history_.lookup_instance(handle_, false);
        if (!result.first)
        {
            finished_ = true;
            return false;
        }

        instance_ = result.second;
        handle_ = instance_.first;
        return true;
    }

    bool add_sample(
            CacheChange_t* change,
            bool& deserialization_error)
    {
        bool ret_val = false;
        deserialization_error = false;

        if (remaining_samples_ > 0)
        {
            // Increment length of collections
            auto new_len = current_slot_ + 1;
            data_values_.length(new_len);
            sample_infos_.length(new_len);

            // Add information
            generate_info(change);
            if (sample_infos_[current_slot_].valid_data)
            {
                if (!deserialize_sample(change))
                {
                    // Decrement length of collections
                    data_values_.length(current_slot_);
                    sample_infos_.length(current_slot_);
                    deserialization_error = true;
                    return false;
                }

                // Mark that some data is available
                return_value_ = ReturnCode_t::RETCODE_OK;
            }

            ++current_slot_;
            --remaining_samples_;
            ret_val = true;
        }

        // Finish when there are no remaining samples
        finished_ = (remaining_samples_ == 0);
        return ret_val;
    }

    bool deserialize_sample(
            CacheChange_t* change)
    {
        auto payload = &(change->serializedPayload);
        if (data_values_.has_ownership())
        {
            // perform deserialization
            return type_->deserialize(payload, data_values_.buffer()[current_slot_]);
        }
        else
        {
            // loan
            void* sample;
            sample_pool_->get_loan(change, sample);
            const_cast<void**>(data_values_.buffer())[current_slot_] = sample;
            return true;
        }
    }

    void generate_info(
            CacheChange_t* change)
    {
        // Loan when necessary
        if (!sample_infos_.has_ownership())
        {
            SampleInfo* item = info_pool_.get_item();
            assert(item != nullptr);
            const_cast<void**>(sample_infos_.buffer())[current_slot_] = item;
        }

        SampleInfo& info = sample_infos_[current_slot_];
        info.sample_state = change->isRead ? READ_SAMPLE_STATE : NOT_READ_SAMPLE_STATE;
        info.view_state = NOT_NEW_VIEW_STATE;
        info.disposed_generation_count = 0;
        info.no_writers_generation_count = 1;
        info.sample_rank = 0;
        info.generation_rank = 0;
        info.absoulte_generation_rank = 0;
        info.source_timestamp = change->sourceTimestamp;
        info.reception_timestamp = change->reader_info.receptionTimestamp;
        info.instance_handle = handle_;
        info.publication_handle = InstanceHandle_t(change->writerGUID);
        info.sample_identity.writer_guid(change->writerGUID);
        info.sample_identity.sequence_number(change->sequenceNumber);
        info.related_sample_identity = change->write_params.sample_identity();
        info.valid_data = true;

        switch (change->kind)
        {
            case eprosima::fastrtps::rtps::ALIVE:
                info.instance_state = ALIVE_INSTANCE_STATE;
                break;
            case eprosima::fastrtps::rtps::NOT_ALIVE_DISPOSED:
            case eprosima::fastrtps::rtps::NOT_ALIVE_DISPOSED_UNREGISTERED:
                info.instance_state = NOT_ALIVE_DISPOSED_INSTANCE_STATE;
                info.valid_data = false;
                break;
            default:
                //TODO [ILG] change this if the other kinds ever get implemented
                info.instance_state = ALIVE_INSTANCE_STATE;
                break;
        }
    }

    bool check_datasharing_validity(
            CacheChange_t* change,
            bool has_ownership,
            WriterProxy* wp)
    {
        bool is_valid = true;
        if (has_ownership)  //< On loans the user must check the validity anyways
        {
            DataSharingPayloadPool* pool = dynamic_cast<DataSharingPayloadPool*>(change->payload_owner());
            if (pool)
            {
                //Check if the payload is dirty
                is_valid = pool->is_sample_valid(*change);
            }
        }

        if (!is_valid)
        {
            logWarning(RTPS_READER,
                    "Change " << change->sequenceNumber << " from " << wp->guid() <<
                    " is overidden");
            return false;
        }

        return true;
    }

};

} /* namespace detail */
} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */

#endif  // _FASTDDS_SUBSCRIBER_DATAREADERIMPL_READTAKECOMMAND_HPP_
