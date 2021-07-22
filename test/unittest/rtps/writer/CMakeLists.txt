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
    # ReaderProxy
    set(WRITERPROXYTESTS_SOURCE ReaderProxyTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/ReaderProxy.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/ReaderQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/LocatorSelectorSender.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/FlowControllerConsts.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp
        )

    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    add_executable(ReaderProxyTests ${WRITERPROXYTESTS_SOURCE})
    target_compile_definitions(ReaderProxyTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(ReaderProxyTests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/Endpoint
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/WriterHistory
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatefulWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatelessWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReaderProxyData
        ${PROJECT_SOURCE_DIR}/test/mock/dds/QosPolicies
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReaderLocator
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSGapBuilder
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TimedEvent
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        ${THIRDPARTY_BOOST_INCLUDE_DIR}
        )
    target_link_libraries(ReaderProxyTests
        GTest::gmock foonathan_memory
        ${CMAKE_DL_LIBS}
        ${THIRDPARTY_BOOST_LINK_LIBS})
    add_gtest(ReaderProxyTests SOURCES ${WRITERPROXYTESTS_SOURCE})

    # LivelinessManager

    set(LIVELINESSMANAGERTESTS_SOURCE LivelinessManagerTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/LivelinessManager.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/ResourceEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEventImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/TimedConditionVariable.cpp
        )
    add_executable(LivelinessManagerTests ${LIVELINESSMANAGERTESTS_SOURCE})
    target_compile_definitions(LivelinessManagerTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(LivelinessManagerTests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(LivelinessManagerTests PRIVATE
        GTest::gmock)
    add_gtest(LivelinessManagerTests SOURCES ${LIVELINESSMANAGERTESTS_SOURCE})

    # RTPSWriter

    set(RTPSWRITERTESTS_SOURCE RTPSWriterTests.cpp)

    add_executable(RTPSWriterTests ${RTPSWRITERTESTS_SOURCE})
    target_compile_definitions(RTPSWriterTests PRIVATE
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(RTPSWriterTests PRIVATE
        ${Asio_INCLUDE_DIR})
    target_link_libraries(RTPSWriterTests fastrtps foonathan_memory
        GTest::gmock
        ${CMAKE_DL_LIBS})
    add_gtest(RTPSWriterTests SOURCES ${RTPSWRITERTESTS_SOURCE})
endif()
