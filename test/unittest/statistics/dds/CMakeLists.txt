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
        add_definitions(-DNOMINMAX)
    endif()

    ## StatisticsDomainParticipantTests
    set(STATISTICS_DOMAINPARTICIPANT_TESTS_SOURCE
        StatisticsDomainParticipantTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/typesPubSubTypes.cxx
        ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/types.cxx)

    add_executable(StatisticsDomainParticipantTests ${STATISTICS_DOMAINPARTICIPANT_TESTS_SOURCE})
    target_compile_definitions(StatisticsDomainParticipantTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNAL_DEBUG> # Internal debug activated.
        )
    target_include_directories(StatisticsDomainParticipantTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include ${PROJECT_SOURCE_DIR}/src/cpp)
    target_link_libraries(StatisticsDomainParticipantTests fastrtps fastcdr GTest::gtest)
    add_gtest(StatisticsDomainParticipantTests SOURCES ${STATISTICS_DOMAINPARTICIPANT_TESTS_SOURCE})

    ## StatisticsQosTests
    set(STATISTICS_QOS_TESTS_SOURCE
        StatisticsQosTests.cpp)

    add_executable(StatisticsQosTests ${STATISTICS_QOS_TESTS_SOURCE})
    target_compile_definitions(StatisticsQosTests PRIVATE FASTRTPS_NO_LIB
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNAL_DEBUG> # Internal debug activated.
        )
    target_include_directories(StatisticsQosTests PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include ${PROJECT_SOURCE_DIR}/src/cpp)
    target_link_libraries(StatisticsQosTests fastrtps fastcdr GTest::gtest)
    add_gtest(StatisticsQosTests SOURCES ${STATISTICS_QOS_TESTS_SOURCE})

    ## StatisticsDomainParticipantListenerTests
    if (FASTDDS_STATISTICS)
        set(DOMAINPARTICIPANTSTATISTICSLISTENER_TESTS_SOURCE
            DomainParticipantStatisticsListenerTests.cpp)

        add_executable(DomainParticipantStatisticsListenerTests
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipantStatisticsListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/typesPubSubTypes.cxx
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/types.cxx
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
            ${DOMAINPARTICIPANTSTATISTICSLISTENER_TESTS_SOURCE})
        target_compile_definitions(DomainParticipantStatisticsListenerTests PRIVATE FASTRTPS_NO_LIB
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNAL_DEBUG> # Internal debug activated.
            )
        target_include_directories(DomainParticipantStatisticsListenerTests PRIVATE
            DomainParticipantStatisticsListenerTests/mock
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_BINARY_DIR}/include
            ${PROJECT_SOURCE_DIR}/src/cpp)
        target_link_libraries(DomainParticipantStatisticsListenerTests
            fastcdr GTest::gmock )
        add_gtest(DomainParticipantStatisticsListenerTests
            SOURCES ${DOMAINPARTICIPANTSTATISTICSLISTENER_TESTS_SOURCE})
    endif()

    ## StatisticsDomainParticipantMockTests
    if (SQLITE3_SUPPORT AND FASTDDS_STATISTICS)
        set(STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE
            StatisticsDomainParticipantMockTests.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/AnnotationDescriptor.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/AnnotationParameterValue.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/BuiltinAnnotationsTypeObject.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicData.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicDataFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicPubSubType.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicType.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilder.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilderFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilderPtr.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeMember.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypePtr.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/MemberDescriptor.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeDescriptor.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeIdentifier.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeIdentifierTypes.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeNamesGenerator.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObject.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObjectFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObjectHashId.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypesBase.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/builtin/common/RPCHeadersImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/builtin/typelookup/TypeLookupManager.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/builtin/typelookup/TypeLookupReplyListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/builtin/typelookup/TypeLookupRequestListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/builtin/typelookup/common/TypeLookupTypes.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/ParameterList.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/QosPolicyUtils.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/DomainParticipant.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/DomainParticipantFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/qos/DomainParticipantQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/FileConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/DataWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/DataWriterImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/PublisherImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/DataWriterQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/PublisherQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/WriterQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/DataReader.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/DataReaderImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/DataReaderQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/ReaderQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/Subscriber.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/SubscriberImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/SubscriberQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/Topic.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/TopicImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/TypeSupport.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/qos/TopicQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastrtps_deprecated/publisher/PublisherHistory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/fastrtps_deprecated/subscriber/SubscriberHistory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/attributes/PropertyPolicy.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/BuiltinProtocols.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/data/ParticipantProxyData.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/data/ReaderProxyData.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/data/WriterProxyData.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/database/backup/SharedBackupFunctions.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/database/DiscoveryDataBase.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/database/DiscoveryParticipantsAckStatus.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/database/DiscoveryParticipantInfo.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/database/DiscoverySharedInfo.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDP.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDPClient.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDPServer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDPServerListeners.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDPSimple.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDPSimpleListeners.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/endpoint/EDPStatic.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/DirectMessageSender.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/PDP.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/PDPClient.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/PDPListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/PDPServer.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/PDPServerListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/PDPSimple.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/timedevent/DSClientEvent.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/discovery/participant/timedevent/DServerEvent.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/liveliness/WLP.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/builtin/liveliness/WLPListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/DataSharing/DataSharingListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/DataSharing/DataSharingNotification.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/DataSharing/DataSharingPayloadPool.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/FlowControllerConsts.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/FlowControllerFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/CacheChangePool.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/History.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/ReaderHistory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/TopicPayloadPool.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/TopicPayloadPoolRegistry.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/WriterHistory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/MessageReceiver.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSGapBuilder.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSMessageCreator.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSMessageGroup.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/SendBuffersManager.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/ReceiverResource.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/participant/RTPSParticipant.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/participant/RTPSParticipantImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/persistence/PersistenceFactory.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/persistence/sqlite3.c
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/persistence/SQLite3PersistenceService.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader/RTPSReader.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader/StatefulPersistentReader.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader/StatefulReader.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader/StatelessPersistentReader.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader/StatelessReader.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/reader/WriterProxy.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/ResourceEvent.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEvent.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEventImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/RTPSDomain.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptor.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptorBasic.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResource.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResourceBasic.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/tcp/TCPControlMessage.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/tcp/RTCPMessageManager.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPTransportInterface.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPv4Transport.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPv6Transport.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPChannelResource.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPTransportInterface.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPv4Transport.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPv6Transport.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/shared_mem/SharedMemTransportDescriptor.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/LivelinessManager.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/LocatorSelectorSender.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/PersistentWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/ReaderLocator.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/ReaderProxy.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/RTPSWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/StatefulPersistentWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/StatefulWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/StatelessPersistentWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/StatelessWriter.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLDynamicParser.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLElementParser.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLEndpointParser.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLParser.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLParserCommon.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLProfileManager.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipant.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipantImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipantStatisticsListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/publisher/qos/DataWriterQos.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/rtps/StatisticsBase.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/rtps/writer/StatisticsWriterImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/rtps/reader/StatisticsReaderImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/types.cxx
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/typesPubSubTypes.cxx
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/StringMatching.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/string_convert.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/SystemInfo.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/utils/TimedConditionVariable.cpp
            )

        # SHM Transport
        if(IS_THIRDPARTY_BOOST_OK)
            list(APPEND STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/shared_mem/test_SharedMemTransport.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/shared_mem/SharedMemTransport.cpp
                )
        endif()

        # TLS Support
        if(TLS_FOUND)
            list(APPEND STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResourceSecure.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptorSecure.cpp
                )
        endif()

        # Security Support
        if(SECURITY)
            list(APPEND STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/exceptions/Exception.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Token.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/exceptions/SecurityException.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/common/SharedSecretHandle.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/logging/Logging.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/SecurityManager.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/rtps/security/SecurityPluginFactory.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/authentication/PKIDH.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/Permissions.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/cryptography/AESGCMGMAC.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/cryptography/AESGCMGMAC_KeyExchange.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/cryptography/AESGCMGMAC_KeyFactory.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/cryptography/AESGCMGMAC_Transform.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/cryptography/AESGCMGMAC_Types.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/authentication/PKIIdentityHandle.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/authentication/PKIHandshakeHandle.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/AccessPermissionsHandle.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/CommonParser.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/GovernanceParser.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/accesscontrol/PermissionsParser.cpp
                ${PROJECT_SOURCE_DIR}/src/cpp/security/logging/LogTopic.cpp
                )
        endif()

        # External sources
        if(TINYXML2_SOURCE_DIR)
            list(APPEND STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE
                ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
                )
        endif()

        include_directories(${TINYXML2_INCLUDE_DIR})

        add_executable(StatisticsDomainParticipantMockTests ${STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE})
        target_compile_definitions(StatisticsDomainParticipantMockTests PRIVATE FASTRTPS_NO_LIB
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
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
        target_include_directories(StatisticsDomainParticipantMockTests PRIVATE
            ${OPENSSL_INCLUDE_DIR}
            StatisticsDomainParticipantMockTests/mock/
            ${PROJECT_SOURCE_DIR}/test/mock/dds/Publisher/
            ${PROJECT_SOURCE_DIR}/test/mock/dds/DomainParticipantImpl/
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_BINARY_DIR}/include
            ${Asio_INCLUDE_DIR}
            ${PROJECT_SOURCE_DIR}/src/cpp
            ${THIRDPARTY_BOOST_INCLUDE_DIR})
        target_link_libraries(StatisticsDomainParticipantMockTests ${PRIVACY}
            fastcdr
            foonathan_memory
            GTest::gmock
            ${CMAKE_DL_LIBS}
            ${TINYXML2_LIBRARY}
            $<$<BOOL:${LINK_SSL}>:OpenSSL::SSL$<SEMICOLON>OpenSSL::Crypto>
            $<$<BOOL:${WIN32}>:iphlpapi$<SEMICOLON>Shlwapi>
            ${THIRDPARTY_BOOST_LINK_LIBS}
            eProsima_atomic
            )
        if(MSVC OR MSVC_IDE)
            target_link_libraries(StatisticsDomainParticipantMockTests ${PRIVACY}
                iphlpapi Shlwapi
                )
        endif()
        add_gtest(StatisticsDomainParticipantMockTests SOURCES ${STATISTICS_DOMAINPARTICIPANT_MOCK_TESTS_SOURCE})
    endif()
endif()
