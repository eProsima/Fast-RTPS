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
 * @file WriterHistory.h
 *
 */

#ifndef _FASTDDS_RTPS_WRITERHISTORY_H_
#define _FASTDDS_RTPS_WRITERHISTORY_H_

#include <fastdds/rtps/history/History.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class RTPSWriter;
class WriteParams;

/**
 * Class WriterHistory, container of the different CacheChanges of a writer
 * @ingroup WRITER_MODULE
 */
class WriterHistory : public rtps::History
{
    friend class RTPSWriter;
    friend class PersistentWriter;
    friend class IPersistenceService;

    WriterHistory(
            WriterHistory&&) = delete;
    WriterHistory& operator =(
            WriterHistory&&) = delete;

public:

    /**
     * Constructor of the WriterHistory.
     */
    RTPS_DllAPI WriterHistory(
            const HistoryAttributes&  att);
    RTPS_DllAPI virtual ~WriterHistory() override;

    /**
     * Add a CacheChange_t to the WriterHistory.
     * @param a_change Pointer to the CacheChange_t to be added.
     * @return True if added.
     */
    RTPS_DllAPI bool add_change(
            CacheChange_t* a_change);

    /**
     * Add a CacheChange_t to the WriterHistory.
     * @param a_change Pointer to the CacheChange_t to be added.
     * @param wparams Extra write parameters.
     * @return True if added.
     */
    RTPS_DllAPI bool add_change(
            CacheChange_t* a_change,
            WriteParams& wparams);

    /**
     * Remove a specific change from the history.
     * No Thread Safe
     * @param removal iterator to the change for removal
     * @param release specifies if the change should be return to the pool
     * @return iterator to the next change if any
     */
    RTPS_DllAPI iterator remove_change_nts(
            const_iterator removal,
            bool release = true) override;

    /**
     * Criteria to search a specific CacheChange_t on history
     * @param inner change to compare
     * @param outer change for comparison
     * @return true if inner matches outer criteria
     */
    RTPS_DllAPI bool matches_change(
            const CacheChange_t* inner,
            CacheChange_t* outer) override;

    //! Introduce base class method into scope
    using History::remove_change;

    RTPS_DllAPI virtual bool remove_change_g(
            CacheChange_t* a_change);

    RTPS_DllAPI bool remove_change(
            const SequenceNumber_t& sequence_number);

    RTPS_DllAPI CacheChange_t* remove_change_and_reuse(
            const SequenceNumber_t& sequence_number);

    /**
     * Remove the CacheChange_t with the minimum sequenceNumber.
     * @return True if correctly removed.
     */
    RTPS_DllAPI bool remove_min_change();

    RTPS_DllAPI SequenceNumber_t next_sequence_number() const
    {
        return m_lastCacheChangeSeqNum + 1;
    }

protected:

    RTPS_DllAPI bool do_reserve_cache(
            CacheChange_t** change,
            uint32_t size) override;

    RTPS_DllAPI void do_release_cache(
            CacheChange_t* ch) override;

    bool add_change_(
            CacheChange_t* a_change,
            WriteParams& wparams,
            std::chrono::time_point<std::chrono::steady_clock> max_blocking_time
            = std::chrono::steady_clock::now() + std::chrono::hours(24));

    //!Last CacheChange Sequence Number added to the History.
    SequenceNumber_t m_lastCacheChangeSeqNum;
    //!Pointer to the associated RTPSWriter;
    RTPSWriter* mp_writer;

    uint32_t high_mark_for_frag_ = 0;

private:

    void set_fragments(
            CacheChange_t* change);
};

} // namespace rtps
} /* namespace fastrtps */
} /* namespace eprosima */

#endif /* _FASTDDS_RTPS_WRITERHISTORY_H_ */
