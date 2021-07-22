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

cmake_minimum_required(VERSION 3.5)
cmake_policy(VERSION 3.5...3.14)

project(fast-discovery-server VERSION 1.0.0 LANGUAGES C CXX)

###############################################################################
# Load external dependencies
###############################################################################

if(NOT fastrtps_FOUND)
    find_package(fastrtps REQUIRED)
endif()

# Check C++11
include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

###############################################################################
# Compilation
###############################################################################

if (MSVC)

    # For isolated builds provide some IDE aids
    if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

        # add fast headers to enable class view on visual studio IDE
        get_target_property(FAST_INCLUDE_DIR fastrtps INTERFACE_INCLUDE_DIRECTORIES)
        list(GET FAST_INCLUDE_DIR 0 FAST_INCLUDE_DIR)

        file(GLOB_RECURSE ALL_FAST_HEADERS
            "${FAST_INCLUDE_DIR}/**/*.h"
            "${FAST_INCLUDE_DIR}/**/*.hpp"
            "${FAST_INCLUDE_DIR}/**/*.hxx"
            )

        source_group("Fast-RTPS Headers" FILES ${ALL_FAST_HEADERS})

    endif()

    # add project headers to enable class view on visual studio IDE
    set(FAST_SERVER_HEADERS server.h optionparser.h)

endif(MSVC)

add_executable(${PROJECT_NAME}
    server.cpp
    ${FAST_SERVER_HEADERS}
    ${ALL_FAST_HEADERS})

# we pass the version as a preprocessor definition
target_compile_definitions(${PROJECT_NAME}
    PRIVATE FAST_SERVER_VERSION=\"${PROJECT_VERSION}\"
    PRIVATE FAST_SERVER_BINARY=\"$<TARGET_FILE_NAME:${PROJECT_NAME}>\" )

target_link_libraries(${PROJECT_NAME} fastrtps fastcdr)

# Properties that change bin names depending on current config.
# This is convenient because on installation all bins share folder
set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX -${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES MINSIZEREL_POSTFIX m-${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX rd-${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d-${PROJECT_VERSION})

###############################################################################
# Installation
###############################################################################

# If not isolated integrate
if(CMAKE_PROJECT_NAME STREQUAL "fastrtps" )
    set(BIN_INSTALL_DIR tools/fds/${BIN_INSTALL_DIR} CACHE PATH "Installation directory for binaries within Fast-DDS")
else()
    set(BIN_INSTALL_DIR bin/ CACHE PATH "Installation directory for binaries")
endif()

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        COMPONENT discovery
        )

    if(INSTALLER_PLATFORM)
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${CMAKE_PROJECT_NAME}-${INSTALLER_PLATFORM}/cmake)
    else()
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${CMAKE_PROJECT_NAME}/cmake)
    endif()

# export library
install(EXPORT ${PROJECT_NAME}-targets
        DESTINATION ${INSTALL_DESTINATION_PATH}
        NAMESPACE fastdds::
        COMPONENT cmake
        )

# symlink creation requires:
#   - install( CODE using generator expressions (legacy code is provided for all cmake versions)
#   - on windows privileges to create symlinks (a .bat file is provided on unprivileged installations)

if (NOT (CMAKE_VERSION VERSION_LESS "3.14"))

    cmake_policy(SET CMP0087 NEW)

    # add symlink
    if( WIN32 )
        # Use powershell to generate the link
        install(
          CODE "execute_process( COMMAND PowerShell -Command \"if( test-path ${PROJECT_NAME}.exe -PathType Leaf ) { rm ${PROJECT_NAME}.exe } ; New-Item -ItemType SymbolicLink -Target $<TARGET_FILE_NAME:${PROJECT_NAME}> -Path ${PROJECT_NAME}.exe \" ERROR_QUIET RESULTS_VARIABLE SYMLINK_FAILED WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}\") \n if( SYMLINK_FAILED ) \n message(STATUS \"Windows requires admin installation rights to create symlinks. A bat script will be provided instead.\") \n set(FAST_SERVER_BINARY_NAME $<TARGET_FILE_NAME:${PROJECT_NAME}>) \n configure_file(${CMAKE_CURRENT_LIST_DIR}/fast-discovery-server.bat.in ${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}/${PROJECT_NAME}.bat @ONLY) \n endif()"
          COMPONENT discovery)
    else()
        # Use ln to create the symbolic link. We remove the version from the file name but keep the debug suffix
        install(
            CODE "execute_process(COMMAND bash -c \"if [[ -h ${PROJECT_NAME} ]]; then rm ${PROJECT_NAME}; fi;  ln -s $<TARGET_FILE_NAME:${PROJECT_NAME}> ${PROJECT_NAME}\" WORKING_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}\")"
            COMPONENT discovery)
    endif()

else() # fallback to all strategies

    function(install_symlink CONFIG)

            # compose the binary name
            set(BINARY_NAME ${PROJECT_NAME})
            set(LINK_NAME ${PROJECT_NAME})
            if("${CONFIG}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
                set(BINARY_NAME ${BINARY_NAME}d)
            elseif("${CONFIG}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
                set(BINARY_NAME ${BINARY_NAME}m)
            elseif("${CONFIG}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
                set(BINARY_NAME ${BINARY_NAME}rd)
            endif()
            set(BINARY_NAME ${BINARY_NAME}-${PROJECT_VERSION})

            if( WIN32 )
                # Use powershell to generate the link
                install(
                    CODE "if(NOT BUILD_TYPE OR BUILD_TYPE MATCHES \"${CONFIG}\") \n execute_process(COMMAND PowerShell -Command \"if( test-path ${LINK_NAME}.exe -PathType Leaf ) { rm ${LINK_NAME}.exe };New-Item -ItemType SymbolicLink -Path ${LINK_NAME}.exe -Target ${BINARY_NAME}.exe \" ERROR_QUIET RESULTS_VARIABLE SYMLINK_FAILED WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}\" ) \n if( SYMLINK_FAILED ) \n message(STATUS \"Windows requires admin installation rights to create symlinks. A bat script will be provided instead.\") \n set(FAST_SERVER_BINARY_NAME ${BINARY_NAME}.exe) \n configure_file(${CMAKE_CURRENT_LIST_DIR}/fast-discovery-server.bat.in ${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}/${LINK_NAME}.bat @ONLY) \n endif() \n endif()"
                    COMPONENT discovery)
            else()
                # Use ln to create the symbolic link. We remove the version from the file name but keep the debug suffix
                install(
                    CODE "if(NOT BUILD_TYPE OR BUILD_TYPE MATCHES \"${CONFIG}\") \n execute_process(COMMAND bash -c \"if [[ -h ${LINK_NAME} ]]; then rm ${LINK_NAME}; fi ; ln -s ${BINARY_NAME} ${LINK_NAME}\" WORKING_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}\") \n endif()"
                    COMPONENT discovery
                    )
            endif()

    endfunction()

    get_property(IS_MULTI GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if( IS_MULTI )

        foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
            install_symlink(${CONFIG})
        endforeach()

    else()

        install_symlink(${CMAKE_BUILD_TYPE})

    endif()

endif()

