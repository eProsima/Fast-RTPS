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
 * @file ChangeForReader.h
 */

#ifndef _FASTDDS_RTPS_CHANGEFORREADER_H_
#define _FASTDDS_RTPS_CHANGEFORREADER_H_

#include <fastdds/rtps/common/CacheChange.h>
#include <fastdds/rtps/common/FragmentNumber.h>
#include <fastdds/rtps/common/SequenceNumber.h>

#include <cassert>

namespace eprosima {
namespace fastrtps {
namespace rtps {

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

/**
 * Enum ChangeForReaderStatus_t, possible states for a CacheChange_t in a ReaderProxy.
 *  @ingroup COMMON_MODULE
 */
enum ChangeForReaderStatus_t
{
    UNSENT = 0,                    //!< UNSENT
    REQUESTED = 1,                 //!< REQUESTED
    UNACKNOWLEDGED = 2,            //!< UNACKNOWLEDGED
    ACKNOWLEDGED = 3,              //!< ACKNOWLEDGED
    UNDERWAY = 4                   //!< UNDERWAY
};

/**
 * Struct ChangeForReader_t used to represent the state of a specific change with respect to a specific reader, as well as its relevance.
 *  @ingroup COMMON_MODULE
 */
class ChangeForReader_t
{
    friend struct ChangeForReaderCmp;

public:

    ChangeForReader_t() = delete;

    ChangeForReader_t(
            const ChangeForReader_t& ch)
        : status_(ch.status_)
        , seq_num_(ch.seq_num_)
        , change_(ch.change_)
        , unsent_fragments_(ch.unsent_fragments_)
    {
    }

    //TODO(Ricardo) Temporal
    //ChangeForReader_t(const CacheChange_t* change) : status_(UNSENT),
    ChangeForReader_t(
            CacheChange_t* change)
        : status_(UNSENT)
        , seq_num_(change->sequenceNumber)
        , change_(change)
    {
        if (change->getFragmentSize() != 0)
        {
            unsent_fragments_.base(1u);
            unsent_fragments_.add_range(1u, change->getFragmentCount() + 1u);
        }
    }

    ~ChangeForReader_t()
    {
    }

    ChangeForReader_t& operator =(
            const ChangeForReader_t& ch)
    {
        status_ = ch.status_;
        seq_num_ = ch.seq_num_;
        change_ = ch.change_;
        unsent_fragments_ = ch.unsent_fragments_;
        return *this;
    }

    /**
     * Get the cache change
     * @return Cache change
     */
    CacheChange_t* getChange() const
    {
        return change_;
    }

    void setStatus(
            const ChangeForReaderStatus_t status)
    {
        status_ = status;
    }

    ChangeForReaderStatus_t getStatus() const
    {
        return status_;
    }

    const SequenceNumber_t getSequenceNumber() const
    {
        return seq_num_;
    }

    FragmentNumber_t get_next_unsent_fragment() const
    {
        if (unsent_fragments_.empty())
        {
            return change_->getFragmentCount() + 1;
        }

        return unsent_fragments_.min();
    }

    FragmentNumberSet_t getUnsentFragments() const
    {
        return unsent_fragments_;
    }

    void markAllFragmentsAsUnsent()
    {
        assert(nullptr != change_);

        if (change_->getFragmentSize() != 0)
        {
            unsent_fragments_.base(1u);
            unsent_fragments_.add_range(1u, change_->getFragmentCount() + 1u);
        }
    }

    void markFragmentsAsSent(
            const FragmentNumber_t& sentFragment)
    {
        unsent_fragments_.remove(sentFragment);

        if (!unsent_fragments_.empty() && unsent_fragments_.max() < change_->getFragmentCount())
        {
            FragmentNumber_t base = unsent_fragments_.base();
            FragmentNumber_t max = unsent_fragments_.max();
            assert(!unsent_fragments_.is_set(base));

            // Update base to first bit set
            base = unsent_fragments_.min();
            unsent_fragments_.base_update(base);

            // Add all possible fragments
            unsent_fragments_.add_range(max + 1u, change_->getFragmentCount() + 1u);
        }
    }

    void markFragmentsAsUnsent(
            const FragmentNumberSet_t& unsentFragments)
    {
        FragmentNumber_t other_base = unsentFragments.base();
        if (other_base < unsent_fragments_.base())
        {
            unsent_fragments_.base_update(other_base);
        }
        unsentFragments.for_each(
            [this](
                FragmentNumber_t element)
            {
                unsent_fragments_.add(element);
            });
    }

    bool has_been_delivered() const
    {
        return delivered_;
    }

    void set_delivered()
    {
        delivered_ = true;
    }

private:

    //!Status
    ChangeForReaderStatus_t status_;

    //!Sequence number
    SequenceNumber_t seq_num_;

    CacheChange_t* change_;

    FragmentNumberSet_t unsent_fragments_;

    //! Indicates if was delivered at least one time.
    bool delivered_ = false;
};

struct ChangeForReaderCmp
{
    bool operator ()(
            const ChangeForReader_t& a,
            const ChangeForReader_t& b) const
    {
        return a.seq_num_ < b.seq_num_;
    }

};

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif /* _FASTDDS_RTPS_CHANGEFORREADER_H_ */
