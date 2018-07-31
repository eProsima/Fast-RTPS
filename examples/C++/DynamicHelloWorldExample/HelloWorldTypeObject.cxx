// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/*! 
 * @file HelloWorldTypeObject.cpp
 * This source file contains the definition of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace { char dummy; }
#endif

#include "HelloWorld.h"
#include "HelloWorldTypeObject.h"
#include <utility>
#include <sstream>
#include <fastrtps/rtps/common/SerializedPayload.h>
#include <fastrtps/utils/md5.h>
#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/TypeNamesGenerator.h>
#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>

using namespace eprosima::fastrtps::rtps;

HelloWorldTypeFactory::HelloWorldTypeFactory()
{
    registerTypes();
}

HelloWorldTypeFactory::~HelloWorldTypeFactory()
{
}

void HelloWorldTypeFactory::registerTypes()
{
    TypeObjectFactory *factory = TypeObjectFactory::GetInstance();
    factory->AddTypeObject("HelloWorld", GetHelloWorldIdentifier(true), GetHelloWorldObject(true));
    factory->AddTypeObject("HelloWorld", GetHelloWorldIdentifier(false), GetHelloWorldObject(false));
}

const TypeIdentifier* HelloWorldTypeFactory::GetTypeIdentifier(const std::string &type_name, bool complete)
{
    // Try general factory
    const TypeIdentifier *type_id = (complete)
            ? TypeObjectFactory::GetInstance()->GetTypeIdentifierTryingComplete(type_name)
            : TypeObjectFactory::GetInstance()->GetTypeIdentifier(type_name, false);
    if (type_id == nullptr) // For basic types, it's ok to accept non-complete
    {
        // Try users types.
        if (type_name == "HelloWorld") return GetHelloWorldIdentifier(complete);
    }
    else
    {
        return type_id;
    }
    return nullptr;
}

const TypeObject* HelloWorldTypeFactory::GetTypeObject(const std::string &type_name, bool complete)
{
    // Try general factory
    const TypeObject *type_id = TypeObjectFactory::GetInstance()->GetTypeObject(type_name, complete);
    if (type_id == nullptr || (complete && type_id->_d() == EK_MINIMAL))
    {
        // Try users types.
        if (type_name == "HelloWorld")
        {
            GetHelloWorldIdentifier(complete);
            return GetTypeObject("HelloWorld", complete);
        }
    }

    return type_id;
}

const TypeIdentifier* HelloWorldTypeFactory::GetHelloWorldIdentifier(bool complete)
{
    const TypeIdentifier * c_identifier = GetTypeIdentifier("HelloWorld", complete);
    if (c_identifier != nullptr && (!complete || c_identifier->_d() == EK_COMPLETE))
    {
        return c_identifier;
    }

    GetHelloWorldObject(complete); // Generated inside
    return GetTypeIdentifier("HelloWorld", complete);
}

const TypeObject* HelloWorldTypeFactory::GetHelloWorldObject(bool complete)
{
    const TypeObject* c_type_object = TypeObjectFactory::GetInstance()->GetTypeObject("HelloWorld", complete);
    if (c_type_object != nullptr)
    {
        return c_type_object;
    }
    else if (complete)
    {
        return GetCompleteHelloWorldObject();
    }
    else
    {
        return GetMinimalHelloWorldObject();
    }
}

const TypeObject* HelloWorldTypeFactory::GetMinimalHelloWorldObject()
{
    const TypeObject* c_type_object = TypeObjectFactory::GetInstance()->GetTypeObject("HelloWorld", false);
    if (c_type_object != nullptr)
    {
        return c_type_object;
    }

    TypeObject *type_object = new TypeObject();
    type_object->_d(EK_MINIMAL);
    type_object->minimal()._d(TK_STRUCTURE);

    type_object->minimal().struct_type().struct_flags().IS_FINAL(false);
    type_object->minimal().struct_type().struct_flags().IS_APPENDABLE(false);
    type_object->minimal().struct_type().struct_flags().IS_MUTABLE(false);
    type_object->minimal().struct_type().struct_flags().IS_NESTED(false);
    type_object->minimal().struct_type().struct_flags().IS_AUTOID_HASH(false);

    MemberId memberId = 0;
    MinimalStructMember mst_index;
    mst_index.common().member_id(memberId++);
    mst_index.common().member_flags().TRY_CONSTRUCT1(false);
    mst_index.common().member_flags().TRY_CONSTRUCT2(false);
    mst_index.common().member_flags().IS_EXTERNAL(false);
    mst_index.common().member_flags().IS_OPTIONAL(false);
    mst_index.common().member_flags().IS_MUST_UNDERSTAND(false);
    mst_index.common().member_flags().IS_KEY(false);
    mst_index.common().member_flags().IS_DEFAULT(false);
    {
        std::string cppType = "uint32_t";
        if (cppType == "long double")
        {
            cppType = "longdouble";
        }
        mst_index.common().member_type_id(*GetTypeIdentifier(cppType, false));
    }

    MD5 index_hash("index");
    for(int i = 0; i < 4; ++i)
    {
        mst_index.detail().name_hash()[i] = index_hash.digest[i];
    }
    type_object->minimal().struct_type().member_seq().emplace_back(mst_index);

    MinimalStructMember mst_message;
    mst_message.common().member_id(memberId++);
    mst_message.common().member_flags().TRY_CONSTRUCT1(false);
    mst_message.common().member_flags().TRY_CONSTRUCT2(false);
    mst_message.common().member_flags().IS_EXTERNAL(false);
    mst_message.common().member_flags().IS_OPTIONAL(false);
    mst_message.common().member_flags().IS_MUST_UNDERSTAND(false);
    mst_message.common().member_flags().IS_KEY(false);
    mst_message.common().member_flags().IS_DEFAULT(false);
    mst_message.common().member_type_id(*TypeObjectFactory::GetInstance()->GetStringIdentifier(255, false));


    MD5 message_hash("message");
    for(int i = 0; i < 4; ++i)
    {
        mst_message.detail().name_hash()[i] = message_hash.digest[i];
    }
    type_object->minimal().struct_type().member_seq().emplace_back(mst_message);


    // Header
    // TODO Inheritance
    //type_object->minimal().struct_type().header().base_type()._d(EK_MINIMAL);
    //type_object->minimal().struct_type().header().base_type().equivalence_hash()[0..13];

    TypeIdentifier identifier;
    identifier._d(EK_MINIMAL);

    SerializedPayload_t payload(static_cast<uint32_t>(
        MinimalStructType::getCdrSerializedSize(type_object->minimal().struct_type()) + 4));
    eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
    // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
    eprosima::fastcdr::Cdr ser(
        fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
        eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
    payload.encapsulation = CDR_LE;

    type_object->serialize(ser);
    payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
    MD5 objectHash;
    objectHash.update((char*)payload.data, payload.length);
    objectHash.finalize();
    for(int i = 0; i < 14; ++i)
    {
        identifier.equivalence_hash()[i] = objectHash.digest[i];
    }

    TypeObjectFactory::GetInstance()->AddTypeObject("HelloWorld", &identifier, type_object);
    delete type_object;
    return GetTypeObject("HelloWorld", false);
}

const TypeObject* HelloWorldTypeFactory::GetCompleteHelloWorldObject()
{
    const TypeObject* c_type_object = TypeObjectFactory::GetInstance()->GetTypeObject("HelloWorld", true);
    if (c_type_object != nullptr && c_type_object->_d() == EK_COMPLETE)
    {
        return c_type_object;
    }

    TypeObject *type_object = new TypeObject();
    type_object->_d(EK_COMPLETE);
    type_object->complete()._d(TK_STRUCTURE);

    type_object->complete().struct_type().struct_flags().IS_FINAL(false);
    type_object->complete().struct_type().struct_flags().IS_APPENDABLE(false);
    type_object->complete().struct_type().struct_flags().IS_MUTABLE(false);
    type_object->complete().struct_type().struct_flags().IS_NESTED(false);
    type_object->complete().struct_type().struct_flags().IS_AUTOID_HASH(false);

    MemberId memberId = 0;
    CompleteStructMember cst_index;
    cst_index.common().member_id(memberId++);
    cst_index.common().member_flags().TRY_CONSTRUCT1(false);
    cst_index.common().member_flags().TRY_CONSTRUCT2(false);
    cst_index.common().member_flags().IS_EXTERNAL(false);
    cst_index.common().member_flags().IS_OPTIONAL(false);
    cst_index.common().member_flags().IS_MUST_UNDERSTAND(false);
    cst_index.common().member_flags().IS_KEY(false);
    cst_index.common().member_flags().IS_DEFAULT(false);
    {
        std::string cppType = "uint32_t";
        if (cppType == "long double")
        {
            cppType = "longdouble";
        }
        cst_index.common().member_type_id(*GetTypeIdentifier(cppType, false));
    }

    cst_index.detail().name("index");
    //cst_index.detail().ann_builtin()...
    //cst_index.detail().ann_custom()...
    type_object->complete().struct_type().member_seq().emplace_back(cst_index);

    CompleteStructMember cst_message;
    cst_message.common().member_id(memberId++);
    cst_message.common().member_flags().TRY_CONSTRUCT1(false);
    cst_message.common().member_flags().TRY_CONSTRUCT2(false);
    cst_message.common().member_flags().IS_EXTERNAL(false);
    cst_message.common().member_flags().IS_OPTIONAL(false);
    cst_message.common().member_flags().IS_MUST_UNDERSTAND(false);
    cst_message.common().member_flags().IS_KEY(false);
    cst_message.common().member_flags().IS_DEFAULT(false);
    cst_message.common().member_type_id(*TypeObjectFactory::GetInstance()->GetStringIdentifier(255, false));


    cst_message.detail().name("message");
    //cst_message.detail().ann_builtin()...
    //cst_message.detail().ann_custom()...
    type_object->complete().struct_type().member_seq().emplace_back(cst_message);


    // Header
    type_object->complete().struct_type().header().detail().type_name("HelloWorld");
    //type_object->complete().struct_type().header().detail().ann_builtin()...
    //type_object->complete().struct_type().header().detail().ann_custom()...
    // TODO inheritance
    //type_object->complete().struct_type().header().base_type()._d(EK_COMPLETE);
    //type_object->complete().struct_type().header().base_type().equivalence_hash()[0..13];

    TypeIdentifier identifier;
    identifier._d(EK_COMPLETE);

    SerializedPayload_t payload(static_cast<uint32_t>(
        CompleteStructType::getCdrSerializedSize(type_object->complete().struct_type()) + 4));
    eprosima::fastcdr::FastBuffer fastbuffer((char*) payload.data, payload.max_size);
    // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
    eprosima::fastcdr::Cdr ser(
        fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
        eprosima::fastcdr::Cdr::DDS_CDR); // Object that serializes the data.
    payload.encapsulation = CDR_LE;

    type_object->serialize(ser);
    payload.length = (uint32_t)ser.getSerializedDataLength(); //Get the serialized length
    MD5 objectHash;
    objectHash.update((char*)payload.data, payload.length);
    objectHash.finalize();
    for(int i = 0; i < 14; ++i)
    {
        identifier.equivalence_hash()[i] = objectHash.digest[i];
    }

    TypeObjectFactory::GetInstance()->AddTypeObject("HelloWorld", &identifier, type_object);
    delete type_object;
    return GetTypeObject("HelloWorld", true);
}
