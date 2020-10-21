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

#ifndef _FASTDDS_STDOUTERR_CONSUMER_HPP_
#define _FASTDDS_STDOUTERR_CONSUMER_HPP_

#include <gmock/gmock.h>

#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/log/OStreamConsumer.hpp>

namespace eprosima {
namespace fastdds {
namespace dds {

class StdoutErrConsumer : public OStreamConsumer
{
public:
    StdoutErrConsumer() = default;

    virtual ~StdoutErrConsumer() {}

    void stderr_threshold(
            const Log::Kind& kind)
    {
        (void)kind;
    }

    std::ostream& get_stream(
                const Log::Entry& entry)
    {
        // If Log::Kind is stderr_threshold_ or more severe, then use STDERR, else use STDOUT
        if (entry.kind <= STDERR_THRESHOLD_DEFAULT)
        {
            return std::cerr;
        }
        else
        {
            return std::cout;
        }
    }

    static const Log::Kind STDERR_THRESHOLD_DEFAULT = Log::Kind::Warning;
};

MATCHER(IsStdoutErrConsumer, "Argument is a StdoutErrConsumer object?")
{
    *result_listener << (typeid(*arg.get()) == typeid(StdoutErrConsumer));
    return typeid(*arg.get()) == typeid(StdoutErrConsumer);
}

} // namespace dds
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_STDOUTERR_CONSUMER_HPP_
