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
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../profiles/test_xml_profiles.xml
        ${CMAKE_CURRENT_BINARY_DIR}/test_xml_profiles.xml
        COPYONLY)

    set(PUBLISHERTESTS_SOURCE PublisherTests.cpp)
    set(DATAWRITERTESTS_SOURCE DataWriterTests.cpp)
    set(PUBLISHERTESTS_SOURCE PublisherTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dds/pub/DataWriter.cpp
        )

    if(WIN32)
        add_definitions(
            -D_WIN32_WINNT=0x0601
            -D_CRT_SECURE_NO_WARNINGS
            )
    endif()

    add_executable(PublisherTests ${PUBLISHERTESTS_SOURCE})
    target_compile_definitions(PublisherTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(PublisherTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(PublisherTests fastrtps fastcdr foonathan_memory
        GTest::gmock
        ${CMAKE_DL_LIBS})
    add_gtest(PublisherTests SOURCES ${PUBLISHERTESTS_SOURCE})

    add_executable(DataWriterTests ${DATAWRITERTESTS_SOURCE})
    target_compile_definitions(DataWriterTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(DataWriterTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(DataWriterTests fastrtps fastcdr foonathan_memory
        GTest::gmock
        ${CMAKE_DL_LIBS})
    add_gtest(DataWriterTests
        SOURCES ${DATAWRITERTESTS_SOURCE}
        ENVIRONMENTS "CERTS_PATH=${PROJECT_SOURCE_DIR}/test/certs")
endif()
