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
    include(${PROJECT_SOURCE_DIR}/cmake/common/gtest.cmake)
    check_gtest()

    if(GTEST_FOUND)
        set(CACHECHANGETESTS_SOURCE CacheChangeTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp)
        set(GUID_UTILS_TESTS_SOURCE GuidUtilsTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp)
        set(SEQUENCENUMBERTESTS_SOURCE SequenceNumberTests.cpp)
        set(PORTPARAMETERSTESTS_SOURCE PortParametersTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp)

        add_executable(CacheChangeTests ${CACHECHANGETESTS_SOURCE})
        target_compile_definitions(CacheChangeTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(CacheChangeTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(CacheChangeTests ${GTEST_LIBRARIES})
        add_gtest(CacheChangeTests SOURCES ${CACHECHANGETESTS_SOURCE})

        add_executable(GuidUtilsTests ${GUID_UTILS_TESTS_SOURCE})
        target_compile_definitions(GuidUtilsTests PRIVATE FASTRTPS_NO_LIB
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            ASIO_DISABLE_VISIBILITY
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(GuidUtilsTests PRIVATE
            ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/src/cpp
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
            ${Asio_INCLUDE_DIR}
            )
        target_link_libraries(GuidUtilsTests ${GTEST_LIBRARIES})
        if(MSVC OR MSVC_IDE)
            target_link_libraries(GuidUtilsTests ws2_32)
        endif()
        add_gtest(GuidUtilsTests SOURCES ${GUID_UTILS_TESTS_SOURCE})

        add_executable(SequenceNumberTests ${SEQUENCENUMBERTESTS_SOURCE})
        target_compile_definitions(SequenceNumberTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(SequenceNumberTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(SequenceNumberTests ${GTEST_LIBRARIES})
        add_gtest(SequenceNumberTests SOURCES ${SEQUENCENUMBERTESTS_SOURCE})

        add_executable(PortParametersTests ${PORTPARAMETERSTESTS_SOURCE})
        target_compile_definitions(PortParametersTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(PortParametersTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(PortParametersTests ${GTEST_LIBRARIES})
        add_gtest(PortParametersTests SOURCES ${PORTPARAMETERSTESTS_SOURCE} LABELS "NoMemoryCheck")
    endif()
endif()
