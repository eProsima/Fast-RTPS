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
    set(READERHISTORYTESTS_SOURCE ReaderHistoryTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/ReaderHistory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/History.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/CacheChangePool.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp)

    set(BASICPOOLSTESTS_SOURCE BasicPoolsTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/CacheChangePool.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp)

    set(CACHECHANGEPOOLTESTS_SOURCE CacheChangePoolTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/CacheChangePool.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp)

    set(TOPICPAYLOADPOOLTESTS_SOURCE
        TopicPayloadPoolTests.cpp TopicPayloadPoolRegistryTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/TopicPayloadPool.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/TopicPayloadPoolRegistry.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp)

    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    add_executable(ReaderHistoryTests ${READERHISTORYTESTS_SOURCE})
    target_compile_definitions(ReaderHistoryTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(ReaderHistoryTests PRIVATE
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/Endpoint
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSReader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatefulReader
        ${PROJECT_SOURCE_DIR}/src/cpp
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(ReaderHistoryTests
        GTest::gmock
        ${CMAKE_DL_LIBS})
    add_gtest(ReaderHistoryTests SOURCES ${READERHISTORYTESTS_SOURCE})

    add_executable(BasicPoolsTests ${BASICPOOLSTESTS_SOURCE})
    target_compile_definitions(BasicPoolsTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(BasicPoolsTests PRIVATE
        ${PROJECT_SOURCE_DIR}/src/cpp
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(BasicPoolsTests
        GTest::gtest
        ${CMAKE_DL_LIBS})
    add_gtest(BasicPoolsTests SOURCES ${BASICPOOLSTESTS_SOURCE})

    add_executable(CacheChangePoolTests ${CACHECHANGEPOOLTESTS_SOURCE})
    target_compile_definitions(CacheChangePoolTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(CacheChangePoolTests PRIVATE
        ${PROJECT_SOURCE_DIR}/src/cpp
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(CacheChangePoolTests
        GTest::gtest
        ${CMAKE_DL_LIBS})
    add_gtest(CacheChangePoolTests SOURCES ${CACHECHANGEPOOLTESTS_SOURCE})

    add_executable(TopicPayloadPoolTests ${TOPICPAYLOADPOOLTESTS_SOURCE})
    target_compile_definitions(TopicPayloadPoolTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(TopicPayloadPoolTests PRIVATE
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TopicPayloadPoolProxy
        ${PROJECT_SOURCE_DIR}/src/cpp
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
    target_link_libraries(TopicPayloadPoolTests
        GTest::gtest
        ${CMAKE_DL_LIBS})
    add_gtest(TopicPayloadPoolTests SOURCES ${TOPICPAYLOADPOOLTESTS_SOURCE})
endif()
