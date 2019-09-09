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

#ifndef TYPES_DYNAMIC_TYPE_H
#define TYPES_DYNAMIC_TYPE_H

#include <fastrtps/types/TypesBase.h>
#include <fastrtps/types/DynamicTypePtr.h>

namespace eprosima {
namespace fastrtps {
namespace types {

class AnnotationDescriptor;
class TypeDescriptor;
class DynamicTypeMember;
class DynamicTypeBuilder;

class DynamicType
{
protected:
    friend class DynamicTypeBuilder;
    friend class DynamicTypeBuilderFactory;
    friend class MemberDescriptor;
    friend class TypeDescriptor;
    friend class DynamicData;
    friend class DynamicDataFactory;
    friend class AnnotationDescriptor;
    friend class TypeObjectFactory;
    friend class DynamicTypeMember;
    friend class DynamicDataHelper;

    DynamicType();

    DynamicType(const TypeDescriptor* descriptor);

    DynamicType(const DynamicTypeBuilder* other);

    virtual ~DynamicType();

    virtual void clear();

    ResponseCode copy_from_builder(const DynamicTypeBuilder* other);

    // Checks if there is a member with the given name.
    bool exists_member_by_name(const std::string& name) const;

    // This method is used by Dynamic Data to override the name of the types based on ALIAS.
    void set_name(const std::string& name);

    ResponseCode apply_annotation(AnnotationDescriptor& descriptor);

    ResponseCode apply_annotation(
            const std::string& annotation_name,
            const std::string& key,
            const std::string& value);

    ResponseCode apply_annotation_to_member(
            MemberId id,
            AnnotationDescriptor& descriptor);

    ResponseCode apply_annotation_to_member(
            MemberId id,
            const std::string& annotation_name,
            const std::string& key,
            const std::string& value);

    ResponseCode get_annotation(
            AnnotationDescriptor& descriptor,
            uint32_t idx);

    uint32_t get_annotation_count();


    ResponseCode get_member(
            DynamicTypeMember& member,
            MemberId id);

    ResponseCode get_member_by_name(
            DynamicTypeMember& member,
            const std::string& name);

    TypeDescriptor* descriptor_;
    std::map<MemberId, DynamicTypeMember*> member_by_id_;         // Aggregated members
    std::map<std::string, DynamicTypeMember*> member_by_name_;    // Uses the pointers from "member_by_id_".
    std::string name_;
    TypeKind kind_;
    bool is_key_defined_;

public:
//  NEW PUBLIC METHODS TO ENABLE STAND-ALONE SERIALIZER
    DynamicType_ptr get_discriminator_type() const;
    DynamicType_ptr get_element_type() const;
    DynamicType_ptr get_base_type() const;
    std::map<MemberId, DynamicTypeMember*> get_member_by_id() const {return member_by_id_;}
    DynamicType_ptr get_key_element_type() const;
				bool is_key_defined()const noexcept{return is_key_defined_ ;}
//  END PUBLIC METHODS TO ENABLE STAND-ALONE SERIALIZER

    bool equals(const DynamicType* other) const;

    ResponseCode get_all_members(std::map<MemberId, DynamicTypeMember*>& members);

    ResponseCode get_all_members_by_name(std::map<std::string, DynamicTypeMember*>& members);

    uint32_t get_bounds(uint32_t index = 0) const;

    uint32_t get_bounds_size() const;

    ResponseCode get_descriptor(TypeDescriptor* descriptor) const;

    const TypeDescriptor* get_descriptor() const;

    TypeDescriptor* get_descriptor();

    bool key_annotation() const;

    inline TypeKind get_kind() const
    {
        return kind_;
    }

    std::string get_name() const;

    MemberId get_members_count() const;

    uint32_t get_total_bounds() const;

    const TypeDescriptor* get_type_descriptor() const
    {
        return descriptor_;
    }

    bool has_children() const;

    bool is_consistent() const;

    bool is_complex_kind() const;

    bool is_discriminator_type() const;

    size_t get_size() const;

};

} // namespace types
} // namespace fastrtps
} // namespace eprosima

#endif // TYPES_DYNAMIC_TYPE_H
