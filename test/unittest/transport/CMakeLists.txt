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
    if(WIN32)
        add_definitions(
            -D_WIN32_WINNT=0x0601
            -D_CRT_SECURE_NO_WARNINGS
            )
    endif()

    # Copy certs
    if(TLS_FOUND)
        configure_file(${PROJECT_SOURCE_DIR}/test/certs/maincacert.pem
            ${CMAKE_CURRENT_BINARY_DIR}/maincacert.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/certs/mainpubcert.pem
            ${CMAKE_CURRENT_BINARY_DIR}/mainpubcert.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/certs/mainpubkey.pem
            ${CMAKE_CURRENT_BINARY_DIR}/mainpubkey.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/certs/mainsubcert.pem
            ${CMAKE_CURRENT_BINARY_DIR}/mainsubcert.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/certs/mainsubkey.pem
            ${CMAKE_CURRENT_BINARY_DIR}/mainsubkey.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/unittest/transport/ca.pem
            ${CMAKE_CURRENT_BINARY_DIR}/ca.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/unittest/transport/server.pem
            ${CMAKE_CURRENT_BINARY_DIR}/server.pem COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/test/unittest/transport/dh2048.pem
            ${CMAKE_CURRENT_BINARY_DIR}/dh2048.pem COPYONLY)
        #configure_file(${PROJECT_SOURCE_DIR}/test/certs/governance_helloworld_all_enable.smime
        #    ${CMAKE_CURRENT_BINARY_DIR}/governance_helloworld_all_enable.smime COPYONLY)
        #configure_file(${PROJECT_SOURCE_DIR}/test/certs/permissions_helloworld.smime
        #    ${CMAKE_CURRENT_BINARY_DIR}/permissions_helloworld.smime COPYONLY)
    endif()

    set(UDPV4TESTS_SOURCE
        UDPv4Tests.cpp
        mock/MockReceiverResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPv4Transport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPTransportInterface.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    set(UDPV6TESTS_SOURCE
        UDPv6Tests.cpp
        mock/MockReceiverResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPv6Transport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPTransportInterface.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    set(TCPV4TESTS_SOURCE
        TCPv4Tests.cpp
        mock/MockReceiverResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSMessageCreator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/ParameterList.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPv4Transport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPTransportInterface.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResourceBasic.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptorBasic.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/tcp/RTCPMessageManager.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/tcp/TCPControlMessage.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSMessageCreator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/ResourceEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEventImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/TimedConditionVariable.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/System.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    if(TLS_FOUND)
        set(TCPV4TESTS_SOURCE
            ${TCPV4TESTS_SOURCE}
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResourceSecure.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptorSecure.cpp
            )
    endif()

    set(TCPV6TESTS_SOURCE
        TCPv6Tests.cpp
        mock/MockReceiverResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSMessageCreator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/ParameterList.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPv6Transport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPTransportInterface.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResourceBasic.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptorBasic.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/tcp/RTCPMessageManager.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/tcp/TCPControlMessage.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/ResourceEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEvent.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/TimedEventImpl.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/TimedConditionVariable.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/System.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    if(TLS_FOUND)
        set(TCPV6TESTS_SOURCE
            ${TCPV6TESTS_SOURCE}
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPChannelResourceSecure.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/TCPAcceptorSecure.cpp
            )
    endif()

    set(TEST_UDPV4TESTS_SOURCE
        test_UDPv4Tests.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/messages/RTPSMessageCreator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/core/policy/ParameterList.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/test_UDPv4Transport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPv4Transport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPTransportInterface.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/UDPChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        )

    set(SHAREDMEMTESTS_SOURCE
        SharedMemTests.cpp
        mock/MockReceiverResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/Log.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/OStreamConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/fastdds/log/StdoutErrConsumer.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/shared_mem/SharedMemTransport.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/shared_mem/SharedMemTransportDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/transport/ChannelResource.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPFinder.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/IPLocator.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/network/NetworkFactory.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/rtps/common/Time_t.cpp
        ${PROJECT_SOURCE_DIR}/src/cpp/utils/md5.cpp
        )

    include_directories(mock/)

    add_executable(UDPv4Tests ${UDPV4TESTS_SOURCE})
    target_compile_definitions(UDPv4Tests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(UDPv4Tests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/MessageReceiver
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReceiverResource
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(UDPv4Tests GTest::gtest ${MOCKS})
    if(MSVC OR MSVC_IDE)
        target_link_libraries(UDPv4Tests ${PRIVACY} iphlpapi Shlwapi )
    endif()
    add_gtest(UDPv4Tests SOURCES ${UDPV4TESTS_SOURCE})
    set(TRANSPORT_XFAIL_LIST XFAIL_UDP4)

    option(DISABLE_UDPV6_TESTS "Disable UDPv6 tests because fails in some systems" OFF)

    if(NOT DISABLE_UDPV6_TESTS)
        add_executable(UDPv6Tests ${UDPV6TESTS_SOURCE})
        target_compile_definitions(UDPv6Tests PRIVATE FASTRTPS_NO_LIB
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(UDPv6Tests PRIVATE
            ${Asio_INCLUDE_DIR}
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/MessageReceiver
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReceiverResource
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
            ${PROJECT_SOURCE_DIR}/src/cpp
            )
        target_link_libraries(UDPv6Tests GTest::gtest ${MOCKS})
        if(MSVC OR MSVC_IDE)
            target_link_libraries(UDPv6Tests ${PRIVACY} iphlpapi Shlwapi )
        endif()
        add_gtest(UDPv6Tests SOURCES ${UDPV6TESTS_SOURCE})
        set(TRANSPORT_XFAIL_LIST ${TRANSPORT_XFAIL_LIST} XFAIL_UDP6)

        add_executable(TCPv6Tests ${TCPV6TESTS_SOURCE})
        target_compile_definitions(TCPv6Tests PRIVATE FASTRTPS_NO_LIB
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )
        target_include_directories(TCPv6Tests PRIVATE
            ${Asio_INCLUDE_DIR}
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/ParticipantProxyData
            ${PROJECT_SOURCE_DIR}/test/mock/dds/QosPolicies
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/MessageReceiver
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReceiverResource
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
            ${PROJECT_SOURCE_DIR}/src/cpp
            )
        target_link_libraries(TCPv6Tests GTest::gtest ${MOCKS}
            $<$<BOOL:${TLS_FOUND}>:OpenSSL::SSL$<SEMICOLON>OpenSSL::Crypto>)
        if(MSVC OR MSVC_IDE)
            target_link_libraries(TCPv6Tests ${PRIVACY} fastcdr iphlpapi Shlwapi)
        else()
            target_link_libraries(TCPv6Tests ${PRIVACY} fastcdr)
        endif()
        add_gtest(TCPv6Tests SOURCES ${TCPV6TESTS_SOURCE})
        set(TRANSPORT_XFAIL_LIST ${TRANSPORT_XFAIL_LIST} XFAIL_TCP6)
    endif()

    add_executable(test_UDPv4Tests ${TEST_UDPV4TESTS_SOURCE})
    target_compile_definitions(test_UDPv4Tests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(test_UDPv4Tests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ParticipantProxyData
        ${PROJECT_SOURCE_DIR}/test/mock/dds/QosPolicies
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/MessageReceiver
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReceiverResource
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(test_UDPv4Tests GTest::gtest ${MOCKS})
    if(MSVC OR MSVC_IDE)
        target_link_libraries(test_UDPv4Tests ${PRIVACY} iphlpapi Shlwapi)
    else()
        target_link_libraries(test_UDPv4Tests ${PRIVACY})
    endif()
    add_gtest(test_UDPv4Tests SOURCES ${TEST_UDPV4TESTS_SOURCE})
    set(TRANSPORT_XFAIL_LIST ${TRANSPORT_XFAIL_LIST} XFAIL_TEST_UDP4)

    add_executable(TCPv4Tests ${TCPV4TESTS_SOURCE})
    target_compile_definitions(TCPv4Tests PRIVATE FASTRTPS_NO_LIB
        BOOST_ASIO_STANDALONE
        ASIO_STANDALONE
        $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
        $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
        )
    target_include_directories(TCPv4Tests PRIVATE
        ${Asio_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ParticipantProxyData
        ${PROJECT_SOURCE_DIR}/test/mock/dds/QosPolicies
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/MessageReceiver
        ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReceiverResource
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/cpp
        )
    target_link_libraries(TCPv4Tests GTest::gtest ${MOCKS}
        $<$<BOOL:${TLS_FOUND}>:OpenSSL::SSL$<SEMICOLON>OpenSSL::Crypto>)
    if(MSVC OR MSVC_IDE)
        target_link_libraries(TCPv4Tests ${PRIVACY} fastcdr iphlpapi Shlwapi)
    else()
        target_link_libraries(TCPv4Tests ${PRIVACY} fastcdr)
    endif()
    add_gtest(TCPv4Tests SOURCES ${TCPV4TESTS_SOURCE})
    set(TRANSPORT_XFAIL_LIST ${TRANSPORT_XFAIL_LIST} XFAIL_TCP4)

    if(IS_THIRDPARTY_BOOST_OK)
        add_executable(SharedMemTests ${SHAREDMEMTESTS_SOURCE})

        target_compile_definitions(SharedMemTests PRIVATE FASTRTPS_NO_LIB
            BOOST_ASIO_STANDALONE
            ASIO_STANDALONE
            $<$<BOOL:${WIN32}>:_ENABLE_ATOMIC_ALIGNMENT_FIX>
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
            $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
            )

        target_include_directories(SharedMemTests PRIVATE
            ${Asio_INCLUDE_DIR}
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/MessageReceiver
            ${PROJECT_SOURCE_DIR}/test/mock/rtps/ReceiverResource
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
            ${PROJECT_SOURCE_DIR}/src/cpp
            ${THIRDPARTY_BOOST_INCLUDE_DIR}
            )
        target_link_libraries(SharedMemTests GTest::gtest ${MOCKS}
            $<$<BOOL:${TLS_FOUND}>:OpenSSL::SSL$<SEMICOLON>OpenSSL::Crypto>
            ${THIRDPARTY_BOOST_LINK_LIBS}
            eProsima_atomic
            )
        if(MSVC OR MSVC_IDE)
            target_link_libraries(SharedMemTests ${PRIVACY} iphlpapi Shlwapi )
        else()
            target_link_libraries(SharedMemTests ${PRIVACY} )
        endif()
        add_gtest(SharedMemTests SOURCES ${SHAREDMEMTESTS_SOURCE})
        set(TRANSPORT_XFAIL_LIST ${TRANSPORT_XFAIL_LIST} XFAIL_SHM)
    endif()

    foreach(TRANSPORT_XFAIL_TEST ${TRANSPORT_XFAIL_LIST})
        add_xfail_label(${CMAKE_CURRENT_SOURCE_DIR}/${TRANSPORT_XFAIL_TEST}.list)
    endforeach()
endif()
