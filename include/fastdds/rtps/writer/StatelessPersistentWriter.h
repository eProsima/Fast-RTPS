// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file StatelessPersistentWriter.h
 */
#ifndef _FASTDDS_RTPS_STATELESSPERSISTENTWRITER_H_
#define _FASTDDS_RTPS_STATELESSPERSISTENTWRITER_H_

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastdds/rtps/writer/StatelessWriter.h>
#include <fastdds/rtps/writer/PersistentWriter.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

class IPersistenceService;

/**
 * Class StatelessPersistentWriter, specialization of StatelessWriter that manages history persistence.
 * @ingroup WRITER_MODULE
 */
class StatelessPersistentWriter : public StatelessWriter, private PersistentWriter
{
    friend class RTPSParticipantImpl;

    StatelessPersistentWriter(
            RTPSParticipantImpl*,
            const GUID_t& guid,
            const WriterAttributes& att,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr,
            IPersistenceService* persistence = nullptr);

    StatelessPersistentWriter(
            RTPSParticipantImpl*,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr,
            IPersistenceService* persistence = nullptr);

    StatelessPersistentWriter(
            RTPSParticipantImpl*,
            const GUID_t& guid,
            const WriterAttributes& att,
            const std::shared_ptr<IPayloadPool>& payload_pool,
            const std::shared_ptr<IChangePool>& change_pool,
            fastdds::rtps::FlowController* flow_controller,
            WriterHistory* hist,
            WriterListener* listen = nullptr,
            IPersistenceService* persistence = nullptr);

public:

    virtual ~StatelessPersistentWriter();

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
};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif /* _FASTDDS_RTPS_STATELESSPERSISTENTWRITER_H_ */
