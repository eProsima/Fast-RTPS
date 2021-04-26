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
 * @file DataReaderImpl.hpp
 */

#ifndef _STATISTICS_FASTDDS_SUBSCRIBER_DATAREADERIMPL_HPP_
#define _STATISTICS_FASTDDS_SUBSCRIBER_DATAREADERIMPL_HPP_

#include <fastdds/statistics/IListeners.hpp>

#include <fastdds/subscriber/DataReaderImpl.hpp>
#include <fastdds/rtps/reader/RTPSReader.h>

namespace eprosima {
namespace fastdds {
namespace statistics {
namespace dds {

namespace efd = eprosima::fastdds::dds;

class DataReaderImpl : public efd::DataReaderImpl
{
    using BaseType = efd::DataReaderImpl;

public:

    virtual ~DataReaderImpl() = default;

    DataReaderImpl(
            efd::SubscriberImpl* s,
            const efd::TypeSupport& type,
            efd::TopicDescription* topic,
            const efd::DataReaderQos& qos,
            efd::DataReaderListener* listener,
            std::shared_ptr<IListener> stat_listener)
        : BaseType(s, type, topic, qos, listener)
        , statistics_listener_(stat_listener)
    {
    }

    ReturnCode_t enable() override
    {
        ReturnCode_t ret = BaseType::enable();

        if (ReturnCode_t::RETCODE_OK == ret)
        {
            reader_->add_statistics_listener(statistics_listener_);
        }

        return ret;
    }

    void disable() override
    {
        if (nullptr != reader_)
        {
            reader_->remove_statistics_listener(statistics_listener_);
        }

        BaseType::disable();
    }

private:

    std::shared_ptr<IListener> statistics_listener_;
};

} // dds
} // statistics
} // fastdds
} // eprosima

#endif  // _STATISTICS_FASTDDS_SUBSCRIBER_DATAREADERIMPL_HPP_
