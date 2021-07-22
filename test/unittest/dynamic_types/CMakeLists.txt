# Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(NOT ((MSVC OR MSVC_IDE) AND EPROSIMA_INSTALLER))
    if(WIN32)
        add_definitions(
            -D_WIN32_WINNT=0x0601
            -D_CRT_SECURE_NO_WARNINGS
            )
    endif()

    set(DYNAMIC_TYPES_SOURCE
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/AnnotationDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicData.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicDataFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicType.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicPubSubType.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypePtr.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicDataPtr.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilderPtr.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilderFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeMember.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/MemberDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/AnnotationParameterValue.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeIdentifier.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeIdentifierTypes.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObject.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObjectFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObjectHashId.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeNamesGenerator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypesBase.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/BuiltinAnnotationsTypeObject.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/string_convert.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/FileConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    # External sources
    if(TINYXML2_SOURCE_DIR)
        list(APPEND DYNAMIC_TYPES_SOURCE
            ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
            )
    endif()

    set(DYNAMIC_TYPES_TEST_SOURCE
        DynamicTypesTests.cpp
        idl/Basic.cxx
        idl/BasicPubSubTypes.cxx
        idl/BasicTypeObject.cxx
        ${DYNAMIC_TYPES_SOURCE}

        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLProfileManager.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLDynamicParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLElementParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLParserCommon.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/WriterQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/ReaderQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/FlowControllerConsts.cpp
        )

    set(DYNAMIC_COMPLEX_TYPES_TEST_SOURCE
        DynamicComplexTypesTests.cpp
        idl/Test.cxx
        idl/TestPubSubTypes.cxx
        idl/TestTypeObject.cxx
        ${DYNAMIC_TYPES_SOURCE}
        )

    set(DYNAMIC_TYPES_4_2_TEST_SOURCE
        DynamicTypes_4_2_Tests.cpp
        idl/new_features_4_2.cxx
        idl/new_features_4_2PubSubTypes.cxx
        idl/new_features_4_2TypeObject.cxx
        ${DYNAMIC_TYPES_SOURCE}
        )

    include_directories(mock/)

    add_executable(DynamicTypesTests ${DYNAMIC_TYPES_TEST_SOURCE})
    target_compile_definitions(DynamicTypesTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(DynamicTypesTests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TCPTransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TCPv4TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TCPv6TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/UDPTransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/UDPv4TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/UDPv6TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/SharedMemTransportDescriptor
        ${TINYXML2_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        )
    target_link_libraries(DynamicTypesTests GTest::gtest
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        $<$<BOOL:${WIN32}>:ws2_32>
        ${TINYXML2_LIBRARY}
        fastcdr
        )
    add_gtest(DynamicTypesTests SOURCES ${DYNAMIC_TYPES_TEST_SOURCE})


    add_executable(DynamicComplexTypesTests ${DYNAMIC_COMPLEX_TYPES_TEST_SOURCE})
    target_compile_definitions(DynamicComplexTypesTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(DynamicComplexTypesTests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(DynamicComplexTypesTests GTest::gtest
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        $<$<BOOL:${WIN32}>:ws2_32>
        ${TINYXML2_LIBRARY}
        fastcdr
        )
    add_gtest(DynamicComplexTypesTests SOURCES ${DYNAMIC_COMPLEX_TYPES_TEST_SOURCE})


    add_executable(DynamicTypes_4_2_Tests ${DYNAMIC_TYPES_4_2_TEST_SOURCE})
    target_compile_definitions(DynamicTypes_4_2_Tests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(DynamicTypes_4_2_Tests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(DynamicTypes_4_2_Tests GTest::gtest
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        $<$<BOOL:${WIN32}>:ws2_32>
        ${TINYXML2_LIBRARY}
        fastcdr
        )
    add_gtest(DynamicTypes_4_2_Tests SOURCES ${DYNAMIC_TYPES_4_2_TEST_SOURCE})

endif()

configure_file("types.xml" "types.xml" COPYONLY)
