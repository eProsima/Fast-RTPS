# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

    set(COMMON_SOURCES_ACCESS_CONTROL_TEST_SOURCE
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/attributes/PropertyPolicy.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/ParameterList.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/ResourceEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEventImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Token.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/exceptions/Exception.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/exceptions/SecurityException.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/common/SharedSecretHandle.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp//utils/TimedConditionVariable.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/logging/Logging.cpp
        )

    if(TINYXML2_SOURCE_DIR)
        list(APPEND COMMON_SOURCES_ACCESS_CONTROL_TEST_SOURCE
            ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
            )
    endif()

    include_directories(${TINYXML2_INCLUDE_DIR})

    add_executable(AccessControlTests ${COMMON_SOURCES_ACCESS_CONTROL_TEST_SOURCE}
        ${PROJECT_SOURCE_DIR}/src/cpp/security/authentication/PKIDH.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/authentication/PKIIdentityHandle.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/authentication/PKIHandshakeHandle.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/AccessPermissionsHandle.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/CommonParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/GovernanceParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/Permissions.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/PermissionsParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/StringMatching.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/WriterQos.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AccessControlTests.cpp)

    target_compile_definitions(AccessControlTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )

    target_include_directories(AccessControlTests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/NetworkFactory/
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/PDPSimple
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/EDP
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ParticipantProxyData
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/WriterProxyData/
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReaderProxyData/
        ${PROJECT_SOURCE_DIR}/test/mock/dds/QosPolicies
        ${OPENSSL_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )

    target_link_libraries(AccessControlTests
        GTest::gmock
        ${OPENSSL_LIBRARIES}
        fastcdr
        foonathan_memory
        $<$<BOOL:${WIN32}>:ws2_32>
        ${TINYXML2_LIBRARY}
        )

    if(MSVC OR MSVC_IDE)
        target_link_libraries(AccessControlTests ${PRIVACY} iphlpapi Shlwapi)
    endif()


    add_gtest(AccessControlTests
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/AccessControlTests.cpp
        ENVIRONMENTS "CERTS_PATH=${PROJECT_SOURCE_DIR}/test/certs")
endif()
