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
    set(LISTENERTESTS_SOURCE ListenerTests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/QosPolicyUtils.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/DomainParticipant.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/DomainParticipantFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/DomainParticipantImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/qos/DomainParticipantQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/domain/qos/DomainParticipantFactoryQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/Publisher.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/PublisherImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/DataWriter.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/DataWriterImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/PublisherQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/DataWriterQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/publisher/qos/WriterQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/Subscriber.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/SubscriberImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/DataReader.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/DataReaderImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/SubscriberQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/DataReaderQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/subscriber/qos/ReaderQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/Topic.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/qos/TopicQos.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/TopicImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/topic/TypeSupport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/ParameterList.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/FileConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/attributes/PropertyPolicy.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/FlowControllerConsts.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/TopicPayloadPool.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/history/TopicPayloadPoolRegistry.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/writer/LocatorSelectorSender.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/AnnotationDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicData.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicDataFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicType.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicPubSubType.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypePtr.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicDataPtr.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilderPtr.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeBuilderFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/DynamicTypeMember.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/MemberDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/AnnotationParameterValue.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeIdentifier.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeIdentifierTypes.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObject.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObjectFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeObjectHashId.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypeNamesGenerator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/TypesBase.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/dynamic-types/BuiltinAnnotationsTypeObject.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLProfileManager.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLDynamicParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLElementParser.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/xmlparser/XMLParserCommon.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/TimedConditionVariable.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/SystemInfo.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/string_convert.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastrtps_deprecated/publisher/PublisherHistory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastrtps_deprecated/subscriber/SubscriberHistory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipant.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/publisher/qos/DataWriterQos.cpp
        )

    if (FASTDDS_STATISTICS)

        set(statistics_sources
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipantImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/fastdds/domain/DomainParticipantStatisticsListener.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/types.cxx
            ${PROJECT_SOURCE_DIR}/src/cpp/statistics/types/typesPubSubTypes.cxx
            )

        list(APPEND LISTENERTESTS_SOURCE ${statistics_sources})

    endif()

    # External sources
    if(TINYXML2_SOURCE_DIR)
        list(APPEND LISTENERTESTS_SOURCE
            ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
            )
    endif()

    include_directories(${TINYXML2_INCLUDE_DIR})

    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()

    add_executable(ListenerTests ${LISTENERTESTS_SOURCE})
    target_compile_definitions(ListenerTests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<BOOL:${ANDROID}>,$<NOT:$<BOOL:${HAVE_CXX14}>>,$<NOT:$<BOOL:${HAVE_CXX1Y}>>>:ASIO_DISABLE_STD_STRING_VIEW>
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(ListenerTests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/DataSharingPayloadPool
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSReader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSDomain
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSDomainImpl
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSParticipant
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/RTPSParticipantImpl
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/Endpoint
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/PDP
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/PDPSimple
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatefulWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatelessWriter
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatefulReader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/StatelessReader
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/WriterHistory
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReaderHistory
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ResourceEvent
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TimedEvent
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/UDPTransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/UDPv4TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/UDPv6TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TCPTransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TCPv4TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TCPv6TransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/SharedMemTransportDescriptor
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/TypeLookupManager
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/WLP
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )

    target_link_libraries(ListenerTests fastcdr foonathan_memory
        ${TINYXML2_LIBRARY}
        GTest::gmock
        ${CMAKE_DL_LIBS})
    if(MSVC OR MSVC_IDE)
        target_link_libraries(ListenerTests iphlpapi Shlwapi ws2_32)
    endif()

    add_gtest(ListenerTests SOURCES ${LISTENERTESTS_SOURCE})

endif()
