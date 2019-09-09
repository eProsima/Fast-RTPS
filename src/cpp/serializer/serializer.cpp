#include <fastrtps/serializer/serializer.h>
#include <fastrtps/types/DynamicData.h>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/DynamicType.h>
#include <fastrtps/types/DynamicTypeMember.h>
#include <fastrtps/types/TypeDescriptor.h>
#include <fastrtps/types/MemberDescriptor.h>
#include <fastcdr/Cdr.h>
#include <fastrtps/types/TypeDescriptor.h>
#include <fastrtps/types/TypesBase.h>
#include <fastrtps/log/Log.h>

namespace eprosima{
namespace fastrtps{
namespace serializer{

using namespace eprosima::fastrtps::types ;
template<class Serializable>
void Serializer<Serializable>::serialize(DynamicData *dd, eprosima::fastcdr::Cdr& cdr) const
{
    if ( dd->type() != nullptr && dd->type()->get_descriptor()->annotation_is_non_serialized())
    {
        return;
    }
    switch (dd->get_kind())
    {
    default:
        break;
    case TK_INT32:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_int32_t() ;
#else
        auto it = dd->values().begin();
        cdr << *((int32_t*)it->second);
#endif
        break;
    }
    case TK_UINT32:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_uint32_t();
#else
        auto it = dd->values().begin();
        cdr << *((uint32_t*)it->second);
#endif
        break;
    }
    case TK_INT16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_int16_t() ;
#else
        auto it = dd->values().begin();
        cdr << *((int16_t*)it->second);
#endif
        break;
    }
    case TK_UINT16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_uint16_t() ;
#else
        auto it = dd->values().begin();
        cdr << *((uint16_t*)it->second);
#endif
        break;
    }
    case TK_INT64:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_int64_t();
#else
        auto it = dd->values().begin();
        cdr << *((int64_t*)it->second);
#endif
        break;
    }
    case TK_UINT64:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_uint64_t();
#else
        auto it = dd->values().begin();
        cdr << *((uint64_t*)it->second);
#endif
        break;
    }
    case TK_FLOAT32:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_float32_t();
#else
        auto it = dd->values().begin();
        cdr << *((float*)it->second);
#endif
        break;
    }
    case TK_FLOAT64:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_float64_t() ;
#else
        auto it = dd->values().begin();
        cdr << *((double*)it->second);
#endif
        break;
    }
    case TK_FLOAT128:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_float128_t();
#else
        auto it = dd->values().begin();
        cdr << *((long double*)it->second);
#endif
        break;
    }
    case TK_CHAR8:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_char8_t();
#else
        auto it = dd->values().begin();
        cdr << *((char*)it->second);
#endif
        break;
    }
    case TK_CHAR16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_char16_t();
#else
        auto it = dd->values().begin();
        cdr << *((wchar_t*)it->second);
#endif
        break;
    }
    case TK_BOOLEAN:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_bool_t();
#else
        auto it = dd->values().begin();
        cdr << *((bool*)it->second);
#endif
        break;
    }
    case TK_BYTE:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_octet_t();
#else
        auto it = dd->values().begin();
        cdr << *((octet*)it->second);
#endif
        break;
    }
    case TK_STRING8:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_string_t();
#else
        auto it = dd->values().begin();
        cdr << *((std::string*)it->second);
#endif
        break;
    }
    case TK_STRING16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_wstring_t();
#else
        auto it = dd->values().begin();
        cdr << *((std::wstring*)it->second);
#endif
        break;
    }
    case TK_ENUM:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << dd->get_uint32_t();
#else
        auto it = dd->values().begin();
        cdr << *((uint32_t*)it->second);
#endif
        break;
    }
    case TK_BITMASK:
    {
        size_t type_size = dd->type()->get_size();
#ifdef DYNAMIC_TYPES_CHECKING
        switch (type_size)
        {
            case 1: cdr << (uint8_t)dd->get_uint64_t(); break;
            case 2: cdr << (uint16_t)dd->get_uint64_t(); break;
            case 3: cdr << (uint32_t)dd->get_uint64_t(); break;
            case 4: cdr << dd->get_uint64_t(); break;
            default: logError(DYN_TYPES, "Cannot serialize bitmask of size " << type_size);
        }
#else
        auto it = dd->values().begin();
        switch (type_size)
        {
            case 1: cdr << *((uint8_t*)it->second); break;
            case 2: cdr << *((uint16_t*)it->second); break;
            case 3: cdr << *((uint32_t*)it->second); break;
            case 4: cdr << *((uint64_t*)it->second); break;
            default: logError(DYN_TYPES, "Cannot serialize bitmask of size " << type_size);
        }
#endif
        break;
    }
    case TK_UNION:
    {
        serialize_discriminator( dd->get_union_discriminator(), cdr) ;
        //cdr << union_id_;
        if (dd->get_union_id() != MEMBER_ID_INVALID)
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto it = dd->get_complex_t().at(dd->get_union_id());
#else
            auto it = (DynamicData*) dd->values().at(dd->get_union_id());
#endif
            serialize(&(*it), cdr);
            //FRANAVA: it->serialize(cdr);
        }
        break;
    }
    case TK_SEQUENCE: // Sequence is like structure, but with size
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << static_cast<uint32_t>(dd->get_complex_t().size());
        for (uint32_t idx = 0; idx < static_cast<uint32_t>(dd->get_complex_t().size()); ++idx)
        {
            auto it = dd->get_complex_t().at(idx);
            serialize(&(*it), cdr);
            //FRANAVA: it->serialize(cdr);
        }
#else
        cdr << static_cast<uint32_t>(dd->values().size());
        for (uint32_t idx = 0; idx < static_cast<uint32_t>(dd->values().size()); ++idx)
        {
            auto it = dd->values().at(idx);
            serialize(((DynamicData*)it), cdr);
            //FRANAVA: ((DynamicData*)it)->serialize(cdr);
        }
#endif
        break;
    }
    case TK_STRUCTURE:
    case TK_BITSET:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        for (uint32_t idx = 0; idx < static_cast<uint32_t>(dd->get_complex_t().size()); ++idx)
        {
            auto d_it = dd->get_descriptors().find(idx);
            if (d_it != dd->get_descriptors().end())
            {
                const MemberDescriptor* member_desc = d_it->second;
                if (!member_desc->annotation_is_non_serialized())
                {
                    auto it = dd->get_complex_t().at(idx);
                    Serializer(it).serialize(cdr);
																				
                }
            }
            else
            {
                logError(DYN_TYPES, "Missing MemberDescriptor " << idx);
            }
        }
#else
        for (uint32_t idx = 0; idx < static_cast<uint32_t>(dd->values().size()); ++idx)
        {
            auto d_it = dd->get_descriptors().find(idx);
            if (d_it != dd->get_descriptors().end())
            {
                const MemberDescriptor* member_desc = d_it->second;
                if (!member_desc->annotation_is_non_serialized())
                {
                    auto it = dd->values().at(idx);
                    serialize(((DynamicData*)it), cdr);
                    //FRANAVA: ((DynamicData*)it)->serialize(cdr);
                }
            }
            else
            {
                logError(DYN_TYPES, "Missing MemberDescriptor " << idx);
            }
        }
#endif
        break;
    }
    case TK_ARRAY:
    {
        uint32_t arraySize = dd->type()->get_total_bounds();
        for (uint32_t idx = 0; idx < arraySize; ++idx)
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto it = dd->get_complex_t().find(idx);
            if (it != dd->get_complex_t().end())
#else
            auto it = dd->values().find(idx);
            if (it != dd->values().end())
#endif
            {
                serialize(((DynamicData*)it->second), cdr);
                //FRANAVA: ((DynamicData*)it->second)->serialize(cdr);
            }
            else
            {
//                serialize_empty_data(dd->type()->get_element_type(), cdr);
                serialize_empty_data(dd->type()->get_element_type().get(), cdr);
            }
        }
        break;
            }
    case TK_MAP:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr << static_cast<uint32_t>(dd->get_complex_t().size() / 2); // Number of pairs
        for (auto it = dd->get_complex_t().begin(); it != dd->get_complex_t().end(); ++it)
        {
            Serializer(it->second).serialize(cdr);
        }
#else
        cdr << static_cast<uint32_t>(dd->values().size() / 2);
        for (auto it = dd->values().begin(); it != dd->values().end(); ++it)
        {
            serialize(((DynamicData*)it->second), cdr);
            //FRANAVA: ((DynamicData*)it->second)->serialize(cdr);
        }
#endif
        break;
    }
    case TK_ALIAS:
        break;
    }
}
template<class Serializable>
void Serializer<Serializable>::serialize_discriminator(DynamicData *dd, eprosima::fastcdr::Cdr& cdr) const
{
    switch (dd->get_kind())
    {
    case TK_INT32:
    {
        int32_t aux = static_cast<int32_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_UINT32:
    {
        uint32_t aux = static_cast<uint32_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_INT16:
    {
        int16_t aux = static_cast<int16_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_UINT16:
    {
        uint16_t aux = static_cast<uint16_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_INT64:
    {
        int64_t aux = static_cast<int64_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_UINT64:
    {
        uint64_t aux = static_cast<uint64_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_CHAR8:
    {
        char aux = static_cast<char>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_CHAR16:
    {
        wchar_t aux = static_cast<wchar_t>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_BOOLEAN:
    {
        bool aux = !!(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_BYTE:
    {
        octet aux = static_cast<octet>(dd->get_discriminator_value());
        cdr << aux;
        break;
    }
    case TK_ENUM:
    {
        uint32_t aux = static_cast<uint32_t>(dd->get_discriminator_value());
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

template<class Serializable>
void Serializer<Serializable>::serialize_empty_data(DynamicType *dt, eprosima::fastcdr::Cdr& cdr) const
{
    if (dt->get_descriptor()->annotation_is_non_serialized())
    {
        return;
    }

    switch (dt->get_kind())
    {
        default:
            break;
        case TK_ALIAS:
        {
            serialize_empty_data(dt->get_base_type().get(), cdr);
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
            size_t type_size = dt->get_size();
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
            for (uint32_t idx = 0; idx < dt->get_member_by_id().size(); ++idx)
            {
                auto it = dt->get_member_by_id().at(idx); //DynamicTypeMember *
                if (!it->get_descriptor()->annotation_is_non_serialized())
                {
                    serialize_empty_data(it->get_descriptor()->get_type().get(), cdr);
                }
            }
            break;
        }
        case TK_ARRAY:
        {
            uint32_t arraySize = dt->get_total_bounds();
            //cdr << arraySize;
            for (uint32_t i = 0; i < arraySize; ++i)
            {
                serialize_empty_data(dt->get_element_type().get(), cdr);
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

//deserialize_dyn_data
template<class Serializable>
bool Serializer<Serializable>::deserialize(DynamicData *dd, eprosima::fastcdr::Cdr& cdr)const
{
    if (dd->type() != nullptr && dd->type()->get_descriptor()->annotation_is_non_serialized())
    {
        return true;
    }

    switch (dd->get_kind())
    {
    default:
        break;
    case TK_INT32:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_int32_t();

#else
        auto it = dd->values().begin();
        cdr >> *((int32_t*)it->second);
#endif
        break;
    }
    case TK_UINT32:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_uint32_t();

#else
        auto it = dd->values().begin();
        cdr >> *((uint32_t*)it->second);
#endif
        break;
    }
    case TK_INT16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_int16_t();

#else
        auto it = dd->values().begin();
        cdr >> *((int16_t*)it->second);
#endif
        break;
    }
    case TK_UINT16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_uint16_t();

#else
        auto it = dd->values().begin();
        cdr >> *((uint16_t*)it->second);
#endif
        break;
    }
    case TK_INT64:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_int64_t();

#else
        auto it = dd->values().begin();
        cdr >> *((int64_t*)it->second);
#endif
        break;
    }
    case TK_UINT64:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_uint64_t();

#else
        auto it = dd->values().begin();
        cdr >> *((uint64_t*)it->second);
#endif
        break;
    }
    case TK_FLOAT32:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_float32_t();
#else
        auto it = dd->values().begin();
        cdr >> *((float*)it->second);
#endif
        break;
    }
    case TK_FLOAT64:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_float64_t();

#else
        auto it = dd->values().begin();
        cdr >> *((double*)it->second);
#endif
        break;
    }
    case TK_FLOAT128:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_float128_t();

#else
        auto it = dd->values().begin();
        cdr >> *((long double*)it->second);
#endif
        break;
    }
    case TK_CHAR8:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_char8_t();

#else
        auto it = dd->values().begin();
        cdr >> *((char*)it->second);
#endif
        break;
    }
    case TK_CHAR16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_char16_t();

#else
        auto it = dd->values().begin();
        cdr >> *((wchar_t*)it->second);
#endif
        break;
    }
    case TK_BOOLEAN:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_bool_t();

#else
        auto it = dd->values().begin();
        cdr >> *((bool*)it->second);
#endif
        break;
    }
    case TK_BYTE:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_octet_t();

#else
        auto it = dd->values().begin();
        cdr >> *((octet*)it->second);
#endif
        break;
    }
    case TK_STRING8:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_string_t();

#else
        auto it = dd->values().begin();
        cdr >> *((std::string*)it->second);
#endif
        break;
    }
    case TK_STRING16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_wstring_t();

#else
        auto it = dd->values().begin();
        cdr >> *((std::wstring*)it->second);
#endif
        break;
    }
    case TK_ENUM:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        cdr >> dd->assign_to_uint32_t() ;

#else
        auto it = dd->values().begin();
        cdr >> *((uint32_t*)it->second);
#endif
        break;
    }
    case TK_BITMASK:
    {
        size_t type_size = dd->type()->get_size();
#ifdef DYNAMIC_TYPES_CHECKING
        switch (type_size)
        {
            case 1:
            {
                uint8_t temp;
                cdr >> temp;
                dd->assign_to_uint64_t() = temp;
                break;
            }
            case 2:
            {
                uint16_t temp;
                cdr >> temp;
                dd->assign_to_uint64_t() = temp;
                break;
            }
            case 3:
            {
                uint32_t temp;
                cdr >> temp;
                dd->assign_to_uint64_t() = temp;
                break;
            }
            case 4: cdr >> dd->assign_to_uint64_t(); break;
            default: logError(DYN_TYPES, "Cannot deserialize bitmask of size " << type_size);
        }
#else
        auto it = dd->values().begin();
        switch (type_size)
        {
            case 1: cdr >> *((uint8_t*)it->second); break;
            case 2: cdr >> *((uint16_t*)it->second); break;
            case 3: cdr >> *((uint32_t*)it->second); break;
            case 4: cdr >> *((uint64_t*)it->second); break;
            default: logError(DYN_TYPES, "Cannot deserialize bitmask of size " << type_size);
        }
#endif
        break;
    }
    case TK_UNION:
    {
        dd->get_union_discriminator()->deserialize_discriminator(cdr);
        dd->update_union_discriminator();
        dd->set_union_id(dd->get_union_id());
        if (dd->get_union_id() != MEMBER_ID_INVALID)
        {

#ifdef DYNAMIC_TYPES_CHECKING
            auto it = dd->get_complex_t().find(dd->get_union_id());
            if (it != dd->get_complex_t().end())
            {
                Serializer(it->second).deserialize(cdr);
            }
#else
            auto it = dd->values().find(dd->get_union_id());
            if (it != dd->values().end())
            {
                Serializer((DynamicData*)it->second).deserialize(cdr) ;
            }
#endif
        }
        break;
    }
    case TK_STRUCTURE:
    case TK_BITSET:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        //uint32_t size(static_cast<uint32_t>(dd->get_complex_t().size())), memberId(MEMBER_ID_INVALID);
        for (uint32_t i = 0; i < dd->get_complex_t().size(); ++i)
        {
            //cdr >> memberId;
            MemberDescriptor* member_desc = dd->get_descriptors()[i];
            if (member_desc != nullptr)
            {
                if (!member_desc->annotation_is_non_serialized())
                {
                    auto it = dd->get_complex_t().find(i);
                    if (it != dd->get_complex_t().end())
                    {
                        Serializer(it->second).deserialize(cdr);
                    }
                    else
                    {
                        DynamicData* pData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_element_type());
                        Serializer(pData).deserialize(cdr);
                        dd->get_complex_t().insert(std::make_pair(i, pData));
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
        for (uint32_t i = 0; i < dd->values().size(); ++i)
        {
            //cdr >> memberId;
            MemberDescriptor* member_desc = dd->get_descriptors()[i];
            if (member_desc != nullptr)
            {
                if (!member_desc->annotation_is_non_serialized())
                {
                    auto it = dd->values().find(i);
                    if (it != dd->values().end())
                    {
                        Serializer((DynamicData*)it->second).deserialize(cdr);
                    }
                    else
                    {
                        DynamicData* pData = DynamicDataFactory::get_instance()->create_data( dd->type()->get_element_type() );
                        Serializer(pData).deserialize(cdr);
                        dd->values().insert(std::make_pair(i, pData));
                    }
                }
            }
            else
            {
                logError(DYN_TYPES, "Missing MemberDescriptor " << i);
            }
        }
#endif
    }
    break;
    case TK_ARRAY:
    {
        uint32_t size(dd->type()->get_total_bounds());
        if (size > 0)
        {
            DynamicData* inputData(nullptr);
            for (uint32_t i = 0; i < size; ++i)
            {
#ifdef DYNAMIC_TYPES_CHECKING
                auto it = dd->get_complex_t().find(i);
                if (it != dd->get_complex_t().end())
                {
                    Serializer(it->second).deserialize(cdr);
                }
                else
                {
                    if (inputData == nullptr)
                    {
                        inputData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_element_type());
                    }

                    Serializer(inputData).deserialize(cdr);
                    if (!inputData->equals(dd->get_default_array_value()))
                    {
                        dd->get_complex_t().insert(std::make_pair(i, inputData));
                        inputData = nullptr;
                    }
                }
#else
                auto it = dd->values().find(i);
                if (it != dd->values().end())
                {
                    Serializer((DynamicData*)it->second).deserialize(cdr);
                }
                else
                {
                    if (inputData == nullptr)
                    {
                        inputData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_element_type());
                    }
                    Serializer(inputData).deserialize(cdr);
                    if (!inputData->equals(dd->get_default_array_value()))
                    {
                        dd->values().insert(std::make_pair(i, inputData));
                        inputData = nullptr;
                    }
                }
#endif
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

        if (dd->get_kind() == TK_MAP)
        {
            size *= 2; // We serialize the number of pairs.
        }
        for (uint32_t i = 0; i < size; ++i)
        {
            //cdr >> memberId;
            if (dd->get_kind() == TK_MAP)
            {
                bKeyElement = !bKeyElement;
            }

#ifdef DYNAMIC_TYPES_CHECKING
            auto it = dd->get_complex_t().find(i);
            if (it != dd->get_complex_t().end())
            {
                Serializer(it->second).deserialize(cdr);
                it->second->set_key_element(bKeyElement) ;
            }
            else
            {
                DynamicData* pData = nullptr;
                if (bKeyElement)
                {
                    pData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_key_element_type());
                }
                else
                {
                    pData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_element_type());
                }
                Serializer(pData).deserialize(cdr);
                pData->set_key_element(bKeyElement);
                dd->get_complex_t().insert(std::make_pair(i, pData));
            }
#else
            auto it = dd->values().find(i);
            if (it != dd->values().end())
            {
                Serializer((DynamicData*)it->second).deserialize(cdr);
                ((DynamicData*)it->second)->set_key_element(bKeyElement) ;
            }
            else
            {
                DynamicData* pData = nullptr;
                if (bKeyElement)
                {
                    pData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_key_element_type());
                }
                else
                {
                    pData = DynamicDataFactory::get_instance()->create_data(dd->type()->get_element_type());
                }
                Serializer(pData).deserialize(cdr);
                pData->set_key_element(bKeyElement);
                dd->values().insert(std::make_pair(i, pData));
            }
#endif
        }
        break;
    }

    case TK_ALIAS:
        break;
    }
    return true;
}

template<class Serializable>
bool Serializer<Serializable>::deserialize_discriminator(eprosima::fastcdr::Cdr& cdr)
{
    switch (r_->get_kind())
    {
    case TK_INT32:
    {
        int32_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<int32_t>(aux));
        break;
    }
    case TK_UINT32:
    {
        uint32_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<uint32_t>(aux));
        break;
    }
    case TK_INT16:
    {
        int16_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<int16_t>(aux));
        break;
    }
    case TK_UINT16:
    {
        uint16_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<uint16_t>(aux));
        break;
    }
    case TK_INT64:
    {
        int64_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<int64_t>(aux));
        break;
    }
    case TK_UINT64:
    {
        uint64_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<uint64_t>(aux));
        break;
    }
    case TK_CHAR8:
    {
        char aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<char>(aux));
        break;
    }
    case TK_CHAR16:
    {
        wchar_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<wchar_t>(aux));
        break;
    }
    case TK_BOOLEAN:
    {
        bool aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<bool>(aux));
        break;
    }
    case TK_BYTE:
    {
        octet aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<octet>(aux));
        break;
    }
    case TK_ENUM:
    {
        uint32_t aux;
        cdr >> aux;
        r_->set_discriminator_value(static_cast<uint32_t>(aux));
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
template<class Serializable>
void Serializer<Serializable>::serializeKey(DynamicData *dd,eprosima::fastcdr::Cdr& cdr) const
{
    // Structures check the the size of the key for their children
    if (dd->type()->get_kind() == TK_STRUCTURE || dd->type()->get_kind() == TK_BITSET)
    {
#ifdef DYNAMIC_TYPES_CHECKING
        for (auto it = dd->get_complex_t().begin(); it != dd->get_complex_t().end(); ++it)
        {
            Serializer(it->second).serializeKey(it->second, cdr);
        }
#else
        for (auto it = dd->values().begin(); it != dd->values().end(); ++it)
        {
            Serializer((DynamicData*)it->second).serializeKey((DynamicData*)it->second, cdr);
        }
#endif
    }
    else if (dd->type()->is_key_defined())
    {
				    //New Serialize instance to avoid Const/this issues
        Serializer(dd).serialize(cdr);
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(
        const DynamicData* data,
        size_t current_alignment /*= 0*/)
{
    if (data->type() != nullptr && data->type()->get_descriptor()->annotation_is_non_serialized())
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
        size_t type_size = data->type()->get_size();
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
            data->get_string_t().length() + 1;
#else
        auto it = data->values().begin();
        // string content (length + characters + 1)
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
            ((std::string*)it->second)->length() + 1;
#endif
        break;
    }
    case TK_STRING16:
    {
#ifdef DYNAMIC_TYPES_CHECKING
        // string content (length + (characters * 4) )
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
            ((data->get_wstring_t().length()) * 4);
#else
        auto it = data->values().begin();
        // string content (length + (characters * 4) )
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
            (((std::wstring*)it->second)->length() * 4);
#endif
        break;
    }
    case TK_UNION:
    {
        // Union discriminator
        current_alignment += getCdrSerializedSize(data->get_union_discriminator(), current_alignment);

        if (data->get_union_id() != MEMBER_ID_INVALID)
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto it = data->get_complex_t().at(data->get_union_id());
#else
            auto it = (DynamicData*)data->values().at(data->get_union_id());
#endif
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
        for (uint32_t i = 0; i < data->get_complex_t().size(); ++i)
        {
            //cdr >> memberId;
            auto d_it = data->get_descriptors().find(i);
            if (d_it != data->get_descriptors().end())
            {
                const MemberDescriptor* member_desc = d_it->second;
                if (!member_desc->annotation_is_non_serialized())
                {
                    auto it = data->get_complex_t().find(i);
                    if (it != data->get_complex_t().end())
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
        for (uint32_t i = 0; i < data->values().size(); ++i)
        {
            //cdr >> memberId;
            auto d_it = data->get_descriptors().find(i);
            if (d_it != data->get_descriptors().end())
            {
                const MemberDescriptor* member_desc = d_it->second;
                if (!member_desc->annotation_is_non_serialized())
                {
                    auto it = data->values().find(i);
                    if (it != data->values().end())
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
#endif
        break;
    }
    case TK_ARRAY:
    {
        uint32_t arraySize = data->type()->get_total_bounds();
        size_t emptyElementSize = getEmptyCdrSerializedSize(data->type()->get_element_type().get(), current_alignment);
        for (uint32_t idx = 0; idx < arraySize; ++idx)
        {
#ifdef DYNAMIC_TYPES_CHECKING
            auto it = data->get_complex_t().find(idx);
            if (it != data->get_complex_t().end())
#else
            auto it = data->values().find(idx);
            if (it != data->values().end())
#endif
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
        for (auto it = data->get_complex_t().begin(); it != data->get_complex_t().end(); ++it)
        {
            // Element Size
            current_alignment += getCdrSerializedSize(it->second, current_alignment);
        }
#else
        for (auto it = data->values().begin(); it != data->values().end(); ++it)
        {
            // Element Size
            current_alignment += getCdrSerializedSize((DynamicData*)it->second, current_alignment);
        }
#endif
        break;
    }
    case TK_ALIAS:
        break;
    }

    return current_alignment - initial_alignment;
}

template<class Serializable>
size_t Serializer<Serializable>::getEmptyCdrSerializedSize(
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
        for (auto it = type->get_member_by_id().begin(); it != type->get_member_by_id().end(); ++it)
        {
            if (!it->second->get_descriptor()->annotation_is_non_serialized())
            {
                current_alignment += getEmptyCdrSerializedSize(it->second->get_descriptor()->get_type().get(), current_alignment);
            }
        }
        break;
    }
    case TK_ARRAY:
    {
        // Elements count
        //current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        // Element size with the maximum size
        current_alignment += type->get_total_bounds() * (getEmptyCdrSerializedSize(type->get_descriptor()->get_element_type().get()));
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
template<class Serializable>
size_t Serializer<Serializable>::getKeyMaxCdrSerializedSize(
    const eprosima::fastrtps::types::DynamicType_ptr type,
    size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    // Structures check the the size of the key for their children
    if (type->get_kind() == TK_STRUCTURE || type->get_kind() == TK_BITSET)
    {
        for (auto it = type->get_member_by_id().begin(); it != type->get_member_by_id().end(); ++it)
        {
            if (it->second->key_annotation())
            {
                current_alignment += getKeyMaxCdrSerializedSize(it->second->get_descriptor()->get_type(), current_alignment);
            }
        }
    }
    else if (type->is_key_defined())
    {
        return getMaxCdrSerializedSize(type, current_alignment);
    }
    return current_alignment - initial_alignment;
}
template<class Serializable>
size_t Serializer<Serializable>::getMaxCdrSerializedSize(
    const eprosima::fastrtps::types::DynamicType_ptr type,
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
        for (auto it = type->get_member_by_id().begin(); it != type->get_member_by_id().end(); ++it)
        {
            temp_size = getMaxCdrSerializedSize(it->second->get_descriptor()->get_type(), current_alignment);
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
        for (auto it = type->get_member_by_id().begin(); it != type->get_member_by_id().end(); ++it)
        {
            if (!it->second->get_descriptor()->annotation_is_non_serialized())
            {
                current_alignment += getMaxCdrSerializedSize(it->second->get_descriptor()->get_type(), current_alignment);
            }
        }
        break;
    }
    case TK_ARRAY:
    {
        // Element size with the maximum size
        current_alignment += type->get_total_bounds() * (getMaxCdrSerializedSize(type->get_descriptor()->get_element_type()));
        break;
    }
    case TK_SEQUENCE:
    {
        // Elements count
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        // Element size with the maximum size
        current_alignment += type->get_total_bounds() * (getMaxCdrSerializedSize(type->get_descriptor()->get_element_type()));
        break;
    }
    case TK_MAP:
    {
        // Elements count
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        // Key Elements size with the maximum size
        current_alignment += type->get_total_bounds() * (getMaxCdrSerializedSize(type->get_descriptor()->get_key_element_type()));

        // Value Elements size with the maximum size
        current_alignment += type->get_total_bounds() * (getMaxCdrSerializedSize(type->get_descriptor()->get_element_type()));
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
template<class Serializable>
void Serializer<Serializable>::serialize(MemberFlag *mf, eprosima::fastcdr::Cdr &cdr) const 
{
    //cdr << m_MemberFlag;
    uint16_t bits = static_cast<uint16_t>(mf->getM_memFlag().to_ulong());
    cdr << bits;
}
template<class Serializable>
void Serializer<Serializable>::deserialize(MemberFlag *mf, eprosima::fastcdr::Cdr &cdr)
{
    //cdr >> (uint16_t)m_MemberFlag;
    uint16_t bits;
    cdr >> bits;
    mf->setM_memFlag(bits);
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const MemberFlag &mf, size_t current_alignment)
{
    return 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
}
template<class Serializable>
void Serializer<Serializable>::serialize(TypeFlag *tf, eprosima::fastcdr::Cdr &cdr) const
{
    //cdr << m_TypeFlag;
    uint16_t bits = static_cast<uint16_t>(tf->getM_typeFlag().to_ulong());
    cdr << bits;
}
template<class Serializable>
void Serializer<Serializable>::deserialize(TypeFlag *tf, eprosima::fastcdr::Cdr &cdr)
{
    //cdr >> (uint16_t)m_TypeFlag;
    uint16_t bits;
    cdr >> bits;
    tf->setM_typeFlag(bits);
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const TypeFlag &tf, size_t current_alignment)
{
    return 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
}


template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const TypeObjectHashId& data, size_t current_alignment)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    switch (data.get_m_d())
    {
        case EK_COMPLETE:
        case EK_MINIMAL:
            current_alignment += ((14) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1); break;
        default:
            break;
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(TypeObjectHashId *tohi, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << tohi->get_m_d() ;
    switch (tohi->get_m_d())
    {
        case EK_COMPLETE:
        case EK_MINIMAL:
            for (int i = 0; i < 14; ++i)
            {
                cdr << tohi->get_m_hash()[i];
            }
            break;
        default:
            break;
    }
}

template<class Serializable>
void Serializer<Serializable>::deserialize(TypeObjectHashId *tohi, eprosima::fastcdr::Cdr &cdr)
{
    uint8_t m_d;
    cdr >> m_d;
				tohi->set_m_d(m_d);

    switch (tohi->get_m_d())
    {
        case EK_COMPLETE:
        case EK_MINIMAL:
            for (int i = 0; i < 14; ++i)
            {
                cdr >> tohi->get_m_hash()[i];
            }
            break;
        default:
            break;
    }
}
//CommonStructMember
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const CommonStructMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += StructMemberFlag::getCdrSerializedSize(data.member_flags(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.member_type_id(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(CommonStructMember *csm, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << csm->member_id();
    cdr << csm->member_flags();
    cdr << csm->member_type_id();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(CommonStructMember *csm, eprosima::fastcdr::Cdr &cdr)
{
    cdr >> csm->member_id();
    cdr >> csm->member_flags();
    cdr >> csm->member_type_id();
}

//CompleteMemberDetail

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const CompleteMemberDetail& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.name().size()  + 1;
    current_alignment += AppliedBuiltinMemberAnnotations::getCdrSerializedSize(data.ann_builtin(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.ann_custom().size(); ++a)
    {
        current_alignment += AppliedAnnotation::getCdrSerializedSize(data.ann_custom().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(CompleteMemberDetail *cmd,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << cmd->name() ;
    cdr << cmd->ann_builtin() ;
    cdr << cmd->ann_custom() ;
}

template<class Serializable>
void Serializer<Serializable>::deserialize(CompleteMemberDetail *cmd,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> cmd->name() ;
    cdr >> cmd->ann_builtin() ;
    cdr >> cmd->ann_custom() ;
}

//MinimalMemberDetai

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const MinimalMemberDetail& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += ((4) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(MinimalMemberDetail *v, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->name_hash();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(MinimalMemberDetail *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->name_hash();
}

//CompleteStructMember
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteStructMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonStructMember::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteStructMember *v, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteStructMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
//MinimalStructMember
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const MinimalStructMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonStructMember::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += MinimalMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(MinimalStructMember *v, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(MinimalStructMember *v, eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const  AppliedBuiltinTypeAnnotations& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AppliedVerbatimAnnotation::getCdrSerializedSize(data.verbatim(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(AppliedBuiltinTypeAnnotations *v, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->verbatim();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(AppliedBuiltinTypeAnnotations *v, eprosima::fastcdr::Cdr &cdr)
{   
    cdr >> v->verbatim();
}
// DA QUI IN POI E' WORK IN PROGRESS, BABY
//====================================================================================================
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const MinimalTypeDetail& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;
    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(MinimalTypeDetail *v, eprosima::fastcdr::Cdr &cdr) const
{
}
template<class Serializable>
void Serializer<Serializable>::deserialize(MinimalTypeDetail *v, eprosima::fastcdr::Cdr &cdr)
{
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const CompleteTypeDetail& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AppliedBuiltinTypeAnnotations::getCdrSerializedSize(data.ann_builtin(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.ann_custom().size(); ++a)
    {
        current_alignment += AppliedAnnotation::getCdrSerializedSize(data.ann_custom().at(a), current_alignment);
    }
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.type_name().size() + 1;

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(CompleteTypeDetail *v, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->ann_builtin();
    cdr << v->ann_custom();
    cdr << v->type_name();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(CompleteTypeDetail *v, eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->ann_builtin();
    cdr >> v->ann_custom();
    cdr >> v->type_name();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteStructHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.base_type(), current_alignment);
    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteStructHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->base_type();
    cdr << v->detail();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteStructHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->base_type();
    cdr >> v->detail();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const MinimalStructHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.base_type(), current_alignment);
    current_alignment += MinimalTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(MinimalStructHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->base_type();
    cdr << v->detail();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(MinimalStructHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->base_type();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteStructType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += StructTypeFlag::getCdrSerializedSize(data.struct_flags(), current_alignment);
    current_alignment += CompleteStructHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.member_seq().size(); ++a)
    {
        current_alignment += CompleteStructMember::getCdrSerializedSize(data.member_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteStructType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->struct_flags();
    cdr << v->header();
    cdr << v->member_seq();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteStructType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->struct_flags();
    cdr >> v->header();
    cdr >> v->member_seq();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalStructType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += StructTypeFlag::getCdrSerializedSize(data.struct_flags(), current_alignment);
    current_alignment += MinimalStructHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.member_seq().size(); ++a)
    {
        current_alignment += MinimalStructMember::getCdrSerializedSize(data.member_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalStructType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->struct_flags();
    cdr << v->header();
    cdr << v->member_seq();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalStructType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr << v->struct_flags();
    cdr << v->header();
    cdr << v->member_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonUnionMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += UnionMemberFlag::getCdrSerializedSize(data.member_flags(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type_id(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.label_seq().size(); ++a)
    {
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonUnionMember *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->member_id();
    cdr << v->member_flags();
    cdr << v->type_id();
    cdr << v->label_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonUnionMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->member_id();
    cdr >> v->member_flags();
    cdr >> v->type_id();
    cdr >> v->label_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteUnionMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonUnionMember::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteUnionMember *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteUnionMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalUnionMember& data, size_t current_alignment/* = 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonUnionMember::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += MinimalMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalUnionMember *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalUnionMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonDiscriminatorMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += UnionDiscriminatorFlag::getCdrSerializedSize(data.member_flags(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type_id(), current_alignment);

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonDiscriminatorMember *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->member_flags();
    cdr << v->type_id();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonDiscriminatorMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->member_flags();
    cdr >> v->type_id();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteDiscriminatorMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonDiscriminatorMember::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += AppliedBuiltinTypeAnnotations::getCdrSerializedSize(data.ann_builtin(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.ann_custom().size(); ++a)
    {
        current_alignment += AppliedAnnotation::getCdrSerializedSize(data.ann_custom().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteDiscriminatorMember *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->ann_builtin();
    cdr << v->ann_custom();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteDiscriminatorMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->ann_builtin();
    cdr >> v->ann_custom();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalDiscriminatorMember& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonDiscriminatorMember::getCdrSerializedSize(data.common(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalDiscriminatorMember *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalDiscriminatorMember *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteUnionHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteUnionHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteUnionHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->detail();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalUnionHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += MinimalTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalUnionHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalUnionHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteUnionType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += UnionTypeFlag::getCdrSerializedSize(data.union_flags(), current_alignment);
    current_alignment += CompleteUnionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += CompleteDiscriminatorMember::getCdrSerializedSize(data.discriminator(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.member_seq().size(); ++a)
    {
        current_alignment += CompleteUnionMember::getCdrSerializedSize(data.member_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}

template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteUnionType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->union_flags();
    cdr << v->header();
    cdr << v->discriminator();
    cdr << v->member_seq();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteUnionType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->union_flags();
    cdr >> v->header();
    cdr >> v->discriminator();
    cdr >> v->member_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalUnionType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += UnionTypeFlag::getCdrSerializedSize(data.union_flags(), current_alignment);
    current_alignment += MinimalUnionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += MinimalDiscriminatorMember::getCdrSerializedSize(data.discriminator(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.member_seq().size(); ++a)
    {
        current_alignment += MinimalUnionMember::getCdrSerializedSize(data.member_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalUnionType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->union_flags();
    cdr << v->header();
    cdr << v->discriminator();
    cdr << v->member_seq();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalUnionType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr << v->union_flags();
    cdr << v->header();
    cdr << v->discriminator();
    cdr << v->member_seq();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonAnnotationParameter& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AnnotationParameterFlag::getCdrSerializedSize(data.member_flags(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.member_type_id(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->member_flags();
    cdr << v->member_type_id();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->member_flags();
    cdr >> v->member_type_id();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteAnnotationParameter& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonAnnotationParameter::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.name().size() + 1;
    current_alignment += AnnotationParameterValue::getCdrSerializedSize(data.default_value(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->name();
    cdr << v->default_value();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->name();
    cdr >> v->default_value();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalAnnotationParameter& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonAnnotationParameter::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.name().size() + 1;
    current_alignment += AnnotationParameterValue::getCdrSerializedSize(data.default_value(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->name();
    cdr << v->default_value();
}

template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->name();
    cdr >> v->default_value();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteAnnotationHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.annotation_name().size() + 1;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteAnnotationHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->annotation_name();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteAnnotationHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->annotation_name();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalAnnotationHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalAnnotationHeader *v,eprosima::fastcdr::Cdr &cdr) const
{}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalAnnotationHeader *v,eprosima::fastcdr::Cdr &cdr)
{}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteAnnotationType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AnnotationTypeFlag::getCdrSerializedSize(data.annotation_flag(), current_alignment);
    current_alignment += CompleteAnnotationHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.member_seq().size(); ++a)
    {
        current_alignment += CompleteAnnotationParameter::getCdrSerializedSize(data.member_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteAnnotationType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->annotation_flag();
    cdr << v->header();
    cdr << v->member_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteAnnotationType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->annotation_flag();
    cdr >> v->header();
    cdr >> v->member_seq();
}    
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalAnnotationType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AnnotationTypeFlag::getCdrSerializedSize(data.annotation_flag(), current_alignment);
    current_alignment += MinimalAnnotationHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.member_seq().size(); ++a)
    {
        current_alignment += MinimalAnnotationParameter::getCdrSerializedSize(data.member_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalAnnotationType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->annotation_flag();
    cdr << v->header();
    cdr << v->member_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalAnnotationType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->annotation_flag();
    cdr << v->header();
    cdr << v->member_seq();
}

/*CommonAliasBody Methods*/
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonAliasBody& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AliasMemberFlag::getCdrSerializedSize(data.related_flags(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.related_type(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonAliasBody *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->related_flags();
    cdr << v->related_type();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonAliasBody *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->related_flags();
    cdr >> v->related_type();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteAliasBody& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonAliasBody::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += AppliedBuiltinMemberAnnotations::getCdrSerializedSize(data.ann_builtin(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.ann_custom().size(); ++a)
    {
        current_alignment += AppliedAnnotation::getCdrSerializedSize(data.ann_custom().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteAliasBody *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->ann_builtin();
    cdr << v->ann_custom();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteAliasBody *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->ann_builtin();
    cdr >> v->ann_custom();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalAliasBody& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonAliasBody::getCdrSerializedSize(data.common(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalAliasBody *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalAliasBody *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteAliasHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteAliasHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteAliasHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalAliasHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalAliasHeader *v,eprosima::fastcdr::Cdr &cdr) const
{}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalAliasHeader *v,eprosima::fastcdr::Cdr &cdr)
{}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteAliasType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AliasTypeFlag::getCdrSerializedSize(data.alias_flags(), current_alignment);
    current_alignment += CompleteAliasHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += CompleteAliasBody::getCdrSerializedSize(data.body(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteAliasType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->alias_flags();
    cdr << v->header();
    cdr << v->body();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteAliasType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->alias_flags();
    cdr >> v->header();
    cdr >> v->body();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalAliasType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AliasTypeFlag::getCdrSerializedSize(data.alias_flags(), current_alignment);
    current_alignment += MinimalAliasHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += MinimalAliasBody::getCdrSerializedSize(data.body(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalAliasType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->alias_flags();
    cdr << v->header();
    cdr << v->body();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalAliasType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->alias_flags();
    cdr >> v->header();
    cdr >> v->body();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteElementDetail& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += AppliedBuiltinMemberAnnotations::getCdrSerializedSize(data.ann_builtin(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.ann_custom().size(); ++a)
    {
        current_alignment += AppliedAnnotation::getCdrSerializedSize(data.ann_custom().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteElementDetail *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->ann_builtin();
    cdr << v->ann_custom();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteElementDetail *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->ann_builtin();
    cdr >> v->ann_custom();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonCollectionElement& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CollectionElementFlag::getCdrSerializedSize(data.element_flags(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonCollectionElement *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->element_flags();
    cdr << v->type();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonCollectionElement *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->element_flags();
    cdr >> v->type();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteCollectionElement& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonCollectionElement::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteElementDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteCollectionElement *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteCollectionElement *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalCollectionElement& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonCollectionElement::getCdrSerializedSize(data.common(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>   
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalCollectionElement *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalCollectionElement *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonCollectionHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + 255  + 1;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonCollectionHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bound();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonCollectionHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bound();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteCollectionHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonCollectionHeader::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteCollectionHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteCollectionHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalCollectionHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonCollectionHeader::getCdrSerializedSize(data.common(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalCollectionHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalCollectionHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteSequenceType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    // FIXED_SIXE current_alignment += ((4) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    current_alignment += CollectionTypeFlag::getCdrSerializedSize(data.collection_flag(), current_alignment);
    current_alignment += CompleteCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += CompleteCollectionElement::getCdrSerializedSize(data.element(), current_alignment);

    // STRING current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.str().size() + 1;
    // SEQUENCE
    /*
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.param_seq().size(); ++a)
    {
        current_alignment += AppliedAnnotationParameter::getCdrSerializedSize(data.param_seq().at(a), current_alignment);
    }
    */

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteSequenceType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->collection_flag();
    cdr << v->header();
    cdr << v->element();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteSequenceType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->collection_flag();
    cdr >> v->header();
    cdr >> v->element();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalSequenceType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    // FIXED_SIXE current_alignment += ((4) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    current_alignment += CollectionTypeFlag::getCdrSerializedSize(data.collection_flag(), current_alignment);
    current_alignment += MinimalCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += MinimalCollectionElement::getCdrSerializedSize(data.element(), current_alignment);

    // STRING current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.str().size() + 1;
    // SEQUENCE
    /*
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.param_seq().size(); ++a)
    {
        current_alignment += AppliedAnnotationParameter::getCdrSerializedSize(data.param_seq().at(a), current_alignment);
    }
    */

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalSequenceType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->collection_flag();
    cdr << v->header();
    cdr << v->element();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalSequenceType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->collection_flag();
    cdr >> v->header();
    cdr >> v->element();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonArrayHeader& data, size_t current_alignment /*= 0*/)
 {
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.bound_seq().size(); ++a)
    {
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonArrayHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bound_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonArrayHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bound_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteArrayHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonArrayHeader::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteArrayHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteArrayHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalArrayHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonArrayHeader::getCdrSerializedSize(data.common(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalArrayHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalArrayHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteArrayType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CollectionTypeFlag::getCdrSerializedSize(data.collection_flag(), current_alignment);
    current_alignment += CompleteArrayHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += CompleteCollectionElement::getCdrSerializedSize(data.element(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteArrayType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->collection_flag();
    cdr << v->header();
    cdr << v->element();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteArrayType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->collection_flag();
    cdr >> v->header();
    cdr >> v->element();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalArrayType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CollectionTypeFlag::getCdrSerializedSize(data.collection_flag(), current_alignment);
    current_alignment += MinimalArrayHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += MinimalCollectionElement::getCdrSerializedSize(data.element(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalArrayType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->collection_flag();
    cdr << v->header();
    cdr << v->element();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalArrayType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->collection_flag();
    cdr >> v->header();
    cdr >> v->element();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteMapType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CollectionTypeFlag::getCdrSerializedSize(data.collection_flag(), current_alignment);
    current_alignment += CompleteCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += CompleteCollectionElement::getCdrSerializedSize(data.key(), current_alignment);
    current_alignment += CompleteCollectionElement::getCdrSerializedSize(data.element(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteMapType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->collection_flag();
    cdr << v->header();
    cdr << v->key();
    cdr << v->element();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteMapType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->collection_flag();
    cdr >> v->header();
    cdr >> v->key();
    cdr >> v->element();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalMapType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CollectionTypeFlag::getCdrSerializedSize(data.collection_flag(), current_alignment);
    current_alignment += MinimalCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += MinimalCollectionElement::getCdrSerializedSize(data.key(), current_alignment);
    current_alignment += MinimalCollectionElement::getCdrSerializedSize(data.element(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalMapType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->collection_flag();
    cdr << v->header();
    cdr << v->key();
    cdr << v->element();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalMapType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->collection_flag();
    cdr >> v->header();
    cdr >> v->key();
    cdr >> v->element();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonEnumeratedLiteral& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += EnumeratedLiteralFlag::getCdrSerializedSize(data.flags(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonEnumeratedLiteral *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->value();
    cdr << v->flags();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonEnumeratedLiteral *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->value();
    cdr >> v->flags();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteEnumeratedLiteral& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonEnumeratedLiteral::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>   
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteEnumeratedLiteral *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteEnumeratedLiteral *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalEnumeratedLiteral& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonEnumeratedLiteral::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += MinimalMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalEnumeratedLiteral *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalEnumeratedLiteral *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonEnumeratedHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonEnumeratedHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bit_bound();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonEnumeratedHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bit_bound();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteEnumeratedHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonEnumeratedHeader::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteEnumeratedHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteEnumeratedHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalEnumeratedHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonEnumeratedHeader::getCdrSerializedSize(data.common(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalEnumeratedHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalEnumeratedHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteEnumeratedType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += EnumTypeFlag::getCdrSerializedSize(data.enum_flags(), current_alignment);
    current_alignment += CompleteEnumeratedHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.literal_seq().size(); ++a)
    {
        current_alignment += CompleteEnumeratedLiteral::getCdrSerializedSize(data.literal_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteEnumeratedType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->enum_flags();
    cdr << v->header();
    cdr << v->literal_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteEnumeratedType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->enum_flags();
    cdr >> v->header();
    cdr >> v->literal_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalEnumeratedType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += EnumTypeFlag::getCdrSerializedSize(data.enum_flags(), current_alignment);
    current_alignment += MinimalEnumeratedHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.literal_seq().size(); ++a)
    {
        current_alignment += MinimalEnumeratedLiteral::getCdrSerializedSize(data.literal_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalEnumeratedType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->enum_flags();
    cdr << v->header();
    cdr << v->literal_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalEnumeratedType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->enum_flags();
    cdr >> v->header();
    cdr >> v->literal_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonBitflag& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
    current_alignment += BitflagFlag::getCdrSerializedSize(data.flags(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonBitflag *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->position();
    cdr << v->flags();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonBitflag *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->position();
    cdr >> v->flags();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteBitflag& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonBitflag::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteBitflag *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteBitflag *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalBitflag& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonBitflag::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += MinimalMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalBitflag *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalBitflag *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonBitmaskHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonBitmaskHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bit_bound();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonBitmaskHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bit_bound();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteBitmaskType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += BitmaskTypeFlag::getCdrSerializedSize(data.bitmask_flags(), current_alignment);
    current_alignment += CompleteBitmaskHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.flag_seq().size(); ++a)
    {
        current_alignment += CompleteBitflag::getCdrSerializedSize(data.flag_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteBitmaskType *v, eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bitmask_flags();
    cdr << v->header();
    cdr << v->flag_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteBitmaskType *v, eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bitmask_flags();
    cdr >> v->header();
    cdr >> v->flag_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalBitmaskType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += BitmaskTypeFlag::getCdrSerializedSize(data.bitmask_flags(), current_alignment);
    current_alignment += MinimalBitmaskHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.flag_seq().size(); ++a)
    {
        current_alignment += MinimalBitflag::getCdrSerializedSize(data.flag_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalBitmaskType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bitmask_flags();
    cdr << v->header();
    cdr << v->flag_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalBitmaskType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bitmask_flags();
    cdr >> v->header();
    cdr >> v->flag_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CommonBitfield& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
    current_alignment += BitsetMemberFlag::getCdrSerializedSize(data.flags(), current_alignment);
    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CommonBitfield *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->position();
    cdr << v->flags();
    cdr << v->bitcount();
    cdr << v->holder_type();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CommonBitfield *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->position();
    cdr >> v->flags();
    cdr >> v->bitcount();
    cdr >> v->holder_type();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteBitfield& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonBitfield::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += CompleteMemberDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteBitfield *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteBitfield *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalBitfield& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += CommonBitfield::getCdrSerializedSize(data.common(), current_alignment);
    current_alignment += ((4) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalBitfield *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->common();
    cdr << v->name_hash();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalBitfield *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->common();
    cdr >> v->name_hash();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteBitsetHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.base_type(), current_alignment);
    current_alignment += CompleteTypeDetail::getCdrSerializedSize(data.detail(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteBitsetHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->base_type();
    cdr << v->detail();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteBitsetHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->base_type();
    cdr >> v->detail();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalBitsetHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.base_type(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalBitsetHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->base_type();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalBitsetHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->base_type();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteBitsetType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += BitsetTypeFlag::getCdrSerializedSize(data.bitset_flags(), current_alignment);
    current_alignment += CompleteBitsetHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.field_seq().size(); ++a)
    {
        current_alignment += CompleteBitfield::getCdrSerializedSize(data.field_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteBitsetType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bitset_flags();
    cdr << v->header();
    cdr << v->field_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteBitsetType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bitset_flags();
    cdr >> v->header();
    cdr >> v->field_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalBitsetType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += BitsetTypeFlag::getCdrSerializedSize(data.bitset_flags(), current_alignment);
    current_alignment += MinimalBitsetHeader::getCdrSerializedSize(data.header(), current_alignment);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.field_seq().size(); ++a)
    {
        current_alignment += MinimalBitfield::getCdrSerializedSize(data.field_seq().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalBitsetType *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bitset_flags();
    cdr << v->header();
    cdr << v->field_seq();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalBitsetType *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bitset_flags();
    cdr >> v->header();
    cdr >> v->field_seq();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteExtendedType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteExtendedType *v,eprosima::fastcdr::Cdr &cdr) const
{}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteExtendedType *v,eprosima::fastcdr::Cdr &cdr)
{}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalExtendedType& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalExtendedType *v,eprosima::fastcdr::Cdr &cdr) const
{}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalExtendedType *v,eprosima::fastcdr::Cdr &cdr)
{}

/*CompleteTypeObject Methods*/
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::CompleteTypeObject& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    switch(data.m__d)
    {
        case TK_ALIAS:
        current_alignment += Serializer(data.alias_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_ANNOTATION:
        current_alignment += Serializer(data.annotation_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_STRUCTURE:
        current_alignment += Serializer(data.struct_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_UNION:
        current_alignment += Serializer(data.union_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_BITSET:
        current_alignment += Serializer(data.bitset_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_SEQUENCE:
        current_alignment += Serializer(data.sequence_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_ARRAY:
        current_alignment += Serializer(data.array_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_MAP:
        current_alignment += Serializer(data.map_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_ENUM:
        current_alignment += Serializer(data.enumerated_type()).getCdrSerializedSize(current_alignment);
        break;
        case TK_BITMASK:
        current_alignment += Serializer(data.bitmask_type()).getCdrSerializedSize(current_alignment);
        break;
        default:
        current_alignment += Serializer(data.extended_type()).getCdrSerializedSize(current_alignment);
        break;
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::CompleteTypeObject *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->_d();

    switch(v->_d)
    {
        case TK_ALIAS:
        cdr << v->alias_type();
        break;
        case TK_ANNOTATION:
        cdr << v->annotation_type();
        break;
        case TK_STRUCTURE:
        cdr << v->struct_type();
        break;
        case TK_UNION:
        cdr << v->union_type();
        break;
        case TK_BITSET:
        cdr << v->bitset_type();
        break;
        case TK_SEQUENCE:
        cdr << v->sequence_type();
        break;
        case TK_ARRAY:
        cdr << v->array_type();
        break;
        case TK_MAP:
        cdr << v->map_type();
        break;
        case TK_ENUM:
        cdr << v->enumerated_type();
        break;
        case TK_BITMASK:
        cdr << v->bitmask_type();
        break;
        default:
        cdr << v->extended_type();
        break;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::CompleteTypeObject *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->_d();

    switch(v->_d())
    {
        case TK_ALIAS:
        cdr >> v->alias_type();
        break;
        case TK_ANNOTATION:
        cdr >> v->annotation_type();
        break;
        case TK_STRUCTURE:
        cdr >> v->struct_type();
        break;
        case TK_UNION:
        cdr >> v->union_type();
        break;
        case TK_BITSET:
        cdr >> v->bitset_type();
        break;
        case TK_SEQUENCE:
        cdr >> v->sequence_type();
        break;
        case TK_ARRAY:
        cdr >> v->array_type();
        break;
        case TK_MAP:
        cdr >> v->map_type();
        break;
        case TK_ENUM:
        cdr >> v->enumerated_type();
        break;
        case TK_BITMASK:
        cdr >> v->bitmask_type();
        break;
        default:
        cdr >> v->extended_type();
        break;
    }
}

template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::MinimalTypeObject& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    switch(data._d())
    {
        case TK_ALIAS:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.alias_type(), current_alignment);
        break;
        case TK_ANNOTATION:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.annotation_type(), current_alignment);
        break;
        case TK_STRUCTURE:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.struct_type(), current_alignment);
        break;
        case TK_UNION:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.union_type(), current_alignment);
        break;
        case TK_BITSET:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.bitset_type(), current_alignment);
        break;
        case TK_SEQUENCE:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.sequence_type(), current_alignment);
        break;
        case TK_ARRAY:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.array_type(), current_alignment);
        break;
        case TK_MAP:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.map_type(), current_alignment);
        break;
        case TK_ENUM:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.enumerated_type(), current_alignment);
        break;
        case TK_BITMASK:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.bitmask_type(), current_alignment);
        break;
        default:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.extended_type(), current_alignment);
        break;
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::MinimalTypeObject *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->_d();

    switch(v->_d())
    {
        case TK_ALIAS:
        cdr << v->alias_type();
        break;
        case TK_ANNOTATION:
        cdr << v->annotation_type();
        break;
        case TK_STRUCTURE:
        cdr << v->struct_type();
        break;
        case TK_UNION:
        cdr << v->union_type();
        break;
        case TK_BITSET:
        cdr << v->bitset_type();
        break;
        case TK_SEQUENCE:
        cdr << v->sequence_type();
        break;
        case TK_ARRAY:
        cdr << v->array_type();
        break;
        case TK_MAP:
        cdr << v->map_type();
        break;
        case TK_ENUM:
        cdr << v->enumerated_type();
        break;
        case TK_BITMASK:
        cdr << v->bitmask_type();
        break;
        default:
        cdr << v->extended_type();
        break;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::MinimalTypeObject *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->_d();

    switch(v->_d())
    {
        case TK_ALIAS:
        cdr >> v->alias_type();
        break;
        case TK_ANNOTATION:
        cdr >> v->annotation_type();
        break;
        case TK_STRUCTURE:
        cdr >> v->struct_type();
        break;
        case TK_UNION:
        cdr >> v->union_type();
        break;
        case TK_BITSET:
        cdr >> v->bitset_type();
        break;
        case TK_SEQUENCE:
        cdr >> v->sequence_type();
        break;
        case TK_ARRAY:
        cdr >> v->array_type();
        break;
        case TK_MAP:
        cdr >> v->map_type();
        break;
        case TK_ENUM:
        cdr >> v->enumerated_type();
        break;
        case TK_BITMASK:
        cdr >> v->bitmask_type();
        break;
        default:
        cdr >> v->extended_type();
        break;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeObject& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    switch(data.m__d)
    {
        case EK_COMPLETE:
        current_alignment += CompleteTypeObject::getCdrSerializedSize(data.complete(), current_alignment);
        break;
        case EK_MINIMAL:
        current_alignment += MinimalTypeObject::getCdrSerializedSize(data.minimal(), current_alignment);
        break;
        default:
        break;
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeObject *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->_d();

    switch(v->_d())
    {
        case EK_COMPLETE:
        cdr << v->complete();
        break;
        case EK_MINIMAL:
        cdr << v->minimal();
        break;
        default:
        break;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeObject *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->_d();

    switch(v->_d())
    {
        case EK_COMPLETE:
        cdr >> v->complete();
        break;
        case EK_MINIMAL:
        cdr >> v->minimal();
        break;
        default:
        break;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeIdentifierTypeObjectPair& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type_identifier(), current_alignment);
    current_alignment += TypeObject::getCdrSerializedSize(data.type_object(), current_alignment);

    return current_alignment - initial_alignment;
}    
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeIdentifierTypeObjectPair *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->type_identifier();
    cdr << v->type_object();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeIdentifierTypeObjectPair *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->type_identifier();
    cdr >> v->type_object();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeIdentifierPair& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type_identifier1(), current_alignment);
    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type_identifier2(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeIdentifierPair *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->type_identifier1();
    cdr << v->type_identifier2();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeIdentifierPair *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->type_identifier1();
    cdr >> v->type_identifier2();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeIdentifierWithSize& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifier::getCdrSerializedSize(data.type_id(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeIdentifierWithSize *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->type_id();
    cdr << v->typeobject_serialized_size();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeIdentifierWithSize *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->type_id();
    cdr >> v->typeobject_serialized_size();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeIdentifierWithDependencies& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifierWithSize::getCdrSerializedSize(data.typeid_with_size(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    for(size_t a = 0; a < data.dependent_typeids().size(); ++a)
    {
        current_alignment += TypeIdentifierWithSize::getCdrSerializedSize(data.dependent_typeids().at(a), current_alignment);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeIdentifierWithDependencies *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->typeid_with_size();
    cdr << v->dependent_typeid_count();
    cdr << v->dependent_typeids();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeIdentifierWithDependencies *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->typeid_with_size();
    cdr >> v->dependent_typeid_count();
    cdr >> v->dependent_typeids();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeInformation& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += TypeIdentifierWithDependencies::getCdrSerializedSize(data.minimal(), current_alignment);
    current_alignment += TypeIdentifierWithDependencies::getCdrSerializedSize(data.complete(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeInformation *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->minimal();
    cdr << v->complete();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeInformation *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->minimal();
    cdr >> v->complete();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::StringSTypeDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::StringSTypeDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bound();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::StringSTypeDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bound();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::StringLTypeDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    return current_alignment - initial_alignment;
}
template<class Serializable>   
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::StringLTypeDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->bound();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::StringLTypeDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->bound();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainCollectionHeader& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainCollectionHeader *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->equiv_kind();
    cdr << v->element_flags();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainCollectionHeader *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->equiv_kind();
    cdr >> v->element_flags();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainSequenceSElemDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += PlainCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    if (data.element_identifier() != nullptr)
    {
        size_t size = TypeIdentifier::getCdrSerializedSize(*data.element_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainSequenceSElemDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->header();
    cdr << v->bound();
    if (v->element_identifier() == nullptr)
    {
        TypeIdentifier emptyId();
        cdr << emptyId;
    }
    else
    {
        cdr << *v->element_identifier();
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainSequenceSElemDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->header();
    cdr >> v->bound();
    if (v->element_identifier() == nullptr)
    {
        TypeIdentifier emptyId();
        cdr >> emptyId;
    }
    else
    {
        cdr >> *v->element_identifier();
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainSequenceLElemDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += PlainCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    if (data.element_identifier() != nullptr)
    {
        size_t size = TypeIdentifier::getCdrSerializedSize(*data.element_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainSequenceLElemDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->header();
    cdr << v->bound();
    if (v->element_identifier() != nullptr)
    {
        cdr << *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainSequenceLElemDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->header();
    cdr >> v->bound();
    if (v->element_identifier() != nullptr)
    {
        cdr >> *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr >> emptyId;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainArraySElemDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += PlainCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += (data.array_bound_seq().size() * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);


    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    if (data.element_identifier() != nullptr)
    {
        size_t size = TypeIdentifier::getCdrSerializedSize(*data.element_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainArraySElemDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->header();
    cdr << v->array_bound_seq();

    if (v->element_identifier() != nullptr)
    {
        cdr << *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainArraySElemDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->header();
    cdr >> v->array_bound_seq();

    if (v->element_identifier() != nullptr)
    {
        cdr >> *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr >> emptyId;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainArrayLElemDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += PlainCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += (data.array_bound_seq().size() * 4) + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    if (data.element_identifier() != nullptr)
    {
        size_t size = TypeIdentifier::getCdrSerializedSize(*data.element_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainArrayLElemDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->header();
    cdr << v->array_bound_seq();
    if (v->element_identifier() != nullptr)
    {
        cdr << *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainArrayLElemDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->header();
    cdr >> v->array_bound_seq();
    if (v->element_identifier() != nullptr)
    {
        cdr >> *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr >> emptyId;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainMapSTypeDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += PlainCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    size_t size = 0;
    if (data.element_identifier() != nullptr)
    {
        size = TypeIdentifier::getCdrSerializedSize(*data.element_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    if (data.key_identifier() != nullptr)
    {
        size = TypeIdentifier::getCdrSerializedSize(*data.key_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainMapSTypeDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->header();
    cdr << v->bound();
    if (v->element_identifier() != nullptr)
    {
        cdr << *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
    cdr << v->key_flags();
    if (v->key_identifier() != nullptr)
    {
        cdr << *v->key_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainMapSTypeDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->header();
    cdr >> v->bound();
    if (v->element_identifier() == nullptr)
    {
        v->element_identifier() = new TypeIdentifier();
    }
    cdr >> *v->element_identifier();
    cdr >> v->key_flags();
    if (v->key_identifier() == nullptr)
    {
        v->key_identifier() = new TypeIdentifier();
    }
    cdr >> *v->key_identifier();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::PlainMapLTypeDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += PlainCollectionHeader::getCdrSerializedSize(data.header(), current_alignment);
    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    size_t size = 0;
    if (data.element_identifier() != nullptr)
    {
        size = TypeIdentifier::getCdrSerializedSize(*data.element_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }

    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

    //current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
    if (data.key_identifier() != nullptr)
    {
        size = TypeIdentifier::getCdrSerializedSize(*data.key_identifier(), current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }
    else
    {
        TypeIdentifier emptyId;
        size_t size = TypeIdentifier::getCdrSerializedSize(emptyId, current_alignment);
        current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);
    }


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::PlainMapLTypeDefn *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->header();
    cdr << v->bound();
    if (v->element_identifier() != nullptr)
    {
        cdr << *v->element_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
    cdr << v->key_flags();
    if (v->key_identifier() != nullptr)
    {
        cdr << *v->key_identifier();
    }
    else
    {
        TypeIdentifier emptyId;
        cdr << emptyId;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::PlainMapLTypeDefn *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->header();
    cdr >> v->bound();
    if (v->element_identifier() == nullptr)
    {
        v->element_identifier() = new TypeIdentifier();
    }
    cdr >> *v->element_identifier();
    cdr >> v->key_flags();
    if (v->key_identifier() == nullptr)
    {
        v->key_identifier() = new TypeIdentifier();
    }
    cdr >> *v->key_identifier();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::StronglyConnectedComponentId& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    //current_alignment += ((14) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    size_t size = TypeObjectHashId::getCdrSerializedSize(data.sc_component_id(), current_alignment);
    current_alignment += size + eprosima::fastcdr::Cdr::alignment(current_alignment, size);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::StronglyConnectedComponentId *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->sc_component_id();
    cdr << v->scc_length();
    cdr << v->scc_index();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::StronglyConnectedComponentId *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->sc_component_id();
    cdr >> v->scc_length();
    cdr >> v->scc_index();
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::ExtendedTypeDefn& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::ExtendedTypeDefn *v,eprosima::fastcdr::Cdr &cdr) const
{}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::ExtendedTypeDefn *v,eprosima::fastcdr::Cdr &cdr)
{}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::TypeIdentifier& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    switch(data._d())
    {
        case TI_STRING8_SMALL:
        case TI_STRING16_SMALL:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.string_sdefn(), current_alignment);

        break;
        case TI_STRING8_LARGE:
        case TI_STRING16_LARGE:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.string_ldefn(), current_alignment);

        break;
        case TI_PLAIN_SEQUENCE_SMALL:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.seq_sdefn(), current_alignment);

        break;
        case TI_PLAIN_SEQUENCE_LARGE:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.seq_ldefn(), current_alignment);

        break;
        case TI_PLAIN_ARRAY_SMALL:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.array_sdefn(), current_alignment);

        break;
        case TI_PLAIN_ARRAY_LARGE:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.array_ldefn(), current_alignment);

        break;
        case TI_PLAIN_MAP_SMALL:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.map_sdefn(), current_alignment);

        break;
        case TI_PLAIN_MAP_LARGE:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.map_ldefn(), current_alignment);

        break;
        case TI_STRONGLY_CONNECTED_COMPONENT:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.sc_component_id(), current_alignment);

        break;
        case EK_COMPLETE:
        case EK_MINIMAL:
        current_alignment += 14 + eprosima::fastcdr::Cdr::alignment(current_alignment, 14);

        break;
        default:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.extended_defn(), current_alignment);
        break;
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::TypeIdentifier *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->_d();

    switch(v->_d())
    {
        case TI_STRING8_SMALL:
        case TI_STRING16_SMALL:
        cdr << v->string_sdefn();
        break;
        case TI_STRING8_LARGE:
        case TI_STRING16_LARGE:
        cdr << v->string_ldefn();
        break;
        case TI_PLAIN_SEQUENCE_SMALL:
        cdr << v->seq_sdefn();
        break;
        case TI_PLAIN_SEQUENCE_LARGE:
        cdr << v->seq_ldefn();
        break;
        case TI_PLAIN_ARRAY_SMALL:
        cdr << v->array_sdefn();
        break;
        case TI_PLAIN_ARRAY_LARGE:
        cdr << v->array_ldefn();
        break;
        case TI_PLAIN_MAP_SMALL:
        cdr << v->map_sdefn();
        break;
        case TI_PLAIN_MAP_LARGE:
        cdr << v->map_ldefn();
        break;
        case TI_STRONGLY_CONNECTED_COMPONENT:
        cdr << v->sc_component_id();
        break;
        case EK_COMPLETE:
        case EK_MINIMAL:
        for (int i = 0; i < 14; ++i)
        {
            cdr << v->equivalence_hash()[i];
        }
        break;
        default:
        cdr << v->extended_defn();
        break;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::TypeIdentifier *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->_d();

    switch(v->_d())
    {
        case TI_STRING8_SMALL:
        case TI_STRING16_SMALL:
        cdr >> v->string_sdefn();
        break;
        case TI_STRING8_LARGE:
        case TI_STRING16_LARGE:
        cdr >> v->string_ldefn();
        break;
        case TI_PLAIN_SEQUENCE_SMALL:
        cdr >> v->seq_sdefn();
        break;
        case TI_PLAIN_SEQUENCE_LARGE:
        cdr >> v->seq_ldefn();
        break;
        case TI_PLAIN_ARRAY_SMALL:
        cdr >> v->array_sdefn();
        break;
        case TI_PLAIN_ARRAY_LARGE:
        cdr >> v->array_ldefn();
        break;
        case TI_PLAIN_MAP_SMALL:
        cdr >> v->map_sdefn();
        break;
        case TI_PLAIN_MAP_LARGE:
        cdr >> v->map_ldefn();
        break;
        case TI_STRONGLY_CONNECTED_COMPONENT:
        cdr >> v->sc_component_id();
        break;
        case EK_COMPLETE:
        case EK_MINIMAL:
        for (int i = 0; i < 14; ++i)
        {
            cdr >> v->equivalence_hash()[i];
        }
        break;
        default:
        cdr >> v->extended_defn();
        break;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::ExtendedAnnotationParameterValue& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;


    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::ExtendedAnnotationParameterValue *v,eprosima::fastcdr::Cdr &cdr) const
{}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::ExtendedAnnotationParameterValue *v,eprosima::fastcdr::Cdr &cdr)
{}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::AnnotationParameterValue& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

    switch(data.m__d)
    {
        case TK_BOOLEAN:
        current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

        break;
        case TK_BYTE:
        current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

        break;
        case TK_INT16:
        current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

        break;
        case TK_UINT16:
        current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);

        break;
        case TK_INT32:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        break;
        case TK_UINT32:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        break;
        case TK_INT64:
        current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);

        break;
        case TK_UINT64:
        current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);

        break;
        case TK_FLOAT32:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        break;
        case TK_FLOAT64:
        current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);

        break;
        case TK_FLOAT128:
        current_alignment += 16 + eprosima::fastcdr::Cdr::alignment(current_alignment, 16);

        break;
        case TK_CHAR8:
        current_alignment += 1 + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);

        break;
        case TK_CHAR16:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        break;
        case TK_ENUM:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        break;
        case TK_STRING8:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.string8_value().size() + 1;
        break;
        case TK_STRING16:
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + data.string16_value().size() + 1;
        break;
        default:
        current_alignment += Serializer<Serializable>::getCdrSerializedSize(data.extended_value(), current_alignment);
        break;
    }

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::AnnotationParameterValue *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->_d();

    switch(v->_d())
    {
        case TK_BOOLEAN:
        cdr << v->boolean_value();
        break;
        case TK_BYTE:
        cdr << v->byte_value();
        break;
        case TK_INT16:
        cdr << v->int16_value();
        break;
        case TK_UINT16:
        cdr << v->uint_16_value();
        break;
        case TK_INT32:
        cdr << v->int32_value();
        break;
        case TK_UINT32:
        cdr << v->uint32_value();
        break;
        case TK_INT64:
        cdr << v->int64_value();
        break;
        case TK_UINT64:
        cdr << v->uint64_value();
        break;
        case TK_FLOAT32:
        cdr << v->float32_value();
        break;
        case TK_FLOAT64:
        cdr << v->float64_value();
        break;
        case TK_FLOAT128:
        cdr << v->float128_value();
        break;
        case TK_CHAR8:
        cdr << v->char_value();
        break;
        case TK_CHAR16:
        cdr << v->wchar_value();
        break;
        case TK_ENUM:
        cdr << v->enumerated_value();
        break;
        case TK_STRING8:
        cdr << v->string8_value();
        break;
        case TK_STRING16:
        cdr << v->string16_value();
        break;
        default:
        cdr << v->extended_value();
        break;
    }
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::AnnotationParameterValue *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->_d();

    switch(v->_d())
    {
        case TK_BOOLEAN:
        cdr >> v->boolean_value();
        break;
        case TK_BYTE:
        cdr >> v->byte_value();
        break;
        case TK_INT16:
        cdr >> v->int16_value();
        break;
        case TK_UINT16:
        cdr >> v->uint_16_value();
        break;
        case TK_INT32:
        cdr >> v->int32_value();
        break;
        case TK_UINT32:
        cdr >> v->uint32_value();
        break;
        case TK_INT64:
        cdr >> v->int64_value();
        break;
        case TK_UINT64:
        cdr >> v->uint64_value();
        break;
        case TK_FLOAT32:
        cdr >> v->float32_value();
        break;
        case TK_FLOAT64:
        cdr >> v->float64_value();
        break;
        case TK_FLOAT128:
        cdr >> v->float128_value();
        break;
        case TK_CHAR8:
        cdr >> v->char_value();
        break;
        case TK_CHAR16:
        cdr >> v->wchar_value();
        break;
        case TK_ENUM:
        cdr >> v->enumerated_value();
        break;
        case TK_STRING8:
        cdr >> v->string8_value();
        break;
        case TK_STRING16:
        cdr >> v->string16_value();
        break;
        default:
        cdr >> v->extended_value();
        break;
    }
}
template<class Serializable>
size_t Serializer<Serializable>::getCdrSerializedSize(const eprosima::fastrtps::types::AppliedAnnotationParameter& data, size_t current_alignment /*= 0*/)
{
    size_t initial_alignment = current_alignment;

    current_alignment += ((4) * 1) + eprosima::fastcdr::Cdr::alignment(current_alignment, 1);
    current_alignment += AnnotationParameterValue::getCdrSerializedSize(data.value(), current_alignment);

    return current_alignment - initial_alignment;
}
template<class Serializable>
void Serializer<Serializable>::serialize(eprosima::fastrtps::types::AppliedAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr) const
{
    cdr << v->paramname_hash();
    cdr << v->value();
}
template<class Serializable>
void Serializer<Serializable>::deserialize(eprosima::fastrtps::types::AppliedAnnotationParameter *v,eprosima::fastcdr::Cdr &cdr)
{
    cdr >> v->paramname_hash();
    cdr >> v->value();
}


} //namespace serializer
} //namespace types
} //namespace eprosima
