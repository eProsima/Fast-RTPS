# Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

    set(TOPICTESTS_SOURCE TopicTests.cpp)

    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    add_executable(TopicTests ${TOPICTESTS_SOURCE})
    target_compile_definitions(TopicTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(TopicTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(TopicTests fastrtps fastcdr foonathan_memory
        GTest::gmock
        ${CMAKE_DL_LIBS})
    add_gtest(TopicTests SOURCES ${TOPICTESTS_SOURCE})
endif()
