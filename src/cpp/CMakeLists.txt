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

add_definitions(-DNOMINMAX)

endif(WIN32)

set(${PROJECT_NAME}_source_files
    ${ALL_HEADERS}

    fastdds/log/Log.cpp
    fastdds/log/OStreamConsumer.cpp
    fastdds/log/StdoutErrConsumer.cpp
    fastdds/log/StdoutConsumer.cpp
    fastdds/log/FileConsumer.cpp

    rtps/common/Time_t.cpp
    rtps/resources/ResourceEvent.cpp
    rtps/resources/TimedEvent.cpp
    rtps/resources/TimedEventImpl.cpp
    rtps/writer/LivelinessManager.cpp
    rtps/writer/LocatorSelectorSender.cpp
    rtps/writer/RTPSWriter.cpp
    rtps/writer/StatefulWriter.cpp
    rtps/writer/ReaderProxy.cpp
    rtps/writer/StatelessWriter.cpp
    rtps/writer/ReaderLocator.cpp
    rtps/history/CacheChangePool.cpp
    rtps/history/History.cpp
    rtps/history/WriterHistory.cpp
    rtps/history/ReaderHistory.cpp
    rtps/history/TopicPayloadPool.cpp
    rtps/history/TopicPayloadPoolRegistry.cpp
    rtps/DataSharing/DataSharingPayloadPool.cpp
    rtps/DataSharing/DataSharingListener.cpp
    rtps/DataSharing/DataSharingNotification.cpp
    rtps/reader/WriterProxy.cpp
    rtps/reader/StatefulReader.cpp
    rtps/reader/StatelessReader.cpp
    rtps/reader/RTPSReader.cpp
    rtps/messages/RTPSMessageCreator.cpp
    rtps/messages/RTPSMessageGroup.cpp
    rtps/messages/RTPSGapBuilder.cpp
    rtps/messages/SendBuffersManager.cpp
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
    fastdds/subscriber/DataReader.cpp
    fastdds/publisher/DataWriter.cpp
    fastdds/subscriber/DataReaderImpl.cpp
    fastdds/publisher/DataWriterImpl.cpp
    fastdds/topic/Topic.cpp
    fastdds/topic/TopicImpl.cpp
    fastdds/topic/TypeSupport.cpp
    fastdds/topic/qos/TopicQos.cpp
    fastdds/publisher/qos/DataWriterQos.cpp
    fastdds/subscriber/qos/DataReaderQos.cpp
    fastdds/topic/TypeSupport.cpp
    fastdds/publisher/PublisherImpl.cpp
    fastdds/publisher/qos/PublisherQos.cpp
    fastdds/publisher/Publisher.cpp
    fastdds/subscriber/SubscriberImpl.cpp
    fastdds/subscriber/qos/SubscriberQos.cpp
    fastdds/subscriber/Subscriber.cpp
    fastdds/subscriber/DataReader.cpp
    fastdds/subscriber/DataReaderImpl.cpp
    fastdds/domain/DomainParticipantFactory.cpp
    fastdds/domain/DomainParticipantImpl.cpp
    fastdds/domain/DomainParticipant.cpp
    fastdds/domain/qos/DomainParticipantQos.cpp
    fastdds/domain/qos/DomainParticipantFactoryQos.cpp
    fastdds/builtin/typelookup/common/TypeLookupTypes.cpp
    fastdds/builtin/common/RPCHeadersImpl.cpp
    fastdds/builtin/typelookup/TypeLookupManager.cpp
    fastdds/builtin/typelookup/TypeLookupRequestListener.cpp
    fastdds/builtin/typelookup/TypeLookupReplyListener.cpp
    rtps/transport/ChannelResource.cpp
    rtps/transport/UDPChannelResource.cpp
    rtps/transport/TCPChannelResource.cpp
    rtps/transport/TCPChannelResourceBasic.cpp
    rtps/transport/TCPAcceptor.cpp
    rtps/transport/TCPAcceptorBasic.cpp
    rtps/transport/UDPv4Transport.cpp
    rtps/transport/TCPTransportInterface.cpp
    rtps/transport/UDPTransportInterface.cpp
    rtps/transport/shared_mem/SharedMemTransportDescriptor.cpp
    rtps/transport/TCPv4Transport.cpp
    rtps/transport/UDPv6Transport.cpp
    rtps/transport/TCPv6Transport.cpp
    rtps/transport/test_UDPv4Transport.cpp
    rtps/transport/tcp/TCPControlMessage.cpp
    rtps/transport/tcp/RTCPMessageManager.cpp

    dynamic-types/AnnotationDescriptor.cpp
    dynamic-types/AnnotationParameterValue.cpp
    dynamic-types/DynamicData.cpp
    dynamic-types/DynamicDataFactory.cpp
    dynamic-types/DynamicType.cpp
    dynamic-types/DynamicPubSubType.cpp
    dynamic-types/DynamicTypePtr.cpp
    dynamic-types/DynamicDataPtr.cpp
    dynamic-types/DynamicTypeBuilder.cpp
    dynamic-types/DynamicTypeBuilderPtr.cpp
    dynamic-types/DynamicTypeBuilderFactory.cpp
    dynamic-types/DynamicTypeMember.cpp
    dynamic-types/MemberDescriptor.cpp
    dynamic-types/TypeDescriptor.cpp
    dynamic-types/TypeIdentifier.cpp
    dynamic-types/TypeIdentifierTypes.cpp
    dynamic-types/TypeObject.cpp
    dynamic-types/TypeObjectHashId.cpp
    dynamic-types/TypeObjectFactory.cpp
    dynamic-types/TypeNamesGenerator.cpp
    dynamic-types/TypesBase.cpp
    dynamic-types/BuiltinAnnotationsTypeObject.cpp
    dynamic-types/DynamicDataHelper.cpp

    fastrtps_deprecated/attributes/TopicAttributes.cpp
    fastdds/core/condition/StatusCondition.cpp
    fastdds/core/condition/WaitSet.cpp
    fastdds/core/policy/ParameterList.cpp
    fastdds/core/policy/QosPolicyUtils.cpp
    fastdds/publisher/qos/WriterQos.cpp
    fastdds/subscriber/qos/ReaderQos.cpp
    rtps/builtin/BuiltinProtocols.cpp
    rtps/builtin/discovery/participant/DirectMessageSender.cpp
    rtps/builtin/discovery/participant/PDP.cpp
    rtps/builtin/discovery/participant/PDPSimple.cpp
    rtps/builtin/discovery/participant/PDPListener.cpp
    rtps/builtin/discovery/endpoint/EDP.cpp
    rtps/builtin/discovery/endpoint/EDPSimple.cpp
    rtps/builtin/discovery/endpoint/EDPSimpleListeners.cpp
    rtps/builtin/discovery/endpoint/EDPStatic.cpp
    rtps/builtin/liveliness/WLP.cpp
    rtps/builtin/liveliness/WLPListener.cpp
    rtps/builtin/data/ParticipantProxyData.cpp
    rtps/builtin/data/WriterProxyData.cpp
    rtps/builtin/data/ReaderProxyData.cpp
    rtps/flowcontrol/ThroughputControllerDescriptor.cpp
    rtps/flowcontrol/FlowControllerConsts.cpp
    rtps/flowcontrol/FlowControllerFactory.cpp
    rtps/exceptions/Exception.cpp
    rtps/attributes/PropertyPolicy.cpp
    rtps/common/Token.cpp
    rtps/xmlparser/XMLParserCommon.cpp
    rtps/xmlparser/XMLElementParser.cpp
    rtps/xmlparser/XMLDynamicParser.cpp
    rtps/xmlparser/XMLEndpointParser.cpp
    rtps/xmlparser/XMLParser.cpp
    rtps/xmlparser/XMLProfileManager.cpp
    rtps/writer/PersistentWriter.cpp
    rtps/writer/StatelessPersistentWriter.cpp
    rtps/writer/StatefulPersistentWriter.cpp
    rtps/reader/StatelessPersistentReader.cpp
    rtps/reader/StatefulPersistentReader.cpp
    rtps/persistence/PersistenceFactory.cpp

    rtps/builtin/discovery/database/backup/SharedBackupFunctions.cpp
    rtps/builtin/discovery/endpoint/EDPClient.cpp
    rtps/builtin/discovery/endpoint/EDPServer.cpp
    rtps/builtin/discovery/endpoint/EDPServerListeners.cpp
    rtps/builtin/discovery/database/DiscoveryDataBase.cpp
    rtps/builtin/discovery/database/DiscoveryParticipantInfo.cpp
    rtps/builtin/discovery/database/DiscoveryParticipantsAckStatus.cpp
    rtps/builtin/discovery/database/DiscoverySharedInfo.cpp
    rtps/builtin/discovery/participant/PDPClient.cpp
    rtps/builtin/discovery/participant/PDPServer.cpp
    rtps/builtin/discovery/participant/PDPServerListener.cpp
    rtps/builtin/discovery/participant/timedevent/DSClientEvent.cpp
    rtps/builtin/discovery/participant/timedevent/DServerEvent.cpp

    utils/IPFinder.cpp
    utils/md5.cpp
    utils/StringMatching.cpp
    utils/IPLocator.cpp
    utils/System.cpp
    utils/SystemInfo.cpp
    utils/TimedConditionVariable.cpp
    utils/string_convert.cpp

    dds/core/types.cpp
    dds/core/Exception.cpp
    dds/domain/DomainParticipant.cpp
    dds/pub/Publisher.cpp
    dds/pub/AnyDataWriter.cpp
    dds/pub/DataWriter.cpp
    dds/sub/Subscriber.cpp
    dds/sub/DataReader.cpp
    dds/topic/Topic.cpp

    statistics/fastdds/domain/DomainParticipant.cpp
    statistics/fastdds/publisher/qos/DataWriterQos.cpp
    statistics/fastdds/subscriber/qos/DataReaderQos.cpp
    )

# Statistics support
if (FASTDDS_STATISTICS)

    set(statistics_sources
        statistics/fastdds/domain/DomainParticipantImpl.cpp
        statistics/fastdds/domain/DomainParticipantStatisticsListener.cpp
        statistics/rtps/StatisticsBase.cpp
        statistics/rtps/reader/StatisticsReaderImpl.cpp
        statistics/rtps/writer/StatisticsWriterImpl.cpp
        statistics/types/typesPubSubTypes.cxx
        statistics/types/types.cxx
        )

    list(APPEND ${PROJECT_NAME}_source_files
        ${statistics_sources}
        )

endif()

# SHM Transport
if(IS_THIRDPARTY_BOOST_OK)
    list(APPEND ${PROJECT_NAME}_source_files
        rtps/transport/shared_mem/test_SharedMemTransport.cpp
        rtps/transport/shared_mem/SharedMemTransport.cpp
        )
endif()

# TLS Support
if(TLS_FOUND)
    list(APPEND ${PROJECT_NAME}_source_files
        rtps/transport/TCPChannelResourceSecure.cpp
        rtps/transport/TCPAcceptorSecure.cpp
        )
endif()

# Add sources to Makefile.am
set_sources(${${PROJECT_NAME}_source_files})


# Security sources
set(${PROJECT_NAME}_security_source_files
    rtps/security/exceptions/SecurityException.cpp
    rtps/security/common/SharedSecretHandle.cpp
    rtps/security/logging/Logging.cpp
    rtps/security/SecurityManager.cpp
    rtps/security/SecurityPluginFactory.cpp
    security/authentication/PKIDH.cpp
    security/accesscontrol/Permissions.cpp
    security/cryptography/AESGCMGMAC.cpp
    security/cryptography/AESGCMGMAC_KeyExchange.cpp
    security/cryptography/AESGCMGMAC_KeyFactory.cpp
    security/cryptography/AESGCMGMAC_Transform.cpp
    security/cryptography/AESGCMGMAC_Types.cpp
    security/authentication/PKIIdentityHandle.cpp
    security/authentication/PKIHandshakeHandle.cpp
    security/accesscontrol/AccessPermissionsHandle.cpp
    security/accesscontrol/CommonParser.cpp
    security/accesscontrol/GovernanceParser.cpp
    security/accesscontrol/PermissionsParser.cpp
    security/logging/LogTopic.cpp
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


#SQLITE3 persistence service sources
set(${PROJECT_NAME}_sqlite3_source_files
    rtps/persistence/SQLite3PersistenceService.cpp
    rtps/persistence/sqlite3.c
    )

# Add sources to Makefile.am
set_sources(SECTION SQLITE3)
set_sources(${${PROJECT_NAME}_sqlite3_source_files})
set_sources(ENDSECTION)

if(SQLITE3_SUPPORT)
    list(APPEND ${PROJECT_NAME}_source_files
        ${${PROJECT_NAME}_sqlite3_source_files}
        )
    set(HAVE_SQLITE3 1)
else()
    set(HAVE_SQLITE3 0)
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

# Option to enable strict real-time. In this case, several API functions have a real-time behaviour.
# * Publisher::write() - Uses ReliabilityQosPolicy.max_blocking_time
# * Subscriber::takeNextData() - Uses ReliabilityQosPolicy.max_blocking_time
# * Subscriber::readNextData() - Uses ReliabilityQosPolicy.max_blocking_time
option(STRICT_REALTIME "Enable a strict real-time behavour." OFF)
if(STRICT_REALTIME)
    set(HAVE_STRICT_REALTIME 1)
else()
    set(HAVE_STRICT_REALTIME 0)
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
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

    option(INTERNAL_DEBUG "Activate developer debug messages" OFF)

    # make visual studio more gcc and clang like. Propagated to dependent targets.
    if(MSVC)
        # C4101 'identifier' : unreferenced local variable 
        # C4189 'identifier' : local variable is initialized but not referenced
        # C4555  expression has no effect; expected expression with side-effect
        # C4715: 'Test': not all control paths return a value
        # C5038 data member 'member1' will be initialized after data member 'member2'
        target_compile_options(${PROJECT_NAME} PUBLIC /w34101 /w34555 /w34189 /w35038 /w34715)
    endif(MSVC)

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ${PROJECT_NAME_UPPER}_SOURCE
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        ASIO_DISABLE_VISIBILITY
        SQLITE_WIN32_GETVERSIONEX=0
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        $<$<AND:$<BOOL:${WIN32}>,$<STREQUAL:"${CMAKE_SYSTEM_NAME}","WindowsStore">>:_WIN32_WINNT=0x0603>
        $<$<AND:$<BOOL:${WIN32}>,$<NOT:$<STREQUAL:"${CMAKE_SYSTEM_NAME}","WindowsStore">>>:_WIN32_WINNT=0x0601>
        $<$<AND:$<BOOL:${WIN32}>,$<STREQUAL:"${CMAKE_SYSTEM_NAME}","WindowsStore">>:SQLITE_OS_WINRT>
        $<$<AND:$<BOOL:${ANDROID}>,$<NOT:$<BOOL:${HAVE_CXX14}>>,$<NOT:$<BOOL:${HAVE_CXX1Y}>>>:ASIO_DISABLE_STD_STRING_VIEW>
        $<$<BOOL:${WIN32}>:_ENABLE_ATOMIC_ALIGNMENT_FIX>
        $<$<NOT:$<BOOL:${IS_THIRDPARTY_BOOST_SUPPORTED}>>:FASTDDS_SHM_TRANSPORT_DISABLED> # Do not compile SHM Transport
        $<$<BOOL:${SHM_TRANSPORT_DEFAULT}>:SHM_TRANSPORT_BUILTIN> # Enable SHM as built-in transport
        $<$<BOOL:${STDOUTERR_LOG_CONSUMER}>:STDOUTERR_LOG_CONSUMER> # Enable StdoutErrConsumer as default LogConsumer
        )

    # Define public headers
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/cpp>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${Asio_INCLUDE_DIR}
        ${TINYXML2_INCLUDE_DIR}
        $<$<BOOL:${ANDROID}>:${ANDROID_IFADDRS_INCLUDE_DIR}>
        ${THIRDPARTY_BOOST_INCLUDE_DIR}
        )

    # Made linked libraries PRIVATE to prevent local directories in Windows installer.
    if(EPROSIMA_INSTALLER_MINION
        # No need to expose linked libs when target is a shared library on MSVC.
            OR (BUILD_SHARED_LIBS AND MSVC))
        set(PRIVACY "PRIVATE")
    else()
        set(PRIVACY "PUBLIC")
    endif()

    # Find out if libatomic link is required in this platform
    find_package(Atomic MODULE)

    # Link library to external libraries.
    target_link_libraries(${PROJECT_NAME} ${PRIVACY} fastcdr foonathan_memory
        ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
        ${TINYXML2_LIBRARY}
        $<$<BOOL:${LINK_SSL}>:OpenSSL::SSL$<SEMICOLON>OpenSSL::Crypto$<$<BOOL:${WIN32}>:$<SEMICOLON>crypt32.lib>>
        $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
        ${THIRDPARTY_BOOST_LINK_LIBS}
        PRIVATE eProsima_atomic
        )

    if(MSVC OR MSVC_IDE)

        # On installed binaries use manifest to specify dependencies
        if(INSTALLER_PLATFORM AND OPENSSL_FOUND)

            # Get Fast-DDS version suitable manifest format
            set(FASTDDS_CANONICAL_VERSION ${fastrtps_VERSION})
            
            if( NOT FASTDDS_CANONICAL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
                message(FATAL_ERROR "Fast-DDS version number ${fastrtps_VERSION} must include major, minor and patch.")
            endif()

            if( NOT fastrtps_VERSION_TWEAK)
                set(FASTDDS_CANONICAL_VERSION "${FASTDDS_CANONICAL_VERSION}.0")
            endif()

            # Get OpenSSL version suitable manifest format
            execute_process( COMMAND PowerShell -NoLogo -Command "&{ param([string]$original) 
                    if ($original -notmatch '\\d+$')
                    { $res = $original.Substring(0,$original.length-1) + '.' +
                            ([int]$original[$original.length-1]-[int][char]'a'+1); }
                    else { while(($original -split '\\.').count -le 3)
                        { $original += '.0'; } $res = $original; } $res }" -original ${OPENSSL_VERSION}
                OUTPUT_VARIABLE OPENSSL_CANONICAL_VERSION
                )
            string(STRIP ${OPENSSL_CANONICAL_VERSION} OPENSSL_CANONICAL_VERSION)

            # replace the values in the manifest template
            configure_file(
                "${CMAKE_SOURCE_DIR}/cmake/packaging/windows/fastrtps.manifest.in"
                "${CMAKE_CURRENT_BINARY_DIR}/fastrtps.manifest"
                @ONLY
                )

            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/fastrtps.manifest" MANIFEST_FILE_PATH)
            target_link_options(${PROJECT_NAME} PRIVATE "/MANIFEST:EMBED" "/MANIFESTINPUT:${MANIFEST_FILE_PATH}")

            unset(WINDOWS_SOURCE_DIR)
            unset(FASTDDS_CANONICAL_VERSION)
            unset(OPENSSL_CANONICAL_VERSION)
            unset(MANIFEST_FILE_PATH)
        endif()

        set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${PROJECT_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

        get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
        if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
            # Export symbols in DLL library
            target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME_UPPER}_DYN_LINK)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                PDB_NAME_DEBUG "${PROJECT_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
                PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
        else()
            # Rename the library to have a "lib" before.
            set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME lib${PROJECT_NAME})
            set_target_properties(${PROJECT_NAME} PROPERTIES
                COMPILE_PDB_NAME_DEBUG "lib${PROJECT_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
                COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
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

if(DEFINED ENV{LIB_FUZZING_ENGINE})
  add_executable(fuzz_processCDRMsg rtps/messages/fuzz_processCDRMsg.cpp)
  target_link_libraries(fuzz_processCDRMsg ${PROJECT_NAME} $ENV{LIB_FUZZING_ENGINE})
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
set_public_headers_directory(${PROJECT_SOURCE_DIR}/include fastdds
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

    set_public_headers_directory(${Asio_INCLUDE_DIR} ""
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

# Generate different target names depending on linking
get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    set(FASTDDS_LINKING dynamic)
else()
    set(FASTDDS_LINKING static)
endif()

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
    install(TARGETS ${PROJECT_NAME} eProsima_atomic
        EXPORT ${PROJECT_NAME}-${FASTDDS_LINKING}-targets
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        COMPONENT libraries${MSVCARCH_EXTENSION}
        )

    export(TARGETS ${PROJECT_NAME} eProsima_atomic FILE ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-${FASTDDS_LINKING}-targets.cmake)

    if(INSTALLER_PLATFORM)
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}-${INSTALLER_PLATFORM}/cmake)
    else()
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}/cmake${MSVCARCH_DIR_EXTENSION_EXT})
    endif()

    install(EXPORT ${PROJECT_NAME}-${FASTDDS_LINKING}-targets
        DESTINATION ${INSTALL_DESTINATION_PATH}
        COMPONENT cmake
        )

    if(MSVC OR MSVC_IDE)

        # first try dll symbols
        get_target_property(PDB_FILE ${PROJECT_NAME} PDB_NAME_DEBUG)
        if(PDB_FILE)
            get_target_property(PDB_DIR ${PROJECT_NAME} PDB_OUTPUT_DIRECTORY_DEBUG)
            set(PDB_FILE "${PDB_DIR}/${PDB_FILE}.pdb")
        else()
            # fallback to static lib symbols
            get_target_property(PDB_FILE ${PROJECT_NAME} COMPILE_PDB_NAME_DEBUG)
            if(PDB_FILE)
                get_target_property(PDB_DIR ${PROJECT_NAME} COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG)
                set(PDB_FILE "${PDB_DIR}/${PDB_FILE}.pdb")
            endif()
        endif()

        # install symbols if any
        if(PDB_FILE)
            install(FILES ${PDB_FILE}
                DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
                COMPONENT symbols
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

    if(COMPILE_TOOLS)
        set(INCLUDE_FASTDS_TARGETS "include(\${CMAKE_CURRENT_LIST_DIR}/fast-discovery-server-targets.cmake)")
    endif()

    # Add fastrtps dependencies in its CMake config file.
    if(LINK_SSL)
        if(INSTALLER_PLATFORM)
            # OpenSSL dependency is only propagated on static linking
            # We must use the deployed OpenSSL binaries to prevent compatibility issues
            set(FASTRTPS_INSTALLER_DEPS_ANCILLARY "\
function(find_fastdds_deployed_openssl)\n\
    if(OPENSSL_FOUND AND NOT(OPENSSL_VERSION STREQUAL \"${OPENSSL_VERSION}\") )\n\
        message(WARNING \"OpenSSL already loaded. Version \${OPENSSL_VERSION} differs from version ${OPENSSL_VERSION}, used in Fast-DDS binaries building, ABI issues may arise\")\n\
    endif()\n\
    set(OPENSSL_USE_STATIC_LIBS TRUE)\n\
    set(OPENSSL_ROOT_DIR  \"\${PACKAGE_PREFIX_DIR}/lib/${INSTALLER_PLATFORM}\")\n\
    find_package(OpenSSL REQUIRED)\n\
endfunction()"
            )
            set(FASTRTPS_INSTALLER_OPT_DEPS "find_fastdds_deployed_openssl()")
        elseif(WIN32)
            # windows from sources only requires OpenSSL on static libraries
            set(FASTRTPS_PACKAGE_WIN32_OPT_DEPS "find_package(OpenSSL REQUIRED)")
        else()
            # linux from sources requires OpenSSL always
            set(FASTRTPS_PACKAGE_UNIX_OPT_DEPS "find_package(OpenSSL REQUIRED)")
        endif()
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
