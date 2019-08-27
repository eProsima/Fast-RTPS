/*
 * Copyright 2010, Object Management Group, Inc.
 * Copyright 2010, PrismTech, Corp.
 * Copyright 2010, Real-Time Innovations, Inc.
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
 */

#ifndef OMG_DDS_CORE_XTYPES_STRUCT_TYPE_HPP_
#define OMG_DDS_CORE_XTYPES_STRUCT_TYPE_HPP_

#include <dds/core/xtypes/detail/StructType.hpp>

#include <dds/core/xtypes/DynamicType.hpp>
#include <dds/core/xtypes/MemberType.hpp>
#include <dds/core/xtypes/PrimitiveTypes.hpp>

namespace dds {
namespace core {
namespace xtypes {

template<typename DELEGATE>
class TStructForwardDeclaration : public TDynamicType<DELEGATE>
{
public:
    TStructForwardDeclaration(
            const std::string& name);
};

/**
 * Create a dynamic structure type. If the members don't have Id associated
 * explicitly, then their ID will be the same as the ordinal position on the
 * members vector.
 */
template<typename DELEGATE>
class TStructType : public TDynamicType<DELEGATE>
{
public:

    TStructType(
            const std::string& name);

    TStructType(
        const std::string& name,
        const TStructType& parent,
        const std::vector<MemberType>& members);

    template<typename MemberIter>
    TStructType(
        const std::string& name,
        const TStructType& parent,
        const MemberIter& begin,
        const MemberIter& end);

    TStructType(
        const std::string& name,
        const TStructType& parent,
        const std::vector<MemberType>& members,
        const Annotation& annotation);

    TStructType(
        const std::string& name,
        const TStructType& parent,
        const std::vector<MemberType>& members,
        const std::vector<Annotation>& annotations);

    template<
            typename AnnotationIter,
            typename MemberIter>
    TStructType(
        const std::string& name,
        const TStructType& parent,
        const MemberIter& member_begin,
        const MemberIter& member_end,
        const AnnotationIter& annotation_begin,
        const AnnotationIter& annotation_end);

public:
    TStructType parent() const;

    const std::vector<MemberType>& members() const;

    const MemberType& member(
            uint32_t id) const;

    const MemberType& member(
            const std::string& name) const;

    const std::vector<Annotation>& annotations() const;

    TStructType add_member(
            const MemberType& member) const ;

    TStructType remove_member(
            const MemberType& member) const;

    TStructType add_annotation(
            const Annotation& annotation) const;

    TStructType remove_annotation(
            const Annotation& annotation) const;
};

typedef TStructType<detail::StructType> StructType;

} //namespace xtypes
} //namespace core
} //namespace dds

#endif //OMG_DDS_CORE_XTYPES_STRUCT_TYPE_HPP_
