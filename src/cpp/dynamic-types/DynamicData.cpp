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
#include <fastrtps/types/DynamicData.h>
#include <fastrtps/types/MemberDescriptor.h>
#include <fastrtps/types/DynamicType.h>
#include <fastrtps/types/DynamicPubSubType.h>
#include <fastrtps/types/DynamicTypeBuilderFactory.h>
#include <fastrtps/types/DynamicTypeMember.h>
#include <fastrtps/types/TypeDescriptor.h>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/DynamicDataPtr.h>
#include <fastdds/dds/log/Log.hpp>
#include <fastcdr/Cdr.h>

#include <dds/core/LengthUnlimited.hpp>

#include <locale>
#include <codecvt>

namespace eprosima {
namespace fastrtps {
namespace types {

template <typename Map>
bool map_compare(
        Map const& left,
        Map const& right)
{
    auto pred = [](decltype(*left.begin()) a, decltype(a) b)
            {
                return a.first == b.first && a.second == b.second;
            };

    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), pred);
}

template <>
bool map_compare(
        const std::map<MemberId, DynamicData*>& left,
        const std::map<MemberId, DynamicData*>& right)
{
    auto pred = [](decltype(*left.begin()) a, decltype(a) b)
            {
                return a.first == b.first && a.second->equals(b.second);
            };

    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), pred);
}

DynamicData::DynamicData()
    : type_(nullptr)
#ifdef DYNAMIC_TYPES_CHECKING
    , int32_value_(0)
    , uint32_value_(0)
    , int16_value_(0)
    , uint16_value_(0)
    , int64_value_(0)
    , uint64_value_(0)
    , float32_value_(0.0f)
    , float64_value_(0.0)
    , float128_value_(0.0)
    , char8_value_(0)
    , char16_value_(0)
    , byte_value_(0)
    , bool_value_(false)
#endif // ifdef DYNAMIC_TYPES_CHECKING
    , key_element_(false)
    , default_array_value_(nullptr)
    , union_label_(UINT64_MAX)
    , union_id_(MEMBER_ID_INVALID)
    , union_discriminator_(nullptr)
{
}

DynamicData::DynamicData(
        DynamicType_ptr pType)
    : type_(pType)
#ifdef DYNAMIC_TYPES_CHECKING
    , int32_value_(0)
    , uint32_value_(0)
    , int16_value_(0)
    , uint16_value_(0)
    , int64_value_(0)
    , uint64_value_(0)
    , float32_value_(0.0f)
    , float64_value_(0.0)
    , float128_value_(0.0)
    , char8_value_(0)
    , char16_value_(0)
    , byte_value_(0)
    , bool_value_(false)
#endif // ifdef DYNAMIC_TYPES_CHECKING
    , key_element_(false)
    , default_array_value_(nullptr)
    , union_label_(UINT64_MAX)
    , union_id_(MEMBER_ID_INVALID)
    , union_discriminator_(nullptr)
{
    create_members(type_);
}

DynamicData::DynamicData(
        const DynamicData* pData)
    : type_(pData->type_)
#ifdef DYNAMIC_TYPES_CHECKING
    , int32_value_(pData->int32_value_)
    , uint32_value_(pData->uint32_value_)
    , int16_value_(pData->int16_value_)
    , uint16_value_(pData->uint16_value_)
    , int64_value_(pData->int64_value_)
    , uint64_value_(pData->uint64_value_)
    , float32_value_(pData->float32_value_)
    , float64_value_(pData->float64_value_)
    , float128_value_(pData->float128_value_)
    , char8_value_(pData->char8_value_)
    , char16_value_(pData->char16_value_)
    , byte_value_(pData->byte_value_)
    , bool_value_(pData->bool_value_)
    , string_value_(pData->string_value_)
    , wstring_value_(pData->wstring_value_)
#endif // ifdef DYNAMIC_TYPES_CHECKING
    , key_element_(pData->key_element_)
    , default_array_value_(pData->default_array_value_)
    , union_label_(pData->union_label_)
    , union_id_(pData->union_id_)
    , union_discriminator_(pData->union_discriminator_)
{
    create_members(pData);
}

DynamicData::~DynamicData()
{
    clean();
}

void DynamicData::create_members(
        const DynamicData* pData)
{
    for (auto it = pData->descriptors_.begin(); it != pData->descriptors_.end(); ++it)
    {
        descriptors_.insert(std::make_pair(it->first, new MemberDescriptor(it->second)));
    }

#ifdef DYNAMIC_TYPES_CHECKING
    for (auto it = pData->complex_values_.begin(); it != pData->complex_values_.end(); ++it)
    {
        complex_values_.insert(std::make_pair(it->first, DynamicDataFactory::get_instance()->create_copy(it->second)));
    }
#else
    if (type_->is_complex_kind())
    {
        for (auto it = pData->values_.begin(); it != pData->values_.end(); ++it)
        {
            values_.insert(std::make_pair(it->first,
                    DynamicDataFactory::get_instance()->create_copy((DynamicData*)it->second)));
        }
    }
    else if (pData->descriptors_.size() > 0)
    {
        for (auto it = pData->descriptors_.begin(); it != pData->descriptors_.end(); ++it)
        {
            values_.insert(std::make_pair(it->first, pData->clone_value(it->first, it->second->get_kind())));
        }
    }
    else
    {
        values_.insert(std::make_pair(MEMBER_ID_INVALID, pData->clone_value(MEMBER_ID_INVALID, pData->get_kind())));
    }
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

void DynamicData::create_members(
        DynamicType_ptr pType)
{
    std::map<MemberId, DynamicTypeMember*> members;
    if (pType->get_all_members(members) == ReturnCode_t::RETCODE_OK)
    {
        if (pType->is_complex_kind())
        {
            // Bitmasks and enums register their members but only manages one value.
            if (pType->get_kind() == TK_BITMASK || pType->get_kind() == TK_ENUM)
            {
                add_value(pType->get_kind(), MEMBER_ID_INVALID);
            }

            for (auto it = members.begin(); it != members.end(); ++it)
            {
                MemberDescriptor* newDescriptor = new MemberDescriptor();
                if (it->second->get_descriptor(newDescriptor) == ReturnCode_t::RETCODE_OK)
                {
                    descriptors_.insert(std::make_pair(it->first, newDescriptor));
                    if (pType->get_kind() != TK_BITMASK && pType->get_kind() != TK_ENUM)
                    {
                        DynamicData* data = DynamicDataFactory::get_instance()->create_data(newDescriptor->type_);
                        if (newDescriptor->type_->get_kind() != TK_BITSET &&
                                newDescriptor->type_->get_kind() != TK_STRUCTURE &&
                                newDescriptor->type_->get_kind() != TK_UNION &&
                                newDescriptor->type_->get_kind() != TK_SEQUENCE &&
                                newDescriptor->type_->get_kind() != TK_ARRAY &&
                                newDescriptor->type_->get_kind() != TK_MAP)
                        {
                            std::string def_value = newDescriptor->annotation_get_default();
                            if (!def_value.empty())
                            {
                                data->set_value(def_value);
                            }
                        }
#ifdef DYNAMIC_TYPES_CHECKING
                        complex_values_.insert(std::make_pair(it->first, data));
#else
                        values_.insert(std::make_pair(it->first, data));
#endif // ifdef DYNAMIC_TYPES_CHECKING
                    }
                }
                else
                {
                    delete newDescriptor;
                }
            }

            // Set the default value for unions.
            if (pType->get_kind() == TK_UNION)
            {
                bool defaultValue = false;
                // Search the default value.
                for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
                {
                    if (it->second->is_default_union_value())
                    {
                        set_union_id(it->first);
                        defaultValue = true;
                        break;
                    }
                }

                // If there isn't a default value... set the first element of the union
                if (!defaultValue && descriptors_.size() > 0)
                {
                    set_union_id(descriptors_.begin()->first);
                }
            }
        }
        else
        {
            add_value(pType->get_kind(), MEMBER_ID_INVALID);
        }
    }
}

ReturnCode_t DynamicData::get_descriptor(
        MemberDescriptor& value,
        MemberId id)
{
    auto it = descriptors_.find(id);
    if (it != descriptors_.end())
    {
        value.copy_from(it->second);
        return ReturnCode_t::RETCODE_OK;
    }
    else
    {
        logWarning(DYN_TYPES, "Error getting MemberDescriptor. MemberId not found.");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::set_descriptor(
        MemberId id,
        const MemberDescriptor* value)
{
    if (descriptors_.find(id) == descriptors_.end())
    {
        descriptors_.insert(std::make_pair(id, new MemberDescriptor(value)));
        return ReturnCode_t::RETCODE_OK;
    }
    else
    {
        logWarning(DYN_TYPES, "Error setting MemberDescriptor. MemberId found.");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

bool DynamicData::equals(
        const DynamicData* other) const
{
    if (other != nullptr)
    {
        if (other == this)
        {
            return true;
        }
        else if (get_item_count() == other->get_item_count() && type_->equals(other->type_.get()) &&
                descriptors_.size() == other->descriptors_.size())
        {
            for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
            {
                auto otherDescIt = other->descriptors_.find(it->first);
                if (otherDescIt == other->descriptors_.end() || !it->second->equals(otherDescIt->second))
                {
                    return false;
                }
            }

            // Optimization for unions, only check the selected element.
            if (get_kind() == TK_UNION)
            {
                if (union_id_ != other->union_id_)
                {
                    return false;
                }
                else if (union_id_ != MEMBER_ID_INVALID)
                {
#ifdef DYNAMIC_TYPES_CHECKING
                    auto it = complex_values_.find(union_id_);
                    auto otherIt = other->complex_values_.find(union_id_);
#else
                    auto it = values_.find(union_id_);
                    auto otherIt = other->values_.find(union_id_);
#endif // ifdef DYNAMIC_TYPES_CHECKING
                    if (!((DynamicData*)it->second)->equals((DynamicData*)otherIt->second))
                    {
                        return false;
                    }
                }
            }
            else
            {
#ifdef DYNAMIC_TYPES_CHECKING
                bool bFail = false;
                bFail = int32_value_ != other->int32_value_;
                bFail = bFail || uint32_value_ != other->uint32_value_;
                bFail = bFail || int16_value_ != other->int16_value_;
                bFail = bFail || uint16_value_ != other->uint16_value_;
                bFail = bFail || int64_value_ != other->int64_value_;
                bFail = bFail || uint64_value_ != other->uint64_value_;
                bFail = bFail || float32_value_ != other->float32_value_;
                bFail = bFail || float64_value_ != other->float64_value_;
                bFail = bFail || float128_value_ != other->float128_value_;
                bFail = bFail || char8_value_ != other->char8_value_;
                bFail = bFail || char16_value_ != other->char16_value_;
                bFail = bFail || byte_value_ != other->byte_value_;
                bFail = bFail || bool_value_ != other->bool_value_;
                bFail = bFail || string_value_ != other->string_value_;
                bFail = bFail || wstring_value_ != other->wstring_value_;
                bFail = bFail || !map_compare(complex_values_, other->complex_values_);
                /*if (int32_value_ != other->int32_value_ || uint32_value_ != other->uint32_value_ ||
                    int16_value_ != other->int16_value_ || uint16_value_ != other->uint16_value_ ||
                    int64_value_ != other->int64_value_ || uint64_value_ != other->uint64_value_ ||
                    float32_value_ != other->float32_value_ || float64_value_ != other->float64_value_ ||
                    float128_value_ != other->float128_value_ || char8_value_ != other->char8_value_ ||
                    char16_value_ != other->char16_value_ || byte_value_ != other->byte_value_ ||
                    bool_value_ != other->bool_value_ || string_value_ != other->string_value_ ||
                    wstring_value_ != other->wstring_value_ ||
                    !map_compare(complex_values_, other->complex_values_))
                 */
                if (bFail)
                {
                    return false;
                }
#else
                if (get_kind() == TK_ENUM)
                {
                    if (!compare_values(TK_UINT32, values_.begin()->second, other->values_.begin()->second))
                    {
                        return false;
                    }
                }
                else if (get_kind() == TK_BITMASK)
                {
                    TypeKind bitmask_kind = TK_BYTE;
                    size_t type_size = type_->get_size();
                    switch (type_size)
                    {
                        case 1: bitmask_kind = TK_BYTE; break;
                        case 2: bitmask_kind = TK_UINT16; break;
                        case 4: bitmask_kind = TK_UINT32; break;
                        case 8: bitmask_kind = TK_UINT64; break;
                    }
                    if (!compare_values(bitmask_kind, values_.begin()->second, other->values_.begin()->second))
                    {
                        return false;
                    }
                }
                else if (type_->is_complex_kind())
                {
                    for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
                    {
                        auto currentIt = values_.find(it->first);
                        auto otherIt = other->values_.find(it->first);
                        if (!((DynamicData*)currentIt->second)->equals(((DynamicData*)otherIt->second)))
                        {
                            return false;
                        }
                    }
                }
                else if (descriptors_.size() > 0)
                {
                    for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
                    {
                        auto currentIt = values_.find(it->first);
                        auto otherIt = other->values_.find(it->first);
                        if (!compare_values(it->second->get_kind(), currentIt->second, otherIt->second))
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    if (!compare_values(get_kind(), values_.begin()->second, other->values_.begin()->second))
                    {
                        return false;
                    }
                }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            }
            return true;
        }
    }
    return false;
}

MemberId DynamicData::get_member_id_by_name(
        const std::string& name) const
{
    for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
    {
        if (it->second->get_name() == name)
        {
            return it->first;
        }
    }
    return MEMBER_ID_INVALID;
}

MemberId DynamicData::get_member_id_at_index(
        uint32_t index) const
{
    for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
    {
        if (it->second->get_index() == index)
        {
            return it->first;
        }
    }
    return MEMBER_ID_INVALID;
}

TypeKind DynamicData::get_kind() const
{
    return type_->get_kind();
}

uint32_t DynamicData::get_item_count() const
{
    if (get_kind() == TK_MAP)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        return static_cast<uint32_t>(complex_values_.size() / 2);
#else
        return static_cast<uint32_t>(values_.size() / 2);
#endif // ifdef DYNAMIC_TYPES_CHECKING
    }
    else if (get_kind() == TK_ARRAY)
    {
        return type_->get_total_bounds();
    }
    else
    {
#ifdef DYNAMIC_TYPES_CHECKING
        return static_cast<uint32_t>(complex_values_.size());
#else
        return static_cast<uint32_t>(values_.size());
#endif // ifdef DYNAMIC_TYPES_CHECKING
    }
}

std::string DynamicData::get_name()
{
    return type_->get_name();
}

void DynamicData::add_value(
        TypeKind kind,
        MemberId id)
{
    switch (kind)
    {
        default:
            break;
        case TK_INT32:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new int32_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_UINT32:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new uint32_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_INT16:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new int16_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_UINT16:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new uint16_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_INT64:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new int64_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_UINT64:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new uint64_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_FLOAT32:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new float()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_FLOAT64:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new double()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_FLOAT128:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new long double()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_CHAR8:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new char()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_CHAR16:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new wchar_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_BOOLEAN:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new bool()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_BYTE:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new octet()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_STRING8:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new std::string()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_STRING16:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new std::wstring()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_ENUM:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new uint32_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_BITMASK:
        {
#ifndef DYNAMIC_TYPES_CHECKING
            values_.insert(std::make_pair(id, new uint64_t()));
#endif // ifndef DYNAMIC_TYPES_CHECKING
        }
    }
    set_default_value(id);
}

void DynamicData::clean()
{
    if (default_array_value_ != nullptr)
    {
        DynamicDataFactory::get_instance()->delete_data(default_array_value_);
        default_array_value_ = nullptr;
    }

    if (union_discriminator_ != nullptr)
    {
        DynamicDataFactory::get_instance()->delete_data(union_discriminator_);
        union_discriminator_ = nullptr;
    }

    clean_members();

    type_ = nullptr;

    for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
    {
        delete it->second;
    }
    descriptors_.clear();
}

ReturnCode_t DynamicData::clear_all_values()
{
    if (type_->is_complex_kind())
    {
        if (get_kind() == TK_SEQUENCE || get_kind() == TK_MAP || get_kind() == TK_ARRAY)
        {
            return clear_data();
        }
        else
        {
            for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
            {
#ifdef DYNAMIC_TYPES_CHECKING
                auto itValue = complex_values_.find(it->first);
                if (itValue != complex_values_.end())
                {
                    itValue->second->clear_all_values();
                }
#else
                auto itValue = values_.find(it->first);
                if (itValue != values_.end())
                {
                    ((DynamicData*)itValue->second)->clear_all_values();
                }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            }
        }
    }
    else
    {
        set_default_value(MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_OK;
}

void DynamicData::clean_members()
{
#ifdef DYNAMIC_TYPES_CHECKING
    for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
    {
        DynamicDataFactory::get_instance()->delete_data(it->second);
    }
    complex_values_.clear();
#else
    if (type_->has_children())
    {
        for (auto it = values_.begin(); it != values_.end(); ++it)
        {
            DynamicDataFactory::get_instance()->delete_data((DynamicData*)it->second);
        }
    }
    else
    {
        switch (get_kind())
        {
            default:
                break;
            case TK_INT32:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((int32_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_UINT32:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((uint32_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_INT16:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((int16_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_UINT16:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((uint16_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_INT64:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((int64_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_UINT64:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((uint64_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_FLOAT32:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((float*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_FLOAT64:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((double*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_FLOAT128:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((long double*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_CHAR8:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((char*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_CHAR16:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((wchar_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_BOOLEAN:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((bool*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_BYTE:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((octet*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_STRING8:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((std::string*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_STRING16:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((std::wstring*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_ENUM:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((uint32_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_BITMASK:
            {
#ifndef DYNAMIC_TYPES_CHECKING
                auto it = values_.begin();
                delete ((uint64_t*)it->second);
#endif // ifndef DYNAMIC_TYPES_CHECKING
                break;
            }
            case TK_UNION:
            case TK_STRUCTURE:
            case TK_ARRAY:
            case TK_SEQUENCE:
            case TK_MAP:
            case TK_ALIAS:
            case TK_BITSET:
            {
                break;
            }
        }
    }
    values_.clear();
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::clear_nonkey_values()
{
    if (type_->is_complex_kind())
    {
        for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto itValue = complex_values_.find(it->first);
            if (itValue != complex_values_.end())
            {
                itValue->second->clear_nonkey_values();
            }
#else
            auto itValue = values_.find(it->first);
            if (itValue != values_.end())
            {
                ((DynamicData*)itValue->second)->clear_nonkey_values();
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
    }
    else
    {
        if (!key_element_)
        {
            set_default_value(MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DynamicData::clear_value(
        MemberId id)
{
    auto it = descriptors_.find(id);
    if (it != descriptors_.end())
    {
        if (type_->is_complex_kind())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto itValue = complex_values_.find(it->first);
            if (itValue != complex_values_.end())
            {
                itValue->second->clear_all_values();
            }
#else
            auto itValue = values_.find(it->first);
            if (itValue != values_.end())
            {
                ((DynamicData*)itValue->second)->clear_all_values();
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            set_default_value(id);
        }
    }
    else
    {
        set_default_value(id);
    }
    return ReturnCode_t::RETCODE_OK;
}

void* DynamicData::clone_value(
        MemberId id,
        TypeKind kind) const
{
    switch (kind)
    {
        default:
            break;
        case TK_INT32:
        {
            int32_t* newInt32 = new int32_t();
            get_int32_value(*newInt32, id);
            return newInt32;
        }
        break;
        case TK_UINT32:
        {
            uint32_t* newUInt32 = new uint32_t();
            get_uint32_value(*newUInt32, id);
            return newUInt32;
        }
        break;
        case TK_INT16:
        {
            int16_t* newInt16 = new int16_t();
            get_int16_value(*newInt16, id);
            return newInt16;
        }
        break;
        case TK_UINT16:
        {
            uint16_t* newUInt16 = new uint16_t();
            get_uint16_value(*newUInt16, id);
            return newUInt16;
        }
        break;
        case TK_INT64:
        {
            int64_t* newInt64 = new int64_t();
            get_int64_value(*newInt64, id);
            return newInt64;
        }
        break;
        case TK_UINT64:
        {
            uint64_t* newUInt64 = new uint64_t();
            get_uint64_value(*newUInt64, id);
            return newUInt64;
        }
        break;
        case TK_FLOAT32:
        {
            float* newFloat32 = new float();
            get_float32_value(*newFloat32, id);
            return newFloat32;
        }
        break;
        case TK_FLOAT64:
        {
            double* newFloat64 = new double();
            get_float64_value(*newFloat64, id);
            return newFloat64;
        }
        break;
        case TK_FLOAT128:
        {
            long double* newFloat128 = new long double();
            get_float128_value(*newFloat128, id);
            return newFloat128;
        }
        break;
        case TK_CHAR8:
        {
            char* newChar8 = new char();
            get_char8_value(*newChar8, id);
            return newChar8;
        }
        break;
        case TK_CHAR16:
        {
            wchar_t* newChar16 = new wchar_t();
            get_char16_value(*newChar16, id);
            return newChar16;
        }
        break;
        case TK_BOOLEAN:
        {
            bool* newBool = new bool();
            get_bool_value(*newBool, id);
            return newBool;
        }
        break;
        case TK_BYTE:
        {
            octet* newByte = new octet();
            get_byte_value(*newByte, id);
            return newByte;
        }
        break;
        case TK_STRING8:
        {
            std::string* newString = new std::string();
            get_string_value(*newString, id);
            return newString;
        }
        break;
        case TK_STRING16:
        {
            std::wstring* newString = new std::wstring();
            get_wstring_value(*newString, id);
            return newString;
        }
        break;
        case TK_ENUM:
        {
            uint32_t* newUInt32 = new uint32_t();
            get_enum_value(*newUInt32, id);
            return newUInt32;
        }
        break;
        case TK_BITMASK:
        {
            uint64_t* newBitset = new uint64_t();
            get_uint64_value(*newBitset, id);
            return newBitset;
        }
    }
    return nullptr;
}

bool DynamicData::compare_values(
        TypeKind kind,
        void* left,
        void* right) const
{
    switch (kind)
    {
        default:
            break;
        case TK_INT32:      {
            return *((int32_t*)left) == *((int32_t*)right);
        }
        case TK_UINT32:     {
            return *((uint32_t*)left) == *((uint32_t*)right);
        }
        case TK_INT16:      {
            return *((int16_t*)left) == *((int16_t*)right);
        }
        case TK_UINT16:     {
            return *((uint16_t*)left) == *((uint16_t*)right);
        }
        case TK_INT64:      {
            return *((int64_t*)left) == *((int64_t*)right);
        }
        case TK_UINT64:     {
            return *((uint64_t*)left) == *((uint64_t*)right);
        }
        case TK_FLOAT32:    {
            return *((float*)left) == *((float*)right);
        }
        case TK_FLOAT64:    {
            return *((double*)left) == *((double*)right);
        }
        case TK_FLOAT128:   {
            return *((long double*)left) == *((long double*)right);
        }
        case TK_CHAR8:      {
            return *((char*)left) == *((char*)right);
        }
        case TK_CHAR16:     {
            return *((wchar_t*)left) == *((wchar_t*)right);
        }
        case TK_BOOLEAN:    {
            return *((bool*)left) == *((bool*)right);
        }
        case TK_BYTE:       {
            return *((octet*)left) == *((octet*)right);
        }
        case TK_STRING8:    {
            return *((std::string*)left) == *((std::string*)right);
        }
        case TK_STRING16:   {
            return *((std::wstring*)left) == *((std::wstring*)right);
        }
        case TK_ENUM:       {
            return *((uint32_t*)left) == *((uint32_t*)right);
        }
    }
    return false;
}

void DynamicData::get_value(
        std::string& sOutValue,
        MemberId id /*= MEMBER_ID_INVALID*/) const
{
    switch (type_->kind_)
    {
        default:
            break;
        case TK_INT32:
        {
            int32_t value(0);
            get_int32_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_UINT32:
        {
            uint32_t value(0);
            get_uint32_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_INT16:
        {
            int16_t value(0);
            get_int16_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_UINT16:
        {
            uint16_t value(0);
            get_uint16_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_INT64:
        {
            int64_t value(0);
            get_int64_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_UINT64:
        {
            uint64_t value(0);
            get_uint64_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_FLOAT32:
        {
            float value(0.0f);
            get_float32_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_FLOAT64:
        {
            double value(0.0f);
            get_float64_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_FLOAT128:
        {
            long double value(0.0f);
            get_float128_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_CHAR8:
        {
            char value = 0;
            get_char8_value(value, id);
            sOutValue = value;
        }
        break;
        case TK_CHAR16:
        {
            wchar_t value(0);
            get_char16_value(value, id);
            std::wstring temp = L"";
            temp += value;
            sOutValue = wstring_to_bytes(temp);
        }
        break;
        case TK_BOOLEAN:
        {
            bool value(false);
            get_bool_value(value, id);
            sOutValue = std::to_string(value ? 1 : 0);
        }
        break;
        case TK_BYTE:
        {
            uint8_t value(0);
            get_byte_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_STRING8:
        {
            sOutValue = get_string_value(id);
        }
        break;
        case TK_STRING16:
        {
            std::wstring value;
            get_wstring_value(value, id);
            sOutValue = wstring_to_bytes(value);
        }
        break;
        case TK_ENUM:
        {
            uint32_t value;
            get_enum_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_BITMASK:
        {
            uint64_t value(0);
            get_uint64_value(value, id);
            sOutValue = std::to_string(value);
        }
        break;
        case TK_ARRAY:
        case TK_SEQUENCE:
        case TK_BITSET:
        case TK_MAP:
        {
            // THESE TYPES DON'T MANAGE VALUES
        }
        break;
    }
}

void DynamicData::set_value(
        const std::string& sValue,
        MemberId id /*= MEMBER_ID_INVALID*/)
{
    switch (type_->kind_)
    {
        default:
            break;
        case TK_INT32:
        {
            int32_t value(0);
            try
            {
                value = stoi(sValue);
            }
            catch (...)
            {
            }
            set_int32_value(value, id);
        }
        break;
        case TK_UINT32:
        {
            uint32_t value(0);
            try
            {
                value = stoul(sValue);
            }
            catch (...)
            {
            }
            set_uint32_value(value, id);
        }
        break;
        case TK_INT16:
        {
            int16_t value(0);
            try
            {
                value = static_cast<int16_t>(stoi(sValue));
            }
            catch (...)
            {
            }
            set_int16_value(value, id);
        }
        break;
        case TK_UINT16:
        {
            uint16_t value(0);
            try
            {
                value = static_cast<uint16_t>(stoul(sValue));
            }
            catch (...)
            {
            }
            set_uint16_value(value, id);
        }
        break;
        case TK_INT64:
        {
            int64_t value(0);
            try
            {
                value = stoll(sValue);
            }
            catch (...)
            {
            }
            set_int64_value(value, id);
        }
        break;
        case TK_UINT64:
        {
            uint64_t value(0);
            try
            {
                value = stoul(sValue);
            }
            catch (...)
            {
            }
            set_uint64_value(value, id);
        }
        break;
        case TK_FLOAT32:
        {
            float value(0.0f);
            try
            {
                value = stof(sValue);
            }
            catch (...)
            {
            }
            set_float32_value(value, id);
        }
        break;
        case TK_FLOAT64:
        {
            double value(0.0f);
            try
            {
                value = stod(sValue);
            }
            catch (...)
            {
            }
            set_float64_value(value, id);
        }
        break;
        case TK_FLOAT128:
        {
            long double value(0.0f);
            try
            {
                value = stold(sValue);
            }
            catch (...)
            {
            }
            set_float128_value(value, id);
        }
        break;
        case TK_CHAR8:
        {
            if (sValue.length() >= 1)
            {
                set_char8_value(sValue[0], id);
            }
        }
        break;
        case TK_CHAR16:
        {
            wchar_t value(0);
            try
            {
                std::wstring temp = std::wstring(sValue.begin(), sValue.end());
                value = temp[0];
            }
            catch (...)
            {
            }

            set_char16_value(value, id);
        }
        break;
        case TK_BOOLEAN:
        {
            int value(0);
            try
            {
                value = stoi(sValue);
            }
            catch (...)
            {
            }
            set_bool_value(value == 1 ? true : false, id);
        }
        break;
        case TK_BYTE:
        {
            if (sValue.length() >= 1)
            {
                uint8_t value(0);
                try
                {
                    value = static_cast<uint8_t>(stoul(sValue));
                }
                catch (...)
                {
                }
                set_byte_value(value, id);
            }
        }
        break;
        case TK_STRING8:
        {
            set_string_value(sValue, id);
        }
        break;
        case TK_STRING16:
        {
            set_wstring_value(std::wstring(sValue.begin(), sValue.end()), id);
        }
        break;
        case TK_ENUM:
        {
            uint32_t value(0);
            try
            {
                value = stoul(sValue);
            }
            catch (...)
            {
            }
            set_enum_value(value, id);
        }
        break;
        case TK_BITMASK:
        {
            uint64_t value(0);
            try
            {
                value = stoul(sValue);
            }
            catch (...)
            {
            }
            set_uint64_value(value, id);
        }
        break;
        case TK_ARRAY:
        case TK_SEQUENCE:
        case TK_BITSET:
        case TK_MAP:
        {
            // THESE TYPES DON'T MANAGE VALUES
        }
        break;
    }
}

void DynamicData::set_default_value(
        MemberId id)
{
    std::string defaultValue = "";
    auto it = descriptors_.find(id);
    if (it != descriptors_.end())
    {
        defaultValue = it->second->get_default_value();
    }

    switch (type_->kind_)
    {
        default:
            break;
        case TK_INT32:
        {
            int32_t value(0);
            try
            {
                value = stoi(defaultValue);
            }
            catch (...)
            {
            }
            set_int32_value(value, id);
        }
        break;
        case TK_UINT32:
        {
            uint32_t value(0);
            try
            {
                value = stoul(defaultValue);
            }
            catch (...)
            {
            }
            set_uint32_value(value, id);
        }
        break;
        case TK_INT16:
        {
            int16_t value(0);
            try
            {
                value = static_cast<int16_t>(stoi(defaultValue));
            }
            catch (...)
            {
            }
            set_int16_value(value, id);
        }
        break;
        case TK_UINT16:
        {
            uint16_t value(0);
            try
            {
                value = static_cast<uint16_t>(stoul(defaultValue));
            }
            catch (...)
            {
            }
            set_uint16_value(value, id);
        }
        break;
        case TK_INT64:
        {
            int64_t value(0);
            try
            {
                value = stoll(defaultValue);
            }
            catch (...)
            {
            }
            set_int64_value(value, id);
        }
        break;
        case TK_UINT64:
        {
            uint64_t value(0);
            try
            {
                value = stoul(defaultValue);
            }
            catch (...)
            {
            }
            set_uint64_value(value, id);
        }
        break;
        case TK_FLOAT32:
        {
            float value(0.0f);
            try
            {
                value = stof(defaultValue);
            }
            catch (...)
            {
            }
            set_float32_value(value, id);
        }
        break;
        case TK_FLOAT64:
        {
            double value(0.0f);
            try
            {
                value = stod(defaultValue);
            }
            catch (...)
            {
            }
            set_float64_value(value, id);
        }
        break;
        case TK_FLOAT128:
        {
            long double value(0.0f);
            try
            {
                value = stold(defaultValue);
            }
            catch (...)
            {
            }
            set_float128_value(value, id);
        }
        break;
        case TK_CHAR8:
        {
            if (defaultValue.length() >= 1)
            {
                set_char8_value(defaultValue[0], id);
            }
        }
        break;
        case TK_CHAR16:
        {
            wchar_t value(0);
            try
            {
                std::wstring temp = std::wstring(defaultValue.begin(), defaultValue.end());
                value = temp[0];
            }
            catch (...)
            {
            }

            set_char16_value(value, id);
        }
        break;
        case TK_BOOLEAN:
        {
            int value(0);
            try
            {
                value = stoi(defaultValue);
            }
            catch (...)
            {
            }
            set_bool_value(value == 1 ? true : false, id);
        }
        break;
        case TK_BYTE:
        {
            if (defaultValue.length() >= 1)
            {
                uint8_t value(0);
                try
                {
                    value = static_cast<uint8_t>(stoul(defaultValue));
                }
                catch (...)
                {
                }
                set_byte_value(value, id);
            }
        }
        break;
        case TK_STRING8:
        {
            set_string_value(defaultValue, id);
        }
        break;
        case TK_STRING16:
        {
            set_wstring_value(std::wstring(defaultValue.begin(), defaultValue.end()), id);
        }
        break;
        case TK_ENUM:
        {
            uint32_t value(0);
            try
            {
                value = stoul(defaultValue);
            }
            catch (...)
            {
            }
            set_enum_value(value, id);
        }
        break;
        case TK_BITMASK:
        {
            uint64_t value(0);
            try
            {
                value = stoul(defaultValue);
            }
            catch (...)
            {
            }
            set_uint64_value(value, id);
        }
        break;
        case TK_ARRAY:
        case TK_SEQUENCE:
        case TK_BITSET:
        case TK_MAP:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto itValue = complex_values_.find(id);
            if (itValue != complex_values_.end())
            {
                if (!itValue->second->key_element_)
                {
                    itValue->second->set_default_value(MEMBER_ID_INVALID);
                }
            }
#else
            auto itValue = values_.find(id);
            if (itValue != values_.end())
            {
                if (!((DynamicData*)itValue->second)->key_element_)
                {
                    ((DynamicData*)itValue->second)->set_default_value(MEMBER_ID_INVALID);
                }
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        break;
    }
}

DynamicData* DynamicData::loan_value(
        MemberId id)
{
    if (id != MEMBER_ID_INVALID)
    {
        if (std::find(loaned_values_.begin(), loaned_values_.end(), id) == loaned_values_.end())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto it = complex_values_.find(id);
            if (it != complex_values_.end())
            {
                if (get_kind() == TK_MAP && it->second->key_element_)
                {
                    logError(DYN_TYPES, "Error loaning Value. Key values can't be loaned.");
                    return nullptr;
                }
                else
                {
                    if (get_kind() == TK_UNION && union_id_ != id)
                    {
                        set_union_id(id);
                    }
                    loaned_values_.push_back(id);
                    return it->second;
                }
            }
            else if (get_kind() == TK_ARRAY)
            {
                if (insert_array_data(id) == ReturnCode_t::RETCODE_OK)
                {
                    loaned_values_.push_back(id);
                    return complex_values_.at(id);
                }
            }
#else
            auto it = values_.find(id);
            if (it != values_.end())
            {
                if (get_kind() == TK_MAP && ((DynamicData*)it->second)->key_element_)
                {
                    logError(DYN_TYPES, "Error loaning Value. Key values can't be loaned.");
                    return nullptr;
                }
                else
                {
                    if (get_kind() == TK_UNION && union_id_ != id)
                    {
                        set_union_id(id);
                    }
                    loaned_values_.push_back(id);
                    return (DynamicData*)it->second;
                }
            }
            else if (get_kind() == TK_ARRAY)
            {
                if (insert_array_data(id) == ReturnCode_t::RETCODE_OK)
                {
                    loaned_values_.push_back(id);
                    return (DynamicData*)values_.at(id);
                }
            }

#endif // ifdef DYNAMIC_TYPES_CHECKING
            else
            {
                logError(DYN_TYPES, "Error loaning Value. MemberId not found.");
            }
        }
        else
        {
            logError(DYN_TYPES, "Error loaning Value. The value has been loaned previously.");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error loaning Value. Invalid MemberId.");
    }
    return nullptr;
}

ReturnCode_t DynamicData::return_loaned_value(
        const DynamicData* value)
{
    for (auto loanIt = loaned_values_.begin(); loanIt != loaned_values_.end(); ++loanIt)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        auto it = complex_values_.find(*loanIt);
        if (it != complex_values_.end() && it->second == value)
        {
            loaned_values_.erase(loanIt);
            return ReturnCode_t::RETCODE_OK;
        }
#else
        auto it = values_.find(*loanIt);
        if (it != values_.end() && it->second == value)
        {
            loaned_values_.erase(loanIt);
            return ReturnCode_t::RETCODE_OK;
        }
#endif // ifdef DYNAMIC_TYPES_CHECKING
    }

    logError(DYN_TYPES, "Error returning loaned Value. The value hasn't been loaned.");
    return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
}

ReturnCode_t DynamicData::get_int32_value(
        int32_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_INT32 && id == MEMBER_ID_INVALID)
    {
        value = int32_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_int32_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_int32_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_INT32 && id == MEMBER_ID_INVALID)
        {
            value = *((int32_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_int32_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_int32_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_int32_value(
        int32_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_INT32 && id == MEMBER_ID_INVALID)
    {
        int32_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_get_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                int32_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_int32_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_int32_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_INT32 && id == MEMBER_ID_INVALID)
        {
            *((int32_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                int32_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_int32_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_int32_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_uint32_value(
        uint32_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_UINT32 && id == MEMBER_ID_INVALID)
    {
        value = uint32_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_uint32_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_uint32_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_UINT32 && id == MEMBER_ID_INVALID)
        {
            value = *((uint32_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_uint32_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_uint32_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_uint32_value(
        uint32_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_UINT32 && id == MEMBER_ID_INVALID)
    {
        uint32_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_is_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                uint32_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_uint32_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_uint32_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_UINT32 && id == MEMBER_ID_INVALID)
        {
            *((uint32_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                uint32_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_uint32_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_uint32_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_int16_value(
        int16_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_INT16 && id == MEMBER_ID_INVALID)
    {
        value = int16_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_int16_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_int16_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_INT16 && id == MEMBER_ID_INVALID)
        {
            value = *((int16_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_int16_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_int16_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_int16_value(
        int16_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_INT16 && id == MEMBER_ID_INVALID)
    {
        int16_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_is_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                int16_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_int16_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_int16_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_INT16 && id == MEMBER_ID_INVALID)
        {
            *((int16_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                int16_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_int16_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_int16_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_uint16_value(
        uint16_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_UINT16 && id == MEMBER_ID_INVALID)
    {
        value = uint16_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_uint16_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_uint16_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_UINT16 && id == MEMBER_ID_INVALID)
        {
            value = *((uint16_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_uint16_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_uint16_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_uint16_value(
        uint16_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_UINT16 && id == MEMBER_ID_INVALID)
    {
        uint16_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_is_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                uint16_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_uint16_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_uint16_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_UINT16 && id == MEMBER_ID_INVALID)
        {
            *((uint16_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                uint16_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_uint16_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_uint16_value(value, id);
        }
        return insertResult;
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_int64_value(
        int64_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_INT64 && id == MEMBER_ID_INVALID)
    {
        value = int64_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_int64_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_int64_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_INT64 && id == MEMBER_ID_INVALID)
        {
            value = *((int64_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_int64_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_int64_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_int64_value(
        int64_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_INT64 && id == MEMBER_ID_INVALID)
    {
        int64_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_is_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                int64_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_int64_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_int64_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_INT64 && id == MEMBER_ID_INVALID)
        {
            *((int64_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                int64_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_int64_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_int64_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_uint64_value(
        uint64_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if ((get_kind() == TK_UINT64 || get_kind() == TK_BITMASK) && id == MEMBER_ID_INVALID)
    {
        value = uint64_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_uint64_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_uint64_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if ((get_kind() == TK_UINT64 || get_kind() == TK_BITMASK) && id == MEMBER_ID_INVALID)
        {
            value = *((uint64_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_uint64_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_uint64_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_uint64_value(
        uint64_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if ((get_kind() == TK_UINT64 || get_kind() == TK_BITMASK) && id == MEMBER_ID_INVALID)
    {
        uint64_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_is_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                uint64_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_uint64_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_uint64_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if ((get_kind() == TK_UINT64 || get_kind() == TK_BITMASK) && id == MEMBER_ID_INVALID)
        {
            *((uint64_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                uint64_t mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_uint64_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_uint64_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_float32_value(
        float& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_FLOAT32 && id == MEMBER_ID_INVALID)
    {
        value = float32_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_float32_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_float32_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_FLOAT32 && id == MEMBER_ID_INVALID)
        {
            value = *((float*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_float32_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_float32_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_float32_value(
        float value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_FLOAT32 && id == MEMBER_ID_INVALID)
    {
        float32_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_float32_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_float32_value(value, id);
            }
            return insertResult;
        }
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_FLOAT32 && id == MEMBER_ID_INVALID)
        {
            *((float*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_float32_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_float32_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_float64_value(
        double& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_FLOAT64 && id == MEMBER_ID_INVALID)
    {
        value = float64_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_float64_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_float64_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_FLOAT64 && id == MEMBER_ID_INVALID)
        {
            value = *((double*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_float64_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_float64_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_float64_value(
        double value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_FLOAT64 && id == MEMBER_ID_INVALID)
    {
        float64_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_float64_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_float64_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_FLOAT64 && id == MEMBER_ID_INVALID)
        {
            *((double*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_float64_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_float64_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_float128_value(
        long double& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_FLOAT128 && id == MEMBER_ID_INVALID)
    {
        value = float128_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_float128_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_float128_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_FLOAT128 && id == MEMBER_ID_INVALID)
        {
            value = *((long double*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_float128_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_float128_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_float128_value(
        long double value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_FLOAT128 && id == MEMBER_ID_INVALID)
    {
        float128_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_float128_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_float128_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_FLOAT128 && id == MEMBER_ID_INVALID)
        {
            *((long double*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_float128_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_float128_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_char8_value(
        char& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_CHAR8 && id == MEMBER_ID_INVALID)
    {
        value = char8_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_char8_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_char8_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_CHAR8 && id == MEMBER_ID_INVALID)
        {
            value = *((char*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_char8_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_char8_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_char8_value(
        char value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_CHAR8 && id == MEMBER_ID_INVALID)
    {
        char8_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_char8_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_char8_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_CHAR8 && id == MEMBER_ID_INVALID)
        {
            *((char*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_char8_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_char8_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_char16_value(
        wchar_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_CHAR16 && id == MEMBER_ID_INVALID)
    {
        value = char16_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_char16_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_char16_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_CHAR16 && id == MEMBER_ID_INVALID)
        {
            value = *((wchar_t*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_char16_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_char16_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_char16_value(
        wchar_t value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_CHAR16 && id == MEMBER_ID_INVALID)
    {
        char16_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_char16_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_char16_value(value, id);
            }
            return insertResult;
        }
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_CHAR16 && id == MEMBER_ID_INVALID)
        {
            *((wchar_t*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_char16_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_char16_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_byte_value(
        octet& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_BYTE && id == MEMBER_ID_INVALID)
    {
        value = byte_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_byte_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_byte_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_BYTE && id == MEMBER_ID_INVALID)
        {
            value = *((octet*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_byte_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_byte_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_byte_value(
        octet value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_BYTE && id == MEMBER_ID_INVALID)
    {
        byte_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicData* data = it->second;
            if (get_kind() == TK_BITSET && data->type_->get_descriptor()->annotation_is_bit_bound())
            {
                uint16_t bit_bound = data->type_->get_descriptor()->annotation_get_bit_bound();
                octet mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = it->second->set_byte_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_byte_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_BYTE && id == MEMBER_ID_INVALID)
        {
            *((octet*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            auto itDescriptor = descriptors_.find(id);
            if (get_kind() == TK_BITSET)
            {
                if (itDescriptor == descriptors_.end())
                {
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                uint16_t bit_bound = ((MemberDescriptor*)itDescriptor->second)->annotation_get_bit_bound();
                octet mask = 0x00;
                for (uint16_t i = 0; i < bit_bound; ++i)
                {
                    mask = mask << 1;
                    mask += 1;
                }
                value &= mask;
            }
            ReturnCode_t result = ((DynamicData*)it->second)->set_byte_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_byte_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_bool_value(
        bool& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_BOOLEAN && id == MEMBER_ID_INVALID)
    {
        value = bool_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (get_kind() == TK_BITMASK && id < type_->get_bounds())
    {
        value = (uint64_value_ & ((uint64_t)1 << id)) != 0;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_bool_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_bool_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.end();
    if (get_kind() == TK_BITMASK)
    {
        it = values_.find(MEMBER_ID_INVALID);
    }
    else
    {
        it = values_.find(id);
    }
    if (it != values_.end())
    {
        if (get_kind() == TK_BOOLEAN && id == MEMBER_ID_INVALID)
        {
            value = *((bool*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (get_kind() == TK_BITMASK && id < type_->get_bounds())
        {
            auto m_id = descriptors_.find(id);
            MemberDescriptor* member = m_id->second;
            uint16_t position = member->annotation_get_position();
            value = (*((uint64_t*)it->second) & ((uint64_t)1 << position)) != 0;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_bool_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_bool_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_bool_value(
        bool value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_BOOLEAN && id == MEMBER_ID_INVALID)
    {
        bool_value_ = value;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (get_kind() == TK_BITMASK)
    {
        if (id == MEMBER_ID_INVALID)
        {
            if (value)
            {
                uint64_value_ = ~((uint64_t)0);
            }
            else
            {
                uint64_value_ = 0;
            }
        }
        else if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || id < type_->get_bounds())
        {
            if (value)
            {
                uint64_value_ |= ((uint64_t)1 << id);
            }
            else
            {
                uint64_value_ &= ~((uint64_t)1 << id);
            }
            return ReturnCode_t::RETCODE_OK;
        }
        else
        {
            logError(DYN_TYPES, "Error setting bool value. The given index is greater than the limit.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_bool_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_bool_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.end();
    if (get_kind() == TK_BITMASK)
    {
        it = values_.find(MEMBER_ID_INVALID);
    }
    else
    {
        it = values_.find(id);
    }

    if (it != values_.end())
    {
        if (get_kind() == TK_BOOLEAN && id == MEMBER_ID_INVALID)
        {
            *((bool*)it->second) = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else if (get_kind() == TK_BITMASK)
        {
            if (id == MEMBER_ID_INVALID)
            {
                if (value)
                {
                    *((uint64_t*)it->second) = ~((uint64_t)0);
                }
                else
                {
                    *((uint64_t*)it->second) = 0;
                }
                return ReturnCode_t::RETCODE_OK;
            }
            else if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || id < type_->get_bounds())
            {
                auto m_id = descriptors_.find(id);
                MemberDescriptor* member = m_id->second;
                uint16_t position = member->annotation_get_position();
                if (value)
                {
                    *((uint64_t*)it->second) |= ((uint64_t)1 << position);
                }
                else
                {
                    *((uint64_t*)it->second) &= ~((uint64_t)1 << position);
                }
                return ReturnCode_t::RETCODE_OK;
            }
            else
            {
                logError(DYN_TYPES, "Error setting bool value. The given index is greater than the limit.");
                return ReturnCode_t::RETCODE_BAD_PARAMETER;
            }
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_bool_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_bool_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_string_value(
        std::string& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_STRING8 && id == MEMBER_ID_INVALID)
    {
        value = string_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_string_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_string_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_STRING8 && id == MEMBER_ID_INVALID)
        {
            value = *((std::string*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_string_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_string_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_string_value(
        const std::string& value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_STRING8 && id == MEMBER_ID_INVALID)
    {
        if (value.length() <= type_->get_bounds())
        {
            string_value_ = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else
        {
            logError(DYN_TYPES, "Error setting string value. The given string is greater than the length limit.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_string_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_string_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_STRING8 && id == MEMBER_ID_INVALID)
        {
            if (value.length() <= type_->get_bounds())
            {
                *((std::string*)it->second) = value;
                return ReturnCode_t::RETCODE_OK;
            }
            else
            {
                logError(DYN_TYPES, "Error setting string value. The given string is greater than the length limit.");
                return ReturnCode_t::RETCODE_BAD_PARAMETER;
            }
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_string_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_string_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

void DynamicData::set_type_name(
        const std::string& name)
{
    if (type_ != nullptr)
    {
        type_->set_name(name);
    }
}

void DynamicData::update_union_discriminator()
{
    if (get_kind() == TK_UNION)
    {
        uint64_t sUnionValue;
        union_discriminator_->get_discriminator_value(sUnionValue);
        for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
        {
            std::vector<uint64_t> unionLabels = it->second->get_union_labels();
            for (uint64_t label : unionLabels)
            {
                if (sUnionValue == label)
                {
                    union_id_ = it->first;
                    union_label_ = label;
                    break;
                }
            }
        }
    }
    else
    {
        logError(DYN_TYPES, "Error updating union id. The kind: " << get_kind() << " doesn't support it.");
    }
}

void DynamicData::set_union_discriminator(
        DynamicData* pData)
{
    union_discriminator_ = pData;
    if (union_discriminator_ != nullptr)
    {
        union_discriminator_->set_discriminator_value(union_label_);
    }
}

MemberId DynamicData::get_union_id() const
{
    return union_id_;
}

ReturnCode_t DynamicData::set_union_id(
        MemberId id)
{
    if (get_kind() == TK_UNION)
    {
        auto it = descriptors_.find(id);
        if (id == MEMBER_ID_INVALID || it != descriptors_.end())
        {
            union_id_ = id;
            if (it != descriptors_.end())
            {
                std::vector<uint64_t> unionLabels = it->second->get_union_labels();
                if (unionLabels.size() > 0)
                {
                    union_label_ = unionLabels[0];
                    if (union_discriminator_ != nullptr)
                    {
                        union_discriminator_->set_discriminator_value(union_label_);
                    }
                }
            }
            return ReturnCode_t::RETCODE_OK;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error setting union id. The kind: " << get_kind() << " doesn't support it.");
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_wstring_value(
        std::wstring& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_STRING16 && id == MEMBER_ID_INVALID)
    {
        value = wstring_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_wstring_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_wstring_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_STRING16 && id == MEMBER_ID_INVALID)
        {
            value = *((std::wstring*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)it->second)->get_wstring_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_wstring_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_wstring_value(
        const std::wstring& value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_STRING16 && id == MEMBER_ID_INVALID)
    {
        if (value.length() <= type_->get_bounds())
        {
            wstring_value_ = value;
            return ReturnCode_t::RETCODE_OK;
        }
        else
        {
            logError(DYN_TYPES, "Error setting wstring value. The given string is greater than the length limit.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_wstring_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_wstring_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto it = values_.find(id);
    if (it != values_.end())
    {
        if (get_kind() == TK_STRING16 && id == MEMBER_ID_INVALID)
        {
            if (value.length() <= type_->get_bounds())
            {
                *((std::wstring*)it->second) = value;
                return ReturnCode_t::RETCODE_OK;
            }
            else
            {
                logError(DYN_TYPES, "Error setting wstring value. The given string is greater than the length limit.");
                return ReturnCode_t::RETCODE_BAD_PARAMETER;
            }
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)it->second)->set_wstring_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_wstring_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_enum_value(
        uint32_t& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
    {
        value = uint32_value_;
        return ReturnCode_t::RETCODE_OK;
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_enum_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_enum_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto itValue = values_.find(id);
    if (itValue != values_.end())
    {
        if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
        {
            value = *((uint32_t*)itValue->second);
            return ReturnCode_t::RETCODE_OK;
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)itValue->second)->get_enum_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_enum_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_enum_value(
        const uint32_t& value,
        MemberId id /*= MEMBER_ID_INVALID*/)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
    {
        if (descriptors_.find(value) != descriptors_.end())
        {
            uint32_value_ = value;
            return ReturnCode_t::RETCODE_OK;
        }
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_enum_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_enum_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto itValue = values_.find(id);
    if (itValue != values_.end())
    {
        if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
        {
            if (descriptors_.find(value) != descriptors_.end())
            {
                *((uint32_t*)itValue->second) = value;
                return ReturnCode_t::RETCODE_OK;
            }
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)itValue->second)->set_enum_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_enum_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::get_enum_value(
        std::string& value,
        MemberId id) const
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
    {
        auto it = descriptors_.find(uint32_value_);
        if (it != descriptors_.end())
        {
            value = it->second->get_name();
            return ReturnCode_t::RETCODE_OK;
        }
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return it->second->get_enum_value(value, MEMBER_ID_INVALID);
            }
        }
        else if (get_kind() == TK_ARRAY)
        {
            return default_array_value_->get_enum_value(value, MEMBER_ID_INVALID);
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto itValue = values_.find(id);
    if (itValue != values_.end())
    {
        if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
        {
            auto it = descriptors_.find(*((uint32_t*)itValue->second));
            if (it != descriptors_.end())
            {
                value = it->second->get_name();
                return ReturnCode_t::RETCODE_OK;
            }
        }
        else if (id != MEMBER_ID_INVALID)
        {
            if (get_kind() != TK_UNION || id == union_id_)
            {
                return ((DynamicData*)itValue->second)->get_enum_value(value, MEMBER_ID_INVALID);
            }
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        return default_array_value_->get_enum_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_enum_value(
        const std::string& value,
        MemberId id)
{
#ifdef DYNAMIC_TYPES_CHECKING
    if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
    {
        for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
        {
            if (it->second->get_name() == value)
            {
                uint32_value_ = it->first;
                return ReturnCode_t::RETCODE_OK;
            }
        }
    }
    else if (id != MEMBER_ID_INVALID)
    {
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            ReturnCode_t result = it->second->set_enum_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
        else if (get_kind() == TK_ARRAY)
        {
            ReturnCode_t insertResult = insert_array_data(id);
            if (insertResult == ReturnCode_t::RETCODE_OK)
            {
                return set_enum_value(value, id);
            }
            return insertResult;
        }
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
    auto itValue = values_.find(id);
    if (itValue != values_.end())
    {
        if (get_kind() == TK_ENUM && id == MEMBER_ID_INVALID)
        {
            for (auto it = descriptors_.begin(); it != descriptors_.end(); ++it)
            {
                if (it->second->get_name() == value)
                {
                    *((uint32_t*)itValue->second) = it->first;
                    return ReturnCode_t::RETCODE_OK;
                }
            }
        }
        else if (id != MEMBER_ID_INVALID)
        {
            ReturnCode_t result = ((DynamicData*)itValue->second)->set_enum_value(value, MEMBER_ID_INVALID);
            if (result == ReturnCode_t::RETCODE_OK && get_kind() == TK_UNION)
            {
                set_union_id(id);
            }
            return result;
        }
    }
    else if (get_kind() == TK_ARRAY && id != MEMBER_ID_INVALID)
    {
        ReturnCode_t insertResult = insert_array_data(id);
        if (insertResult == ReturnCode_t::RETCODE_OK)
        {
            return set_enum_value(value, id);
        }
        return insertResult;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

ReturnCode_t DynamicData::set_bitmask_value(
        uint64_t value)
{
    if (type_->kind_ == TK_BITMASK)
    {
        return set_uint64_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_bitmask_value(
        uint64_t& value) const
{
    if (type_->kind_ == TK_BITMASK)
    {
        return get_uint64_value(value, MEMBER_ID_INVALID);
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

void DynamicData::sort_member_ids(
        MemberId startId)
{
    MemberId index = startId;
    MemberId curID = startId + 1;
    uint32_t distance = 1;
#ifdef DYNAMIC_TYPES_CHECKING
    while (index <= complex_values_.size())
    {
        auto it = complex_values_.find(curID);
        if (it != complex_values_.end())
        {
            complex_values_[curID - distance] = it->second;
            complex_values_.erase(it);
        }
        else
        {
            ++distance;
        }
        ++index;
        ++curID;
    }
#else
    while (curID <= values_.size())
    {
        auto it = values_.find(curID);
        if (it != values_.end())
        {
            values_[curID - distance] = it->second;
            values_.erase(it);
        }
        else
        {
            ++distance;
        }
        ++index;
        ++curID;
    }
#endif // ifdef DYNAMIC_TYPES_CHECKING
}

MemberId DynamicData::get_array_index(
        const std::vector<uint32_t>& position)
{
    if (get_kind() == TK_ARRAY)
    {
        MemberId outPosition(0);
        uint32_t offset(1);
        if (position.size() == type_->get_bounds_size())
        {
            for (int32_t i = static_cast<int32_t>(position.size() - 1); i >= 0; --i)
            {
                outPosition += position[i] * offset;
                offset *= type_->get_bounds(static_cast<uint32_t>(i));
            }
            return outPosition;
        }
        else
        {
            logError(DYN_TYPES, "Error getting array index. Invalid dimension count.");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error getting array index. The kind " << get_kind() << "doesn't support it.");
    }
    return MEMBER_ID_INVALID;
}

ReturnCode_t DynamicData::insert_array_data(
        MemberId indexId)
{
    if (get_kind() == TK_ARRAY)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        if (indexId < type_->get_total_bounds())
        {
            auto it = complex_values_.find(indexId);
            if (it != complex_values_.end())
            {
                DynamicDataFactory::get_instance()->delete_data(it->second);
                complex_values_.erase(it);
            }
            DynamicData* value = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
            complex_values_.insert(std::make_pair(indexId, value));
            return ReturnCode_t::RETCODE_OK;
        }
#else
        if (indexId < type_->get_total_bounds())
        {
            auto it = values_.find(indexId);
            if (it != values_.end())
            {
                DynamicDataFactory::get_instance()->delete_data((DynamicData*)it->second);
                values_.erase(it);
            }
            DynamicData* value = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
            values_.insert(std::make_pair(indexId, value));
            return ReturnCode_t::RETCODE_OK;
        }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        else
        {
            logError(DYN_TYPES, "Error inserting data. Index out of bounds");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The kind " << get_kind() << " doesn't support this method");
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::clear_array_data(
        MemberId indexId)
{
    if (get_kind() == TK_ARRAY)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        if (indexId < type_->get_total_bounds())
        {
            auto it = complex_values_.find(indexId);
            if (it != complex_values_.end())
            {
                DynamicDataFactory::get_instance()->delete_data(it->second);
                complex_values_.erase(it);
            }
            return ReturnCode_t::RETCODE_OK;
        }
#else
        if (indexId < type_->get_total_bounds())
        {
            auto it = values_.find(indexId);
            if (it != values_.end())
            {
                DynamicDataFactory::get_instance()->delete_data((DynamicData*)it->second);
                values_.erase(it);
            }
            return ReturnCode_t::RETCODE_OK;
        }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        else
        {
            logError(DYN_TYPES, "Error removing data. Index out of bounds");
        }
    }
    else
    {
        logError(DYN_TYPES, "Error removing data. The kind " << get_kind() << " doesn't support this method");
    }
    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::insert_int32_value(
        int32_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_INT32)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_int32_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_uint32_value(
        uint32_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_UINT32)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_uint32_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_int16_value(
        int16_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_INT16)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_int16_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_uint16_value(
        uint16_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_UINT16)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_uint16_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_int64_value(
        int64_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_INT64)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_int64_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_uint64_value(
        uint64_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_UINT64)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_uint64_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_float32_value(
        float value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_FLOAT32)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_float32_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_float64_value(
        double value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_FLOAT64)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_float64_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_float128_value(
        long double value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_FLOAT128)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_float128_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_char8_value(
        char value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_CHAR8)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_char8_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_char16_value(
        wchar_t value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_CHAR16)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_char16_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_byte_value(
        octet value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_BYTE)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_byte_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_bool_value(
        bool value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_BOOLEAN)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_bool_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_string_value(
        const std::string& value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_STRING8)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_string_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_wstring_value(
        const std::wstring& value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_STRING16)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_wstring_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_enum_value(
        const std::string& value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->get_kind() == TK_ENUM)
    {
        ReturnCode_t result = insert_sequence_data(outId);
        if (result == ReturnCode_t::RETCODE_OK)
        {
            result = set_enum_value(value, outId);
        }
        return result;
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_complex_value(
        const DynamicData* value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->equals(value->type_.get()))
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            outId = static_cast<MemberId>(complex_values_.size());
            complex_values_.insert(std::make_pair(outId, DynamicDataFactory::get_instance()->create_copy(value)));
            return ReturnCode_t::RETCODE_OK;
#else
            outId = static_cast<MemberId>(values_.size());
            values_.insert(std::make_pair(outId, DynamicDataFactory::get_instance()->create_copy(value)));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting data. The container is full.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_complex_value(
        DynamicData_ptr value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->equals(value->type_.get()))
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            outId = static_cast<MemberId>(complex_values_.size());
            complex_values_.insert(std::make_pair(outId, DynamicDataFactory::get_instance()->create_copy(value.get())));
            return ReturnCode_t::RETCODE_OK;
#else
            outId = static_cast<MemberId>(values_.size());
            values_.insert(std::make_pair(outId, DynamicDataFactory::get_instance()->create_copy(value.get())));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting data. The container is full.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_complex_value(
        DynamicData* value,
        MemberId& outId)
{
    if (get_kind() == TK_SEQUENCE && type_->get_element_type()->equals(value->type_.get()))
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            outId = static_cast<MemberId>(complex_values_.size());
            complex_values_.insert(std::make_pair(outId, value));
            return ReturnCode_t::RETCODE_OK;
#else
            outId = static_cast<MemberId>(values_.size());
            values_.insert(std::make_pair(outId, value));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting data. The container is full.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The current kinds don't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_sequence_data(
        MemberId& outId)
{
    outId = MEMBER_ID_INVALID;
    if (get_kind() == TK_SEQUENCE)
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            DynamicData* new_element = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
            outId = static_cast<MemberId>(complex_values_.size());
            complex_values_.insert(std::make_pair(outId, new_element));
            return ReturnCode_t::RETCODE_OK;
#else
            DynamicData* new_element = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
            outId = static_cast<MemberId>(values_.size());
            values_.insert(std::make_pair(outId, new_element));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting data. The container is full.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting data. The kind " << get_kind() << " doesn't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::remove_sequence_data(
        MemberId id)
{
    if (get_kind() == TK_SEQUENCE || get_kind() == TK_ARRAY)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            DynamicDataFactory::get_instance()->delete_data(it->second);
            complex_values_.erase(it);
            sort_member_ids(id);
            return ReturnCode_t::RETCODE_OK;
        }
#else
        auto it = values_.find(id);
        if (it != values_.end())
        {
            DynamicDataFactory::get_instance()->delete_data((DynamicData*)it->second);
            values_.erase(it);
            sort_member_ids(id);
            return ReturnCode_t::RETCODE_OK;
        }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        logError(DYN_TYPES, "Error removing data. Member not found");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    logError(DYN_TYPES, "Error removing data. The current Kind " << get_kind()
                                                                 << " doesn't support this method");

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::insert_map_data(
        const DynamicData* key,
        MemberId& outKeyId,
        MemberId& outValueId)
{
    if (get_kind() == TK_MAP && type_->get_key_element_type()->equals(key->type_.get()))
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
            {
                if (it->second->key_element_ && it->second->equals(key))
                {
                    logError(DYN_TYPES, "Error inserting to map. The key already exists.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
            }
            outKeyId = static_cast<MemberId>(complex_values_.size());
            DynamicData* keyCopy = DynamicDataFactory::get_instance()->create_copy(key);
            keyCopy->key_element_ = true;
            complex_values_.insert(std::make_pair(outKeyId, keyCopy));

            DynamicData* new_element = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
            outValueId = static_cast<MemberId>(complex_values_.size());
            complex_values_.insert(std::make_pair(outValueId, new_element));
            return ReturnCode_t::RETCODE_OK;
#else
            for (auto it = values_.begin(); it != values_.end(); ++it)
            {
                if (((DynamicData*)it->second)->key_element_ && ((DynamicData*)it->second)->equals(key))
                {
                    logError(DYN_TYPES, "Error inserting to map. The key already exists.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
            }
            outKeyId = static_cast<MemberId>(values_.size());
            DynamicData* keyCopy = DynamicDataFactory::get_instance()->create_copy(key);
            keyCopy->key_element_ = true;
            values_.insert(std::make_pair(outKeyId, keyCopy));

            DynamicData* new_element = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
            outValueId = static_cast<MemberId>(values_.size());
            values_.insert(std::make_pair(outValueId, new_element));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting to map. The map is full");
            return ReturnCode_t::RETCODE_ERROR;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting to map. The current Kind " << get_kind()
                                                                        << " doesn't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_map_data(
        const DynamicData* key,
        DynamicData* value,
        MemberId& outKey,
        MemberId& outValue)
{
    if (get_kind() == TK_MAP && type_->get_key_element_type()->equals(key->type_.get()) &&
            type_->get_element_type()->equals(value->type_.get()))
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
            {
                if (it->second->key_element_ && it->second->equals(key))
                {
                    logError(DYN_TYPES, "Error inserting to map. The key already exists.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
            }
            outKey = static_cast<MemberId>(complex_values_.size());
            DynamicData* keyCopy = DynamicDataFactory::get_instance()->create_copy(key);
            keyCopy->key_element_ = true;
            complex_values_.insert(std::make_pair(outKey, keyCopy));

            outValue = static_cast<MemberId>(complex_values_.size());
            complex_values_.insert(std::make_pair(outValue, value));
            return ReturnCode_t::RETCODE_OK;
#else
            for (auto it = values_.begin(); it != values_.end(); ++it)
            {
                if (it->second == key)
                {
                    logError(DYN_TYPES, "Error inserting to map. The key already exists.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
            }
            outKey = static_cast<MemberId>(values_.size());
            DynamicData* keyCopy = DynamicDataFactory::get_instance()->create_copy(key);
            keyCopy->key_element_ = true;
            values_.insert(std::make_pair(outKey, keyCopy));

            outValue = static_cast<MemberId>(values_.size());
            values_.insert(std::make_pair(outValue, value));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting to map. The map is full");
            return ReturnCode_t::RETCODE_ERROR;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting to map. The current Kind " << get_kind()
                                                                        << " doesn't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_map_data(
        const DynamicData* key,
        const DynamicData* value,
        MemberId& outKey,
        MemberId& outValue)
{
    if (get_kind() == TK_MAP && type_->get_key_element_type()->equals(key->type_.get()) &&
            type_->get_element_type()->equals(value->type_.get()))
    {
        if (type_->get_bounds() == ::dds::core::LENGTH_UNLIMITED || get_item_count() < type_->get_bounds())
        {
#ifdef DYNAMIC_TYPES_CHECKING
            for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
            {
                if (it->second->key_element_ && it->second->equals(key))
                {
                    logError(DYN_TYPES, "Error inserting to map. The key already exists.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
            }
            outKey = static_cast<MemberId>(complex_values_.size());
            DynamicData* keyCopy = DynamicDataFactory::get_instance()->create_copy(key);
            keyCopy->key_element_ = true;
            complex_values_.insert(std::make_pair(outKey, keyCopy));

            outValue = static_cast<MemberId>(complex_values_.size());
            DynamicData* valueCopy = DynamicDataFactory::get_instance()->create_copy(value);
            complex_values_.insert(std::make_pair(outValue, valueCopy));
            return ReturnCode_t::RETCODE_OK;
#else
            for (auto it = values_.begin(); it != values_.end(); ++it)
            {
                if (it->second == key)
                {
                    logError(DYN_TYPES, "Error inserting to map. The key already exists.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
            }
            outKey = static_cast<MemberId>(values_.size());
            DynamicData* keyCopy = DynamicDataFactory::get_instance()->create_copy(key);
            keyCopy->key_element_ = true;
            values_.insert(std::make_pair(outKey, keyCopy));

            outValue = static_cast<MemberId>(values_.size());
            DynamicData* valueCopy = DynamicDataFactory::get_instance()->create_copy(value);
            values_.insert(std::make_pair(outValue, valueCopy));
            return ReturnCode_t::RETCODE_OK;
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        else
        {
            logError(DYN_TYPES, "Error inserting to map. The map is full");
            return ReturnCode_t::RETCODE_ERROR;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error inserting to map. The current Kind " << get_kind()
                                                                        << " doesn't support this method");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::insert_map_data(
        const DynamicData* key,
        DynamicData_ptr value,
        MemberId& outKey,
        MemberId& outValue)
{
    return insert_map_data(key, reinterpret_cast<const DynamicData*>(value.get()), outKey, outValue);
}

ReturnCode_t DynamicData::remove_map_data(
        MemberId keyId)
{
    if (get_kind() == TK_MAP)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        auto itKey = complex_values_.find(keyId);
        auto itValue = complex_values_.find(keyId + 1);
        if (itKey != complex_values_.end() && itValue != complex_values_.end() && itKey->second->key_element_)
        {
            DynamicDataFactory::get_instance()->delete_data(itKey->second);
            DynamicDataFactory::get_instance()->delete_data(itValue->second);
            complex_values_.erase(itKey);
            complex_values_.erase(itValue);
            sort_member_ids(keyId);
            return ReturnCode_t::RETCODE_OK;
        }
#else
        auto itKey = values_.find(keyId);
        auto itValue = values_.find(keyId + 1);
        if (itKey != values_.end() && itValue != values_.end() && ((DynamicData*)itKey->second)->key_element_)
        {
            DynamicDataFactory::get_instance()->delete_data(((DynamicData*)itKey->second));
            DynamicDataFactory::get_instance()->delete_data(((DynamicData*)itValue->second));
            values_.erase(itKey);
            values_.erase(itValue);
            sort_member_ids(keyId);
            return ReturnCode_t::RETCODE_OK;
        }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        else
        {
            logError(DYN_TYPES, "Error removing from map. Invalid input KeyId");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error removing from map. The current Kind " << get_kind()
                                                                         << " doesn't support this method");
        return ReturnCode_t::RETCODE_ERROR;
    }
}

ReturnCode_t DynamicData::clear_data()
{
    if (get_kind() == TK_SEQUENCE || get_kind() == TK_MAP || get_kind() == TK_ARRAY)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
        {
            DynamicDataFactory::get_instance()->delete_data(it->second);
        }
        complex_values_.clear();
#else
        for (auto it = values_.begin(); it != values_.end(); ++it)
        {
            DynamicDataFactory::get_instance()->delete_data((DynamicData*)it->second);
        }
        values_.clear();
#endif // ifdef DYNAMIC_TYPES_CHECKING
        return ReturnCode_t::RETCODE_OK;
    }

    logError(DYN_TYPES, "Error clearing data. The current Kind " << get_kind()
                                                                 << " doesn't support this method");

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_complex_value(
        DynamicData** value,
        MemberId id) const
{
    // Check that the type is complex and in case of dynamic containers, check that the index is valid
    if (id != MEMBER_ID_INVALID && (get_kind() == TK_STRUCTURE || get_kind() == TK_UNION ||
            get_kind() == TK_SEQUENCE || get_kind() == TK_ARRAY || get_kind() == TK_MAP || get_kind() == TK_BITSET))
    {
#ifdef DYNAMIC_TYPES_CHECKING
        auto it = complex_values_.find(id);
        if (it != complex_values_.end())
        {
            *value = DynamicDataFactory::get_instance()->create_copy(it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
#else
        auto it = values_.find(id);
        if (it != values_.end())
        {
            *value = DynamicDataFactory::get_instance()->create_copy((DynamicData*)it->second);
            return ReturnCode_t::RETCODE_OK;
        }
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif // ifdef DYNAMIC_TYPES_CHECKING
    }
    else
    {
        logError(DYN_TYPES, "Error settings complex value. The kind " << get_kind() << "doesn't support it");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::set_complex_value(
        DynamicData* value,
        MemberId id)
{
    // Check that the type is complex and in case of dynamic containers, check that the index is valid
    if (id != MEMBER_ID_INVALID && (get_kind() == TK_STRUCTURE || get_kind() == TK_UNION ||
            get_kind() == TK_SEQUENCE || get_kind() == TK_ARRAY || get_kind() == TK_MAP || get_kind() == TK_BITSET))
    {
        // With containers, check that the index is valid
        if ((get_kind() == TK_SEQUENCE || get_kind() == TK_ARRAY || get_kind() == TK_MAP) &&
                id >= type_->get_total_bounds())
        {
            logError(DYN_TYPES, "Error setting complex value. id out of bounds.");
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        }
        else
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto it = complex_values_.find(id);
            if (it != complex_values_.end())
            {
                if (get_kind() == TK_MAP && it->second->key_element_)
                {
                    logError(DYN_TYPES, "Error setting complex value. The given id is a Key value.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                else
                {
                    if (it->second != nullptr)
                    {
                        DynamicDataFactory::get_instance()->delete_data(it->second);
                    }
                    complex_values_.erase(it);

                    complex_values_.insert(std::make_pair(id, value));
                    if (get_kind() == TK_UNION && union_id_ != id)
                    {
                        set_union_id(id);
                    }
                }
            }
            else if (get_kind() == TK_ARRAY)
            {
                complex_values_.insert(std::make_pair(id, value));
                return ReturnCode_t::RETCODE_OK;
            }

#else
            auto it = values_.find(id);
            if (it != values_.end())
            {
                if (get_kind() == TK_MAP && ((DynamicData*)it->second)->key_element_)
                {
                    logError(DYN_TYPES, "Error setting complex value. The given id is a Key value.");
                    return ReturnCode_t::RETCODE_BAD_PARAMETER;
                }
                else
                {
                    if (it->second != nullptr)
                    {
                        DynamicDataFactory::get_instance()->delete_data((DynamicData*)it->second);
                    }
                    values_.erase(it);
                    values_.insert(std::make_pair(id, value));
                    if (get_kind() == TK_UNION && union_id_ != id)
                    {
                        set_union_id(id);
                    }
                }
            }
            else if (get_kind() == TK_ARRAY)
            {
                values_.insert(std::make_pair(id, value));
                return ReturnCode_t::RETCODE_OK;
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        return ReturnCode_t::RETCODE_OK;
    }
    else
    {
        logError(DYN_TYPES, "Error setting complex value. The kind " << get_kind() << "doesn't support it");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

ReturnCode_t DynamicData::get_union_label(
        uint64_t& value) const
{
    if (get_kind() == TK_UNION)
    {
        if (union_id_ != MEMBER_ID_INVALID)
        {
            value = union_label_;
            return ReturnCode_t::RETCODE_OK;
        }
        else
        {
            logError(DYN_TYPES, "Error getting union label. There isn't any label selected");
            return ReturnCode_t::RETCODE_ERROR;
        }
    }
    else
    {
        logError(DYN_TYPES, "Error getting union label. The kind " << get_kind() << "doesn't support it");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
}

bool DynamicData::deserialize(
        eprosima::fastcdr::Cdr& cdr)
{
    if (type_ != nullptr && type_->get_descriptor()->annotation_is_non_serialized())
    {
        return true;
    }

    switch (get_kind())
    {
        default:
            break;
        case TK_INT32:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> int32_value_;

#else
            auto it = values_.begin();
            cdr >> *((int32_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UINT32:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> uint32_value_;

#else
            auto it = values_.begin();
            cdr >> *((uint32_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_INT16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> int16_value_;

#else
            auto it = values_.begin();
            cdr >> *((int16_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UINT16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> uint16_value_;

#else
            auto it = values_.begin();
            cdr >> *((uint16_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_INT64:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> int64_value_;

#else
            auto it = values_.begin();
            cdr >> *((int64_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UINT64:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> uint64_value_;

#else
            auto it = values_.begin();
            cdr >> *((uint64_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_FLOAT32:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> float32_value_;
#else
            auto it = values_.begin();
            cdr >> *((float*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_FLOAT64:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> float64_value_;

#else
            auto it = values_.begin();
            cdr >> *((double*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_FLOAT128:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> float128_value_;

#else
            auto it = values_.begin();
            cdr >> *((long double*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_CHAR8:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> char8_value_;

#else
            auto it = values_.begin();
            cdr >> *((char*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_CHAR16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> char16_value_;

#else
            auto it = values_.begin();
            cdr >> *((wchar_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_BOOLEAN:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> bool_value_;

#else
            auto it = values_.begin();
            cdr >> *((bool*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_BYTE:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> byte_value_;

#else
            auto it = values_.begin();
            cdr >> *((octet*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_STRING8:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> string_value_;

#else
            auto it = values_.begin();
            cdr >> *((std::string*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_STRING16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> wstring_value_;

#else
            auto it = values_.begin();
            cdr >> *((std::wstring*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_ENUM:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr >> uint32_value_;

#else
            auto it = values_.begin();
            cdr >> *((uint32_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_BITMASK:
        {
            size_t type_size = type_->get_size();
#ifdef DYNAMIC_TYPES_CHECKING
            switch (type_size)
            {
                case 1:
                {
                    uint8_t temp;
                    cdr >> temp;
                    uint64_value_ = temp;
                    break;
                }
                case 2:
                {
                    uint16_t temp;
                    cdr >> temp;
                    uint64_value_ = temp;
                    break;
                }
                case 3:
                {
                    uint32_t temp;
                    cdr >> temp;
                    uint64_value_ = temp;
                    break;
                }
                case 4: cdr >> uint64_value_; break;
                default: logError(DYN_TYPES, "Cannot deserialize bitmask of size " << type_size);
            }
#else
            auto it = values_.begin();
            switch (type_size)
            {
                case 1: cdr >> *((uint8_t*)it->second); break;
                case 2: cdr >> *((uint16_t*)it->second); break;
                case 3: cdr >> *((uint32_t*)it->second); break;
                case 4: cdr >> *((uint64_t*)it->second); break;
                default: logError(DYN_TYPES, "Cannot deserialize bitmask of size " << type_size);
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UNION:
        {
            union_discriminator_->deserialize_discriminator(cdr);
            update_union_discriminator();
            set_union_id(union_id_);
            if (union_id_ != MEMBER_ID_INVALID)
            {

#ifdef DYNAMIC_TYPES_CHECKING
                auto it = complex_values_.find(union_id_);
                if (it != complex_values_.end())
                {
                    it->second->deserialize(cdr);
                }
#else
                auto it = values_.find(union_id_);
                if (it != values_.end())
                {
                    ((DynamicData*)it->second)->deserialize(cdr);
                }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            }
            break;
        }
        case TK_STRUCTURE:
        case TK_BITSET:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            //uint32_t size(static_cast<uint32_t>(complex_values_.size())), memberId(MEMBER_ID_INVALID);
            for (uint32_t i = 0; i < complex_values_.size(); ++i)
            {
                //cdr >> memberId;
                MemberDescriptor* member_desc = descriptors_[i];
                if (member_desc != nullptr)
                {
                    if (!member_desc->annotation_is_non_serialized())
                    {
                        auto it = complex_values_.find(i);
                        if (it != complex_values_.end())
                        {
                            it->second->deserialize(cdr);
                        }
                        else
                        {
                            DynamicData* pData = DynamicDataFactory::get_instance()->create_data(
                                type_->get_element_type());
                            pData->deserialize(cdr);
                            complex_values_.insert(std::make_pair(i, pData));
                        }
                    }
                }
                else
                {
                    logError(DYN_TYPES, "Missing MemberDescriptor " << i);
                }
            }
#else
            //uint32_t size(static_cast<uint32_t>(values_.size())), memberId(MEMBER_ID_INVALID);
            for (uint32_t i = 0; i < values_.size(); ++i)
            {
                //cdr >> memberId;
                MemberDescriptor* member_desc = descriptors_[i];
                if (member_desc != nullptr)
                {
                    if (!member_desc->annotation_is_non_serialized())
                    {
                        auto it = values_.find(i);
                        if (it != values_.end())
                        {
                            ((DynamicData*)it->second)->deserialize(cdr);
                        }
                        else
                        {
                            DynamicData* pData = DynamicDataFactory::get_instance()->create_data(
                                type_->get_element_type());
                            pData->deserialize(cdr);
                            values_.insert(std::make_pair(i, pData));
                        }
                    }
                }
                else
                {
                    logError(DYN_TYPES, "Missing MemberDescriptor " << i);
                }
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
        }
        break;
        case TK_ARRAY:
        {
            uint32_t size(type_->get_total_bounds());
            if (size > 0)
            {
                DynamicData* inputData(nullptr);
                for (uint32_t i = 0; i < size; ++i)
                {
#ifdef DYNAMIC_TYPES_CHECKING
                    auto it = complex_values_.find(i);
                    if (it != complex_values_.end())
                    {
                        it->second->deserialize(cdr);
                    }
                    else
                    {
                        if (inputData == nullptr)
                        {
                            inputData = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
                        }

                        inputData->deserialize(cdr);
                        if (!inputData->equals(default_array_value_))
                        {
                            complex_values_.insert(std::make_pair(i, inputData));
                            inputData = nullptr;
                        }
                    }
#else
                    auto it = values_.find(i);
                    if (it != values_.end())
                    {
                        ((DynamicData*)it->second)->deserialize(cdr);
                    }
                    else
                    {
                        if (inputData == nullptr)
                        {
                            inputData = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
                        }

                        inputData->deserialize(cdr);
                        if (!inputData->equals(default_array_value_))
                        {
                            values_.insert(std::make_pair(i, inputData));
                            inputData = nullptr;
                        }
                    }
#endif // ifdef DYNAMIC_TYPES_CHECKING
                }
                if (inputData != nullptr)
                {
                    DynamicDataFactory::get_instance()->delete_data(inputData);
                }
            }
            break;
        }
        case TK_SEQUENCE:
        case TK_MAP:
        {
            uint32_t size(0);
            bool bKeyElement(false);
            cdr >> size;

            if (get_kind() == TK_MAP)
            {
                size *= 2; // We serialize the number of pairs.
            }
            for (uint32_t i = 0; i < size; ++i)
            {
                //cdr >> memberId;
                if (get_kind() == TK_MAP)
                {
                    bKeyElement = !bKeyElement;
                }

#ifdef DYNAMIC_TYPES_CHECKING
                auto it = complex_values_.find(i);
                if (it != complex_values_.end())
                {
                    it->second->deserialize(cdr);
                    it->second->key_element_ = bKeyElement;
                }
                else
                {
                    DynamicData* pData = nullptr;
                    if (bKeyElement)
                    {
                        pData = DynamicDataFactory::get_instance()->create_data(type_->get_key_element_type());
                    }
                    else
                    {
                        pData = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
                    }
                    pData->deserialize(cdr);
                    pData->key_element_ = bKeyElement;
                    complex_values_.insert(std::make_pair(i, pData));
                }
#else
                auto it = values_.find(i);
                if (it != values_.end())
                {
                    ((DynamicData*)it->second)->deserialize(cdr);
                    ((DynamicData*)it->second)->key_element_ = bKeyElement;
                }
                else
                {
                    DynamicData* pData = nullptr;
                    if (bKeyElement)
                    {
                        pData = DynamicDataFactory::get_instance()->create_data(type_->get_key_element_type());
                    }
                    else
                    {
                        pData = DynamicDataFactory::get_instance()->create_data(type_->get_element_type());
                    }
                    pData->deserialize(cdr);
                    pData->key_element_ = bKeyElement;
                    values_.insert(std::make_pair(i, pData));
                }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            }
            break;
        }

        case TK_ALIAS:
            break;
    }
    return true;
}

bool DynamicData::deserialize_discriminator(
        eprosima::fastcdr::Cdr& cdr)
{
    switch (get_kind())
    {
        case TK_INT32:
        {
            int32_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<int32_t>(aux);
            break;
        }
        case TK_UINT32:
        {
            uint32_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<uint32_t>(aux);
            break;
        }
        case TK_INT16:
        {
            int16_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<int16_t>(aux);
            break;
        }
        case TK_UINT16:
        {
            uint16_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<uint16_t>(aux);
            break;
        }
        case TK_INT64:
        {
            int64_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<int64_t>(aux);
            break;
        }
        case TK_UINT64:
        {
            uint64_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<uint64_t>(aux);
            break;
        }
        case TK_CHAR8:
        {
            char aux;
            cdr >> aux;
            discriminator_value_ = static_cast<char>(aux);
            break;
        }
        case TK_CHAR16:
        {
            wchar_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<wchar_t>(aux);
            break;
        }
        case TK_BOOLEAN:
        {
            bool aux;
            cdr >> aux;
            discriminator_value_ = static_cast<bool>(aux);
            break;
        }
        case TK_BYTE:
        {
            octet aux;
            cdr >> aux;
            discriminator_value_ = static_cast<octet>(aux);
            break;
        }
        case TK_ENUM:
        {
            uint32_t aux;
            cdr >> aux;
            discriminator_value_ = static_cast<uint32_t>(aux);
            break;
        }
        case TK_FLOAT32:
        case TK_FLOAT64:
        case TK_FLOAT128:
        case TK_STRING8:
        case TK_STRING16:
        case TK_BITMASK:
        case TK_UNION:
        case TK_STRUCTURE:
        case TK_BITSET:
        case TK_ARRAY:
        case TK_SEQUENCE:
        case TK_MAP:
        case TK_ALIAS:
        default:
            break;
    }
    return true;
}

size_t DynamicData::getCdrSerializedSize(
        const DynamicData* data,
        size_t current_alignment /*= 0*/)
{
    if (data->type_ != nullptr && data->type_->get_descriptor()->annotation_is_non_serialized())
    {
        return 0;
    }

    size_t initial_alignment = current_alignment;

    switch (data->get_kind())
    {
        default:
            break;
        case TK_INT32:
        case TK_UINT32:
        case TK_FLOAT32:
        case TK_ENUM:
        case TK_CHAR16: // WCHARS NEED 32 Bits on Linux & MacOS
        {
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
            break;
        }
        case TK_INT16:
        case TK_UINT16:
        {
            current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
            break;
        }
        case TK_INT64:
        case TK_UINT64:
        case TK_FLOAT64:
        {
            current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
            break;
        }
        case TK_BITMASK:
        {
            size_t type_size = data->type_->get_size();
            current_alignment += type_size + eprosima::fastcdr::Cdr::alignment(current_alignment, type_size);
            break;
        }
        case TK_FLOAT128:
        {
            current_alignment += 16 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
            break;
        }
        case TK_CHAR8:
        case TK_BOOLEAN:
        case TK_BYTE:
        {
            current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
            break;
        }
        case TK_STRING8:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            // string content (length + characters + 1)
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
                    data->string_value_.length() + 1;
#else
            auto it = data->values_.begin();
            // string content (length + characters + 1)
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
                    ((std::string*)it->second)->length() + 1;
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_STRING16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            // string content (length + (characters * 4) )
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
                    ((data->wstring_value_.length()) * 4);
#else
            auto it = data->values_.begin();
            // string content (length + (characters * 4) )
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
                    (((std::wstring*)it->second)->length() * 4);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UNION:
        {
            // Union discriminator
            current_alignment += getCdrSerializedSize(data->union_discriminator_, current_alignment);

            if (data->union_id_ != MEMBER_ID_INVALID)
            {
#ifdef DYNAMIC_TYPES_CHECKING
                auto it = data->complex_values_.at(data->union_id_);
#else
                auto it = (DynamicData*)data->values_.at(data->union_id_);
#endif // ifdef DYNAMIC_TYPES_CHECKING
                current_alignment += getCdrSerializedSize(it, current_alignment);
            }
            break;
        }
        case TK_STRUCTURE:
        case TK_BITSET:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            //for (auto it = data->complex_values_.begin(); it != data->complex_values_.end(); ++it)
            //{
            //    current_alignment += getCdrSerializedSize(it->second, current_alignment);
            //}
            for (uint32_t i = 0; i < data->complex_values_.size(); ++i)
            {
                //cdr >> memberId;
                auto d_it = data->descriptors_.find(i);
                if (d_it != data->descriptors_.end())
                {
                    const MemberDescriptor* member_desc = d_it->second;
                    if (!member_desc->annotation_is_non_serialized())
                    {
                        auto it = data->complex_values_.find(i);
                        if (it != data->complex_values_.end())
                        {
                            current_alignment += getCdrSerializedSize(it->second, current_alignment);
                        }
                    }
                }
                else
                {
                    logError(DYN_TYPES, "Missing MemberDescriptor " << i);
                }
            }

#else
            //for (auto it = data->values_.begin(); it != data->values_.end(); ++it)
            //{
            //    current_alignment += getCdrSerializedSize((DynamicData*)it->second, current_alignment);
            //}
            for (uint32_t i = 0; i < data->values_.size(); ++i)
            {
                //cdr >> memberId;
                auto d_it = data->descriptors_.find(i);
                if (d_it != data->descriptors_.end())
                {
                    const MemberDescriptor* member_desc = d_it->second;
                    if (!member_desc->annotation_is_non_serialized())
                    {
                        auto it = data->values_.find(i);
                        if (it != data->values_.end())
                        {
                            current_alignment += getCdrSerializedSize((DynamicData*)it->second, current_alignment);
                        }
                    }
                }
                else
                {
                    logError(DYN_TYPES, "Missing MemberDescriptor " << i);
                }
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_ARRAY:
        {
            uint32_t arraySize = data->type_->get_total_bounds();
            size_t emptyElementSize =
                    getEmptyCdrSerializedSize(data->type_->get_element_type().get(), current_alignment);
            for (uint32_t idx = 0; idx < arraySize; ++idx)
            {
#ifdef DYNAMIC_TYPES_CHECKING
                auto it = data->complex_values_.find(idx);
                if (it != data->complex_values_.end())
#else
                auto it = data->values_.find(idx);
                if (it != data->values_.end())
#endif // ifdef DYNAMIC_TYPES_CHECKING
                {
                    // Element Size
                    current_alignment += getCdrSerializedSize((DynamicData*)it->second, current_alignment);
                }
                else
                {
                    current_alignment += emptyElementSize;
                }
            }
            break;
        }
        case TK_SEQUENCE:
        case TK_MAP:
        {
            // Elements count
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
#ifdef DYNAMIC_TYPES_CHECKING
            for (auto it = data->complex_values_.begin(); it != data->complex_values_.end(); ++it)
            {
                // Element Size
                current_alignment += getCdrSerializedSize(it->second, current_alignment);
            }
#else
            for (auto it = data->values_.begin(); it != data->values_.end(); ++it)
            {
                // Element Size
                current_alignment += getCdrSerializedSize((DynamicData*)it->second, current_alignment);
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_ALIAS:
            break;
    }

    return current_alignment - initial_alignment;
}

size_t DynamicData::getKeyMaxCdrSerializedSize(
        const DynamicType_ptr type,
        size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    // Structures check the the size of the key for their children
    if (type->get_kind() == TK_STRUCTURE || type->get_kind() == TK_BITSET)
    {
        for (auto it = type->member_by_id_.begin(); it != type->member_by_id_.end(); ++it)
        {
            if (it->second->key_annotation())
            {
                current_alignment += getKeyMaxCdrSerializedSize(it->second->descriptor_.type_, current_alignment);
            }
        }
    }
    else if (type->is_key_defined_)
    {
        return getMaxCdrSerializedSize(type, current_alignment);
    }
    return current_alignment - initial_alignment;
}

size_t DynamicData::getMaxCdrSerializedSize(
        const DynamicType_ptr type,
        size_t current_alignment /*= 0*/)
{
    if (type->get_descriptor()->annotation_is_non_serialized())
    {
        return 0;
    }

    size_t initial_alignment = current_alignment;

    switch (type->get_kind())
    {
        default:
            break;
        case TK_INT32:
        case TK_UINT32:
        case TK_FLOAT32:
        case TK_ENUM:
        case TK_CHAR16: // WCHARS NEED 32 Bits on Linux & MacOS
        {
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
            break;
        }
        case TK_INT16:
        case TK_UINT16:
        {
            current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
            break;
        }
        case TK_INT64:
        case TK_UINT64:
        case TK_FLOAT64:
        {
            current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
            break;
        }
        case TK_BITMASK:
        {
            size_t type_size = type->get_size();
            current_alignment += type_size + eprosima::fastcdr::Cdr::alignment(current_alignment, type_size);
            break;
        }
        case TK_FLOAT128:
        {
            current_alignment += 16 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
            break;
        }
        case TK_CHAR8:
        case TK_BOOLEAN:
        case TK_BYTE:
        {
            current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
            break;
        }
        case TK_STRING8:
        {
            // string length + string content + 1
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + type->get_bounds() + 1;
            break;
        }
        case TK_STRING16:
        {
            // string length + ( string content * 4 )
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + (type->get_bounds() * 4);

            break;
        }
        case TK_UNION:
        {
            // union id
            current_alignment += getMaxCdrSerializedSize(type->get_discriminator_type(), current_alignment);

            // Check the size of all members and take the size of the biggest one.
            size_t temp_size(0);
            size_t max_element_size(0);
            for (auto it = type->member_by_id_.begin(); it != type->member_by_id_.end(); ++it)
            {
                temp_size = getMaxCdrSerializedSize(it->second->descriptor_.type_, current_alignment);
                if (temp_size > max_element_size)
                {
                    max_element_size = temp_size;
                }
            }
            current_alignment += max_element_size;
            break;
        }
        case TK_STRUCTURE:
        case TK_BITSET:
        {
            for (auto it = type->member_by_id_.begin(); it != type->member_by_id_.end(); ++it)
            {
                if (!it->second->descriptor_.annotation_is_non_serialized())
                {
                    current_alignment += getMaxCdrSerializedSize(it->second->descriptor_.type_, current_alignment);
                }
            }
            break;
        }
        case TK_ARRAY:
        {
            // Element size with the maximum size
            current_alignment += type->get_total_bounds() *
                    (getMaxCdrSerializedSize(type->descriptor_->get_element_type()));
            break;
        }
        case TK_SEQUENCE:
        {
            // Elements count
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

            // Element size with the maximum size
            current_alignment += type->get_total_bounds() *
                    (getMaxCdrSerializedSize(type->descriptor_->get_element_type()));
            break;
        }
        case TK_MAP:
        {
            // Elements count
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

            // Key Elements size with the maximum size
            current_alignment += type->get_total_bounds() *
                    (getMaxCdrSerializedSize(type->descriptor_->get_key_element_type()));

            // Value Elements size with the maximum size
            current_alignment += type->get_total_bounds() *
                    (getMaxCdrSerializedSize(type->descriptor_->get_element_type()));
            break;
        }

        case TK_ALIAS:
        {
            current_alignment += getMaxCdrSerializedSize(type->get_base_type());
            break;
        }
    }

    return current_alignment - initial_alignment;
}

void DynamicData::serialize(
        eprosima::fastcdr::Cdr& cdr) const
{
    if (type_ != nullptr && type_->get_descriptor()->annotation_is_non_serialized())
    {
        return;
    }

    switch (get_kind())
    {
        default:
            break;
        case TK_INT32:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << int32_value_;
#else
            auto it = values_.begin();
            cdr << *((int32_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UINT32:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << uint32_value_;
#else
            auto it = values_.begin();
            cdr << *((uint32_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_INT16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << int16_value_;
#else
            auto it = values_.begin();
            cdr << *((int16_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UINT16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << uint16_value_;
#else
            auto it = values_.begin();
            cdr << *((uint16_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_INT64:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << int64_value_;
#else
            auto it = values_.begin();
            cdr << *((int64_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UINT64:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << uint64_value_;
#else
            auto it = values_.begin();
            cdr << *((uint64_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_FLOAT32:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << float32_value_;
#else
            auto it = values_.begin();
            cdr << *((float*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_FLOAT64:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << float64_value_;
#else
            auto it = values_.begin();
            cdr << *((double*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_FLOAT128:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << float128_value_;
#else
            auto it = values_.begin();
            cdr << *((long double*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_CHAR8:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << char8_value_;
#else
            auto it = values_.begin();
            cdr << *((char*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_CHAR16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << char16_value_;
#else
            auto it = values_.begin();
            cdr << *((wchar_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_BOOLEAN:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << bool_value_;
#else
            auto it = values_.begin();
            cdr << *((bool*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_BYTE:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << byte_value_;
#else
            auto it = values_.begin();
            cdr << *((octet*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_STRING8:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << string_value_;
#else
            auto it = values_.begin();
            cdr << *((std::string*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_STRING16:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << wstring_value_;
#else
            auto it = values_.begin();
            cdr << *((std::wstring*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_ENUM:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << uint32_value_;
#else
            auto it = values_.begin();
            cdr << *((uint32_t*)it->second);
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_BITMASK:
        {
            size_t type_size = type_->get_size();
#ifdef DYNAMIC_TYPES_CHECKING
            switch (type_size)
            {
                case 1: cdr << (uint8_t)uint64_value_; break;
                case 2: cdr << (uint16_t)uint64_value_; break;
                case 3: cdr << (uint32_t)uint64_value_; break;
                case 4: cdr << uint64_value_; break;
                default: logError(DYN_TYPES, "Cannot serialize bitmask of size " << type_size);
            }
#else
            auto it = values_.begin();
            switch (type_size)
            {
                case 1: cdr << *((uint8_t*)it->second); break;
                case 2: cdr << *((uint16_t*)it->second); break;
                case 3: cdr << *((uint32_t*)it->second); break;
                case 4: cdr << *((uint64_t*)it->second); break;
                default: logError(DYN_TYPES, "Cannot serialize bitmask of size " << type_size);
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_UNION:
        {
            union_discriminator_->serialize_discriminator(cdr);
            //cdr << union_id_;
            if (union_id_ != MEMBER_ID_INVALID)
            {
#ifdef DYNAMIC_TYPES_CHECKING
                auto it = complex_values_.at(union_id_);
#else
                auto it = (DynamicData*) values_.at(union_id_);
#endif // ifdef DYNAMIC_TYPES_CHECKING
                it->serialize(cdr);
            }
            break;
        }
        case TK_SEQUENCE: // Sequence is like structure, but with size
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << static_cast<uint32_t>(complex_values_.size());
            for (uint32_t idx = 0; idx < static_cast<uint32_t>(complex_values_.size()); ++idx)
            {
                auto it = complex_values_.at(idx);
                it->serialize(cdr);
            }
#else
            cdr << static_cast<uint32_t>(values_.size());
            for (uint32_t idx = 0; idx < static_cast<uint32_t>(values_.size()); ++idx)
            {
                auto it = values_.at(idx);
                ((DynamicData*)it)->serialize(cdr);
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_STRUCTURE:
        case TK_BITSET:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            for (uint32_t idx = 0; idx < static_cast<uint32_t>(complex_values_.size()); ++idx)
            {
                auto d_it = descriptors_.find(idx);
                if (d_it != descriptors_.end())
                {
                    const MemberDescriptor* member_desc = d_it->second;
                    if (!member_desc->annotation_is_non_serialized())
                    {
                        auto it = complex_values_.at(idx);
                        it->serialize(cdr);
                    }
                }
                else
                {
                    logError(DYN_TYPES, "Missing MemberDescriptor " << idx);
                }
            }
#else
            for (uint32_t idx = 0; idx < static_cast<uint32_t>(values_.size()); ++idx)
            {
                auto d_it = descriptors_.find(idx);
                if (d_it != descriptors_.end())
                {
                    const MemberDescriptor* member_desc = d_it->second;
                    if (!member_desc->annotation_is_non_serialized())
                    {
                        auto it = values_.at(idx);
                        ((DynamicData*)it)->serialize(cdr);
                    }
                }
                else
                {
                    logError(DYN_TYPES, "Missing MemberDescriptor " << idx);
                }
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_ARRAY:
        {
            uint32_t arraySize = type_->get_total_bounds();
            for (uint32_t idx = 0; idx < arraySize; ++idx)
            {
#ifdef DYNAMIC_TYPES_CHECKING
                auto it = complex_values_.find(idx);
                if (it != complex_values_.end())
#else
                auto it = values_.find(idx);
                if (it != values_.end())
#endif // ifdef DYNAMIC_TYPES_CHECKING
                {
                    ((DynamicData*)it->second)->serialize(cdr);
                }
                else
                {
                    serialize_empty_data(type_->get_element_type(), cdr);
                }
            }
            break;
        }
        case TK_MAP:
        {
#ifdef DYNAMIC_TYPES_CHECKING
            cdr << static_cast<uint32_t>(complex_values_.size() / 2); // Number of pairs
            for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
            {
                it->second->serialize(cdr);
            }
#else
            cdr << static_cast<uint32_t>(values_.size() / 2);
            for (auto it = values_.begin(); it != values_.end(); ++it)
            {
                ((DynamicData*)it->second)->serialize(cdr);
            }
#endif // ifdef DYNAMIC_TYPES_CHECKING
            break;
        }
        case TK_ALIAS:
            break;
    }
}

void DynamicData::serialize_discriminator(
        eprosima::fastcdr::Cdr& cdr) const
{
    switch (get_kind())
    {
        case TK_INT32:
        {
            int32_t aux = static_cast<int32_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_UINT32:
        {
            uint32_t aux = static_cast<uint32_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_INT16:
        {
            int16_t aux = static_cast<int16_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_UINT16:
        {
            uint16_t aux = static_cast<uint16_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_INT64:
        {
            int64_t aux = static_cast<int64_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_UINT64:
        {
            uint64_t aux = static_cast<uint64_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_CHAR8:
        {
            char aux = static_cast<char>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_CHAR16:
        {
            wchar_t aux = static_cast<wchar_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_BOOLEAN:
        {
            bool aux = !!(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_BYTE:
        {
            octet aux = static_cast<octet>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_ENUM:
        {
            uint32_t aux = static_cast<uint32_t>(discriminator_value_);
            cdr << aux;
            break;
        }
        case TK_FLOAT32:
        case TK_FLOAT64:
        case TK_FLOAT128:
        case TK_STRING8:
        case TK_STRING16:
        case TK_BITMASK:
        case TK_UNION:
        case TK_SEQUENCE:
        case TK_STRUCTURE:
        case TK_BITSET:
        case TK_ARRAY:
        case TK_MAP:
        case TK_ALIAS:
        default:
            break;
    }
}

void DynamicData::serializeKey(
        eprosima::fastcdr::Cdr& cdr) const
{
    // Structures check the the size of the key for their children
    if (type_->get_kind() == TK_STRUCTURE || type_->get_kind() == TK_BITSET)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        for (auto it = complex_values_.begin(); it != complex_values_.end(); ++it)
        {
            it->second->serializeKey(cdr);
        }
#else
        for (auto it = values_.begin(); it != values_.end(); ++it)
        {
            ((DynamicData*)it->second)->serializeKey(cdr);
        }
#endif // ifdef DYNAMIC_TYPES_CHECKING
    }
    else if (type_->is_key_defined_)
    {
        serialize(cdr);
    }
}

size_t DynamicData::getEmptyCdrSerializedSize(
        const DynamicType* type,
        size_t current_alignment /*= 0*/)
{
    if (type->get_descriptor()->annotation_is_non_serialized())
    {
        return 0;
    }

    size_t initial_alignment = current_alignment;

    switch (type->get_kind())
    {
        default:
            break;
        case TK_INT32:
        case TK_UINT32:
        case TK_FLOAT32:
        case TK_ENUM:
        case TK_CHAR16: // WCHARS NEED 32 Bits on Linux & MacOS
        {
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
            break;
        }
        case TK_INT16:
        case TK_UINT16:
        {
            current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
            break;
        }
        case TK_INT64:
        case TK_UINT64:
        case TK_FLOAT64:
        {
            current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
            break;
        }
        case TK_BITMASK:
        {
            size_t type_size = type->get_size();
            current_alignment += type_size + eprosima::fastcdr::Cdr::alignment(current_alignment, type_size);
            break;
        }
        case TK_FLOAT128:
        {
            current_alignment += 16 + eprosima::fastcdr::Cdr::alignment(current_alignment, 16);
            break;
        }
        case TK_CHAR8:
        case TK_BOOLEAN:
        case TK_BYTE:
        {
            current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
            break;
        }
        case TK_STRING8:
        {
            // string length + 1
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + 1;
            break;
        }
        case TK_STRING16:
        {
            // string length
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
            break;
        }
        case TK_UNION:
        {
            // union discriminator
            current_alignment += getEmptyCdrSerializedSize(type->get_discriminator_type().get(), current_alignment);
            break;
        }
        case TK_STRUCTURE:
        case TK_BITSET:
        {
            for (auto it = type->member_by_id_.begin(); it != type->member_by_id_.end(); ++it)
            {
                if (!it->second->descriptor_.annotation_is_non_serialized())
                {
                    current_alignment += getEmptyCdrSerializedSize(
                        it->second->descriptor_.type_.get(), current_alignment);
                }
            }
            break;
        }
        case TK_ARRAY:
        {
            // Elements count
            //current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

            // Element size with the maximum size
            current_alignment += type->get_total_bounds() *
                    (getEmptyCdrSerializedSize(type->descriptor_->get_element_type().get()));
            break;
        }
        case TK_SEQUENCE:
        case TK_MAP:
        {
            // Elements count
            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
            break;
        }

        case TK_ALIAS:
            current_alignment += getEmptyCdrSerializedSize(type->get_base_type().get());
            break;
    }

    return current_alignment - initial_alignment;
}

void DynamicData::serialize_empty_data(
        const DynamicType_ptr pType,
        eprosima::fastcdr::Cdr& cdr) const
{
    if (pType->get_descriptor()->annotation_is_non_serialized())
    {
        return;
    }

    switch (pType->get_kind())
    {
        default:
            break;
        case TK_ALIAS:
        {
            serialize_empty_data(pType->get_base_type(), cdr);
            break;
        }
        case TK_INT32:
        {
            cdr << static_cast<int32_t>(0);
            break;
        }
        case TK_UINT32:
        {
            cdr << static_cast<uint32_t>(0);
            break;
        }
        case TK_INT16:
        {
            cdr << static_cast<int16_t>(0);
            break;
        }
        case TK_UINT16:
        {
            cdr << static_cast<uint16_t>(0);
            break;
        }
        case TK_INT64:
        {
            cdr << static_cast<int64_t>(0);
            break;
        }
        case TK_UINT64:
        {
            cdr << static_cast<uint64_t>(0);
            break;
        }
        case TK_FLOAT32:
        {
            cdr << static_cast<float>(0.0f);
            break;
        }
        case TK_FLOAT64:
        {
            cdr << static_cast<double>(0.0);
            break;
        }
        case TK_FLOAT128:
        {
            cdr << static_cast<long double>(0.0);
            break;
        }
        case TK_CHAR8:
        {
            cdr << static_cast<char>(0);
            break;
        }
        case TK_CHAR16:
        {
            cdr << static_cast<uint32_t>(0);
            break;
        }
        case TK_BOOLEAN:
        {
            cdr << static_cast<uint8_t>(0);
            break;
        }
        case TK_BYTE:
        {
            cdr << static_cast<uint8_t>(0);
            break;
        }
        case TK_STRING8:
        {
            cdr << std::string();
            break;
        }
        case TK_STRING16:
        {
            cdr << std::wstring();
            break;
        }
        case TK_ENUM:
        {
            cdr << static_cast<uint32_t>(0);
            break;
        }
        case TK_BITMASK:
        {
            size_t type_size = pType->get_size();
            switch (type_size)
            {
                case 1: cdr << static_cast<uint8_t>(0); break;
                case 2: cdr << static_cast<uint16_t>(0); break;
                case 3: cdr << static_cast<uint32_t>(0); break;
                case 4: cdr << static_cast<uint64_t>(0); break;
                default: logError(DYN_TYPES, "Cannot deserialize bitmask of size " << type_size);
            }
            break;
        }
        case TK_UNION:
        {
            cdr << static_cast<uint32_t>(MEMBER_ID_INVALID);
            break;
        }
        case TK_SEQUENCE: // Sequence is like structure, but with size
        {
            cdr << static_cast<uint32_t>(0);
            break;
        }
        case TK_STRUCTURE:
        case TK_BITSET:
        {
            for (uint32_t idx = 0; idx < pType->member_by_id_.size(); ++idx)
            {
                auto it = pType->member_by_id_.at(idx);
                if (!it->descriptor_.annotation_is_non_serialized())
                {
                    serialize_empty_data(it->descriptor_.type_, cdr);
                }
            }
            break;
        }
        case TK_ARRAY:
        {
            uint32_t arraySize = pType->get_total_bounds();
            //cdr << arraySize;
            for (uint32_t i = 0; i < arraySize; ++i)
            {
                serialize_empty_data(pType->get_element_type(), cdr);
            }
            break;
        }
        case TK_MAP:
        {
            cdr << static_cast<uint32_t>(0);
            break;
        }
    }
}

} // namespace types
} // namespace fastrtps
} // namespace eprosima
