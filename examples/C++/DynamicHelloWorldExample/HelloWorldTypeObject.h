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
 * @file HelloWorldTypeObject.h
 * This header file contains the declaration of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifndef _HELLOWORLD_TYPE_OBJECT_H_
#define _HELLOWORLD_TYPE_OBJECT_H_


#include <fastrtps/types/TypeObject.h>
#include <map>

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#define eProsima_user_DllExport __declspec( dllexport )
#else
#define eProsima_user_DllExport
#endif
#else
#define eProsima_user_DllExport
#endif

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#if defined(HelloWorld_SOURCE)
#define HelloWorld_DllAPI __declspec( dllexport )
#else
#define HelloWorld_DllAPI __declspec( dllimport )
#endif // HelloWorld_SOURCE
#else
#define HelloWorld_DllAPI
#endif
#else
#define HelloWorld_DllAPI
#endif // _WIN32

using namespace eprosima::fastrtps::types;

class HelloWorldTypeFactory
{
public:
    HelloWorldTypeFactory();
    ~HelloWorldTypeFactory();

    eProsima_user_DllExport void registerTypes();

    eProsima_user_DllExport EquivalenceKind getEquivalenceKind(const std::string &type_name) const;

    eProsima_user_DllExport TypeIdentifier* tryCreateTypeIdentifier(const std::string &type_name);

    eProsima_user_DllExport TypeIdentifier* getTypeIdentifier(const std::string &basic_type_name) const;

    eProsima_user_DllExport TypeIdentifier* getStringIdentifier(uint32_t bound, bool wide = false);

    eProsima_user_DllExport TypeIdentifier* getSequenceIdentifier(const std::string &type_name, uint32_t bound);

    eProsima_user_DllExport TypeIdentifier* getArrayIdentifier(const std::string &type_name, const std::vector<uint32_t> &bound);

    /** Dimensions must be separated by a single space */
    eProsima_user_DllExport TypeIdentifier* getArrayIdentifier(const std::string &type_name, const std::string &bound);

    eProsima_user_DllExport TypeIdentifier* getMapIdentifier(const std::string &key_type_name,
        const std::string &value_type_name, uint32_t bound);

    eProsima_user_DllExport TypeIdentifier* getHelloWorldIdentifier();
    eProsima_user_DllExport TypeObject* getHelloWorldObject();


private:
    std::map<std::string, TypeIdentifier*> m_Identifiers;
    std::map<std::string, TypeObject*> m_Objects;
    std::map<std::string, std::string> m_Aliases;

    std::string getStringTypeName(uint32_t bound, bool wide, bool generate_identifier = true);

    std::string getSequenceTypeName(const std::string &type_name, uint32_t bound, bool generate_identifier = true);

    std::string getArrayTypeName(const std::string &type_name, const std::string &bound,
        bool generate_identifier = true);

    std::string getArrayTypeName(const std::string &type_name, const std::vector<uint32_t> &bound,
        bool generate_identifier = true);

    std::string getArrayTypeName(const std::string &type_name, const std::vector<uint32_t> &bound, uint32_t &ret_size,
        bool generate_identifier = true);

    std::string getMapTypeName(const std::string &key_type_name, const std::string &value_type_name, uint32_t bound,
        bool generate_identifier = true);
};

#endif // _HELLOWORLD_TYPE_OBJECT_H_