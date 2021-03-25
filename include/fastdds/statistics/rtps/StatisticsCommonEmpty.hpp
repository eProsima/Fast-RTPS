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
 * @file StatisticsCommonEmpty.hpp
 */

#ifndef _FASTDDS_STATISTICS_RTPS_STATISTICSCOMMON_HPP_
#define _FASTDDS_STATISTICS_RTPS_STATISTICSCOMMON_HPP_

#include <fastdds/statistics/IListeners.hpp>

namespace eprosima {
namespace fastdds {
namespace statistics {

class StatisticsListenersImpl
{
    inline bool add_statistics_listener_impl(
            std::shared_ptr<fastdds::statistics::IListener> listener)
    {
        return true;
    }

    inline bool remove_statistics_listener_impl(
            std::shared_ptr<fastdds::statistics::IListener> listener)
    {
        return true;
    }

    // lambda function to traverse the listener collection
    template<class Function>
    inline Function for_each_listener(
            Function f)
    {
        return f;
    }
};

class StatisticsWriterImpl
    : protected StatisticsListenersImpl
{
protected:

    // TODO: methods for listeners callbacks
};

class StatisticsReaderImpl
    : protected StatisticsListenersImpl
{
protected:

    // TODO: methods for listeners callbacks
};

} // namespace statistics
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_STATISTICS_RTPS_STATISTICSCOMMON_HPP_
