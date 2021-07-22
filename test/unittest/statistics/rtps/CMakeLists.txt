# Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    set(STATISTICS_RTPS_TESTS_SOURCE
        RTPSStatisticsTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/types.cxx
        )

    add_executable(RTPSStatisticsTests ${STATISTICS_RTPS_TESTS_SOURCE})

    target_compile_definitions(RTPSStatisticsTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNAL_DEBUG> # Internal debug activated.
        )

    target_include_directories(RTPSStatisticsTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        ${Asio_INCLUDE_DIR}
        )

    target_link_libraries(RTPSStatisticsTests fastrtps fastcdr GTest::gtest GTest::gmock)
    add_gtest(RTPSStatisticsTests SOURCES ${STATISTICS_RTPS_TESTS_SOURCE})
endif()
