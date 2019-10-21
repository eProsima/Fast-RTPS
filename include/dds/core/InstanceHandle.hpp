/*
 * Copyright 2010, Object Management Group, Inc.
 * Copyright 2010, PrismTech, Corp.
 * Copyright 2010, Real-Time Innovations, Inc.
 * Copyright 2019, Proyectos y Sistemas de Mantenimiento SL (eProsima).
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OMG_DDS_CORE_INSTANCE_HANDLE_HPP_
#define OMG_DDS_CORE_INSTANCE_HANDLE_HPP_

#include <vector>
#include <iostream>

#include <dds/core/types.hpp>
#include <dds/core/Value.hpp>

#include <dds/core/detail/InstanceHandle.hpp>


namespace dds {
namespace core {

/**
 * @brief
 * Class to hold the handle associated with in sample instance.
 */
class InstanceHandle : public Value<detail::InstanceHandle>
{
public:
    /**
     * Create an nil instance handle.
     */
    InstanceHandle();

    /**
     * Create an nil instance handle.
     *
     * @param nullHandle placeholder
    InstanceHandle(
            const null_type& nullHandle);
     */

    /**
     * Copy an existing InstancHandle
     *
     * @param other InstanceHandle to copy
     */
    InstanceHandle(
            const InstanceHandle& other);

    /** @cond */
    ~InstanceHandle();
    /** @endcond */

    /**
     * @cond
     * Parametric constructor for creating an instance-handle
     * from some other type. This function is intended for internal
     * usage.
     */
    template<typename ARG0>
    InstanceHandle(
            const ARG0& arg0);
    /** @endcond */

public:
    /**
     * Assign an existing InstancHandle to this InstancHandle
     *
     * @param that The InstanceHandle to assign to this
     */
    InstanceHandle& operator =(
            const InstanceHandle& that);

    /**
     * Compare this InstanceHandle to another InstanceHandle
     *
     * @param that The InstanceHandle to compare
     * @return true if they match
     */
    bool operator ==(
            const InstanceHandle& that) const;

    /**
     * Compare this InstanceHandle to another InstanceHandle
     *
     * @param that The InstanceHandle to compare
     * @return true if this is less than that
     */
    bool operator <(
            const InstanceHandle& that) const;

    /**
     * Compare this InstanceHandle to another InstanceHandle
     *
     * @param that The InstanceHandle to compare
     * @return true if this is greater than that
     */
    bool operator >(
            const InstanceHandle& that) const;

public:
    /**
     * Create an nil instance handle.
     *
     * @return a nil InstanceHandle
     */
    static const InstanceHandle nil();

    /**
     * Check if the InstanceHandle is nil.
     *
     * @return true if the InstanceHandle is nil
     */
    bool is_nil() const;
};

typedef std::vector<InstanceHandle> InstanceHandleSeq;

} //namespace core
} //namespace dds

/*
inline std::ostream& operator <<(
        std::ostream& os,
        const dds::core::InstanceHandle& h);
*/

#endif //OMG_DDS_CORE_INSTANCE_HANDLE_HPP_
