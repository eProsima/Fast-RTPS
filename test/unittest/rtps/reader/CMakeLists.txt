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
    set(WRITERPROXYTESTS_SOURCE WriterProxyTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/WriterQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    add_executable(WriterProxyTests ${WRITERPROXYTESTS_SOURCE})
    target_compile_definitions(WriterProxyTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(WriterProxyTests PRIVATE
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/Endpoint
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSReader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSParticipantImpl
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSDomainImpl
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TimedEvent
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatefulReader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/WriterProxyData
        ${PROJECT_SOURCE_DIR}/test/mock/dds/QosPolicies
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ResourceEvent
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(WriterProxyTests foonathan_memory
        GTest::gmock
        ${CMAKE_DL_LIBS})
    add_gtest(WriterProxyTests SOURCES ${WRITERPROXYTESTS_SOURCE})
endif()
