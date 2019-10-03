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
 * @file TopicDescription.hpp
 */

#ifndef _FASTDDS_TOPIC_DESCRIPTION_HPP_
#define _FASTDDS_TOPIC_DESCRIPTION_HPP_

#include <string>

namespace eprosima {
namespace fastdds {
namespace dds {

class DomainParticipant;

/**
 * Class TopicDescription, represents the fact that both publications
 * and subscriptions are tied to a single data-type
 * @ingroup FASTDDS_MODULE
 */
class TopicDescription
{
protected:
    TopicDescription(
            const std::string& name,
            const std::string& type_name)
        : name_(name)
        , type_name_(type_name)
    {}

    ~TopicDescription()
    {}

   virtual DomainParticipant* get_participant() const = 0;

    const std::string& get_name() const
    {
        return name_;
    }

    const std::string& get_type_name() const
    {
        return type_name_;
    }

protected:

    //! Name that allows the TopicDescription to be retrieved locally
    std::string name_;

    //! Name that defines a unique resulting type for the publication or the subscription
    std::string type_name_;
};

} /* namespace dds */
} /* namespace fastdds */
} /* namespace eprosima */

#endif /* _FASTDDS_TYPE_SUPPORT_HPP_ */
