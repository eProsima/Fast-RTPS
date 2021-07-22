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

option(PERFORMANCE_TESTS "Activate the building and execution of performance tests" OFF)
option(SYSTEM_TESTS "Activate the building and execution of system tests" OFF)
option(PROFILING_TESTS "Activate the building and execution of profiling tests" OFF)
option(EPROSIMA_BUILD_TESTS "Activate the building and execution unit tests and integral tests" OFF)

if(EPROSIMA_BUILD AND NOT EPROSIMA_INSTALLER AND NOT EPROSIMA_INSTALLER_MINION)
    set(EPROSIMA_BUILD_TESTS ON)
    set(SYSTEM_TESTS ON)
endif()

file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}/valgrind.supp" MEMORYCHECK_SUPPRESSIONS_FILE_TMP)
set(MEMORYCHECK_SUPPRESSIONS_FILE ${MEMORYCHECK_SUPPRESSIONS_FILE_TMP} CACHE FILEPATH "Valgrind suppresions file")

###############################################################################
# Testing
###############################################################################
if(EPROSIMA_BUILD_TESTS AND IS_TOP_LEVEL AND NOT EPROSIMA_INSTALLER)
    find_package(GTest CONFIG REQUIRED)
    include(${PROJECT_SOURCE_DIR}/cmake/common/gtest.cmake)

    add_subdirectory(blackbox)
    add_subdirectory(communication)
    add_subdirectory(unittest)
    add_subdirectory(xtypes)
    add_subdirectory(dds/communication)

    if(UNIX AND NOT APPLE AND STRICT_REALTIME)
        add_subdirectory(realtime)
    endif()
endif()

###############################################################################
# Profiling tests using valgrind
###############################################################################
if(NOT ((MSVC OR MSVC_IDE)) AND PROFILING_TESTS)
   find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
   if(CTEST_MEMORYCHECK_COMMAND)
      add_subdirectory(profiling)
   endif()
endif()

###############################################################################
# Performance tests
###############################################################################
if(PERFORMANCE_TESTS)
    add_subdirectory(performance)
endif()

###############################################################################
# System tests
###############################################################################
if(SYSTEM_TESTS)
    add_subdirectory(system/tools/fastdds)
    add_subdirectory(system/tools/fds)
endif()
