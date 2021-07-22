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

option(GTEST_INDIVIDUAL "Activate the execution of GTest tests" OFF)

macro(add_gtest)
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
                        # Check if is a real target or a target interface
                        get_target_property(type ${LIBRARY_LINKED} TYPE)
                        if(NOT type STREQUAL "INTERFACE_LIBRARY")
                            set(WIN_PATH "$<TARGET_FILE_DIR:${LIBRARY_LINKED}>;${WIN_PATH}")
                        endif()
                        unset(type)
                    endif()
                endforeach()
            endif()
            foreach(DEP ${GTEST_DEPENDENCIES})
                set(WIN_PATH "$<TARGET_FILE_DIR:${DEP}>;${WIN_PATH}")
            endforeach()
            string(REPLACE ";" "\\;" WIN_PATH "${WIN_PATH}")

        endif()

        foreach(GTEST_SOURCE_FILE ${GTEST_SOURCES})
            # Normal tests
            file(STRINGS ${GTEST_SOURCE_FILE} GTEST_TEST_NAMES REGEX "^([T][Y][P][E][D][_])?TEST")
            foreach(GTEST_TEST_NAME ${GTEST_TEST_NAMES})
                string(REGEX REPLACE ["\) \(,"] ";" GTEST_TEST_NAME ${GTEST_TEST_NAME})
                list(GET GTEST_TEST_NAME 1 GTEST_GROUP_NAME)
                list(GET GTEST_TEST_NAME 3 GTEST_TEST_NAME)
                add_test(NAME ${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}
                    COMMAND ${command}
                    --gtest_filter=${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}:*/${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}/*:${GTEST_GROUP_NAME}/*.${GTEST_TEST_NAME})

                # Add environment
                set(GTEST_ENVIRONMENT "")
                if(WIN32)
                    set(GTEST_ENVIRONMENT "PATH=${WIN_PATH}")
                endif()

                foreach(property ${GTEST_ENVIRONMENTS})
                    list(APPEND GTEST_ENVIRONMENT "${property}")
                endforeach()

                if(GTEST_ENVIRONMENT)
                    set_tests_properties(${GTEST_GROUP_NAME}.${GTEST_TEST_NAME}
                        PROPERTIES ENVIRONMENT "${GTEST_ENVIRONMENT}")
                endif()
                unset(GTEST_ENVIRONMENT)

                # Add labels
                set_property(TEST ${GTEST_GROUP_NAME}.${GTEST_TEST_NAME} PROPERTY LABELS "${GTEST_LABELS}")

            endforeach()

        endforeach()
    else()
        add_test(NAME ${test} COMMAND ${command})

        # Add environment
        set(GTEST_ENVIRONMENT "")
        if(WIN32)
            set(WIN_PATH "$ENV{PATH}")
            get_target_property(LINK_LIBRARIES_ ${command} LINK_LIBRARIES)
            if(NOT "${LINK_LIBRARIES_}" STREQUAL "LINK_LIBRARIES_-NOTFOUND")
                foreach(LIBRARY_LINKED ${LINK_LIBRARIES_})
                    if(TARGET ${LIBRARY_LINKED})
                        # Check if is a real target or a target interface
                        get_target_property(type ${LIBRARY_LINKED} TYPE)
                        if(NOT type STREQUAL "INTERFACE_LIBRARY")
                            set(WIN_PATH "$<TARGET_FILE_DIR:${LIBRARY_LINKED}>;${WIN_PATH}")
                        endif()
                        unset(type)
                    endif()
                endforeach()
            endif()
            foreach(DEP ${GTEST_DEPENDENCIES})
                set(WIN_PATH "$<TARGET_FILE_DIR:${DEP}>;${WIN_PATH}")
            endforeach()
            string(REPLACE ";" "\\;" WIN_PATH "${WIN_PATH}")

            set(GTEST_ENVIRONMENT "PATH=${WIN_PATH}")

        endif()

        foreach(property ${GTEST_ENVIRONMENTS})
            list(APPEND GTEST_ENVIRONMENT "${property}")
        endforeach()

        if(GTEST_ENVIRONMENT)
            set_tests_properties(${test}
                PROPERTIES ENVIRONMENT "${GTEST_ENVIRONMENT}")
        endif()
        unset(GTEST_ENVIRONMENT)

        # Add labels
        set_property(TEST ${test} PROPERTY LABELS "${GTEST_LABELS}")
    endif()
endmacro()

macro(add_xfail_label LIST_FILE)
    if(GTEST_INDIVIDUAL AND EXISTS ${LIST_FILE})
        file(STRINGS ${LIST_FILE} TEST_LIST)
        foreach(XFAIL_TEST ${TEST_LIST})
            set_property(TEST ${XFAIL_TEST} PROPERTY LABELS xfail)
        endforeach()
    endif()
endmacro()
