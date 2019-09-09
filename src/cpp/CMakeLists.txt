# Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

include(${PROJECT_SOURCE_DIR}/cmake/dev/set_sources.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dev/generate_msvc_libraries.cmake)

# Set source files

if(WIN32)

file(GLOB_RECURSE ALL_HEADERS
    ${PROJECT_SOURCE_DIR}/include/**/*.h
    ${PROJECT_SOURCE_DIR}/include/**/*.hpp
    ${PROJECT_SOURCE_DIR}/src/cpp/**/*.h
    ${PROJECT_SOURCE_DIR}/src/cpp/**/*.hpp
    )

endif(WIN32)

set(${PROJECT_NAME}_source_files
    ${ALL_HEADERS}

    fastrtps_deprecated/log/Log.cpp
    fastrtps_deprecated/log/StdoutConsumer.cpp
    fastrtps_deprecated/log/FileConsumer.cpp
    fastrtps_deprecated/utils/eClock.cpp
    fastrtps_deprecated/utils/IPFinder.cpp
    fastrtps_deprecated/utils/md5.cpp
    fastrtps_deprecated/utils/StringMatching.cpp
    fastrtps_deprecated/utils/IPLocator.cpp
    fastrtps_deprecated/utils/System.cpp
    rtps/common/Time_t.cpp
    rtps/resources/ResourceEvent.cpp
    rtps/resources/TimedEvent.cpp
    rtps/resources/TimedEventImpl.cpp
    rtps/resources/AsyncWriterThread.cpp
    rtps/resources/AsyncInterestTree.cpp
    rtps/writer/LivelinessManager.cpp
    rtps/writer/RTPSWriter.cpp
    rtps/writer/StatefulWriter.cpp
    rtps/writer/ReaderProxy.cpp
    rtps/writer/StatelessWriter.cpp
    rtps/writer/ReaderLocator.cpp
    rtps/history/CacheChangePool.cpp
    rtps/history/History.cpp
    rtps/history/WriterHistory.cpp
    rtps/history/ReaderHistory.cpp
    rtps/reader/WriterProxy.cpp
    rtps/reader/StatefulReader.cpp
    rtps/reader/StatelessReader.cpp
    rtps/reader/RTPSReader.cpp
    rtps/reader/FragmentedChangePitStop.cpp
    rtps/messages/RTPSMessageCreator.cpp
    rtps/messages/RTPSMessageGroup.cpp
    rtps/messages/MessageReceiver.cpp
    rtps/messages/submessages/AckNackMsg.hpp
    rtps/messages/submessages/DataMsg.hpp
    rtps/messages/submessages/GapMsg.hpp
    rtps/messages/submessages/HeartbeatMsg.hpp
    rtps/network/NetworkFactory.cpp
    rtps/network/ReceiverResource.cpp
    rtps/participant/RTPSParticipant.cpp
    rtps/participant/RTPSParticipantImpl.cpp
    rtps/RTPSDomain.cpp
    fastrtps_deprecated/Domain.cpp
    fastrtps_deprecated/participant/Participant.cpp
    fastrtps_deprecated/participant/ParticipantImpl.cpp
    fastrtps_deprecated/publisher/Publisher.cpp
    fastrtps_deprecated/publisher/PublisherImpl.cpp
    fastrtps_deprecated/publisher/PublisherHistory.cpp
    fastrtps_deprecated/subscriber/Subscriber.cpp
    fastrtps_deprecated/subscriber/SubscriberImpl.cpp
    fastrtps_deprecated/subscriber/SubscriberHistory.cpp
    dds/topic/DataReader.cpp
    dds/topic/DataWriter.cpp
    dds/topic/DataReaderImpl.cpp
    dds/topic/DataWriterImpl.cpp
    dds/topic/TypeSupport.cpp
    dds/topic/TypeSupport.cpp
    dds/publisher/PublisherImpl.cpp
    dds/publisher/qos/PublisherQos.cpp
    dds/publisher/Publisher.cpp
    dds/subscriber/SubscriberImpl.cpp
    dds/subscriber/qos/SubscriberQos.cpp
    dds/subscriber/Subscriber.cpp
    dds/domain/DomainParticipantFactory.cpp
    dds/domain/DomainParticipantImpl.cpp
    dds/domain/DomainParticipant.cpp
    fastrtps_deprecated/transport/ChannelResource.cpp
    fastrtps_deprecated/transport/UDPChannelResource.cpp
    fastrtps_deprecated/transport/TCPChannelResource.cpp
    fastrtps_deprecated/transport/TCPChannelResourceBasic.cpp
    fastrtps_deprecated/transport/TCPAcceptor.cpp
    fastrtps_deprecated/transport/TCPAcceptorBasic.cpp
    fastrtps_deprecated/transport/UDPv4Transport.cpp
    fastrtps_deprecated/transport/TCPTransportInterface.cpp
    fastrtps_deprecated/transport/UDPTransportInterface.cpp
    fastrtps_deprecated/transport/TCPv4Transport.cpp
    fastrtps_deprecated/transport/UDPv6Transport.cpp
    fastrtps_deprecated/transport/TCPv6Transport.cpp
    fastrtps_deprecated/transport/test_UDPv4Transport.cpp
    fastrtps_deprecated/transport/tcp/TCPControlMessage.cpp
    fastrtps_deprecated/transport/tcp/RTCPMessageManager.cpp

    fastrtps_deprecated/types/AnnotationDescriptor.cpp
    fastrtps_deprecated/types/AnnotationParameterValue.cpp
    fastrtps_deprecated/types/DynamicData.cpp
    fastrtps_deprecated/types/DynamicDataFactory.cpp
    fastrtps_deprecated/types/DynamicType.cpp
    fastrtps_deprecated/types/DynamicPubSubType.cpp
    fastrtps_deprecated/types/DynamicTypePtr.cpp
    fastrtps_deprecated/types/DynamicDataPtr.cpp
    fastrtps_deprecated/types/DynamicTypeBuilder.cpp
    fastrtps_deprecated/types/DynamicTypeBuilderPtr.cpp
    fastrtps_deprecated/types/DynamicTypeBuilderFactory.cpp
    fastrtps_deprecated/types/DynamicTypeMember.cpp
    fastrtps_deprecated/types/MemberDescriptor.cpp
    fastrtps_deprecated/types/TypeDescriptor.cpp
    fastrtps_deprecated/types/TypeIdentifier.cpp
    fastrtps_deprecated/types/TypeIdentifierTypes.cpp
    fastrtps_deprecated/types/TypeObject.cpp
    fastrtps_deprecated/types/TypeObjectHashId.cpp
    fastrtps_deprecated/types/TypeObjectFactory.cpp
    fastrtps_deprecated/types/TypeNamesGenerator.cpp
    fastrtps_deprecated/types/TypesBase.cpp
    fastrtps_deprecated/types/BuiltinAnnotationsTypeObject.cpp

    fastrtps_deprecated/attributes/TopicAttributes.cpp
    fastrtps_deprecated/qos/ParameterList.cpp
    fastrtps_deprecated/qos/ParameterTypes.cpp
    fastrtps_deprecated/qos/QosPolicies.cpp
    fastrtps_deprecated/qos/WriterQos.cpp
    fastrtps_deprecated/qos/ReaderQos.cpp
    rtps/builtin/BuiltinProtocols.cpp
    rtps/builtin/discovery/participant/DirectMessageSender.cpp
    rtps/builtin/discovery/participant/PDP.cpp
    rtps/builtin/discovery/participant/PDPSimple.cpp
    rtps/builtin/discovery/participant/PDPListener.cpp
    rtps/builtin/discovery/participant/PDPServerListener.cpp
    rtps/builtin/discovery/participant/PDPClient.cpp
    rtps/builtin/discovery/participant/PDPServer.cpp
    rtps/builtin/discovery/participant/timedevent/DSClientEvent.cpp
    rtps/builtin/discovery/participant/timedevent/DServerEvent.cpp
    rtps/builtin/discovery/endpoint/EDP.cpp
    rtps/builtin/discovery/endpoint/EDPSimple.cpp
    rtps/builtin/discovery/endpoint/EDPClient.cpp
    rtps/builtin/discovery/endpoint/EDPServer.cpp
    rtps/builtin/discovery/endpoint/EDPSimpleListeners.cpp
    rtps/builtin/discovery/endpoint/EDPServerListeners.cpp
    rtps/builtin/discovery/endpoint/EDPStatic.cpp
    rtps/builtin/liveliness/WLP.cpp
    rtps/builtin/liveliness/WLPListener.cpp
    rtps/builtin/data/ParticipantProxyData.cpp
    rtps/builtin/data/WriterProxyData.cpp
    rtps/builtin/data/ReaderProxyData.cpp
    rtps/flowcontrol/ThroughputController.cpp
    rtps/flowcontrol/ThroughputControllerDescriptor.cpp
    rtps/flowcontrol/FlowController.cpp
    rtps/exceptions/Exception.cpp
    rtps/attributes/PropertyPolicy.cpp
    rtps/common/Token.cpp
    fastrtps_deprecated/xmlparser/XMLParserCommon.cpp
    fastrtps_deprecated/xmlparser/XMLElementParser.cpp
    fastrtps_deprecated/xmlparser/XMLEndpointParser.cpp
    fastrtps_deprecated/xmlparser/XMLParser.cpp
    fastrtps_deprecated/xmlparser/XMLProfileManager.cpp
    rtps/writer/PersistentWriter.cpp
    rtps/writer/StatelessPersistentWriter.cpp
    rtps/writer/StatefulPersistentWriter.cpp
    rtps/reader/StatelessPersistentReader.cpp
    rtps/reader/StatefulPersistentReader.cpp
    rtps/persistence/PersistenceFactory.cpp
    rtps/persistence/SQLite3PersistenceService.cpp
    rtps/persistence/sqlite3.c
    fastrtps_deprecated/utils/TimedConditionVariable.cpp
    
    serializer/serializer.cpp
    )

# TLS Support
if(TLS_FOUND)
    list(APPEND ${PROJECT_NAME}_source_files
        fastrtps_deprecated/transport/TCPChannelResourceSecure.cpp
        fastrtps_deprecated/transport/TCPAcceptorSecure.cpp
        )
endif()

# Add sources to Makefile.am
set_sources(${${PROJECT_NAME}_source_files})


# Security sources
set(${PROJECT_NAME}_security_source_files
    rtps/security/exceptions/SecurityException.cpp
    rtps/security/common/SharedSecretHandle.cpp
    rtps/security/SecurityManager.cpp
    rtps/security/SecurityPluginFactory.cpp
    fastrtps_deprecated/security/OpenSSLInit.cpp
    fastrtps_deprecated/security/authentication/PKIDH.cpp
    fastrtps_deprecated/security/accesscontrol/Permissions.cpp
    fastrtps_deprecated/security/cryptography/AESGCMGMAC.cpp
    fastrtps_deprecated/security/cryptography/AESGCMGMAC_KeyExchange.cpp
    fastrtps_deprecated/security/cryptography/AESGCMGMAC_KeyFactory.cpp
    fastrtps_deprecated/security/cryptography/AESGCMGMAC_Transform.cpp
    fastrtps_deprecated/security/cryptography/AESGCMGMAC_Types.cpp
    fastrtps_deprecated/security/authentication/PKIIdentityHandle.cpp
    fastrtps_deprecated/security/authentication/PKIHandshakeHandle.cpp
    fastrtps_deprecated/security/accesscontrol/AccessPermissionsHandle.cpp
    fastrtps_deprecated/security/accesscontrol/CommonParser.cpp
    fastrtps_deprecated/security/accesscontrol/GovernanceParser.cpp
    fastrtps_deprecated/security/accesscontrol/PermissionsParser.cpp
    )

# Add sources to Makefile.am
set_sources(SECTION SECURITY)
set_sources(${${PROJECT_NAME}_security_source_files})
set_sources(ENDSECTION)

if(SECURITY)
    list(APPEND ${PROJECT_NAME}_source_files
        ${${PROJECT_NAME}_security_source_files}
        )
    set(HAVE_SECURITY 1)
else()
    set(HAVE_SECURITY 0)
endif()

if(WIN32 AND (MSVC OR MSVC_IDE))
    list(APPEND ${PROJECT_NAME}_source_files
        ${PROJECT_SOURCE_DIR}/src/cpp/fastrtps.rc
        )
endif()

# External sources
if(TINYXML2_SOURCE_DIR)
    set(TINYXML2_SOURCE_DIR_ ${TINYXML2_SOURCE_DIR})
    if(UNIX AND EPROSIMA_INSTALLER)
        file(RELATIVE_PATH TINYXML2_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR} ${TINYXML2_SOURCE_DIR})
    endif()
    list(APPEND ${PROJECT_NAME}_source_files
        ${TINYXML2_SOURCE_DIR_}/tinyxml2.cpp
        )
    set_sources(${TINYXML2_SOURCE_DIR_}/tinyxml2.cpp)
endif()

if(ANDROID)
    list(APPEND ${PROJECT_NAME}_source_files
        ${ANDROID_IFADDRS_SOURCE_DIR}/ifaddrs.c
        )
endif()

configure_file(${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/config.h.in
    ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}/config.h)

if(NOT ANDROID)
    find_package(Threads REQUIRED)
endif()

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

# Create libraries
if((MSVC OR MSVC_IDE) AND EPROSIMA_INSTALLER AND IS_TOP_LEVEL)
    generate_msvc_libraries(i86Win32VS2015)
    generate_msvc_libraries(x64Win64VS2015)
    generate_msvc_libraries(i86Win32VS2017)
    generate_msvc_libraries(x64Win64VS2017)
elseif(NOT EPROSIMA_INSTALLER)
    #Create library
    add_library(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ${PROJECT_NAME_UPPER}_SOURCE
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        $<$<AND:$<BOOL:${WIN32}>,$<STREQUAL:"${CMAKE_SYSTEM_NAME}","WindowsStore">>:_WIN32_WINNT=0x0603>
        $<$<AND:$<BOOL:${WIN32}>,$<NOT:$<STREQUAL:"${CMAKE_SYSTEM_NAME}","WindowsStore">>>:_WIN32_WINNT=0x0601>
        $<$<AND:$<BOOL:${WIN32}>,$<STREQUAL:"${CMAKE_SYSTEM_NAME}","WindowsStore">>:SQLITE_OS_WINRT>
        $<$<AND:$<BOOL:${ANDROID}>,$<NOT:$<BOOL:${HAVE_CXX14}>>,$<NOT:$<BOOL:${HAVE_CXX1Y}>>>:ASIO_DISABLE_STD_STRING_VIEW>
        )

    # Define public headers
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/cpp>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${ASIO_INCLUDE_DIR}
        ${TINYXML2_INCLUDE_DIR}
        $<$<BOOL:${ANDROID}>:${ANDROID_IFADDRS_INCLUDE_DIR}>
        )

    # Made linked libraries PRIVATE to prevent local directories in Windows installer.
    if(EPROSIMA_INSTALLER_MINION)
        set(PRIVACY "PRIVATE")
    endif()

    # No need to expose linked libs when target is a shared library on MSVC.
    if(BUILD_SHARED_LIBS AND MSVC)
        set(PRIVACY "PRIVATE")
    endif()

    # Link library to external libraries.
    target_link_libraries(${PROJECT_NAME} ${PRIVACY} fastcdr foonathan_memory
        ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
        ${TINYXML2_LIBRARY}
        $<$<BOOL:${LINK_SSL}>:OpenSSL::SSL$<SEMICOLON>OpenSSL::Crypto>
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        )

    if(MSVC OR MSVC_IDE)
        set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${PROJECT_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

        get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
        if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
            # Export symbols in DLL library
            target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME_UPPER}_DYN_LINK)
        else()
            # Rename the library to have a "lib" before.
            set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME lib${PROJECT_NAME})
        endif()

        if("${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsStore")
            set_target_properties(${PROJECT_NAME} PROPERTIES VS_WINRT_COMPONENT "true")
        endif()

        # For installing purpose.
        if(EPROSIMA_INSTALLER_MINION)
            set(MSVCARCH_EXTENSION "_${MSVC_ARCH}")
            set(MSVCARCH_DIR_EXTENSION "/${MSVC_ARCH}")
            set(MSVCARCH_DIR_EXTENSION_EXT "/${MSVC_ARCH}")
            if(NOT TARGET_TYPE STREQUAL "SHARED_LIBRARY")
                set(MSVCARCH_DIR_EXTENSION_EXT "${MSVCARCH_DIR_EXTENSION_EXT}_static")
            endif()
        endif()
    endif()
endif()

###############################################################################
# Packaging
###############################################################################

# Install public headers
set_public_headers_directory(${PROJECT_SOURCE_DIR}/include ${PROJECT_NAME}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    COMPONENT headers
    INSTALL
    )

# Install config.h header
set_public_header(${PROJECT_BINARY_DIR}/include ${PROJECT_NAME} config.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}
    COMPONENT headers
    INSTALL
    )


# Install external sources
if(UNIX AND EPROSIMA_INSTALLER)
    set_public_header(${TINYXML2_INCLUDE_DIR} "" tinyxml2.h
        DESTINATION thirdparty/tinyxml2
        COMPONENT headers
        )

    set_public_headers_directory(${ASIO_INCLUDE_DIR} ""
        DESTINATION thirdparty/asio
        COMPONENT headers
        )
endif()

set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers" PARENT_SCOPE)
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} C++ Headers" PARENT_SCOPE)

# Install sources
if(UNIX AND EPROSIMA_INSTALLER)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/cpp
        DESTINATION src
        COMPONENT sources
        FILES_MATCHING
        PATTERN "*.c"
        PATTERN "*.cpp"
        PATTERN "*.h"
        PATTERN "*.hpp"
        )

    # Install external sources
    install(FILES ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
        ${TINYXML2_SOURCE_DIR}/tinyxml2.h
        DESTINATION thirdparty/tinyxml2
        COMPONENT sources
        )

    install(DIRECTORY ${PROJECT_BINARY_DIR}/autotools/
        DESTINATION .
        USE_SOURCE_PERMISSIONS
        OPTIONAL
        COMPONENT sources
        )
endif()
# Install libraries
if((MSVC OR MSVC_IDE) AND EPROSIMA_INSTALLER)
    if(IS_TOP_LEVEL)
        install_msvc_libraries(i86Win32VS2015)
        install_msvc_libraries(x64Win64VS2015)
        install_msvc_libraries(i86Win32VS2017)
        install_msvc_libraries(x64Win64VS2017)
    endif()

    install(FILES ${PROJECT_SOURCE_DIR}/cmake/packaging/windows/${PROJECT_NAME}-config.cmake
        DESTINATION ${DATA_INSTALL_DIR}/${PROJECT_NAME}/cmake
        COMPONENT cmake
        )
elseif(NOT EPROSIMA_INSTALLER)
    install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        COMPONENT libraries${MSVCARCH_EXTENSION}
        )

    export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-targets.cmake)

    if(INSTALLER_PLATFORM)
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}-${INSTALLER_PLATFORM}/cmake)
    else()
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}/cmake${MSVCARCH_DIR_EXTENSION_EXT})
    endif()

    install(EXPORT ${PROJECT_NAME}-targets
        DESTINATION ${INSTALL_DESTINATION_PATH}
        COMPONENT cmake
        )

    if(MSVC OR MSVC_IDE)
        if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
            # Install pdb files
            install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug/
                DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
                COMPONENT libraries${MSVCARCH_EXTENSION}
                CONFIGURATIONS Debug
                FILES_MATCHING
                PATTERN "*.pdb"
                )
        else()
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dir/Debug/${PROJECT_NAME}.pdb
                DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
                COMPONENT libraries${MSVCARCH_EXTENSION}
                CONFIGURATIONS Debug
                )
        endif()

        if(EPROSIMA_INSTALLER_MINION AND LINK_SSL)
            # Libraries from https://www.npcglib.org/~stathis/blog/precompiled-openssl/
            install(FILES ${OPENSSL_ROOT_DIR}/bin/libeay32MD.dll
                ${OPENSSL_ROOT_DIR}/bin/libeay32MDd.dll
                ${OPENSSL_ROOT_DIR}/bin/ssleay32MD.dll
                ${OPENSSL_ROOT_DIR}/bin/ssleay32MDd.dll
                DESTINATION ${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
                COMPONENT libraries${MSVCARCH_EXTENSION}
                )

            install(FILES ${OPENSSL_ROOT_DIR}/lib/libeay32MD.lib
                ${OPENSSL_ROOT_DIR}/lib/libeay32MDd.lib
                ${OPENSSL_ROOT_DIR}/lib/ssleay32MD.lib
                ${OPENSSL_ROOT_DIR}/lib/ssleay32MDd.lib
                DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
                COMPONENT libraries${MSVCARCH_EXTENSION}
                )
        endif()
    endif()

    ###############################################################################
    # Create CMake package config file
    ###############################################################################
    include(CMakePackageConfigHelpers)

    if(BUILD_SHARED_LIBS)
        set(FASTRTPS_PACKAGE_OPT_BIN_DIR_CONDITION "if(MSVC OR MSVC_IDE)")
    else()
        set(FASTRTPS_PACKAGE_OPT_BIN_DIR_CONDITION "if(0)")
    endif()

    # Add fastrtps dependencies in its CMake config file.
    if(LINK_SSL AND NOT EPROSIMA_INSTALLER_MINION)
        set(FASTRTPS_PACKAGE_OPT_DEPS "find_package(OpenSSL REQUIRED)")
    endif()

    configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/packaging/Config.cmake.in
        ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config.cmake
	INSTALL_DESTINATION ${INSTALL_DESTINATION_PATH}
        PATH_VARS BIN_INSTALL_DIR INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
        )
    write_basic_package_version_file(${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        )
    install(FILES ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config.cmake
        ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config-version.cmake
	DESTINATION ${INSTALL_DESTINATION_PATH}
        COMPONENT cmake
        )

    if(MSVCARCH_EXTENSION)
        string(TOUPPER "${MSVC_ARCH}" MSVC_ARCH_UPPER)
        set(CPACK_COMPONENT_LIBRARIES_${MSVC_ARCH_UPPER}_DISPLAY_NAME "${MSVC_ARCH}" PARENT_SCOPE)
        set(CPACK_COMPONENT_LIBRARIES_${MSVC_ARCH_UPPER}_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} libraries for platform ${MSVC_ARCH}" PARENT_SCOPE)
        set(CPACK_COMPONENT_LIBRARIES_${MSVC_ARCH_UPPER}_GROUP "Libraries" PARENT_SCOPE)
        set(CPACK_COMPONENT_GROUP_LIBRARIES_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} libraries" PARENT_SCOPE)
    else()
        set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries" PARENT_SCOPE)
        set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} libraries" PARENT_SCOPE)
    endif()
endif()
