/*
 * Copyright 2019, Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 *
*/

#ifndef EPROSIMA_DDS_SUB_SHARED_SAMPLES_IMPL_HPP_
#define EPROSIMA_DDS_SUB_SHARED_SAMPLES_IMPL_HPP_

#include <dds/sub/SharedSamples.hpp>

/**
 * @file
 */

/*
 * OMG PSM class declaration
 */

namespace dds {
namespace sub {

template<
        typename T,
        template <typename Q> class DELEGATE>
SharedSamples<T, DELEGATE>::SharedSamples()
    : delegate_(new DELEGATE<T>())
{
}

template<
        typename T,
        template <typename Q> class DELEGATE>
SharedSamples<T, DELEGATE>::SharedSamples(
        dds::sub::LoanedSamples<T> ls)
    : delegate_(new DELEGATE<T>(ls))
{
}

template<
        typename T,
        template <typename Q> class DELEGATE>
SharedSamples<T, DELEGATE>::~SharedSamples()
{
}

template <
        typename T,
        template <typename Q> class DELEGATE>
SharedSamples<T, DELEGATE>::SharedSamples(
        const SharedSamples& other)
{
    //To implement
}

template<
        typename T,
        template <typename Q> class DELEGATE>
typename SharedSamples<T, DELEGATE>::const_iterator SharedSamples<T, DELEGATE>::begin() const
{
    //To implement
}

template<
        typename T,
        template <typename Q> class DELEGATE>
typename SharedSamples<T, DELEGATE>::const_iterator SharedSamples<T, DELEGATE>::end() const
{
    //To implement
}

template<
        typename T,
        template <typename Q> class DELEGATE>
const typename SharedSamples<T, DELEGATE>::DELEGATE_REF_T& SharedSamples<T, DELEGATE>::delegate() const
{
    //To implement
}

template<
        typename T,
        template <typename Q> class DELEGATE>
typename SharedSamples<T, DELEGATE>::DELEGATE_REF_T& SharedSamples<T, DELEGATE>::delegate()
{
    //To implement
}

template<
        typename T,
        template <typename Q> class DELEGATE>
uint32_t SharedSamples<T, DELEGATE>::length() const
{
    //To implement
}

} //namespace sub
} //namespace dds

#endif //EPROSIMA_DDS_SUB_SHARED_SAMPLES_IMPL_HPP_
