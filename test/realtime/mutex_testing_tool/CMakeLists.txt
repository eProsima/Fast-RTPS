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

add_library(mutex_testing_tool SHARED TMutex.cpp)

add_library(mutex_testing_tool_preload SHARED Mutex.cpp)
target_link_libraries(mutex_testing_tool_preload PRIVATE mutex_testing_tool ${CMAKE_DL_LIBS})

add_executable(TMutexTests TMutexTests.cpp)
target_link_libraries(TMutexTests mutex_testing_tool GTest::gtest)
STRING(REPLACE " " "\\ " MUTEX_PRELOAD_LIBRARY_FILE "$<TARGET_FILE:mutex_testing_tool_preload>")
add_gtest(TMutexTests SOURCES TMutexTests.cpp
    ENVIRONMENTS
    "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:mutex_testing_tool_preload>"
    "LD_PRELOAD=$<TARGET_FILE_NAME:mutex_testing_tool_preload>"
    LABELS "NoMemoryCheck"
    )
