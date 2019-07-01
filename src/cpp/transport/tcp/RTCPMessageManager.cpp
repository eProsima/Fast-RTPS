// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file RTCPMessageManager.cpp
 *
 */
#include <fastrtps/transport/tcp/RTCPHeader.h>
#include <fastrtps/transport/tcp/RTCPMessageManager.h>
#include <fastrtps/transport/TCPChannelResource.h>
#include <fastrtps/log/Log.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/utils/System.h>
#include <fastrtps/transport/TCPTransportInterface.h>
#include <fastrtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/transport/TCPv6TransportDescriptor.h>


#define IDSTRING "(ID:" << std::this_thread::get_id() <<") "<<

using namespace eprosima::fastrtps;

namespace eprosima {
namespace fastrtps{
namespace rtps {

static void endpoint_to_locator(
        const asio::ip::tcp::endpoint& endpoint,
        Locator_t& locator)
{
    if (endpoint.protocol() == asio::ip::tcp::v4())
    {
        locator.kind = LOCATOR_KIND_TCPv4;
        auto ipBytes = endpoint.address().to_v4().to_bytes();
        IPLocator::setIPv4(locator, ipBytes.data());
    }
    else if (endpoint.protocol() == asio::ip::tcp::v6())
    {
        locator.kind = LOCATOR_KIND_TCPv6;
        auto ipBytes = endpoint.address().to_v6().to_bytes();
        IPLocator::setIPv6(locator, ipBytes.data());
    }
    IPLocator::setPhysicalPort(locator, endpoint.port());
}

static void readSerializedPayload(
        SerializedPayload_t &payload,
        const octet* data,
        size_t size)
{
    payload.reserve(static_cast<uint32_t>(size));
    memcpy(&payload.encapsulation, data, 2);
    memcpy(&payload.length, &data[2], 4);
    memcpy(payload.data, &data[6], size);
    payload.pos = 0;
}

RTCPMessageManager::~RTCPMessageManager()
{
    dispose();
}

size_t RTCPMessageManager::sendMessage(
        TCPChannelResource* channel,
        const CDRMessage_t &msg) const
{
    if (!alive())
    {
        return 0;
    }

    asio::error_code ec;
    size_t send = channel->send(nullptr, 0, msg.buffer, msg.length, ec);
    if (send != msg.length || ec)
    {
        logInfo(RTCP, "Bad sent size..." << send << " bytes of " << msg.length << " bytes: " << ec.message());
        send = 0;
    }

    //logInfo(RTCP, "Sent " << send << " bytes");
    return send;
}

bool RTCPMessageManager::sendData(
        std::shared_ptr<TCPChannelResource>& channel,
        TCPCPMKind kind,
        const TCPTransactionId& transaction_id,
        const SerializedPayload_t* payload,
        const ResponseCode respCode)
{
    if(sendData(channel.get(), kind, transaction_id, payload, respCode))
    {
        return true;
    }

    if(TCPChannelResource::TCPConnectionType::TCP_CONNECT_TYPE == channel->tcp_connection_type() &&
            TCPChannelResource::eConnectionStatus::eDisconnected == channel->connection_status())
    {
        channel->set_all_ports_pending();
        channel->connect(channel);
    }

    return false;
}

bool RTCPMessageManager::sendData(
        TCPChannelResource* channel,
        TCPCPMKind kind,
        const TCPTransactionId &transaction_id,
        const SerializedPayload_t *payload,
        const ResponseCode respCode)
{
    if (!alive())
    {
        return 0;
    }

    TCPHeader header;
    TCPControlMsgHeader ctrlHeader;
    CDRMessage_t msg;
    CDRMessage::initCDRMsg(&msg);
    const ResponseCode* code = (respCode != RETCODE_VOID) ? &respCode : nullptr;

    fillHeaders(kind, transaction_id, ctrlHeader, header, payload, code);

    RTPSMessageCreator::addCustomContent(&msg, (octet*)&header, TCPHeader::size());
    RTPSMessageCreator::addCustomContent(&msg, (octet*)&ctrlHeader, TCPControlMsgHeader::size());
    if (code != nullptr)
    {
        RTPSMessageCreator::addCustomContent(&msg, (octet*)code, 4); // uint32_t
    }
    if (payload != nullptr)
    {
        RTPSMessageCreator::addCustomContent(&msg, (octet*)&payload->encapsulation, 2); // Encapsulation
        RTPSMessageCreator::addCustomContent(&msg, (octet*)&payload->length, 4); // Length
        RTPSMessageCreator::addCustomContent(&msg, payload->data, payload->length); // Data
    }

    return sendMessage(channel, msg) > 0;
}

uint32_t& RTCPMessageManager::addToCRC(
        uint32_t &crc,
        octet data)
{
    static uint32_t max = 0xffffffff;
    if (crc + data < crc)
    {
        crc -= (max - data);
    }
    else
    {
        crc += data;
    }
    return crc;
}

void RTCPMessageManager::fillHeaders(
        TCPCPMKind kind,
        const TCPTransactionId &transaction_id,
        TCPControlMsgHeader &retCtrlHeader,
        TCPHeader &header,
        const SerializedPayload_t *payload,
        const ResponseCode *respCode)
{
    retCtrlHeader.kind(kind);
    retCtrlHeader.length() = static_cast<uint16_t>(TCPControlMsgHeader::size());
    retCtrlHeader.length() += static_cast<uint16_t>((payload != nullptr) ? (payload->length + 6) : 0);
    retCtrlHeader.length() += static_cast<uint16_t>((respCode != nullptr) ? 4 : 0);
    retCtrlHeader.transaction_id() = transaction_id;

    switch (kind)
    {
    case BIND_CONNECTION_REQUEST:
    case OPEN_LOGICAL_PORT_REQUEST:
    case CHECK_LOGICAL_PORT_REQUEST:
    case KEEP_ALIVE_REQUEST:
        retCtrlHeader.flags(false, true, true);
        addTransactionId(retCtrlHeader.transaction_id());
        break;
    case LOGICAL_PORT_IS_CLOSED_REQUEST:
    case BIND_CONNECTION_RESPONSE:
    case OPEN_LOGICAL_PORT_RESPONSE:
    case CHECK_LOGICAL_PORT_RESPONSE:
    case KEEP_ALIVE_RESPONSE:
        retCtrlHeader.flags(false, true, false);
        break;
    case UNBIND_CONNECTION_REQUEST:
        retCtrlHeader.flags(false, false, false);
        break;
    }

    retCtrlHeader.endianess(DEFAULT_ENDIAN); // Override "false" endianess set on the switch
    header.logical_port = 0; // This is a control message
    header.length = static_cast<uint32_t>(retCtrlHeader.length() + TCPHeader::size());

    // Finally, calculate the CRC

    uint32_t crc = 0;
    if (alive() && mTransport->configuration()->calculate_crc)
    {
        octet* it = (octet*)&retCtrlHeader;
        for (size_t i = 0; i < TCPControlMsgHeader::size(); ++i)
        {
            crc = addToCRC(crc, it[i]);
        }
        if (respCode != nullptr)
        {
            it = (octet*)respCode;
            for (int i = 0; i < 4; ++i)
            {
                crc = addToCRC(crc, it[i]);
            }
        }
        if (payload != nullptr)
        {
            octet* pay = (octet*)&(payload->encapsulation);
            for (uint32_t i = 0; i < 2; ++i)
            {
                crc = addToCRC(crc, pay[i]);
            }
            pay = (octet*)&(payload->length);
            for (uint32_t i = 0; i < 4; ++i)
            {
                crc = addToCRC(crc, pay[i]);
            }
            for (uint32_t i = 0; i < payload->length; ++i)
            {
                crc = addToCRC(crc, payload->data[i]);
            }
        }
    }
    header.crc = crc;
    //logInfo(RTCP, "Send (CRC= " << header.crc << ")");

    // LOG
    /*
    switch (kind)
    {
    case BIND_CONNECTION_REQUEST:
        logInfo(RTCP_SEQ, "Send [BIND_CONNECTION_REQUEST] Seq: " << retCtrlHeader.transaction_id());
        break;
    case OPEN_LOGICAL_PORT_REQUEST:
        logInfo(RTCP_SEQ, "Send [OPEN_LOGICAL_PORT_REQUEST] Seq: " << retCtrlHeader.transaction_id());
        break;
    case CHECK_LOGICAL_PORT_REQUEST:
        logInfo(RTCP_SEQ, "Send [CHECK_LOGICAL_PORT_REQUEST]: Seq: " << retCtrlHeader.transaction_id());
        break;
    case KEEP_ALIVE_REQUEST:
        logInfo(RTCP_SEQ, "Send [KEEP_ALIVE_REQUEST] Seq: " << retCtrlHeader.transaction_id());
        break;
    case LOGICAL_PORT_IS_CLOSED_REQUEST:
        logInfo(RTCP_SEQ, "Send [LOGICAL_PORT_IS_CLOSED_REQUEST] Seq: " << retCtrlHeader.transaction_id());
        break;
    case BIND_CONNECTION_RESPONSE:
        logInfo(RTCP_SEQ, "Send [BIND_CONNECTION_RESPONSE] Seq: " << retCtrlHeader.transaction_id());
        break;
    case OPEN_LOGICAL_PORT_RESPONSE:
        logInfo(RTCP_SEQ, "Send [OPEN_LOGICAL_PORT_RESPONSE] Seq: " << retCtrlHeader.transaction_id());
        break;
    case CHECK_LOGICAL_PORT_RESPONSE:
        logInfo(RTCP_SEQ, "Send [CHECK_LOGICAL_PORT_RESPONSE] Seq: " << retCtrlHeader.transaction_id());
        break;
    case KEEP_ALIVE_RESPONSE:
        logInfo(RTCP_SEQ, "Send [KEEP_ALIVE_RESPONSE] Seq: " << retCtrlHeader.transaction_id());
        break;
    case UNBIND_CONNECTION_REQUEST:
        logInfo(RTCP_SEQ, "Send [UNBIND_CONNECTION_REQUEST] Seq: " << retCtrlHeader.transaction_id());
        break;
    }
    */
}

TCPTransactionId RTCPMessageManager::sendConnectionRequest(
        std::shared_ptr<TCPChannelResource>& channel)
{
    ConnectionRequest_t request;
    Locator_t locator;
    mTransport->endpoint_to_locator(channel->local_endpoint(), locator);

    auto config = mTransport->configuration();
    if (!config->listening_ports.empty())
    {
        IPLocator::setPhysicalPort(locator, *(config->listening_ports.begin()));
    }
    else
    {
        IPLocator::setPhysicalPort(locator, static_cast<uint16_t>(System::GetPID()));
    }

    if (locator.kind == LOCATOR_KIND_TCPv4)
    {
        const TCPv4TransportDescriptor* pTCPv4Desc = static_cast<TCPv4TransportDescriptor*>(config);
        IPLocator::setWan(locator, pTCPv4Desc->wan_addr[0], pTCPv4Desc->wan_addr[1], pTCPv4Desc->wan_addr[2],
            pTCPv4Desc->wan_addr[3]);
    }
    request.protocolVersion(c_rtcpProtocolVersion);
    request.transportLocator(locator);

    SerializedPayload_t payload(static_cast<uint32_t>(ConnectionRequest_t::getBufferCdrSerializedSize(request)));
    request.serialize(&payload);

    logInfo(RTCP_MSG, "Send [BIND_CONNECTION_REQUEST] PhysicalPort: " << IPLocator::getPhysicalPort(locator));
    //logError(DEBUG, "Sending Connection Request with locator: " << IPLocator::to_string(request.transportLocator()));
    channel->change_status(TCPChannelResource::eConnectionStatus::eWaitingForBindResponse);
    TCPTransactionId id = getTransactionId();
    bool success = sendData(channel, BIND_CONNECTION_REQUEST, id, &payload);
    if (!success)
    {
        logError(RTCP, "Failed sending Connection Request");
    }
    return id;
}

TCPTransactionId RTCPMessageManager::sendOpenLogicalPortRequest(
        TCPChannelResource* channel,
        uint16_t port)
{
    OpenLogicalPortRequest_t request;
    request.logicalPort(port);
    return sendOpenLogicalPortRequest(channel, request);
}

TCPTransactionId RTCPMessageManager::sendOpenLogicalPortRequest(
        TCPChannelResource* channel,
        OpenLogicalPortRequest_t &request)
{
    SerializedPayload_t payload(static_cast<uint32_t>(OpenLogicalPortRequest_t::getBufferCdrSerializedSize(request)));
    request.serialize(&payload);
    logInfo(RTCP_MSG, "Send [OPEN_LOGICAL_PORT_REQUEST] LogicalPort: " << request.logicalPort());
    TCPTransactionId id = getTransactionId();
    sendData(channel, OPEN_LOGICAL_PORT_REQUEST, id, &payload);
    return id;
}

TCPTransactionId RTCPMessageManager::sendCheckLogicalPortsRequest(
        TCPChannelResource* channel,
        std::vector<uint16_t> &ports)
{
    CheckLogicalPortsRequest_t request;
    request.logicalPortsRange(ports);
    return sendCheckLogicalPortsRequest(channel, request);
}

TCPTransactionId RTCPMessageManager::sendCheckLogicalPortsRequest(
        TCPChannelResource* channel,
        CheckLogicalPortsRequest_t &request)
{
    SerializedPayload_t payload(static_cast<uint32_t>(CheckLogicalPortsRequest_t::getBufferCdrSerializedSize(request)));
    request.serialize(&payload);
    logInfo(RTCP_MSG, "Send [CHECK_LOGICAL_PORT_REQUEST]");
    TCPTransactionId id = getTransactionId();
    sendData(channel, CHECK_LOGICAL_PORT_REQUEST, id, &payload);
    return id;
}

TCPTransactionId RTCPMessageManager::sendKeepAliveRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        KeepAliveRequest_t &request,
        const std::chrono::time_point<std::chrono::system_clock>& new_timeout)
{
    SerializedPayload_t payload(static_cast<uint32_t>(KeepAliveRequest_t::getBufferCdrSerializedSize(request)));
    request.serialize(&payload);
    logInfo(RTCP_MSG, "Send [KEEP_ALIVE_REQUEST]");
    TCPTransactionId id = getTransactionId();
    channel->update_keep_alive_timeout(new_timeout);
    sendData(channel, KEEP_ALIVE_REQUEST, id, &payload, RETCODE_VOID);
    return id;
}

TCPTransactionId RTCPMessageManager::sendKeepAliveRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        const std::chrono::time_point<std::chrono::system_clock>& new_timeout)
{
    KeepAliveRequest_t request;
    request.locator(channel->locator());
    return sendKeepAliveRequest(channel, request, new_timeout);
}

TCPTransactionId RTCPMessageManager::sendLogicalPortIsClosedRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        LogicalPortIsClosedRequest_t &request)
{
    SerializedPayload_t payload(static_cast<uint32_t>(
        LogicalPortIsClosedRequest_t::getBufferCdrSerializedSize(request)));

    request.serialize(&payload);
    logInfo(RTCP_MSG, "Send [LOGICAL_PORT_IS_CLOSED_REQUEST] LogicalPort: " << request.logicalPort());
    TCPTransactionId id = getTransactionId();
    sendData(channel, LOGICAL_PORT_IS_CLOSED_REQUEST, id, &payload);
    return id;
}

TCPTransactionId RTCPMessageManager::sendLogicalPortIsClosedRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        uint16_t port)
{
    LogicalPortIsClosedRequest_t request;
    request.logicalPort(port);
    return sendLogicalPortIsClosedRequest(channel, request);
}

TCPTransactionId RTCPMessageManager::sendUnbindConnectionRequest(
        std::shared_ptr<TCPChannelResource>& channel)
{
    logInfo(RTCP_MSG, "Send [UNBIND_CONNECTION_REQUEST]");
    TCPTransactionId id = getTransactionId();
    sendData(channel, UNBIND_CONNECTION_REQUEST, id);
    return id;
}

ResponseCode RTCPMessageManager::processBindConnectionRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        const ConnectionRequest_t &request,
        const TCPTransactionId &transaction_id,
        Locator_t &localLocator)
{
    BindConnectionResponse_t response;

    if (localLocator.kind == LOCATOR_KIND_TCPv4)
    {
        const TCPv4TransportDescriptor* pTCPv4Desc = (TCPv4TransportDescriptor*)mTransport->get_configuration();
        IPLocator::setWan(localLocator, pTCPv4Desc->wan_addr[0], pTCPv4Desc->wan_addr[1], pTCPv4Desc->wan_addr[2],
            pTCPv4Desc->wan_addr[3]);
    }
    else if (localLocator.kind == LOCATOR_KIND_TCPv6)
    {
    }
    else
    {
        assert(false);
    }

    response.locator(localLocator);

    SerializedPayload_t payload(static_cast<uint32_t>(BindConnectionResponse_t::getBufferCdrSerializedSize(response)));
    response.serialize(&payload);

    if (!isCompatibleProtocol(request.protocolVersion()))
    {
        sendData(channel, BIND_CONNECTION_RESPONSE, transaction_id, &payload, RETCODE_INCOMPATIBLE_VERSION);
        logWarning(RTCP, "Rejected client due to INCOMPATIBLE_VERSION: Expected: " << c_rtcpProtocolVersion
            << " but received " << request.protocolVersion());
        return RETCODE_INCOMPATIBLE_VERSION;
    }

    //logError(DEBUG, "Receive Connection Request with locator: " << IPLocator::to_string(request.transportLocator())
    //    << " and will respond with our locator: " << response.locator());

    ResponseCode code = channel->process_bind_request(request.transportLocator());

    if(RETCODE_OK == code)
    {
        // In server side, at this moment, the channel has to be included in the map.
        mTransport->bind_socket(channel);
    }

    sendData(channel, BIND_CONNECTION_RESPONSE, transaction_id, &payload, code);

    return RETCODE_OK;
}

ResponseCode RTCPMessageManager::processOpenLogicalPortRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        const OpenLogicalPortRequest_t &request,
        const TCPTransactionId &transaction_id)
{
    if (!channel->connection_established())
    {
        sendData(channel, CHECK_LOGICAL_PORT_RESPONSE, transaction_id, nullptr, RETCODE_SERVER_ERROR);
    }
    else if (request.logicalPort() == 0 || !mTransport->is_input_port_open(request.logicalPort()))
    {
        logInfo(RTCP_MSG, "Send [OPEN_LOGICAL_PORT_RESPONSE] Not found: " << request.logicalPort());
        sendData(channel, OPEN_LOGICAL_PORT_RESPONSE, transaction_id, nullptr, RETCODE_INVALID_PORT);
    }
    else
    {
        logInfo(RTCP_MSG, "Send [OPEN_LOGICAL_PORT_RESPONSE] Found: " << request.logicalPort());
        sendData(channel, OPEN_LOGICAL_PORT_RESPONSE, transaction_id, nullptr, RETCODE_OK);
    }
    return RETCODE_OK;
}

void RTCPMessageManager::processCheckLogicalPortsRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        const CheckLogicalPortsRequest_t &request,
        const TCPTransactionId &transaction_id)
{
    CheckLogicalPortsResponse_t response;
    if (!channel->connection_established())
    {
        sendData(channel, CHECK_LOGICAL_PORT_RESPONSE, transaction_id, nullptr, RETCODE_SERVER_ERROR);
    }
    else
    {
        if (request.logicalPortsRange().empty())
        {
            logWarning(RTCP, "No available logical ports.");
        }
        else
        {
            for (uint16_t port : request.logicalPortsRange())
            {
                if (mTransport->is_input_port_open(port))
                {
                    if (port == 0)
                    {
                        logInfo(RTCP, "FoundOpenedLogicalPort 0, but will not be considered");
                    }
                    logInfo(RTCP, "FoundOpenedLogicalPort: " << port);
                    response.availableLogicalPorts().emplace_back(port);
                }
            }
        }

        SerializedPayload_t payload(static_cast<uint32_t>(
            CheckLogicalPortsResponse_t::getBufferCdrSerializedSize(response)));
        response.serialize(&payload);
        sendData(channel, CHECK_LOGICAL_PORT_RESPONSE, transaction_id, &payload, RETCODE_OK);
    }
}

ResponseCode RTCPMessageManager::processKeepAliveRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        const KeepAliveRequest_t &request,
        const TCPTransactionId &transaction_id)
{
    if (!channel->connection_established())
    {
        sendData(channel, KEEP_ALIVE_RESPONSE, transaction_id, nullptr, RETCODE_SERVER_ERROR);
    }
    else if (IPLocator::getLogicalPort(channel->locator()) == IPLocator::getLogicalPort(request.locator()))
    {
        sendData(channel, KEEP_ALIVE_RESPONSE, transaction_id, nullptr, RETCODE_OK);
    }
    else
    {
        sendData(channel, KEEP_ALIVE_RESPONSE, transaction_id, nullptr, RETCODE_UNKNOWN_LOCATOR);
        return RETCODE_UNKNOWN_LOCATOR;
    }
    return RETCODE_OK;
}

void RTCPMessageManager::processLogicalPortIsClosedRequest(
        std::shared_ptr<TCPChannelResource>& channel,
        const LogicalPortIsClosedRequest_t &request,
        const TCPTransactionId & transaction_id)
{
    if (!channel->connection_established())
    {
        sendData(channel, CHECK_LOGICAL_PORT_RESPONSE, transaction_id, nullptr, RETCODE_SERVER_ERROR);
    }
    else
    {
        channel->set_logical_port_pending(request.logicalPort());
    }
}

ResponseCode RTCPMessageManager::processBindConnectionResponse(
        std::shared_ptr<TCPChannelResource>& channel,
        const BindConnectionResponse_t &,
        const TCPTransactionId &transaction_id)
{
    if (findTransactionId(transaction_id))
    {
        logInfo(RTCP, "Connection established (Resp) (physical: "
                << IPLocator::getPhysicalPort(channel->locator()) << ")");
        channel->change_status(TCPChannelResource::eConnectionStatus::eEstablished, this);
        removeTransactionId(transaction_id);
        //logError(DEBUG, "Received Connection Response with locator: " << response.locator());
        return RETCODE_OK;
    }
    else
    {
        logWarning(RTCP, "Received BindConnectionResponse with an invalid transaction_id: " << transaction_id);
        return RETCODE_VOID;
    }
}

ResponseCode RTCPMessageManager::processCheckLogicalPortsResponse(
        std::shared_ptr<TCPChannelResource>& channel,
        const CheckLogicalPortsResponse_t &response,
        const TCPTransactionId &transaction_id)
{
    if (findTransactionId(transaction_id))
    {
        channel->process_check_logical_ports_response(transaction_id, response.availableLogicalPorts(), this);
        removeTransactionId(transaction_id);
        return RETCODE_OK;
    }
    else
    {
        logWarning(RTCP, "Received CheckLogicalPortsResponse with an invalid transaction_id: " << transaction_id);
        return RETCODE_VOID;
    }
}

ResponseCode RTCPMessageManager::processOpenLogicalPortResponse(
        std::shared_ptr<TCPChannelResource>& channel,
        ResponseCode respCode,
        const TCPTransactionId &transaction_id)
{
    if (findTransactionId(transaction_id))
    {
        switch (respCode)
        {
        case RETCODE_OK:
        {
            channel->add_logical_port_response(transaction_id, true, this);
        }
        break;
        case RETCODE_INVALID_PORT:
        {
            channel->add_logical_port_response(transaction_id, false, this);
        }
        break;
        default:
            logWarning(RTCP, "Received response for OpenLogicalPort with error code: "
                << ((respCode == RETCODE_BAD_REQUEST) ? "BAD_REQUEST" : "SERVER_ERROR"));
            break;
        }
        removeTransactionId(transaction_id);
    }
    else
    {
        logWarning(RTCP, "Received OpenLogicalPortResponse with an invalid transaction_id: " << transaction_id);
    }
    return RETCODE_OK;
}

ResponseCode RTCPMessageManager::processKeepAliveResponse(
        std::shared_ptr<TCPChannelResource>& channel,
        ResponseCode respCode,
        const TCPTransactionId &transaction_id)
{
    if (findTransactionId(transaction_id))
    {
        switch (respCode)
        {
        case RETCODE_OK:
            channel->update_keep_alive_timeout(
                std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(0)));
            //logError(DEBUG, "Received keep alive " << IPLocator::to_string(channel->locator()));
            break;
        case RETCODE_UNKNOWN_LOCATOR:
            return RETCODE_UNKNOWN_LOCATOR;
        default:
            break;
        }
        removeTransactionId(transaction_id);
    }
    else
    {
        logWarning(RTCP, "Received response for KeepAlive with an unexpected transaction_id: " << transaction_id);
    }
    return RETCODE_OK;
}

ResponseCode RTCPMessageManager::processRTCPMessage(
        std::shared_ptr<TCPChannelResource>& channel,
        octet* receive_buffer,
        size_t receivedSize)
{
    ResponseCode responseCode(RETCODE_OK);

    TCPControlMsgHeader controlHeader = *(reinterpret_cast<TCPControlMsgHeader*>(receive_buffer));
    //memcpy(&controlHeader, receive_buffer, TCPControlMsgHeader::size());
    size_t dataSize = controlHeader.length() - TCPControlMsgHeader::size();
    size_t bufferSize = dataSize + 4;

    // Message size checking.
    if (dataSize + TCPControlMsgHeader::size() != receivedSize)
    {
        sendData(channel, controlHeader.kind(), controlHeader.transaction_id(),
            nullptr, RETCODE_BAD_REQUEST);
        return RETCODE_OK;
    }

    switch (controlHeader.kind())
    {
    case BIND_CONNECTION_REQUEST:
    {
        //logInfo(RTCP_SEQ, "Receive [BIND_CONNECTION_REQUEST] Seq: " << controlHeader.transaction_id());
        ConnectionRequest_t request;
        Locator_t myLocator;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        endpoint_to_locator(channel->local_endpoint(), myLocator);

        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size()]), dataSize);
        request.deserialize(&payload);

        logInfo(RTCP_MSG, "Receive [BIND_CONNECTION_REQUEST] " <<
            "LogicalPort: " << IPLocator::getLogicalPort(request.transportLocator())
            << ", Physical remote: " << IPLocator::getPhysicalPort(request.transportLocator()));

        responseCode = processBindConnectionRequest(channel, request, controlHeader.transaction_id(), myLocator);
    }
    break;
    case BIND_CONNECTION_RESPONSE:
    {
        //logInfo(RTCP_SEQ, "Receive [BIND_CONNECTION_RESPONSE] Seq: " << controlHeader.transaction_id());
        ResponseCode respCode;
        BindConnectionResponse_t response;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        memcpy(&respCode, &(receive_buffer[TCPControlMsgHeader::size()]), 4); // uint32_t
        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size() + 4]), dataSize);
        response.deserialize(&payload);

        logInfo(RTCP_MSG, "Receive [BIND_CONNECTION_RESPONSE] LogicalPort: " \
            << IPLocator::getLogicalPort(response.locator()) << ", Physical remote: " \
            << IPLocator::getPhysicalPort(response.locator()));

        if (respCode == RETCODE_OK || respCode == RETCODE_EXISTING_CONNECTION)
        {
            std::unique_lock<std::recursive_mutex> scopedLock(channel->pending_logical_mutex_);
            if (!channel->pending_logical_output_ports_.empty())
            {
                responseCode = processBindConnectionResponse(channel, response, controlHeader.transaction_id());
            }
        }
        else
        {
            // If the bind message fails, close the connection and try again.
            if (respCode == RETCODE_INCOMPATIBLE_VERSION)
            {
                logError(RTCP, "Received RETCODE_INCOMPATIBLE_VERSION from server.");
            }
            responseCode = respCode;
        }
    }
    break;
    case OPEN_LOGICAL_PORT_REQUEST:
    {
        //logInfo(RTCP_SEQ, "Receive [OPEN_LOGICAL_PORT_REQUEST] Seq: " << controlHeader.transaction_id());
        OpenLogicalPortRequest_t request;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size()]), dataSize);
        request.deserialize(&payload);
        logInfo(RTCP_MSG, "Receive [OPEN_LOGICAL_PORT_REQUEST] LogicalPort: " << request.logicalPort());
        responseCode = processOpenLogicalPortRequest(channel, request, controlHeader.transaction_id());
    }
    break;
    case CHECK_LOGICAL_PORT_REQUEST:
    {
        //logInfo(RTCP_SEQ, "Receive [CHECK_LOGICAL_PORT_REQUEST] Seq: " << controlHeader.transaction_id());
        CheckLogicalPortsRequest_t request;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size()]), dataSize);
        request.deserialize(&payload);
        logInfo(RTCP_MSG, "Receive [CHECK_LOGICAL_PORT_REQUEST]");
        processCheckLogicalPortsRequest(channel, request, controlHeader.transaction_id());
    }
    break;
    case CHECK_LOGICAL_PORT_RESPONSE:
    {
        //logInfo(RTCP_SEQ, "Receive [CHECK_LOGICAL_PORT_RESPONSE] Seq: " << controlHeader.transaction_id());
        ResponseCode respCode;
        CheckLogicalPortsResponse_t response;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        memcpy(&respCode, &(receive_buffer[TCPControlMsgHeader::size()]), 4); // uint32_t
        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size() + 4]), dataSize);
        response.deserialize(&payload);
        logInfo(RTCP_MSG, "Receive [CHECK_LOGICAL_PORT_RESPONSE]");
        processCheckLogicalPortsResponse(channel, response, controlHeader.transaction_id());
    }
    break;
    case KEEP_ALIVE_REQUEST:
    {
        //logInfo(RTCP_SEQ, "Receive [KEEP_ALIVE_REQUEST] Seq: " << controlHeader.transaction_id());
        KeepAliveRequest_t request;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size()]), dataSize);
        request.deserialize(&payload);
        logInfo(RTCP_MSG, "Receive [KEEP_ALIVE_REQUEST]");
        responseCode = processKeepAliveRequest(channel, request, controlHeader.transaction_id());
    }
    break;
    case LOGICAL_PORT_IS_CLOSED_REQUEST:
    {
        //logInfo(RTCP_SEQ, "Receive [LOGICAL_PORT_IS_CLOSED_REQUEST] Seq: " << controlHeader.transaction_id());
        LogicalPortIsClosedRequest_t request;
        SerializedPayload_t payload(static_cast<uint32_t>(bufferSize));
        readSerializedPayload(payload, &(receive_buffer[TCPControlMsgHeader::size()]), dataSize);
        request.deserialize(&payload);
        logInfo(RTCP_MSG, "Receive [LOGICAL_PORT_IS_CLOSED_REQUEST] LogicalPort: " << request.logicalPort());
        processLogicalPortIsClosedRequest(channel, request, controlHeader.transaction_id());
    }
    break;
    case UNBIND_CONNECTION_REQUEST:
    {
        //logInfo(RTCP_SEQ, "Receive [UNBIND_CONNECTION_REQUEST] Seq:" << controlHeader.transaction_id());
        logInfo(RTCP_MSG, "Receive [UNBIND_CONNECTION_REQUEST]");
        if (alive())
        {
            mTransport->close_tcp_socket(channel);
            //channel.reset();
        }
        responseCode = RETCODE_OK;
    }
    break;
    case OPEN_LOGICAL_PORT_RESPONSE:
    {
        //logInfo(RTCP_SEQ, "Receive [OPEN_LOGICAL_PORT_RESPONSE] Seq: " << controlHeader.transaction_id());
        ResponseCode respCode;
        memcpy(&respCode, &(receive_buffer[TCPControlMsgHeader::size()]), 4);
        logInfo(RTCP_MSG, "Receive [OPEN_LOGICAL_PORT_RESPONSE]");
        processOpenLogicalPortResponse(channel, respCode, controlHeader.transaction_id());
    }
    break;
    case KEEP_ALIVE_RESPONSE:
    {
        //logInfo(RTCP_SEQ, "Receive [KEEP_ALIVE_RESPONSE] Seq: " << controlHeader.transaction_id());
        ResponseCode respCode;
        memcpy(&respCode, &(receive_buffer[TCPControlMsgHeader::size()]), 4);
        logInfo(RTCP_MSG, "Receive [KEEP_ALIVE_RESPONSE]");
        responseCode = processKeepAliveResponse(channel, respCode, controlHeader.transaction_id());
    }
    break;
    default:
        sendData(channel, controlHeader.kind(), controlHeader.transaction_id(), nullptr, RETCODE_BAD_REQUEST);
        break;
    }
    return responseCode;
}

bool RTCPMessageManager::isCompatibleProtocol(const ProtocolVersion_t &protocol) const
{
    return protocol == c_rtcpProtocolVersion;
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
