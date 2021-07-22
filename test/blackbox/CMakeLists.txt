# Copyright 2016, 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
macro(add_blackbox_gtest)
    # Parse arguments
    if("${ARGV0}" STREQUAL "NAME")
        set(uniValueArgs NAME COMMAND)
        unset(test)
        unset(command)
    else()
        set(test "${ARGV0}")
        set(command "${test}")
    endif()
    set(multiValueArgs SOURCES ENVIRONMENTS DEPENDENCIES LABELS)
    cmake_parse_arguments(GTEST "" "${uniValueArgs}" "${multiValueArgs}" ${ARGN})

    if(GTEST_NAME)
        set(test ${GTEST_NAME})
        set(command ${GTEST_COMMAND})
    endif()

    if(GTEST_INDIVIDUAL)
        if(WIN32)
            set(WIN_PATH "$ENV{PATH}")
            get_target_property(LINK_LIBRARIES_ ${command} LINK_LIBRARIES)
            if(NOT "${LINK_LIBRARIES_}" STREQUAL "LINK_LIBRARIES_-NOTFOUND")
                foreach(LIBRARY_LINKED ${LINK_LIBRARIES_})
                    if(TARGET ${LIBRARY_LINKED})
                        set(WIN_PATH "$<TARGET_FILE_DIR:${LIBRARY_LINKED}>;${WIN_PATH}")
                    endif()
                endforeach()
            endif()
            foreach(DEP ${GTEST_DEPENDENCIES})
                set(WIN_PATH "$<TARGET_FILE_DIR:${DEP}>;${WIN_PATH}")
            endforeach()
            string(REPLACE ";" "\\;" WIN_PATH "${WIN_PATH}")
        endif()

        foreach(GTEST_SOURCE_FILE ${GTEST_SOURCES})
            file(STRINGS ${GTEST_SOURCE_FILE} GTEST_TEST_NAMES REGEX "^TEST(\\(|_F)" )
            foreach(GTEST_TEST_NAME ${GTEST_TEST_NAMES})
                string(REGEX REPLACE ["\) \(,"] ";" GTEST_TEST_NAME ${GTEST_TEST_NAME})
                list(GET GTEST_TEST_NAME 1 GTEST_GROUP_NAME)
                list(GET GTEST_TEST_NAME 3 GTEST_TEST_NAME)
                add_test(NAME ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}
                    COMMAND ${command} --gtest_filter=${GTEST_GROUP_NAME}.${GTEST_TEST_NAME})

                # Add environment
                if(WIN32)
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME} APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH}")
                endif()

                foreach(property ${GTEST_ENVIRONMENTS})
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME} APPEND PROPERTY ENVIRONMENT "${property}")
                endforeach()

                # Add labels
                set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME} PROPERTY LABELS "${GTEST_LABELS}")

            endforeach()
            file(STRINGS ${GTEST_SOURCE_FILE} GTEST_TEST_NAMES REGEX "^TEST_P" )
            foreach(GTEST_TEST_NAME ${GTEST_TEST_NAMES})
                string(REGEX REPLACE ["\) \(,"] ";" GTEST_TEST_NAME ${GTEST_TEST_NAME})
                list(GET GTEST_TEST_NAME 1 GTEST_GROUP_NAME)
                list(GET GTEST_TEST_NAME 3 GTEST_TEST_NAME)

                add_test(NAME ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Transport
                    COMMAND ${command} --gtest_filter=*/${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}/Transport*)

                # Add environment
                if(WIN32)
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Transport APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH}")
                endif()

                foreach(property ${GTEST_ENVIRONMENTS})
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Transport APPEND PROPERTY ENVIRONMENT "${property}")
                endforeach()

                # Add labels
                set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Transport PROPERTY LABELS "${GTEST_LABELS}")

                add_test(NAME ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Intraprocess
                    COMMAND ${command} --gtest_filter=*/${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}/Intraprocess*)

                # Add environment
                if(WIN32)
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Intraprocess APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH}")
                endif()

                foreach(property ${GTEST_ENVIRONMENTS})
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Intraprocess APPEND PROPERTY ENVIRONMENT "${property}")
                endforeach()

                # Add labels
                set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Intraprocess PROPERTY LABELS "${GTEST_LABELS}")

                if(${test} MATCHES ".*_DDS_PIM$")
                    add_test(NAME ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Datasharing
                        COMMAND ${command} --gtest_filter=*/${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}/Datasharing*)

                    # Add environment
                    if(WIN32)
                        set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Datasharing APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH}")
                    endif()

                    foreach(property ${GTEST_ENVIRONMENTS})
                        set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Datasharing APPEND PROPERTY ENVIRONMENT "${property}")
                    endforeach()

                    # Add labels
                    set_property(TEST ${test}.${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}.Datasharing PROPERTY LABELS "${GTEST_LABELS}")
                endif()

            endforeach()
        endforeach()
    else()
        add_test(NAME ${test} COMMAND ${command})

        # Add environment
        if(WIN32)
            set(WIN_PATH "$ENV{PATH}")
            get_target_property(LINK_LIBRARIES_ ${command} LINK_LIBRARIES)
            if(NOT "${LINK_LIBRARIES_}" STREQUAL "LINK_LIBRARIES_-NOTFOUND")
                foreach(LIBRARY_LINKED ${LINK_LIBRARIES_})
                    if(TARGET ${LIBRARY_LINKED})
                        set(WIN_PATH "$<TARGET_FILE_DIR:${LIBRARY_LINKED}>;${WIN_PATH}")
                    endif()
                endforeach()
            endif()
            foreach(DEP ${GTEST_DEPENDENCIES})
                set(WIN_PATH "$<TARGET_FILE_DIR:${DEP}>;${WIN_PATH}")
            endforeach()
            string(REPLACE ";" "\\;" WIN_PATH "${WIN_PATH}")
            set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH}")
        endif()

        foreach(property ${GTEST_ENVIRONMENTS})
            set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "${property}")
        endforeach()

        # Add labels
        set_property(TEST ${test} PROPERTY LABELS "${GTEST_LABELS}")
    endif()
endmacro()

option(RTPS_API_TESTS "Enable tests using RTPS API" ON)
option(FASTRTPS_API_TESTS "Enable tests using FastRTPS API" ON)
option(FASTDDS_PIM_API_TESTS "Enable tests using FastDDS API" OFF)

if(NOT ((MSVC OR MSVC_IDE) AND EPROSIMA_INSTALLER) AND fastcdr_FOUND)
    if(WIN32)
        add_definitions(
            -D_WIN32_WINNT=0x0601
            -D_CRT_SECURE_NO_WARNINGS
            )
    endif()

    ###############################################################################
    # Unit tests
    ###############################################################################
    file(GLOB RTPS_BLACKBOXTESTS_TEST_SOURCE "common/RTPSBlackboxTests*.cpp")
    set(RTPS_BLACKBOXTESTS_SOURCE ${RTPS_BLACKBOXTESTS_TEST_SOURCE}
        types/HelloWorld.cpp
        types/HelloWorldType.cpp
        types/KeyedHelloWorld.cpp
        types/KeyedHelloWorldType.cpp
        types/String.cpp
        types/StringType.cpp
        types/Data64kb.cpp
        types/Data64kbType.cpp
        types/Data1mb.cpp
        types/Data1mbType.cpp
        types/FixedSized.cpp
        types/FixedSizedType.cpp

        utils/data_generators.cpp
        utils/lambda_functions.cpp
        utils/print_functions.cpp
        )
    add_executable(BlackboxTests_RTPS ${RTPS_BLACKBOXTESTS_SOURCE})
    target_compile_definitions(BlackboxTests_RTPS PRIVATE
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<NOT:$<BOOL:${IS_THIRDPARTY_BOOST_SUPPORTED}>>:FASTDDS_SHM_TRANSPORT_DISABLED> # Do not compile SHM Transport
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(BlackboxTests_RTPS PRIVATE
        ${Asio_INCLUDE_DIR})
    target_link_libraries(BlackboxTests_RTPS fastrtps fastcdr foonathan_memory GTest::gtest)
    add_blackbox_gtest(BlackboxTests_RTPS SOURCES ${RTPS_BLACKBOXTESTS_TEST_SOURCE})

    file(GLOB BLACKBOXTESTS_TEST_SOURCE "common/BlackboxTests*.cpp")
    set(BLACKBOXTESTS_SOURCE ${BLACKBOXTESTS_TEST_SOURCE}
        types/HelloWorld.cpp
        types/HelloWorldType.cpp
        types/KeyedHelloWorld.cpp
        types/KeyedHelloWorldType.cpp
        types/String.cpp
        types/StringType.cpp
        types/Data64kb.cpp
        types/Data64kbType.cpp
        types/Data1mb.cpp
        types/Data1mbType.cpp
        types/FixedSized.cpp
        types/FixedSizedType.cpp

        utils/data_generators.cpp
        utils/lambda_functions.cpp
        utils/print_functions.cpp

        common/TCPReqRepHelloWorldRequester.cpp
        common/TCPReqRepHelloWorldReplier.cpp
        )

    file(GLOB DDS_BLACKBOXTESTS_TEST_SOURCE "common/DDSBlackboxTests*.cpp")
    set(DDS_BLACKBOXTESTS_SOURCE
        ${DDS_BLACKBOXTESTS_TEST_SOURCE}
        ${BLACKBOXTESTS_SOURCE}
        )

    # Prepare static discovery xml file for blackbox tests.
    string(RANDOM LENGTH 4 ALPHABET 0123456789 TOPIC_RANDOM_NUMBER)
    math(EXPR TOPIC_RANDOM_NUMBER "${TOPIC_RANDOM_NUMBER} + 0") # Remove extra leading 0s.
    string(RANDOM LENGTH 4 ALPHABET 0123456789 W_UNICAST_PORT_RANDOM_NUMBER)
    math(EXPR W_UNICAST_PORT_RANDOM_NUMBER "${W_UNICAST_PORT_RANDOM_NUMBER} + 0") # Remove extra leading 0s.
    if(W_UNICAST_PORT_RANDOM_NUMBER LESS 1025)
        math(EXPR W_UNICAST_PORT_RANDOM_NUMBER "${W_UNICAST_PORT_RANDOM_NUMBER} + 1024") # Remove extra leading 0s.
    endif()
    math(EXPR R_UNICAST_PORT_RANDOM_NUMBER "${W_UNICAST_PORT_RANDOM_NUMBER} + 1")
    math(EXPR MULTICAST_PORT_RANDOM_NUMBER "${R_UNICAST_PORT_RANDOM_NUMBER} + 1")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PubSubWriter.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/PubSubWriter.xml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PubSubReader.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/PubSubReader.xml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/discovery_participant_flags.xml
        ${CMAKE_CURRENT_BINARY_DIR}/discovery_participant_flags.xml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/persistence.xml
        ${CMAKE_CURRENT_BINARY_DIR}/persistence.xml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/StatisticsDomainParticipant.xml
        ${CMAKE_CURRENT_BINARY_DIR}/StatisticsDomainParticipant.xml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/utils/check_guid.py
        ${CMAKE_CURRENT_BINARY_DIR}/check_guid.py)

    if(FASTRTPS_API_TESTS)
        set(BLACKBOXTESTS_FASTRTPS_SOURCE
            ${BLACKBOXTESTS_SOURCE}
            api/fastrtps_deprecated/ReqRepHelloWorldRequester.cpp
            api/fastrtps_deprecated/ReqRepHelloWorldReplier.cpp
            )

        add_executable(BlackboxTests_FastRTPS ${BLACKBOXTESTS_FASTRTPS_SOURCE})
        target_compile_definitions(BlackboxTests_FastRTPS PRIVATE
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            $<$<NOT:$<BOOL:${IS_THIRDPARTY_BOOST_SUPPORTED}>>:FASTDDS_SHM_TRANSPORT_DISABLED> # Do not compile SHM Transport
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(BlackboxTests_FastRTPS PRIVATE
            ${Asio_INCLUDE_DIR}
            api/fastrtps_deprecated)
        target_link_libraries(BlackboxTests_FastRTPS fastrtps fastcdr foonathan_memory GTest::gtest)
        add_blackbox_gtest(BlackboxTests_FastRTPS SOURCES ${BLACKBOXTESTS_TEST_SOURCE}
            ENVIRONMENTS "CERTS_PATH=${PROJECT_SOURCE_DIR}/test/certs"
            "TOPIC_RANDOM_NUMBER=${TOPIC_RANDOM_NUMBER}"
            "W_UNICAST_PORT_RANDOM_NUMBER=${W_UNICAST_PORT_RANDOM_NUMBER}"
            "R_UNICAST_PORT_RANDOM_NUMBER=${R_UNICAST_PORT_RANDOM_NUMBER}"
            "MULTICAST_PORT_RANDOM_NUMBER=${MULTICAST_PORT_RANDOM_NUMBER}"
            )
    endif(FASTRTPS_API_TESTS)

    if(FASTDDS_PIM_API_TESTS)
        set(BLACKBOXTESTS_FASTDDS_PIM_SOURCE
            ${DDS_BLACKBOXTESTS_SOURCE}
            api/dds-pim/ReqRepHelloWorldRequester.cpp
            api/dds-pim/ReqRepHelloWorldReplier.cpp
            )

        if (FASTDDS_STATISTICS)

            set(statistics_sources
                ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/types.cxx
                ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/typesPubSubTypes.cxx
                )

            list(APPEND BLACKBOXTESTS_FASTDDS_PIM_SOURCE ${statistics_sources})

        endif()

        add_executable(BlackboxTests_DDS_PIM ${BLACKBOXTESTS_FASTDDS_PIM_SOURCE})
        target_compile_definitions(BlackboxTests_DDS_PIM PRIVATE
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            $<$<NOT:$<BOOL:${IS_THIRDPARTY_BOOST_SUPPORTED}>>:FASTDDS_SHM_TRANSPORT_DISABLED> # Do not compile SHM Transport
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(BlackboxTests_DDS_PIM PRIVATE
            ${Asio_INCLUDE_DIR}
            api/dds-pim)
        target_link_libraries(BlackboxTests_DDS_PIM fastrtps fastcdr foonathan_memory GTest::gtest)
        add_blackbox_gtest(BlackboxTests_DDS_PIM SOURCES ${DDS_BLACKBOXTESTS_SOURCE}
            ENVIRONMENTS "CERTS_PATH=${PROJECT_SOURCE_DIR}/test/certs"
            "TOPIC_RANDOM_NUMBER=${TOPIC_RANDOM_NUMBER}"
            "W_UNICAST_PORT_RANDOM_NUMBER=${W_UNICAST_PORT_RANDOM_NUMBER}"
            "R_UNICAST_PORT_RANDOM_NUMBER=${R_UNICAST_PORT_RANDOM_NUMBER}"
            "MULTICAST_PORT_RANDOM_NUMBER=${MULTICAST_PORT_RANDOM_NUMBER}"
            )
    endif(FASTDDS_PIM_API_TESTS)

    # Add 'xfail' label to flaky tests
    set(BLACKBOX_XFAIL_LIST XFAIL_RTPS)
    if(FASTRTPS_API_TESTS)
        set(BLACKBOX_XFAIL_LIST ${BLACKBOX_XFAIL_LIST} XFAIL_FASTRTPS)
    endif()
    if(FASTDDS_PIM_API_TESTS)
        set(BLACKBOX_XFAIL_LIST ${BLACKBOX_XFAIL_LIST} XFAIL_DDS_PIM)
    endif()

    foreach(BLACKBOX_XFAIL_TEST ${BLACKBOX_XFAIL_LIST})
        add_xfail_label(${CMAKE_CURRENT_SOURCE_DIR}/${BLACKBOX_XFAIL_TEST}.list)
        if(SECURITY)
            add_xfail_label(${CMAKE_CURRENT_SOURCE_DIR}/${BLACKBOX_XFAIL_TEST}_SECURITY.list)
        endif()
    endforeach()
endif()
