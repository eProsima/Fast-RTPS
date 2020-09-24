// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file DiscoveryDataBase.hpp
 *
 */

#ifndef _FASTDDS_RTPS_DISCOVERY_DATABASE_H_
#define _FASTDDS_RTPS_DISCOVERY_DATABASE_H_

#include <vector>
#include <map>
#include <mutex>  // For std::unique_lock
#include <shared_mutex>

#include <fastrtps/utils/fixed_size_string.hpp>
#include <fastdds/rtps/writer/ReaderProxy.h>
#include <fastdds/rtps/common/CacheChange.h>
#include <fastrtps/utils/DBQueue.h>

#include "./DiscoveryDataFilter.hpp"
#include "./DiscoveryParticipantInfo.hpp"
#include "./DiscoveryEndpointInfo.hpp"
#include "./DiscoveryDataQueueInfo.hpp"

namespace eprosima {
namespace fastdds {
namespace rtps {
namespace ddb {

//typedef std::shared_timed_mutex share_mutex_t;
// only working in C++14
typedef std::shared_timed_mutex share_mutex_t;

struct CacheChangeCmp
{
    bool operator ()(
            const eprosima::fastrtps::rtps::CacheChange_t& a,
            const eprosima::fastrtps::rtps::CacheChange_t& b) const
    {
        return a.write_params.sample_identity() < b.write_params.sample_identity();
    }

};

/**
 * Class to manage the discovery data base
 *@ingroup DISCOVERY_MODULE
 */
class DiscoveryDataBase
    : public eprosima::fastdds::rtps::ddb::PDPDataFilter<DiscoveryDataBase>
    , public eprosima::fastdds::rtps::ddb::EDPDataFilter<DiscoveryDataBase>
    , public eprosima::fastdds::rtps::ddb::EDPDataFilter<DiscoveryDataBase, false>
{

public:

    class AckedFunctor
    {
        using argument_type = eprosima::fastrtps::rtps::ReaderProxy*;
        using result_type = void;

    public:

        AckedFunctor(
                DiscoveryDataBase* db,
                eprosima::fastrtps::rtps::CacheChange_t* change);

        ~AckedFunctor();

        void operator () (
                eprosima::fastrtps::rtps::ReaderProxy* reader_proxy);

        bool pending()
        {
            return pending_;
        }

    private:

        DiscoveryDataBase* db_;
        eprosima::fastrtps::rtps::CacheChange_t* change_;
        bool pending_ = false;

    };
    friend class AckedFunctor;


    ////////////
    // Functions to update queue from listener
    /* Add a new CacheChange_t to database queue
     *    1. Check whether the change is already in the database (queue lock)
     *    2. If the change is new, then add it to data_queue_ (queue lock)
     * @return: True if the change was added, false otherwise.
     */
    bool update(
            eprosima::fastrtps::rtps::CacheChange_t* change,
            std::string topic_name,
            eprosima::fastrtps::rtps::GUID_t* entity);


    ////////////
    // Functions to is_relevant
    // Return whether a PDP change is relevant for a given reader
    bool pdp_is_relevant(
            const eprosima::fastrtps::rtps::CacheChange_t& change,
            const eprosima::fastrtps::rtps::GUID_t& reader_guid) const;

    // Return whether a EDP publications change is relevant for a given reader
    bool edp_publications_is_relevant(
            const eprosima::fastrtps::rtps::CacheChange_t& change,
            const eprosima::fastrtps::rtps::GUID_t& reader_guid) const;

    // Return whether a EDP subscription change is relevant for a given reader
    bool edp_subscriptions_is_relevant(
            const eprosima::fastrtps::rtps::CacheChange_t& change,
            const eprosima::fastrtps::rtps::GUID_t& reader_guid) const;


    ////////////
    // Functions to process_writers_acknowledgements()
    // Return the functor, class that works as a lambda
    AckedFunctor functor(
            eprosima::fastrtps::rtps::CacheChange_t* change)
    {
        return DiscoveryDataBase::AckedFunctor(this, change);
    }

    /* Delete all information relative to the entity that produced a CacheChange
     * @change: That entity's CacheChange.
     * @return: True if the entity was deleted, false otherwise.
     */
    bool delete_entity_of_change(
            fastrtps::rtps::CacheChange_t* change);


    ////////////
    // Functions to process_data_queue()
    bool process_data_queue();


    ////////////
    // Functions to process_dirty_topics()
    bool process_dirty_topics();


    ////////////
    // Functions to process_disposals()
    const std::vector<eprosima::fastrtps::rtps::CacheChange_t*> changes_to_dispose()
    {
        return disposals_;
    }

    void clear_changes_to_dispose()
    {
        exclusive_lock_();
        disposals_.clear();
        exclusive_unlock_();
    }

    ////////////
    // Functions to process_to_send_lists()
    const std::vector<eprosima::fastrtps::rtps::CacheChange_t*> pdp_to_send()
    {
        return pdp_to_send_;
    }

    void clear_pdp_to_send()
    {
        exclusive_lock_();
        pdp_to_send_.clear();
        exclusive_unlock_();
    }

    const std::vector<eprosima::fastrtps::rtps::CacheChange_t*> edp_publications_to_send()
    {
        return edp_publications_to_send_;
    }

    void clear_edp_publications_to_send()
    {
        exclusive_lock_();
        edp_publications_to_send_.clear();
        exclusive_unlock_();
    }

    const std::vector<eprosima::fastrtps::rtps::CacheChange_t*> edp_subscriptions_to_send()
    {
        return edp_subscriptions_to_send_;
    }

    void clear_edp_subscriptions_to_send()
    {
        exclusive_lock_();
        edp_subscriptions_to_send_.clear();
        exclusive_unlock_();
    }

protected:

    // update the acks
    void add_ack_(
            const eprosima::fastrtps::rtps::CacheChange_t* change,
            const eprosima::fastrtps::rtps::GuidPrefix_t* acked_entity);


    ////////////
    // Static Functions to work with GUIDs
    static bool is_participant_(
            const eprosima::fastrtps::rtps::CacheChange_t* ch);

    static bool is_writer_(
            const eprosima::fastrtps::rtps::CacheChange_t* ch);

    static bool is_reader_(
            const eprosima::fastrtps::rtps::CacheChange_t* ch);

    static eprosima::fastrtps::rtps::GUID_t guid_from_change_(
            const eprosima::fastrtps::rtps::CacheChange_t* ch);

    ////////////
    // Mutex Functions
    void exclusive_lock_()
    {
        sh_mtx_.lock();
    }

    void shared_lock_()
    {
        sh_mtx_.lock_shared();
        //sh_mtx_.lock();
    }

    void exclusive_unlock_()
    {
        sh_mtx_.unlock();
    }

    void shared_unlock_()
    {
        sh_mtx_.unlock_shared();
        //sh_mtx_.unlock();
    }

    fastrtps::DBQueue<eprosima::fastdds::rtps::ddb::DiscoveryDataQueueInfo> data_queue_;

    //std::map<eprosima::fastrtps::rtps::CacheChange_t*, eprosima::fastrtps::rtps::GUID_t, CacheChangeCmp> data_map_;

    std::map<eprosima::fastrtps::string_255, eprosima::fastrtps::rtps::GUID_t> readers_by_topic_;

    std::map<eprosima::fastrtps::string_255, eprosima::fastrtps::rtps::GUID_t> writers_by_topic_;

    std::map<eprosima::fastrtps::rtps::GuidPrefix_t, DiscoveryParticipantInfo> participants_;

    std::map<eprosima::fastrtps::rtps::GUID_t, DiscoveryEndpointInfo> readers_;

    std::map<eprosima::fastrtps::rtps::GUID_t, DiscoveryEndpointInfo> writers_;

    std::vector<eprosima::fastrtps::rtps::CacheChange_t*> disposals_;

    std::vector<eprosima::fastrtps::string_255> dirty_topics_;

    std::vector<eprosima::fastrtps::rtps::CacheChange_t*> pdp_to_send_;

    std::vector<eprosima::fastrtps::rtps::CacheChange_t*> edp_publications_to_send_;

    std::vector<eprosima::fastrtps::rtps::CacheChange_t*> edp_subscriptions_to_send_;


    // mutexes

    mutable share_mutex_t sh_mtx_;

};


} /* namespace ddb */
} /* namespace rtps */
} /* namespace fastdds */
} /* namespace eprosima */

#endif /* _FASTDDS_RTPS_DISCOVERY_DATABASE_H_ */
