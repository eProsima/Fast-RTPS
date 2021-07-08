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
        if(WIN32)
            add_definitions(-D_WIN32_WINNT=0x0601)
        endif()


        set(STRINGMATCHINGTESTS_SOURCE
            StringMatchingTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/StringMatching.cpp)

        set(FIXEDSIZESTRINGTESTS_SOURCE
            FixedSizeStringTests.cpp)

        set(BITMAPRANGETESTS_SOURCE
            BitmapRangeTests.cpp)

        set(RESOURCELIMITEDVECTORTESTS_SOURCE
            ResourceLimitedVectorTests.cpp)

        set(LOCATORTESTS_SOURCE
            LocatorTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp)

        set(FIXEDSIZEQUEUETESTS_SOURCE
            FixedSizeQueueTests.cpp)

        set(SYSTEMINFOTESTS_SOURCE
            SystemInfoTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/SystemInfo.cpp)

        include_directories(mock/)

        add_executable(StringMatchingTests ${STRINGMATCHINGTESTS_SOURCE})
        target_compile_definitions(StringMatchingTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(StringMatchingTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(StringMatchingTests ${GTEST_LIBRARIES} ${MOCKS})
        if(MSVC OR MSVC_IDE)
            target_link_libraries(StringMatchingTests ${PRIVACY} iphlpapi Shlwapi
                )
        endif()
        add_gtest(StringMatchingTests SOURCES ${STRINGMATCHINGTESTS_SOURCE})


        add_executable(FixedSizeStringTests ${FIXEDSIZESTRINGTESTS_SOURCE})
        target_compile_definitions(FixedSizeStringTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(FixedSizeStringTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(FixedSizeStringTests ${GTEST_LIBRARIES} ${MOCKS})
        add_gtest(FixedSizeStringTests SOURCES ${FIXEDSIZESTRINGTESTS_SOURCE})


        add_executable(BitmapRangeTests ${BITMAPRANGETESTS_SOURCE})
        target_compile_definitions(BitmapRangeTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(BitmapRangeTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(BitmapRangeTests ${GTEST_LIBRARIES} ${MOCKS})
        add_gtest(BitmapRangeTests SOURCES ${BITMAPRANGETESTS_SOURCE})


        add_executable(ResourceLimitedVectorTests ${RESOURCELIMITEDVECTORTESTS_SOURCE})
        target_compile_definitions(ResourceLimitedVectorTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(ResourceLimitedVectorTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(ResourceLimitedVectorTests ${GTEST_LIBRARIES} ${MOCKS})
        add_gtest(ResourceLimitedVectorTests SOURCES ${RESOURCELIMITEDVECTORTESTS_SOURCE})

        add_executable(LocatorTests ${LOCATORTESTS_SOURCE})
        target_compile_definitions(LocatorTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(LocatorTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(LocatorTests ${GTEST_LIBRARIES} ${MOCKS})
        add_gtest(LocatorTests SOURCES ${LOCATORTESTS_SOURCE})


        add_executable(FixedSizeQueueTests ${FIXEDSIZEQUEUETESTS_SOURCE})
        target_compile_definitions(FixedSizeQueueTests PRIVATE FASTRTPS_NO_LIB)
        target_include_directories(FixedSizeQueueTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/cpp ${PROJECT_BINARY_DIR}/include)
        target_link_libraries(FixedSizeQueueTests ${GTEST_LIBRARIES} ${MOCKS})
        add_gtest(FixedSizeQueueTests SOURCES ${FIXEDSIZEQUEUETESTS_SOURCE})

        add_executable(SystemInfoTests ${SYSTEMINFOTESTS_SOURCE})
        target_compile_definitions(SystemInfoTests PRIVATE FASTRTPS_NO_LIB
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            ASIO_DISABLE_VISIBILITY
            )
        target_include_directories(SystemInfoTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/src/cpp
            ${PROJECT_BINARY_DIR}/include
            ${Asio_INCLUDE_DIR}
            )
        target_link_libraries(SystemInfoTests ${GTEST_LIBRARIES})
        add_gtest(SystemInfoTests SOURCES ${SYSTEMINFOTESTS_SOURCE})

    endif()
endif()
