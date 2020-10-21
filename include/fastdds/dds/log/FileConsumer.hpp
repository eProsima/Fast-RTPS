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
 * @file FileConsumer.hpp
 *
 */

#ifndef _FASTDDS_FILE_CONSUMER_HPP_
#define _FASTDDS_FILE_CONSUMER_HPP_

#include <fastdds/dds/log/Log.hpp>

#include <fstream>

namespace eprosima {
namespace fastdds {
namespace dds {

/**
 * Log consumer that writes the log events to a file.
 *
 * @file FileConsumer.hpp
 */
class FileConsumer : public LogConsumer
{
public:

    //! Default constructor: filename = "output.log", append = false.
    RTPS_DllAPI FileConsumer();

    /** Constructor with parameters.
     * @param filename path of the output file where the log will be wrote.
     * @param append indicates if the consumer must append the content in the filename.
     */
    RTPS_DllAPI FileConsumer(
            const std::string& filename,
            bool append = false);

    /** \internal
     * Called by Log to ask us to consume the Entry.
     * @param Log::Entry to consume.
     */
    RTPS_DllAPI virtual void Consume(
            const Log::Entry& entry);

    virtual ~FileConsumer();

private:

    void print_header(
            const Log::Entry& entry);

    void print_context(
            const Log::Entry& entry);

    std::string output_file_;
    std::ofstream file_;
    bool append_;
};

} // namespace dds
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_FILE_CONSUMER_HPP_
