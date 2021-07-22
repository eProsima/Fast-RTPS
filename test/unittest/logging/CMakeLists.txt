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

add_subdirectory(log_macros)

if(NOT ((MSVC OR MSVC_IDE) AND EPROSIMA_INSTALLER))
    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    set(LOG_COMMON_SOURCE
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        )

    set(LOGTESTS_TEST_SOURCE LogTests.cpp)

    set(LOGTESTS_SOURCE
        ${LOG_COMMON_SOURCE}
        ${LOGTESTS_TEST_SOURCE})

    include_directories(mock/)

    add_executable(LogTests ${LOGTESTS_SOURCE})
    target_compile_definitions(LogTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(LogTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(LogTests GTest::gtest ${MOCKS}
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        )
    add_gtest(LogTests SOURCES ${LOGTESTS_TEST_SOURCE})

    set(LOGFILETESTS_TEST_SOURCE LogFileTests.cpp)

    set(LOGFILETESTS_SOURCE
        ${LOG_COMMON_SOURCE}
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/FileConsumer.cpp
        ${LOGFILETESTS_TEST_SOURCE})

    # External sources
    if(TINYXML2_SOURCE_DIR)
        list(APPEND LOGFILETESTS_SOURCE
            ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
            )
    endif()

    include_directories(${TINYXML2_INCLUDE_DIR})

    add_executable(LogFileTests ${LOGFILETESTS_SOURCE})
    target_compile_definitions(LogFileTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(LogFileTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(LogFileTests GTest::gtest ${MOCKS}
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        ${TINYXML2_LIBRARY}
        fastcdr
        )
    add_gtest(LogFileTests SOURCES ${LOGFILETESTS_TEST_SOURCE})
endif()
