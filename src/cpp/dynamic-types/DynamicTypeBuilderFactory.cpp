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

#include <fastrtps/types/DynamicTypeBuilderFactory.h>
#include <fastrtps/types/DynamicTypeBuilder.h>
#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/TypeDescriptor.h>
#include <fastrtps/types/TypeObject.h>
#include <fastrtps/types/DynamicType.h>
#include <fastrtps/types/DynamicTypePtr.h>
#include <fastrtps/types/DynamicTypeMember.h>
#include <fastrtps/types/MemberDescriptor.h>
#include <fastrtps/types/TypeNamesGenerator.h>
#include <fastrtps/types/AnnotationDescriptor.h>
#include <fastdds/dds/log/Log.hpp>

#include <fastdds/rtps/common/SerializedPayload.h>
#include <fastrtps/utils/md5.h>
#include <fastrtps/utils/string_convert.hpp>
#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>

namespace eprosima {
namespace fastrtps {
namespace types {

static std::string get_type_name(
        TypeKind kind)
{
    switch (kind)
    {
        // Primitive types, already defined (never will be asked, but ok)
        case TK_BOOLEAN: return TKNAME_BOOLEAN;
        case TK_INT16: return TKNAME_INT16;
        case TK_INT32: return TKNAME_INT32;
        case TK_UINT16: return TKNAME_UINT16;
        case TK_UINT32: return TKNAME_UINT32;
        case TK_FLOAT32: return TKNAME_FLOAT32;
        case TK_FLOAT64: return TKNAME_FLOAT64;
        case TK_CHAR8: return TKNAME_CHAR8;
        case TK_BYTE: return TKNAME_BYTE;
        case TK_INT64: return TKNAME_INT64;
        case TK_UINT64: return TKNAME_UINT64;
        case TK_FLOAT128: return TKNAME_FLOAT128;
        case TK_CHAR16: return TKNAME_CHAR16;
        /*
           case TK_STRING8: return TKNAME_STRING8;
           case TK_STRING16: return TKNAME_STRING16;
           case TK_ALIAS: return TKNAME_ALIAS;
           case TK_ENUM: return TKNAME_ENUM;
         */
        case TK_BITMASK: return TKNAME_BITMASK;
        /*
           case TK_ANNOTATION: return TKNAME_ANNOTATION;
           case TK_STRUCTURE: return TKNAME_STRUCTURE;
           case TK_UNION: return TKNAME_UNION;
         */
        case TK_BITSET: return TKNAME_BITSET;
        /*
           case TK_SEQUENCE: return TKNAME_SEQUENCE;
           case TK_ARRAY: return TKNAME_ARRAY;
           case TK_MAP: return TKNAME_MAP;
         */
        default:
            break;
    }
    return "UNDEF";
}

//static uint32_t s_typeNameCounter = 0;
static std::string GenerateTypeName(
        const std::string& kind)
{
    std::string tempKind = kind;
    std::replace(tempKind.begin(), tempKind.end(), ' ', '_');
    return tempKind;// + "_" + std::to_string(++s_typeNameCounter);
}

class DynamicTypeBuilderFactoryReleaser
{
public:

    ~DynamicTypeBuilderFactoryReleaser()
    {
        DynamicTypeBuilderFactory::delete_instance();
    }

};

static DynamicTypeBuilderFactoryReleaser s_releaser;
static DynamicTypeBuilderFactory* g_instance = nullptr;
DynamicTypeBuilderFactory* DynamicTypeBuilderFactory::get_instance()
{
    if (g_instance == nullptr)
    {
        g_instance = new DynamicTypeBuilderFactory();
    }
    return g_instance;
}

ReturnCode_t DynamicTypeBuilderFactory::delete_instance()
{
    if (g_instance != nullptr)
    {
        delete g_instance;
        g_instance = nullptr;
        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_ERROR;
}

DynamicTypeBuilderFactory::DynamicTypeBuilderFactory()
{
}

DynamicTypeBuilderFactory::~DynamicTypeBuilderFactory()
{
#ifndef DISABLE_DYNAMIC_MEMORY_CHECK
    std::unique_lock<std::recursive_mutex> scoped(mutex_);
    for (auto it = builders_list_.begin(); it != builders_list_.end(); ++it)
    {
        delete *it;
    }
    builders_list_.clear();
#endif // ifndef DISABLE_DYNAMIC_MEMORY_CHECK
}

void DynamicTypeBuilderFactory::add_builder_to_list(
        DynamicTypeBuilder* pBuilder)
{
    (void)pBuilder;
#ifndef DISABLE_DYNAMIC_MEMORY_CHECK
    std::unique_lock<std::recursive_mutex> scoped(mutex_);
    builders_list_.push_back(pBuilder);
#endif // ifndef DISABLE_DYNAMIC_MEMORY_CHECK
}

DynamicType_ptr DynamicTypeBuilderFactory::build_type(
        DynamicType_ptr other)
{
    return other;
}

DynamicType_ptr DynamicTypeBuilderFactory::create_type(
        const TypeDescriptor* descriptor,
        const std::string& name)
{
    if (descriptor != nullptr)
    {
        DynamicType_ptr pNewType(new DynamicType(descriptor));
        if (name.length() > 0)
        {
            pNewType->set_name(name);
        }
        return pNewType;
    }
    else
    {
        logError(DYN_TYPES, "Error building type, invalid input descriptor");
        return DynamicType_ptr(nullptr);
    }
}

DynamicType_ptr DynamicTypeBuilderFactory::create_type(
        const DynamicTypeBuilder* other)
{
    if (other != nullptr)
    {
        DynamicType_ptr pNewType(new DynamicType(other));
        return pNewType;
    }
    else
    {
        logError(DYN_TYPES, "Error building type, invalid input parameter");
        return DynamicType_ptr(nullptr);
    }
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_alias_builder(
        DynamicTypeBuilder* base_type,
        const std::string& sName)
{
    if (base_type != nullptr)
    {
        DynamicType_ptr pType = create_type(base_type);
        if (pType != nullptr)
        {
            return create_alias_builder(pType, sName);
        }
        else
        {
            logError(DYN_TYPES, "Error creating alias type, Error creating dynamic type");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error creating alias type, base_type must be valid");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_alias_builder(
        DynamicType_ptr base_type,
        const std::string& sName)
{
    if (base_type != nullptr)
    {
        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_ALIAS;
        pDescriptor.base_type_ = base_type;
        if (sName.length() > 0)
        {
            pDescriptor.name_ = sName;
        }
        else
        {
            //pDescriptor.name_ = GenerateTypeName(get_type_name(TK_ALIAS));
            pDescriptor.name_ = base_type->get_name();
        }

        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error creating alias type, base_type must be valid");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_array_builder(
        const DynamicTypeBuilder* element_type,
        const std::vector<uint32_t>& bounds)
{
    if (element_type != nullptr)
    {
        DynamicType_ptr pType = create_type(element_type);
        if (pType != nullptr)
        {
            return create_array_builder(pType, bounds);
        }
        else
        {
            logError(DYN_TYPES, "Error creating array, error creating dynamic type");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error creating array, element_type must be valid");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_array_builder(
        const DynamicType_ptr type,
        const std::vector<uint32_t>& bounds)
{
    if (type != nullptr)
    {
        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_ARRAY;
        pDescriptor.name_ = TypeNamesGenerator::get_array_type_name(type->get_name(), bounds, false);
        pDescriptor.element_type_ = type;
        pDescriptor.bound_ = bounds;

        for (uint32_t i = 0; i < pDescriptor.bound_.size(); ++i)
        {
            if (pDescriptor.bound_[i] == 0)
            {
                pDescriptor.bound_[i] = MAX_ELEMENTS_COUNT;
            }
        }


        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error creating array, element_type must be valid");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_bitmask_builder(
        uint32_t bound)
{
    if (bound <= MAX_BITMASK_LENGTH)
    {
        TypeDescriptor pBoolDescriptor;
        pBoolDescriptor.kind_ = TK_BOOLEAN;
        pBoolDescriptor.name_ = GenerateTypeName(get_type_name(TK_BOOLEAN));

        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_BITMASK;
        // TODO review on implementation for IDL
        pDescriptor.name_ = GenerateTypeName(get_type_name(TK_BITMASK));
        pDescriptor.element_type_ = create_type(&pBoolDescriptor);
        pDescriptor.bound_.push_back(bound);

        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error creating bitmask, length exceeds the maximum value '" << MAX_BITMASK_LENGTH << "'");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_bitset_builder()
{
    TypeDescriptor pDescriptor;
    pDescriptor.kind_ = TK_BITSET;
    // TODO Review on implementation for IDL
    pDescriptor.name_ = GenerateTypeName(get_type_name(TK_BITSET));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_bool_builder()
{
    TypeDescriptor pBoolDescriptor;
    pBoolDescriptor.kind_ = TK_BOOLEAN;
    pBoolDescriptor.name_ = GenerateTypeName(get_type_name(TK_BOOLEAN));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pBoolDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_byte_builder()
{
    TypeDescriptor pByteDescriptor;
    pByteDescriptor.kind_ = TK_BYTE;
    pByteDescriptor.name_ = GenerateTypeName(get_type_name(TK_BYTE));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pByteDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_char8_builder()
{
    TypeDescriptor pChar8Descriptor;
    pChar8Descriptor.kind_ = TK_CHAR8;
    pChar8Descriptor.name_ = GenerateTypeName(get_type_name(TK_CHAR8));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pChar8Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_char16_builder()
{
    TypeDescriptor pChar16Descriptor;
    pChar16Descriptor.kind_ = TK_CHAR16;
    pChar16Descriptor.name_ = GenerateTypeName(get_type_name(TK_CHAR16));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pChar16Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicType_ptr DynamicTypeBuilderFactory::create_annotation_primitive(
        const std::string& name)
{
    TypeDescriptor pEnumDescriptor;
    pEnumDescriptor.kind_ = TK_ANNOTATION;
    //pEnumDescriptor.name_ = GenerateTypeName(get_type_name(TK_ANNOTATION));
    pEnumDescriptor.name_ = name;

    return DynamicType_ptr(new DynamicType(&pEnumDescriptor));
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_enum_builder()
{
    TypeDescriptor pEnumDescriptor;
    pEnumDescriptor.kind_ = TK_ENUM;
    //pEnumDescriptor.name_ = GenerateTypeName(get_type_name(TK_ENUM));
    // Enum currently is an alias for uint32_t
    pEnumDescriptor.name_ = GenerateTypeName(get_type_name(TK_UINT32));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pEnumDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_float32_builder()
{
    TypeDescriptor pFloat32Descriptor;
    pFloat32Descriptor.kind_ = TK_FLOAT32;
    pFloat32Descriptor.name_ = GenerateTypeName(get_type_name(TK_FLOAT32));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pFloat32Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_float64_builder()
{
    TypeDescriptor pFloat64Descriptor;
    pFloat64Descriptor.kind_ = TK_FLOAT64;
    pFloat64Descriptor.name_ = GenerateTypeName(get_type_name(TK_FLOAT64));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pFloat64Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_float128_builder()
{
    TypeDescriptor pFloat128Descriptor;
    pFloat128Descriptor.kind_ = TK_FLOAT128;
    pFloat128Descriptor.name_ = GenerateTypeName(get_type_name(TK_FLOAT128));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pFloat128Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_int16_builder()
{
    TypeDescriptor pInt16Descriptor;
    pInt16Descriptor.kind_ = TK_INT16;
    pInt16Descriptor.name_ = GenerateTypeName(get_type_name(TK_INT16));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pInt16Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_int32_builder()
{
    TypeDescriptor pInt32Descriptor;
    pInt32Descriptor.kind_ = TK_INT32;
    pInt32Descriptor.name_ = GenerateTypeName(get_type_name(TK_INT32));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pInt32Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_int64_builder()
{
    TypeDescriptor pInt64Descriptor;
    pInt64Descriptor.kind_ = TK_INT64;
    pInt64Descriptor.name_ = GenerateTypeName(get_type_name(TK_INT64));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pInt64Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_map_builder(
        DynamicTypeBuilder* key_element_type,
        DynamicTypeBuilder* element_type,
        uint32_t bound)
{
    if (key_element_type != nullptr && element_type != nullptr)
    {
        DynamicType_ptr pKeyType = create_type(key_element_type);
        DynamicType_ptr pValueType = create_type(element_type);
        if (pKeyType != nullptr && pValueType != nullptr)
        {
            return create_map_builder(pKeyType, pValueType, bound);
        }
        else
        {
            logError(DYN_TYPES, "Error creating map, Error creating dynamic types.");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error creating map, element_type and key_element_type must be valid.");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_map_builder(
        DynamicType_ptr key_type,
        DynamicType_ptr value_type,
        uint32_t bound)
{
    if (key_type != nullptr && value_type != nullptr)
    {
        if (bound == BOUND_UNLIMITED)
        {
            bound = MAX_ELEMENTS_COUNT;
        }

        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_MAP;
        //pDescriptor.name_ = GenerateTypeName(get_type_name(TK_MAP));
        pDescriptor.bound_.push_back(bound);
        pDescriptor.key_element_type_ = key_type;
        pDescriptor.element_type_ = value_type;

        pDescriptor.name_ = TypeNamesGenerator::get_map_type_name(key_type->get_name(), value_type->get_name(),
                        bound, false);

        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error creating map, element_type and key_element_type must be valid.");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_sequence_builder(
        const DynamicTypeBuilder* element_type,
        uint32_t bound)
{
    if (element_type != nullptr)
    {
        DynamicType_ptr pType = create_type(element_type);
        if (pType != nullptr)
        {
            return create_sequence_builder(pType, bound);
        }
        else
        {
            logError(DYN_TYPES, "Error creating sequence, error creating dynamic type.");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error creating sequence, element_type must be valid.");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_sequence_builder(
        const DynamicType_ptr type,
        uint32_t bound)
{
    if (type != nullptr)
    {
        if (bound == BOUND_UNLIMITED)
        {
            bound = MAX_ELEMENTS_COUNT;
        }

        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_SEQUENCE;
        pDescriptor.name_ = TypeNamesGenerator::get_sequence_type_name(type->get_name(), bound, false);
        pDescriptor.bound_.push_back(bound);
        pDescriptor.element_type_ = type;

        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error creating sequence, element_type must be valid.");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_string_builder(
        uint32_t bound)
{
    if (bound == BOUND_UNLIMITED)
    {
        bound = MAX_STRING_LENGTH;
    }

    TypeDescriptor pCharDescriptor;
    pCharDescriptor.kind_ = TK_CHAR8;
    pCharDescriptor.name_ = GenerateTypeName(get_type_name(TK_CHAR8));

    TypeDescriptor pDescriptor;
    pDescriptor.kind_ = TK_STRING8;
    //pDescriptor.name_ = GenerateTypeName(get_type_name(TK_STRING8));
    pDescriptor.element_type_ = create_type(&pCharDescriptor);
    pDescriptor.bound_.push_back(bound);

    pDescriptor.name_ = TypeNamesGenerator::get_string_type_name(bound, false, true);

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_child_struct_builder(
        DynamicTypeBuilder* parent_type)
{
    if (parent_type != nullptr && (parent_type->get_kind() == TK_STRUCTURE || parent_type->get_kind() == TK_BITSET))
    {
        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = parent_type->get_kind();
        pDescriptor.name_ = GenerateTypeName(get_type_name(parent_type->get_kind()));
        pDescriptor.base_type_ = create_type(parent_type);

        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error creating child struct, invalid input type.");
        return nullptr;
    }
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_struct_builder()
{
    TypeDescriptor pDescriptor;
    pDescriptor.kind_ = TK_STRUCTURE;
    pDescriptor.name_ = GenerateTypeName(get_type_name(TK_STRUCTURE));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_custom_builder(
        const TypeDescriptor* descriptor,
        const std::string& name /*= ""*/)
{
    if (descriptor != nullptr)
    {
        TypeKind kind = descriptor->get_kind();
        if (kind == TK_BOOLEAN || kind == TK_BYTE || kind == TK_INT16 || kind == TK_INT32 ||
                kind == TK_INT64 || kind == TK_UINT16 || kind == TK_UINT32 || kind == TK_UINT64 ||
                kind == TK_FLOAT32 || kind == TK_FLOAT64 || kind == TK_FLOAT128 || kind == TK_CHAR8 ||
                kind == TK_CHAR16 || kind == TK_STRING8 || kind == TK_STRING16 || kind == TK_ALIAS ||
                kind == TK_ENUM || kind == TK_BITMASK || kind == TK_STRUCTURE || kind == TK_UNION ||
                kind == TK_BITSET || kind == TK_SEQUENCE || kind == TK_ARRAY || kind == TK_MAP ||
                kind == TK_ANNOTATION)
        {
            DynamicTypeBuilder* pNewType = new DynamicTypeBuilder(descriptor);
            if (pNewType != nullptr && name.length() > 0)
            {
                pNewType->set_name(name);
            }
            add_builder_to_list(pNewType);
            return pNewType;
        }
        else
        {
            logError(DYN_TYPES, "Error creating type, unsupported type kind: " << static_cast<uint32_t>(kind));
        }
    }
    else
    {
        logError(DYN_TYPES, "Error creating type, invalid input descriptor.");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_builder_copy(
        const DynamicTypeBuilder* type)
{
    if (type != nullptr)
    {
        DynamicTypeBuilder* pNewType = new DynamicTypeBuilder(type);
        add_builder_to_list(pNewType);
        return pNewType;
    }
    else
    {
        logError(DYN_TYPES, "Error creating type, invalid input type.");
    }
    return nullptr;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_uint16_builder()
{
    TypeDescriptor pUInt16Descriptor;
    pUInt16Descriptor.kind_ = TK_UINT16;
    pUInt16Descriptor.name_ = GenerateTypeName(get_type_name(TK_UINT16));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pUInt16Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_uint32_builder()
{
    TypeDescriptor pUInt32Descriptor;
    pUInt32Descriptor.kind_ = TK_UINT32;
    pUInt32Descriptor.name_ = GenerateTypeName(get_type_name(TK_UINT32));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pUInt32Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_uint64_builder()
{
    TypeDescriptor pUInt64Descriptor;
    pUInt64Descriptor.kind_ = TK_UINT64;
    pUInt64Descriptor.name_ = GenerateTypeName(get_type_name(TK_UINT64));

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pUInt64Descriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_union_builder(
        DynamicTypeBuilder* discriminator_type)
{
    if (discriminator_type != nullptr && discriminator_type->is_discriminator_type())
    {
        DynamicType_ptr pType = create_type(discriminator_type);
        if (pType != nullptr)
        {
            return create_union_builder(pType);
        }
        else
        {
            logError(DYN_TYPES, "Error building Union, Error creating discriminator type");
            return nullptr;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error building Union, invalid discriminator type");
        return nullptr;
    }
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_union_builder(
        DynamicType_ptr discriminator_type)
{
    if (discriminator_type != nullptr && discriminator_type->is_discriminator_type())
    {
        TypeDescriptor pUnionDescriptor;
        pUnionDescriptor.kind_ = TK_UNION;
        pUnionDescriptor.name_ = GenerateTypeName(get_type_name(TK_UNION));
        pUnionDescriptor.discriminator_type_ = discriminator_type;

        DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pUnionDescriptor);
        add_builder_to_list(pNewTypeBuilder);
        return pNewTypeBuilder;
    }
    else
    {
        logError(DYN_TYPES, "Error building Union, invalid discriminator type");
        return nullptr;
    }
}

DynamicTypeBuilder* DynamicTypeBuilderFactory::create_wstring_builder(
        uint32_t bound)
{
    if (bound == BOUND_UNLIMITED)
    {
        bound = MAX_STRING_LENGTH;
    }

    TypeDescriptor pCharDescriptor;
    pCharDescriptor.kind_ = TK_CHAR16;
    pCharDescriptor.name_ = GenerateTypeName(get_type_name(TK_CHAR16));

    TypeDescriptor pDescriptor;
    pDescriptor.kind_ = TK_STRING16;
    //pDescriptor.name_ = GenerateTypeName(get_type_name(TK_STRING16));
    pDescriptor.element_type_ = create_type(&pCharDescriptor);
    pDescriptor.bound_.push_back(bound);

    pDescriptor.name_ = TypeNamesGenerator::get_string_type_name(bound, true, true);

    DynamicTypeBuilder* pNewTypeBuilder = new DynamicTypeBuilder(&pDescriptor);
    add_builder_to_list(pNewTypeBuilder);
    return pNewTypeBuilder;
}

ReturnCode_t DynamicTypeBuilderFactory::delete_builder(
        DynamicTypeBuilder* builder)
{
    if (builder != nullptr)
    {
#ifndef DISABLE_DYNAMIC_MEMORY_CHECK
        std::unique_lock<std::recursive_mutex> scoped(mutex_);
        auto it = std::find(builders_list_.begin(), builders_list_.end(), builder);
        if (it != builders_list_.end())
        {
            builders_list_.erase(it);
            delete builder;
        }
        else
        {
            logWarning(DYN_TYPES, "The given type has been deleted previously.");
            return ReturnCode_t::RETCODE_ALREADY_DELETED;
        }
#else
        delete builder;
#endif // ifndef DISABLE_DYNAMIC_MEMORY_CHECK
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DynamicTypeBuilderFactory::delete_type(
        DynamicType* type)
{
    if (type != nullptr)
    {
        delete type;
    }
    return ReturnCode_t::RETCODE_OK;
}

DynamicType_ptr DynamicTypeBuilderFactory::get_primitive_type(
        TypeKind kind)
{
    TypeDescriptor pDescriptor;
    pDescriptor.kind_ = kind;
    pDescriptor.name_ = GenerateTypeName(get_type_name(kind));
    return create_type(&pDescriptor);
}

bool DynamicTypeBuilderFactory::is_empty() const
{
#ifndef DISABLE_DYNAMIC_MEMORY_CHECK
    return builders_list_.empty();
#else
    return true;
#endif // ifndef DISABLE_DYNAMIC_MEMORY_CHECK
}

void DynamicTypeBuilderFactory::build_type_identifier(
        const DynamicType_ptr type,
        TypeIdentifier& identifier,
        bool complete) const
{
    const TypeDescriptor* descriptor = type->get_type_descriptor();
    build_type_identifier(descriptor, identifier, complete);
}

void DynamicTypeBuilderFactory::build_type_identifier(
        const TypeDescriptor* descriptor,
        TypeIdentifier& identifier,
        bool complete) const
{
    const TypeIdentifier* id2 = (complete)
        ? TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(descriptor->get_name())
        : TypeObjectFactory::get_instance()->get_type_identifier(descriptor->get_name());
    if (id2 != nullptr)
    {
        identifier = *id2;
    }
    else
    {
        switch (descriptor->kind_)
        {
            // Basic types
            case TK_NONE:
            case TK_BOOLEAN:
            case TK_BYTE:
            case TK_INT16:
            case TK_INT32:
            case TK_INT64:
            case TK_UINT16:
            case TK_UINT32:
            case TK_UINT64:
            case TK_FLOAT32:
            case TK_FLOAT64:
            case TK_FLOAT128:
            case TK_CHAR8:
            case TK_CHAR16:
            {
                identifier._d(descriptor->kind_);
            }
            break;
            // String TKs
            case TK_STRING8:
            {
                if (descriptor->bound_[0] < 256)
                {
                    identifier._d(TI_STRING8_SMALL);
                    identifier.string_sdefn().bound(static_cast<SBound>(descriptor->bound_[0]));
                }
                else
                {
                    identifier._d(TI_STRING8_LARGE);
                    identifier.string_ldefn().bound(descriptor->bound_[0]);
                }
            }
            break;
            case TK_STRING16:
            {
                if (descriptor->bound_[0] < 256)
                {
                    identifier._d(TI_STRING16_SMALL);
                    identifier.string_sdefn().bound(static_cast<SBound>(descriptor->bound_[0]));
                }
                else
                {
                    identifier._d(TI_STRING16_LARGE);
                    identifier.string_ldefn().bound(descriptor->bound_[0]);
                }
            }
            break;
            // Collection TKs
            case TK_SEQUENCE:
            {
                if (descriptor->bound_[0] < 256)
                {
                    identifier._d(TI_PLAIN_SEQUENCE_SMALL);
                    identifier.seq_sdefn().bound(static_cast<SBound>(descriptor->bound_[0]));
                    TypeIdentifier elem_id;
                    build_type_identifier(descriptor->get_element_type()->descriptor_, elem_id, complete);
                    identifier.seq_sdefn().element_identifier(&elem_id);
                }
                else
                {
                    identifier._d(TI_PLAIN_SEQUENCE_LARGE);
                    identifier.seq_ldefn().bound(descriptor->bound_[0]);
                    TypeIdentifier elem_id;
                    build_type_identifier(descriptor->get_element_type()->descriptor_, elem_id, complete);
                    identifier.seq_ldefn().element_identifier(&elem_id);
                }
            }
            break;
            case TK_ARRAY:
            {
                uint32_t size = 0;
                for (uint32_t s : descriptor->bound_)
                {
                    size += s;
                }

                if (size < 256)
                {
                    identifier._d(TI_PLAIN_ARRAY_SMALL);
                    for (uint32_t b : descriptor->bound_)
                    {
                        identifier.array_sdefn().array_bound_seq().emplace_back(static_cast<SBound>(b));
                    }
                    TypeIdentifier elem_id;
                    build_type_identifier(descriptor->get_element_type()->descriptor_, elem_id, complete);
                    identifier.array_sdefn().element_identifier(&elem_id);
                }
                else
                {
                    identifier._d(TI_PLAIN_ARRAY_LARGE);
                    identifier.array_ldefn().array_bound_seq(descriptor->bound_);
                    TypeIdentifier elem_id;
                    build_type_identifier(descriptor->get_element_type()->descriptor_, elem_id, complete);
                    identifier.array_ldefn().element_identifier(&elem_id);
                }
            }
            break;
            case TK_MAP:
            {
                if (descriptor->bound_[0] < 256)
                {
                    identifier._d(TI_PLAIN_MAP_SMALL);
                    identifier.map_sdefn().bound(static_cast<SBound>(descriptor->bound_[0]));
                    TypeIdentifier elem_id;
                    build_type_identifier(descriptor->get_element_type()->descriptor_, elem_id, complete);
                    identifier.map_sdefn().element_identifier(&elem_id);
                    TypeIdentifier key_id;
                    build_type_identifier(descriptor->get_key_element_type()->descriptor_, key_id, complete);
                    identifier.map_sdefn().key_identifier(&key_id);
                }
                else
                {
                    identifier._d(TI_PLAIN_MAP_LARGE);
                    identifier.map_ldefn().bound(static_cast<SBound>(descriptor->bound_[0]));
                    TypeIdentifier elem_id;
                    build_type_identifier(descriptor->get_element_type()->descriptor_, elem_id, complete);
                    identifier.map_ldefn().element_identifier(&elem_id);
                    TypeIdentifier key_id;
                    build_type_identifier(descriptor->get_key_element_type()->descriptor_, key_id, complete);
                    identifier.map_ldefn().key_identifier(&key_id);
                }
            }
            break;
            // Constructed/Named types
            case TK_ALIAS:
            // Enumerated TKs
            case TK_ENUM:
            case TK_BITMASK:
            // Structured TKs
            case TK_ANNOTATION:
            case TK_STRUCTURE:
            case TK_UNION:
            case TK_BITSET:
            {
                // Need to be registered as TypeObject first
                // and return them as EK_MINIMAL or EK_COMPLETE
                logInfo(DYN_TYPE_FACTORY, "Complex types must be built from CompleteTypeObjects.");
            }
            break;
        }

        TypeObjectFactory::get_instance()->add_type_identifier(descriptor->get_name(), &identifier);
    }
}

void DynamicTypeBuilderFactory::build_type_object(
        const DynamicType_ptr type,
        TypeObject& object,
        bool complete,
        bool force) const
{
    const TypeDescriptor* descriptor = type->get_type_descriptor();

    std::map<MemberId, DynamicTypeMember*> membersMap;
    type->get_all_members(membersMap);
    std::vector<const MemberDescriptor*> members;
    for (auto it : membersMap)
    {
        members.push_back(it.second->get_descriptor());
    }

    build_type_object(descriptor, object, &members, complete, force);
}

void DynamicTypeBuilderFactory::build_type_object(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*>* members,
        bool complete,
        bool force) const
{
    const TypeObject* obj2 = (force)
        ? nullptr
        : TypeObjectFactory::get_instance()->get_type_object(descriptor->get_name(), complete);

    if (obj2 != nullptr)
    {
        object = *obj2;
    }
    else
    {
        switch (descriptor->kind_)
        {
            // Basic types
            case TK_NONE:
            case TK_BOOLEAN:
            case TK_BYTE:
            case TK_INT16:
            case TK_INT32:
            case TK_INT64:
            case TK_UINT16:
            case TK_UINT32:
            case TK_UINT64:
            case TK_FLOAT32:
            case TK_FLOAT64:
            case TK_FLOAT128:
            case TK_CHAR8:
            case TK_CHAR16:
            {
                break;
            }
            // String TKs
            case TK_STRING8:
            {
                build_string8_type_code(descriptor);
                break;
            }
            case TK_STRING16:
            {
                build_string16_type_code(descriptor);
                break;
            }
            // Collection TKs
            case TK_SEQUENCE:
            {
                build_sequence_type_code(descriptor, object, complete);
                break;
            }
            case TK_ARRAY:
            {
                build_array_type_code(descriptor, object, complete);
                break;
            }
            case TK_MAP:
            {
                build_map_type_code(descriptor, object, complete);
                break;
            }

            // Constructed/Named types
            case TK_ALIAS:
            {
                build_alias_type_code(descriptor, object, complete);
            }
            break;
            // Enumerated TKs
            case TK_ENUM:
            {
                build_enum_type_code(descriptor, object, *members, complete);
            }
            break;
            case TK_BITMASK:
            {
                build_bitmask_type_code(descriptor, object, *members, complete);
            }
            break;
            // Structured TKs
            case TK_ANNOTATION:
            {
                build_annotation_type_code(descriptor, object, *members, complete);
            }
            break;
            case TK_STRUCTURE:
            {
                build_struct_type_code(descriptor, object, *members, complete);
            }
            break;
            case TK_UNION:
            {
                build_union_type_code(descriptor, object, *members, complete);
            }
            break;
            case TK_BITSET:
            {
                build_bitset_type_code(descriptor, object, *members, complete);
            }
            break;
        }
    }
}

void DynamicTypeBuilderFactory::build_string8_type_code(
        const TypeDescriptor* descriptor) const
{
    const TypeIdentifier* identifier =
            TypeObjectFactory::get_instance()->get_string_identifier(
        descriptor->get_bounds(),
        false);

    TypeObjectFactory::get_instance()->add_type_identifier(descriptor->get_name(), identifier);
}

void DynamicTypeBuilderFactory::build_string16_type_code(
        const TypeDescriptor* descriptor) const
{
    const TypeIdentifier* identifier =
            TypeObjectFactory::get_instance()->get_string_identifier(
        descriptor->get_bounds(),
        true);

    TypeObjectFactory::get_instance()->add_type_identifier(descriptor->get_name(), identifier);
}

void DynamicTypeBuilderFactory::build_sequence_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_SEQUENCE);
        object.complete().sequence_type().collection_flag().IS_FINAL(false);
        object.complete().sequence_type().collection_flag().IS_APPENDABLE(false);
        object.complete().sequence_type().collection_flag().IS_MUTABLE(false);
        object.complete().sequence_type().collection_flag().IS_NESTED(false);
        object.complete().sequence_type().collection_flag().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().sequence_type().header().detail().ann_custom(), descriptor);

        object.complete().sequence_type().header().detail().type_name(descriptor->get_name());
        object.complete().sequence_type().header().common().bound(descriptor->get_bounds());
        object.complete().sequence_type().element().common().element_flags().TRY_CONSTRUCT1(false);
        object.complete().sequence_type().element().common().element_flags().TRY_CONSTRUCT2(false);
        object.complete().sequence_type().element().common().element_flags().IS_EXTERNAL(false);
        object.complete().sequence_type().element().common().element_flags().IS_OPTIONAL(false);
        object.complete().sequence_type().element().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.complete().sequence_type().element().common().element_flags().IS_KEY(false);
        object.complete().sequence_type().element().common().element_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_element_type(), obj, complete);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_element_type()->get_name());

        object.complete().sequence_type().element().common().type(ident);

        const TypeIdentifier* identifier =
                TypeObjectFactory::get_instance()->get_sequence_identifier(
            descriptor->get_element_type()->get_name(),
            descriptor->get_bounds(),
            true);

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_SEQUENCE);
        object.minimal().sequence_type().collection_flag().IS_FINAL(false);
        object.minimal().sequence_type().collection_flag().IS_APPENDABLE(false);
        object.minimal().sequence_type().collection_flag().IS_MUTABLE(false);
        object.minimal().sequence_type().collection_flag().IS_NESTED(false);
        object.minimal().sequence_type().collection_flag().IS_AUTOID_HASH(false);

        // Apply annotations
        object.minimal().sequence_type().header().common().bound(descriptor->get_bounds());
        object.minimal().sequence_type().element().common().element_flags().TRY_CONSTRUCT1(false);
        object.minimal().sequence_type().element().common().element_flags().TRY_CONSTRUCT2(false);
        object.minimal().sequence_type().element().common().element_flags().IS_EXTERNAL(false);
        object.minimal().sequence_type().element().common().element_flags().IS_OPTIONAL(false);
        object.minimal().sequence_type().element().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.minimal().sequence_type().element().common().element_flags().IS_KEY(false);
        object.minimal().sequence_type().element().common().element_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_element_type(), obj);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_element_type()->get_name());

        object.minimal().sequence_type().element().common().type(ident);

        const TypeIdentifier* identifier =
                TypeObjectFactory::get_instance()->get_sequence_identifier(
            descriptor->get_element_type()->get_name(),
            descriptor->get_bounds(),
            false);

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_array_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_ARRAY);
        object.complete().array_type().collection_flag().IS_FINAL(false);
        object.complete().array_type().collection_flag().IS_APPENDABLE(false);
        object.complete().array_type().collection_flag().IS_MUTABLE(false);
        object.complete().array_type().collection_flag().IS_NESTED(false);
        object.complete().array_type().collection_flag().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().array_type().header().detail().ann_custom(), descriptor);

        object.complete().array_type().header().detail().type_name(descriptor->get_name());
        for (uint32_t i = 0; i < descriptor->get_bounds_size(); ++i)
        {
            object.complete().array_type().header().common().bound_seq().push_back(descriptor->get_bounds(i));
        }
        object.complete().array_type().element().common().element_flags().TRY_CONSTRUCT1(false);
        object.complete().array_type().element().common().element_flags().TRY_CONSTRUCT2(false);
        object.complete().array_type().element().common().element_flags().IS_EXTERNAL(false);
        object.complete().array_type().element().common().element_flags().IS_OPTIONAL(false);
        object.complete().array_type().element().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.complete().array_type().element().common().element_flags().IS_KEY(false);
        object.complete().array_type().element().common().element_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_element_type(), obj, complete);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_element_type()->get_name());

        object.complete().array_type().element().common().type(ident);

        const TypeIdentifier* identifier =
                TypeObjectFactory::get_instance()->get_array_identifier(
            descriptor->get_element_type()->get_name(),
            object.complete().array_type().header().common().bound_seq(),
            true);

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_ARRAY);
        object.minimal().array_type().collection_flag().IS_FINAL(false);
        object.minimal().array_type().collection_flag().IS_APPENDABLE(false);
        object.minimal().array_type().collection_flag().IS_MUTABLE(false);
        object.minimal().array_type().collection_flag().IS_NESTED(false);
        object.minimal().array_type().collection_flag().IS_AUTOID_HASH(false);

        // Apply annotations
        for (uint32_t i = 0; i < descriptor->get_bounds_size(); ++i)
        {
            object.minimal().array_type().header().common().bound_seq().push_back(descriptor->get_bounds(i));
        }
        object.minimal().array_type().element().common().element_flags().TRY_CONSTRUCT1(false);
        object.minimal().array_type().element().common().element_flags().TRY_CONSTRUCT2(false);
        object.minimal().array_type().element().common().element_flags().IS_EXTERNAL(false);
        object.minimal().array_type().element().common().element_flags().IS_OPTIONAL(false);
        object.minimal().array_type().element().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.minimal().array_type().element().common().element_flags().IS_KEY(false);
        object.minimal().array_type().element().common().element_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_element_type(), obj);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_element_type()->get_name());

        object.minimal().array_type().element().common().type(ident);

        const TypeIdentifier* identifier =
                TypeObjectFactory::get_instance()->get_array_identifier(
            descriptor->get_element_type()->get_name(),
            object.minimal().array_type().header().common().bound_seq(),
            false);

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_map_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_MAP);
        object.complete().map_type().collection_flag().IS_FINAL(false);
        object.complete().map_type().collection_flag().IS_APPENDABLE(false);
        object.complete().map_type().collection_flag().IS_MUTABLE(false);
        object.complete().map_type().collection_flag().IS_NESTED(false);
        object.complete().map_type().collection_flag().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().map_type().header().detail().ann_custom(), descriptor);

        object.complete().map_type().header().detail().type_name(descriptor->get_name());
        object.complete().map_type().header().common().bound(descriptor->get_bounds());
        object.complete().map_type().element().common().element_flags().TRY_CONSTRUCT1(false);
        object.complete().map_type().element().common().element_flags().TRY_CONSTRUCT2(false);
        object.complete().map_type().element().common().element_flags().IS_EXTERNAL(false);
        object.complete().map_type().element().common().element_flags().IS_OPTIONAL(false);
        object.complete().map_type().element().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.complete().map_type().element().common().element_flags().IS_KEY(false);
        object.complete().map_type().element().common().element_flags().IS_DEFAULT(false);
        object.complete().map_type().key().common().element_flags().TRY_CONSTRUCT1(false);
        object.complete().map_type().key().common().element_flags().TRY_CONSTRUCT2(false);
        object.complete().map_type().key().common().element_flags().IS_EXTERNAL(false);
        object.complete().map_type().key().common().element_flags().IS_OPTIONAL(false);
        object.complete().map_type().key().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.complete().map_type().key().common().element_flags().IS_KEY(false);
        object.complete().map_type().key().common().element_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_element_type(), obj, complete);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_element_type()->get_name());

        build_type_object(descriptor->get_key_element_type(), obj, complete);
        TypeIdentifier ident_key = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_key_element_type()->get_name());

        object.complete().map_type().element().common().type(ident);
        object.complete().map_type().key().common().type(ident_key);

        const TypeIdentifier* identifier =
                TypeObjectFactory::get_instance()->get_map_identifier(
            descriptor->get_key_element_type()->get_name(),
            descriptor->get_element_type()->get_name(),
            descriptor->get_bounds(),
            true);

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_MAP);
        object.minimal().map_type().collection_flag().IS_FINAL(false);
        object.minimal().map_type().collection_flag().IS_APPENDABLE(false);
        object.minimal().map_type().collection_flag().IS_MUTABLE(false);
        object.minimal().map_type().collection_flag().IS_NESTED(false);
        object.minimal().map_type().collection_flag().IS_AUTOID_HASH(false);

        // Apply annotations
        object.minimal().map_type().header().common().bound(descriptor->get_bounds());
        object.minimal().map_type().element().common().element_flags().TRY_CONSTRUCT1(false);
        object.minimal().map_type().element().common().element_flags().TRY_CONSTRUCT2(false);
        object.minimal().map_type().element().common().element_flags().IS_EXTERNAL(false);
        object.minimal().map_type().element().common().element_flags().IS_OPTIONAL(false);
        object.minimal().map_type().element().common().element_flags().IS_MUST_UNDERSTAND(false);
        object.minimal().map_type().element().common().element_flags().IS_KEY(false);
        object.minimal().map_type().element().common().element_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_element_type(), obj);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_element_type()->get_name());

        build_type_object(descriptor->get_key_element_type(), obj, complete);
        TypeIdentifier ident_key = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_key_element_type()->get_name());

        object.minimal().map_type().element().common().type(ident);
        object.minimal().map_type().key().common().type(ident_key);

        const TypeIdentifier* identifier =
                TypeObjectFactory::get_instance()->get_map_identifier(
            descriptor->get_key_element_type()->get_name(),
            descriptor->get_element_type()->get_name(),
            descriptor->get_bounds(),
            false);

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_alias_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_ALIAS);
        object.complete().alias_type().alias_flags().IS_FINAL(false);
        object.complete().alias_type().alias_flags().IS_APPENDABLE(false);
        object.complete().alias_type().alias_flags().IS_MUTABLE(false);
        object.complete().alias_type().alias_flags().IS_NESTED(false);
        object.complete().alias_type().alias_flags().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().alias_type().header().detail().ann_custom(), descriptor);

        object.complete().alias_type().header().detail().type_name(descriptor->get_name());
        object.complete().alias_type().body().common().related_flags().TRY_CONSTRUCT1(false);
        object.complete().alias_type().body().common().related_flags().TRY_CONSTRUCT2(false);
        object.complete().alias_type().body().common().related_flags().IS_EXTERNAL(false);
        object.complete().alias_type().body().common().related_flags().IS_OPTIONAL(false);
        object.complete().alias_type().body().common().related_flags().IS_MUST_UNDERSTAND(false);
        object.complete().alias_type().body().common().related_flags().IS_KEY(false);
        object.complete().alias_type().body().common().related_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_base_type(), obj, complete);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_base_type()->get_name());

        object.complete().alias_type().body().common().related_type(ident);

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteAliasType::getCdrSerializedSize(object.complete().alias_type()) + 4));
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
        // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
        eprosima::fastcdr::Cdr ser(
            fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;

        object.serialize(ser);
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        // Add our alias
        TypeObjectFactory::get_instance()->add_alias(descriptor->get_name(), descriptor->get_base_type()->get_name());

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_ALIAS);
        object.minimal().alias_type().alias_flags().IS_FINAL(false);
        object.minimal().alias_type().alias_flags().IS_APPENDABLE(false);
        object.minimal().alias_type().alias_flags().IS_MUTABLE(false);
        object.minimal().alias_type().alias_flags().IS_NESTED(false);
        object.minimal().alias_type().alias_flags().IS_AUTOID_HASH(false);

        object.minimal().alias_type().body().common().related_flags().TRY_CONSTRUCT1(false);
        object.minimal().alias_type().body().common().related_flags().TRY_CONSTRUCT2(false);
        object.minimal().alias_type().body().common().related_flags().IS_EXTERNAL(false);
        object.minimal().alias_type().body().common().related_flags().IS_OPTIONAL(false);
        object.minimal().alias_type().body().common().related_flags().IS_MUST_UNDERSTAND(false);
        object.minimal().alias_type().body().common().related_flags().IS_KEY(false);
        object.minimal().alias_type().body().common().related_flags().IS_DEFAULT(false);

        //TypeIdentifier ident;
        //build_type_identifier(descriptor->get_base_type()->descriptor_, ident);
        TypeObject obj;
        build_type_object(descriptor->get_base_type()->descriptor_, obj);
        TypeIdentifier ident = *TypeObjectFactory::get_instance()->get_type_identifier(
            descriptor->get_base_type()->get_name());

        object.minimal().alias_type().body().common().related_type(ident);

        TypeIdentifier identifier;
        identifier._d(EK_MINIMAL);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalAliasType::getCdrSerializedSize(object.minimal().alias_type()) + 4));
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
        // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
        eprosima::fastcdr::Cdr ser(
            fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;

        object.serialize(ser);
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        // Add our alias
        TypeObjectFactory::get_instance()->add_alias(descriptor->get_name(), descriptor->get_base_type()->get_name());

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_enum_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*> members,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_ENUM);
        object.complete().enumerated_type().header().common().bit_bound(descriptor->annotation_get_bit_bound());
        object.complete().enumerated_type().header().detail().type_name(descriptor->get_name());

        // Apply annotations
        apply_type_annotations(object.complete().enumerated_type().header().detail().ann_custom(), descriptor);

        for (const MemberDescriptor* member : members)
        {
            CompleteEnumeratedLiteral mel;
            mel.common().flags().IS_DEFAULT(member->annotation_is_default_literal());
            mel.common().value(member->get_index());
            mel.detail().name(member->get_name());

            // Apply member annotations
            TypeDescriptor member_type_descriptor;
            member->type_->get_descriptor(&member_type_descriptor);
            apply_type_annotations(mel.detail().ann_custom(), &member_type_descriptor);

            object.complete().enumerated_type().literal_seq().emplace_back(mel);
        }

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteEnumeratedType::getCdrSerializedSize(object.complete().enumerated_type()) + 4));
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
        // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
        eprosima::fastcdr::Cdr ser(
            fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;

        object.serialize(ser);
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_ENUM);
        object.minimal().enumerated_type().header().common().bit_bound(32); // TODO fixed by IDL, isn't?

        for (const MemberDescriptor* member : members)
        {
            MinimalEnumeratedLiteral mel;
            mel.common().flags().IS_DEFAULT(member->annotation_is_default_literal());
            mel.common().value(member->get_index());
            MD5 hash(member->get_name());
            for (int i = 0; i < 4; ++i)
            {
                mel.detail().name_hash()[i] = hash.digest[i];
            }
            object.minimal().enumerated_type().literal_seq().emplace_back(mel);
        }

        TypeIdentifier identifier;
        identifier._d(EK_MINIMAL);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalEnumeratedType::getCdrSerializedSize(object.minimal().enumerated_type()) + 4));
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
        // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
        eprosima::fastcdr::Cdr ser(
            fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;

        object.serialize(ser);
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_struct_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*> members,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_STRUCTURE);

        object.complete().struct_type().struct_flags().IS_FINAL(descriptor->annotation_is_final());
        object.complete().struct_type().struct_flags().IS_APPENDABLE(descriptor->annotation_is_appendable());
        object.complete().struct_type().struct_flags().IS_MUTABLE(descriptor->annotation_is_mutable());
        object.complete().struct_type().struct_flags().IS_NESTED(descriptor->annotation_get_nested());
        object.complete().struct_type().struct_flags().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().struct_type().header().detail().ann_custom(), descriptor);

        for (const MemberDescriptor* member : members)
        {
            CompleteStructMember msm;
            msm.common().member_id(member->get_index());
            msm.common().member_flags().TRY_CONSTRUCT1(false);
            msm.common().member_flags().TRY_CONSTRUCT2(false);
            msm.common().member_flags().IS_EXTERNAL(false);
            msm.common().member_flags().IS_OPTIONAL(member->annotation_is_optional());
            msm.common().member_flags().IS_MUST_UNDERSTAND(member->annotation_is_must_understand());
            msm.common().member_flags().IS_KEY(member->annotation_is_key());
            msm.common().member_flags().IS_DEFAULT(false);

            // Apply member annotations
            TypeDescriptor member_type_descriptor;
            member->type_->get_descriptor(&member_type_descriptor);
            apply_type_annotations(msm.detail().ann_custom(), &member_type_descriptor);

            std::map<MemberId, DynamicTypeMember*> membersMap;
            member->type_->get_all_members(membersMap);
            std::vector<const MemberDescriptor*> innerMembers;
            for (auto it : membersMap)
            {
                innerMembers.push_back(it.second->get_descriptor());
            }

            TypeObject memObj;
            build_type_object(member->type_->descriptor_, memObj, &innerMembers);
            const TypeIdentifier* typeId =
                    TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(member->type_->get_name());
            if (typeId == nullptr)
            {
                logError(DYN_TYPES, "Member " << member->get_name() << " of struct "
                                              << descriptor->get_name() << " failed.");
            }
            else
            {
                TypeIdentifier memIdent = *typeId;
                msm.common().member_type_id(memIdent);
            }

            msm.detail().name(member->get_name());
            object.complete().struct_type().member_seq().emplace_back(msm);
        }

        object.complete().struct_type().header().detail().type_name(descriptor->get_name());
        //object.complete().struct_type().header().detail().ann_builtin()...
        //object.complete().struct_type().header().detail().ann_custom()...

        if (descriptor->get_base_type().get() != nullptr)
        {
            TypeIdentifier parent;
            build_type_identifier(descriptor->get_base_type(), parent);
            object.complete().struct_type().header().base_type(parent);
        }
        //object.complete().struct_type().header().base_type().equivalence_hash()[0..13];

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteStructType::getCdrSerializedSize(object.complete().struct_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (CompleteStructMember& st : object.complete().struct_type().member_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_STRUCTURE);

        object.minimal().struct_type().struct_flags().IS_FINAL(descriptor->annotation_is_final());
        object.minimal().struct_type().struct_flags().IS_APPENDABLE(descriptor->annotation_is_appendable());
        object.minimal().struct_type().struct_flags().IS_MUTABLE(descriptor->annotation_is_mutable());
        object.minimal().struct_type().struct_flags().IS_NESTED(descriptor->annotation_get_nested());
        object.minimal().struct_type().struct_flags().IS_AUTOID_HASH(false);

        for (const MemberDescriptor* member : members)
        {
            MinimalStructMember msm;
            msm.common().member_id(member->get_index());
            msm.common().member_flags().TRY_CONSTRUCT1(false);
            msm.common().member_flags().TRY_CONSTRUCT2(false);
            msm.common().member_flags().IS_EXTERNAL(false);
            msm.common().member_flags().IS_OPTIONAL(member->annotation_is_optional());
            msm.common().member_flags().IS_MUST_UNDERSTAND(member->annotation_is_must_understand());
            msm.common().member_flags().IS_KEY(member->annotation_is_key());
            msm.common().member_flags().IS_DEFAULT(false);
            //TypeIdentifier memIdent;
            //build_type_identifier(member->type_->descriptor_, memIdent);

            std::map<MemberId, DynamicTypeMember*> membersMap;
            member->type_->get_all_members(membersMap);
            std::vector<const MemberDescriptor*> innerMembers;
            for (auto it : membersMap)
            {
                innerMembers.push_back(it.second->get_descriptor());
            }

            TypeObject memObj;
            build_type_object(member->type_->descriptor_, memObj, &innerMembers, false);
            const TypeIdentifier* typeId =
                    TypeObjectFactory::get_instance()->get_type_identifier(member->type_->get_name());
            if (typeId == nullptr)
            {
                logError(DYN_TYPES, "Member " << member->get_name()
                                              << " of struct " << descriptor->get_name() << " failed.");
            }
            else
            {
                TypeIdentifier memIdent = *typeId;
                msm.common().member_type_id(memIdent);
            }

            MD5 hash(member->get_name());
            for (int i = 0; i < 4; ++i)
            {
                msm.detail().name_hash()[i] = hash.digest[i];
            }
            object.minimal().struct_type().member_seq().emplace_back(msm);
        }

        if (descriptor->get_base_type().get() != nullptr)
        {
            TypeIdentifier parent;
            build_type_identifier(descriptor->get_base_type(), parent, false);
            object.minimal().struct_type().header().base_type(parent);
        }

        TypeIdentifier identifier;
        identifier._d(EK_MINIMAL);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalStructType::getCdrSerializedSize(object.minimal().struct_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (MinimalStructMember& st : object.minimal().struct_type().member_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_union_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*> members,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_UNION);

        object.complete().union_type().union_flags().IS_FINAL(descriptor->annotation_is_final());
        object.complete().union_type().union_flags().IS_APPENDABLE(descriptor->annotation_is_appendable());
        object.complete().union_type().union_flags().IS_MUTABLE(descriptor->annotation_is_mutable());
        object.complete().union_type().union_flags().IS_NESTED(descriptor->annotation_get_nested());
        object.complete().union_type().union_flags().IS_AUTOID_HASH(false);

        object.complete().union_type().discriminator().common().member_flags().TRY_CONSTRUCT1(false);
        object.complete().union_type().discriminator().common().member_flags().TRY_CONSTRUCT2(false);
        object.complete().union_type().discriminator().common().member_flags().IS_EXTERNAL(false);
        object.complete().union_type().discriminator().common().member_flags().IS_OPTIONAL(false);
        object.complete().union_type().discriminator().common().member_flags().IS_MUST_UNDERSTAND(false);
        object.complete().union_type().discriminator().common().member_flags().IS_KEY(
            descriptor->discriminator_type_->descriptor_->annotation_get_key());
        object.complete().union_type().discriminator().common().member_flags().IS_DEFAULT(false);

        // Apply annotations
        apply_type_annotations(object.complete().union_type().header().detail().ann_custom(), descriptor);

        TypeObject discObj;
        build_type_object(descriptor->discriminator_type_->descriptor_, discObj);
        TypeIdentifier discIdent =
                *TypeObjectFactory::get_instance()->get_type_identifier(descriptor->discriminator_type_->get_name());
        object.complete().union_type().discriminator().common().type_id(discIdent);

        for (const MemberDescriptor* member : members)
        {
            CompleteUnionMember mum;
            mum.common().member_id(member->get_index());
            mum.common().member_flags().TRY_CONSTRUCT1(false);
            mum.common().member_flags().TRY_CONSTRUCT2(false);
            mum.common().member_flags().IS_EXTERNAL(false);
            mum.common().member_flags().IS_OPTIONAL(false);
            mum.common().member_flags().IS_MUST_UNDERSTAND(false);
            mum.common().member_flags().IS_KEY(false);
            mum.common().member_flags().IS_DEFAULT(member->is_default_union_value());

            // Apply member annotations
            TypeDescriptor member_type_descriptor;
            member->type_->get_descriptor(&member_type_descriptor);
            apply_type_annotations(mum.detail().ann_custom(), &member_type_descriptor);

            std::map<MemberId, DynamicTypeMember*> membersMap;
            member->type_->get_all_members(membersMap);
            std::vector<const MemberDescriptor*> innerMembers;
            for (auto it : membersMap)
            {
                innerMembers.push_back(it.second->get_descriptor());
            }

            TypeObject memObj;
            build_type_object(member->type_->descriptor_, memObj, &innerMembers);
            const TypeIdentifier* typeId =
                    TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(member->type_->get_name());
            if (typeId == nullptr)
            {
                logError(DYN_TYPES, "Member " << member->get_name()
                                              << " of union " << descriptor->get_name() << " failed.");
            }
            else
            {
                TypeIdentifier memIdent = *typeId;
                mum.common().type_id(memIdent);
            }

            for (uint64_t lab : member->get_union_labels())
            {
                mum.common().label_seq().emplace_back(static_cast<uint32_t>(lab));
            }
            mum.detail().name(member->get_name());
            object.complete().union_type().member_seq().emplace_back(mum);
        }

        object.complete().union_type().header().detail().type_name(descriptor->get_name());

        TypeIdentifier identifier;
        identifier._d(EK_MINIMAL);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteUnionType::getCdrSerializedSize(object.complete().union_type()) + 4));
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
        // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
        eprosima::fastcdr::Cdr ser(
            fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;

        object.serialize(ser);
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_MINIMAL);
        object.minimal()._d(TK_UNION);

        object.minimal().union_type().union_flags().IS_FINAL(descriptor->annotation_is_final());
        object.minimal().union_type().union_flags().IS_APPENDABLE(descriptor->annotation_is_appendable());
        object.minimal().union_type().union_flags().IS_MUTABLE(descriptor->annotation_is_mutable());
        object.minimal().union_type().union_flags().IS_NESTED(descriptor->annotation_get_nested());
        object.minimal().union_type().union_flags().IS_AUTOID_HASH(false);

        object.minimal().union_type().discriminator().common().member_flags().TRY_CONSTRUCT1(false);
        object.minimal().union_type().discriminator().common().member_flags().TRY_CONSTRUCT2(false);
        object.minimal().union_type().discriminator().common().member_flags().IS_EXTERNAL(false);
        object.minimal().union_type().discriminator().common().member_flags().IS_OPTIONAL(false);
        object.minimal().union_type().discriminator().common().member_flags().IS_MUST_UNDERSTAND(false);
        object.minimal().union_type().discriminator().common().member_flags().IS_KEY(
            descriptor->discriminator_type_->descriptor_->annotation_get_key());
        object.minimal().union_type().discriminator().common().member_flags().IS_DEFAULT(false);

        TypeObject discObj;
        build_type_object(descriptor->discriminator_type_->descriptor_, discObj);
        TypeIdentifier discIdent =
                *TypeObjectFactory::get_instance()->get_type_identifier(descriptor->discriminator_type_->get_name());
        object.minimal().union_type().discriminator().common().type_id(discIdent);
        //*TypeObjectFactory::get_instance()->get_type_identifier(descriptor->discriminator_type_->get_name()));

        for (const MemberDescriptor* member : members)
        {
            MinimalUnionMember mum;
            mum.common().member_id(member->get_index());
            mum.common().member_flags().TRY_CONSTRUCT1(false);
            mum.common().member_flags().TRY_CONSTRUCT2(false);
            mum.common().member_flags().IS_EXTERNAL(false);
            mum.common().member_flags().IS_OPTIONAL(false);
            mum.common().member_flags().IS_MUST_UNDERSTAND(false);
            mum.common().member_flags().IS_KEY(false);
            mum.common().member_flags().IS_DEFAULT(member->is_default_union_value());

            //TypeIdentifier memIdent;
            //build_type_identifier(member->type_->descriptor_, memIdent);

            std::map<MemberId, DynamicTypeMember*> membersMap;
            member->type_->get_all_members(membersMap);
            std::vector<const MemberDescriptor*> innerMembers;
            for (auto it : membersMap)
            {
                innerMembers.push_back(it.second->get_descriptor());
            }

            TypeObject memObj;
            build_type_object(member->type_->descriptor_, memObj, &innerMembers);
            const TypeIdentifier* typeId =
                    TypeObjectFactory::get_instance()->get_type_identifier(member->type_->get_name());
            if (typeId == nullptr)
            {
                logError(DYN_TYPES, "Member " << member->get_name()
                                              << " of union " << descriptor->get_name() << " failed.");
            }
            else
            {
                TypeIdentifier memIdent = *typeId;
                mum.common().type_id(memIdent);
            }

            for (uint64_t lab : member->get_union_labels())
            {
                mum.common().label_seq().emplace_back(static_cast<uint32_t>(lab));
            }
            MD5 hash(member->get_name());
            for (int i = 0; i < 4; ++i)
            {
                mum.detail().name_hash()[i] = hash.digest[i];
            }
            object.minimal().union_type().member_seq().emplace_back(mum);
        }

        TypeIdentifier identifier;
        identifier._d(EK_MINIMAL);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalUnionType::getCdrSerializedSize(object.minimal().union_type()) + 4));
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
        // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
        eprosima::fastcdr::Cdr ser(
            fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;

        object.serialize(ser);
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_bitset_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*> members,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_BITSET);

        object.complete().bitset_type().bitset_flags().IS_FINAL(false);
        object.complete().bitset_type().bitset_flags().IS_APPENDABLE(false);
        object.complete().bitset_type().bitset_flags().IS_MUTABLE(false);
        object.complete().bitset_type().bitset_flags().IS_NESTED(false);
        object.complete().bitset_type().bitset_flags().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().bitset_type().header().detail().ann_custom(), descriptor);

        for (const MemberDescriptor* member : members)
        {
            CompleteBitfield msm;
            msm.common().position(member->annotation_get_position()); // Position stored as annotation
            // Bitcount stored as bit_bound annotation
            msm.common().bitcount(static_cast<octet>(member->annotation_get_bit_bound()));
            msm.common().holder_type(member->type_->get_kind());
            msm.detail().name(member->get_name());

            // Apply member annotations
            TypeDescriptor member_type_descriptor;
            member->type_->get_descriptor(&member_type_descriptor);
            apply_type_annotations(msm.detail().ann_custom(), &member_type_descriptor);

            object.complete().bitset_type().field_seq().emplace_back(msm);
        }

        object.complete().bitset_type().header().detail().type_name(descriptor->get_name());
        //object.complete().bitset_type().header().detail().ann_builtin()...
        //object.complete().bitset_type().header().detail().ann_custom()...

        if (descriptor->get_base_type().get() != nullptr)
        {
            TypeIdentifier parent;
            build_type_identifier(descriptor->get_base_type(), parent);
            object.complete().bitset_type().header().base_type(parent);
        }
        //object.complete().bitset_type().header().base_type().equivalence_hash()[0..13];

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteBitsetType::getCdrSerializedSize(object.complete().bitset_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (CompleteBitfield& st : object.complete().bitset_type().field_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_COMPLETE);
        object.minimal()._d(TK_BITSET);

        object.minimal().bitset_type().bitset_flags().IS_FINAL(false);
        object.minimal().bitset_type().bitset_flags().IS_APPENDABLE(false);
        object.minimal().bitset_type().bitset_flags().IS_MUTABLE(false);
        object.minimal().bitset_type().bitset_flags().IS_NESTED(false);
        object.minimal().bitset_type().bitset_flags().IS_AUTOID_HASH(false);

        for (const MemberDescriptor* member : members)
        {
            MinimalBitfield msm;
            msm.common().position(member->annotation_get_position()); // Position stored as annotation
            // Bitcount stored as bit_bound annotation
            msm.common().bitcount(static_cast<octet>(member->annotation_get_bit_bound()));
            msm.common().holder_type(member->type_->get_kind());
            MD5 parent_bitfield_hash(member->get_name());
            for (int i = 0; i < 4; ++i)
            {
                msm.name_hash()[i] = parent_bitfield_hash.digest[i];
            }
            object.minimal().bitset_type().field_seq().emplace_back(msm);
        }

        //object.minimal().bitset_type().header().detail().ann_builtin()...
        //object.minimal().bitset_type().header().detail().ann_custom()...

        if (descriptor->get_base_type().get() != nullptr)
        {
            TypeIdentifier parent;
            build_type_identifier(descriptor->get_base_type(), parent);
            object.minimal().bitset_type().header().base_type(parent);
        }
        //object.minimal().bitset_type().header().base_type().equivalence_hash()[0..13];

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalBitsetType::getCdrSerializedSize(object.minimal().bitset_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (MinimalBitfield& st : object.minimal().bitset_type().field_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_bitmask_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*> members,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_BITMASK);

        object.complete().bitmask_type().bitmask_flags().IS_FINAL(false);
        object.complete().bitmask_type().bitmask_flags().IS_APPENDABLE(false);
        object.complete().bitmask_type().bitmask_flags().IS_MUTABLE(false);
        object.complete().bitmask_type().bitmask_flags().IS_NESTED(false);
        object.complete().bitmask_type().bitmask_flags().IS_AUTOID_HASH(false);

        // Apply annotations
        apply_type_annotations(object.complete().bitmask_type().header().detail().ann_custom(), descriptor);

        for (const MemberDescriptor* member : members)
        {
            CompleteBitflag msm;
            msm.common().position(member->annotation_get_position()); // Position stored as annotation
            msm.detail().name(member->get_name());

            // Apply member annotations
            TypeDescriptor member_type_descriptor;
            member->type_->get_descriptor(&member_type_descriptor);
            apply_type_annotations(msm.detail().ann_custom(), &member_type_descriptor);

            object.complete().bitmask_type().flag_seq().emplace_back(msm);
        }

        object.complete().bitmask_type().header().detail().type_name(descriptor->get_name());

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteBitmaskType::getCdrSerializedSize(object.complete().bitmask_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (CompleteBitflag& st : object.complete().bitmask_type().flag_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_COMPLETE);
        object.minimal()._d(TK_BITMASK);

        object.minimal().bitmask_type().bitmask_flags().IS_FINAL(false);
        object.minimal().bitmask_type().bitmask_flags().IS_APPENDABLE(false);
        object.minimal().bitmask_type().bitmask_flags().IS_MUTABLE(false);
        object.minimal().bitmask_type().bitmask_flags().IS_NESTED(false);
        object.minimal().bitmask_type().bitmask_flags().IS_AUTOID_HASH(false);

        for (const MemberDescriptor* member : members)
        {
            MinimalBitflag msm;
            msm.common().position(member->annotation_get_position()); // Position stored as annotation
            MD5 parent_bitfield_hash(member->get_name());
            for (int i = 0; i < 4; ++i)
            {
                msm.detail().name_hash()[i] = parent_bitfield_hash.digest[i];
            }
            object.minimal().bitmask_type().flag_seq().emplace_back(msm);
        }

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalBitmaskType::getCdrSerializedSize(object.minimal().bitmask_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (MinimalBitflag& st : object.minimal().bitmask_type().flag_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::build_annotation_type_code(
        const TypeDescriptor* descriptor,
        TypeObject& object,
        const std::vector<const MemberDescriptor*> members,
        bool complete) const
{
    if (complete)
    {
        object._d(EK_COMPLETE);
        object.complete()._d(TK_ANNOTATION);

        for (const MemberDescriptor* member : members)
        {
            CompleteAnnotationParameter msm;
            msm.name(member->get_name());

            if (!member->get_default_value().empty())
            {
                AnnotationParameterValue apv;
                set_annotation_default_value(apv, member);
                msm.default_value(apv);
            }

            TypeObject memObj;
            build_type_object(member->type_->descriptor_, memObj);
            const TypeIdentifier* typeId =
                    TypeObjectFactory::get_instance()->get_type_identifier(member->type_->get_name());
            if (typeId == nullptr)
            {
                logError(DYN_TYPES, "Member " << member->get_name()
                                              << " of annotation " << descriptor->get_name() << " failed.");
            }
            else
            {
                TypeIdentifier memIdent = *typeId;
                msm.common().member_type_id(memIdent);
            }

            object.complete().annotation_type().member_seq().emplace_back(msm);
        }

        object.complete().annotation_type().header().annotation_name(descriptor->get_name());

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    CompleteAnnotationType::getCdrSerializedSize(object.complete().annotation_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (CompleteAnnotationParameter& st : object.complete().annotation_type().member_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
    else
    {
        object._d(EK_COMPLETE);
        object.minimal()._d(TK_ANNOTATION);

        for (const MemberDescriptor* member : members)
        {
            MinimalAnnotationParameter msm;
            msm.name(member->get_name());

            if (!member->get_default_value().empty())
            {
                AnnotationParameterValue apv;
                set_annotation_default_value(apv, member);
                msm.default_value(apv);
            }

            TypeObject memObj;
            build_type_object(member->type_->descriptor_, memObj);
            const TypeIdentifier* typeId =
                    TypeObjectFactory::get_instance()->get_type_identifier(member->type_->get_name());
            if (typeId == nullptr)
            {
                logError(DYN_TYPES, "Member " << member->get_name()
                                              << " of annotation " << descriptor->get_name() << " failed.");
            }
            else
            {
                TypeIdentifier memIdent = *typeId;
                msm.common().member_type_id(memIdent);
            }

            object.minimal().annotation_type().member_seq().emplace_back(msm);
        }

        TypeIdentifier identifier;
        identifier._d(EK_COMPLETE);

        eprosima::fastrtps::rtps::SerializedPayload_t payload(static_cast<uint32_t>(
                    MinimalAnnotationType::getCdrSerializedSize(object.minimal().annotation_type()) + 4));
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);

        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
                eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
        payload.encapsulation = CDR_LE;
        // Serialize encapsulation

        for (MinimalAnnotationParameter& st : object.minimal().annotation_type().member_seq())
        {
            ser << st;
        }
        payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
        MD5 objectHash;
        objectHash.update((char*)payload.data, payload.length);
        objectHash.finalize();
        for (int i = 0; i < 14; ++i)
        {
            identifier.equivalence_hash()[i] = objectHash.digest[i];
        }

        TypeObjectFactory::get_instance()->add_type_object(descriptor->get_name(), &identifier, &object);
    }
}

void DynamicTypeBuilderFactory::set_annotation_default_value(
        AnnotationParameterValue& apv,
        const MemberDescriptor* member) const
{
    switch (member->get_kind())
    {
        case TK_BOOLEAN:
        {
            std::string value = member->get_default_value();
            std::transform(value.begin(), value.end(), value.begin(),
                    [](unsigned char c)
                    {
                        return static_cast<char>(std::tolower(c));
                    });
            apv.boolean_value(value.compare("0") != 0 || value.compare(CONST_TRUE) == 0);
        }
        break;
        case TK_BYTE:
        {
            apv.byte_value(static_cast<uint8_t>(std::stoul(member->get_default_value())));
        }
        break;
        case TK_INT16:
        {
            apv.int16_value(static_cast<int16_t>(std::stoi(member->get_default_value())));
        }
        break;
        case TK_INT32:
        {
            apv.int32_value(static_cast<int32_t>(std::stoi(member->get_default_value())));
        }
        break;
        case TK_INT64:
        {
            apv.int64_value(static_cast<int64_t>(std::stoll(member->get_default_value())));
        }
        break;
        case TK_UINT16:
        {
            apv.uint_16_value(static_cast<uint16_t>(std::stoul(member->get_default_value())));
        }
        break;
        case TK_UINT32:
        {
            apv.uint32_value(static_cast<uint32_t>(std::stoul(member->get_default_value())));
        }
        break;
        case TK_UINT64:
        {
            apv.uint64_value(static_cast<uint64_t>(std::stoull(member->get_default_value())));
        }
        break;
        case TK_FLOAT32:
        {
            apv.float32_value(std::stof(member->get_default_value()));
        }
        break;
        case TK_FLOAT64:
        {
            apv.float64_value(std::stod(member->get_default_value()));
        }
        break;
        case TK_FLOAT128:
        {
            apv.float128_value(std::stold(member->get_default_value()));
        }
        break;
        case TK_CHAR8:
        {
            apv.char_value(member->get_default_value().c_str()[0]);
        }
        break;
        case TK_CHAR16:
        {
            apv.wchar_value(wstring_from_bytes(member->get_default_value()).c_str()[0]);
        }
        break;
        case TK_STRING8:
        {
            apv.string8_value(member->get_default_value());
        }
        break;
        case TK_STRING16:
        {
            apv.string16_value(wstring_from_bytes(member->get_default_value()));
        }
        break;
        case TK_ENUM:
        {
            // TODO Translate from enum value name to integer value
            apv.enumerated_value(static_cast<int32_t>(std::stoul(member->get_default_value())));
        }
        break;
        default:
            break;
    }
}

DynamicType_ptr DynamicTypeBuilderFactory::create_alias_type(
        DynamicTypeBuilder* base_type,
        const std::string& sName)
{
    if (base_type != nullptr)
    {
        DynamicType_ptr pType = create_type(base_type);
        if (pType != nullptr)
        {
            return create_alias_type(pType, sName);
        }
        else
        {
            logError(DYN_TYPES, "Error creating alias type, Error creating dynamic type");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error creating alias type, base_type must be valid");
    }
    return DynamicType_ptr(nullptr);
}

DynamicType_ptr DynamicTypeBuilderFactory::create_alias_type(
        DynamicType_ptr base_type,
        const std::string& sName)
{
    if (base_type != nullptr)
    {
        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_ALIAS;
        pDescriptor.base_type_ = base_type;
        if (sName.length() > 0)
        {
            pDescriptor.name_ = sName;
        }
        else
        {
            pDescriptor.name_ = base_type->get_name();
        }

        return create_type(&pDescriptor, sName);
    }
    else
    {
        logError(DYN_TYPES, "Error creating alias type, base_type must be valid");
    }
    return DynamicType_ptr(nullptr);
}

DynamicType_ptr DynamicTypeBuilderFactory::create_int32_type()
{
    TypeDescriptor pInt32Descriptor(GenerateTypeName(get_type_name(TK_INT32)), TK_INT32);
    return DynamicType_ptr(new DynamicType(&pInt32Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_uint32_type()
{
    TypeDescriptor pUint32Descriptor(GenerateTypeName(get_type_name(TK_UINT32)), TK_UINT32);
    return DynamicType_ptr(new DynamicType(&pUint32Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_int16_type()
{
    TypeDescriptor pInt16Descriptor(GenerateTypeName(get_type_name(TK_INT16)), TK_INT16);
    return DynamicType_ptr(new DynamicType(&pInt16Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_uint16_type()
{
    TypeDescriptor pUint16Descriptor(GenerateTypeName(get_type_name(TK_UINT16)), TK_UINT16);
    return DynamicType_ptr(new DynamicType(&pUint16Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_int64_type()
{
    TypeDescriptor pInt64Descriptor(GenerateTypeName(get_type_name(TK_INT64)), TK_INT64);
    return DynamicType_ptr(new DynamicType(&pInt64Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_uint64_type()
{
    TypeDescriptor pUint64Descriptor(GenerateTypeName(get_type_name(TK_UINT64)), TK_UINT64);
    return DynamicType_ptr(new DynamicType(&pUint64Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_float32_type()
{
    TypeDescriptor pFloat32Descriptor(GenerateTypeName(get_type_name(TK_FLOAT32)), TK_FLOAT32);
    return DynamicType_ptr(new DynamicType(&pFloat32Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_float64_type()
{
    TypeDescriptor pFloat64Descriptor(GenerateTypeName(get_type_name(TK_FLOAT64)), TK_FLOAT64);
    return DynamicType_ptr(new DynamicType(&pFloat64Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_float128_type()
{
    TypeDescriptor pFloat128Descriptor(GenerateTypeName(get_type_name(TK_FLOAT128)), TK_FLOAT128);
    return DynamicType_ptr(new DynamicType(&pFloat128Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_char8_type()
{
    TypeDescriptor pChar8Descriptor(GenerateTypeName(get_type_name(TK_CHAR8)), TK_CHAR8);
    return DynamicType_ptr(new DynamicType(&pChar8Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_char16_type()
{
    TypeDescriptor pChar16Descriptor(GenerateTypeName(get_type_name(TK_CHAR16)), TK_CHAR16);
    return DynamicType_ptr(new DynamicType(&pChar16Descriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_bool_type()
{
    TypeDescriptor pBoolDescriptor(GenerateTypeName(get_type_name(TK_BOOLEAN)), TK_BOOLEAN);
    return DynamicType_ptr(new DynamicType(&pBoolDescriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_byte_type()
{
    TypeDescriptor pByteDescriptor(GenerateTypeName(get_type_name(TK_BYTE)), TK_BYTE);
    return DynamicType_ptr(new DynamicType(&pByteDescriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_string_type(
        uint32_t bound /*= MAX_STRING_LENGTH*/)
{
    if (bound == BOUND_UNLIMITED)
    {
        bound = MAX_STRING_LENGTH;
    }
    TypeDescriptor pStringDescriptor("", TK_STRING8);
    pStringDescriptor.name_ = TypeNamesGenerator::get_string_type_name(bound, false, true);
    pStringDescriptor.element_type_ = create_char8_type();
    pStringDescriptor.bound_.push_back(bound);

    return DynamicType_ptr(new DynamicType(&pStringDescriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_wstring_type(
        uint32_t bound /*= MAX_STRING_LENGTH*/)
{
    if (bound == BOUND_UNLIMITED)
    {
        bound = MAX_STRING_LENGTH;
    }

    TypeDescriptor pStringDescriptor("", TK_STRING16);
    pStringDescriptor.name_ = TypeNamesGenerator::get_string_type_name(bound, true, true);
    pStringDescriptor.element_type_ = create_char16_type();
    pStringDescriptor.bound_.push_back(bound);

    return DynamicType_ptr(new DynamicType(&pStringDescriptor));
}

DynamicType_ptr DynamicTypeBuilderFactory::create_bitset_type(
        uint32_t bound)
{
    if (bound <= MAX_BITMASK_LENGTH)
    {
        TypeDescriptor pDescriptor;
        pDescriptor.kind_ = TK_BITSET;
        pDescriptor.name_ = GenerateTypeName(get_type_name(TK_BITSET));
        pDescriptor.bound_.push_back(bound);
        return create_type(&pDescriptor, pDescriptor.name_);
    }
    else
    {
        logError(DYN_TYPES, "Error creating bitmask, length exceeds the maximum value '" << MAX_BITMASK_LENGTH << "'");
    }
    return DynamicType_ptr(nullptr);
}

void DynamicTypeBuilderFactory::apply_type_annotations(
        AppliedAnnotationSeq& annotations,
        const TypeDescriptor* descriptor) const
{
    for (const AnnotationDescriptor* annotation : descriptor->annotation_)
    {
        AppliedAnnotation ann;
        ann.annotation_typeid(
            *TypeObjectFactory::get_instance()->get_type_identifier_trying_complete(annotation->type_->get_name()));
        std::map<std::string, std::string> values;
        annotation->get_all_value(values);
        for (auto it : values)
        {
            AppliedAnnotationParameter ann_param;
            MD5 message_hash(it.first);
            for (int i = 0; i < 4; ++i)
            {
                ann_param.paramname_hash()[i] = message_hash.digest[i];
            }
            AnnotationParameterValue param_value;
            param_value._d(annotation->type_->get_kind());
            param_value.from_string(it.second);
            ann_param.value(param_value);
            ann.param_seq().push_back(ann_param);
        }
        annotations.push_back(ann);
    }
}

} // namespace types
} // namespace fastrtps
} // namespace eprosima
