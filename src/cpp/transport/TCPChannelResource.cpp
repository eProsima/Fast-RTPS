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

#include <fastrtps/transport/TCPChannelResource.h>
#include <fastrtps/transport/TCPTransportInterface.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/utils/eClock.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
 * Search for the base port in the current domain without taking account the participant
 */
static uint16_t GetBaseAutoPort(uint16_t currentPort)
{
    if (currentPort < 7411)
    {
        return currentPort;
    }
    uint16_t aux = currentPort - 7411; // base + offset3
    uint16_t domain = static_cast<uint16_t>(aux / 250.);
    uint16_t part = static_cast<uint16_t>(aux % 250);
    part = part / 2;

    return 7411 + (domain * 250); // And participant 0
}

TCPChannelResource::TCPChannelResource(
        TCPTransportInterface* parent,
        const Locator_t& locator,
        uint32_t maxMsgSize)
    : ChannelResource(maxMsgSize)
    , parent_ (parent)
    , locator_(locator)
    , waiting_for_keep_alive_(std::chrono::milliseconds(0))
    , connection_status_(eConnectionStatus::eDisconnected)
    , tcp_connection_type_(TCPConnectionType::TCP_CONNECT_TYPE)
{
}

TCPChannelResource::TCPChannelResource(
        TCPTransportInterface* parent,
        uint32_t maxMsgSize)
    : ChannelResource(maxMsgSize)
    , parent_(parent)
    , locator_()
    , waiting_for_keep_alive_(std::chrono::milliseconds(0))
    , connection_status_(eConnectionStatus::eConnected)
    , tcp_connection_type_(TCPConnectionType::TCP_ACCEPT_TYPE)
{
}

TCPChannelResource::~TCPChannelResource()
{
    alive_ = false;
}

void TCPChannelResource::disable()
{
    disconnect();
}

ResponseCode TCPChannelResource::process_bind_request(const Locator_t& locator)
{
    eConnectionStatus expected = TCPChannelResource::eConnectionStatus::eWaitingForBind;
    if(connection_status_.compare_exchange_strong(expected, eConnectionStatus::eEstablished))
    {
        locator_ = IPLocator::toPhysicalLocator(locator);
        logInfo(RTCP_MSG, "Connection Stablished");
        return RETCODE_OK;
    }
    else if (expected == eConnectionStatus::eEstablished)
    {
        return RETCODE_EXISTING_CONNECTION;
    }

    return RETCODE_SERVER_ERROR;
}

void TCPChannelResource::set_all_ports_pending()
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    pending_logical_output_ports_.insert(pending_logical_output_ports_.end(),
        logical_output_ports_.begin(),
        logical_output_ports_.end());
    logical_output_ports_.clear();
}

bool TCPChannelResource::is_logical_port_opened(uint16_t port)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    return std::find(logical_output_ports_.begin(), logical_output_ports_.end(), port) != logical_output_ports_.end();
}

bool TCPChannelResource::is_logical_port_added(uint16_t port)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    return std::find(logical_output_ports_.begin(), logical_output_ports_.end(), port) != logical_output_ports_.end()
        || std::find(pending_logical_output_ports_.begin(), pending_logical_output_ports_.end(), port)
        != pending_logical_output_ports_.end();
}

void TCPChannelResource::add_logical_port(uint16_t port, RTCPMessageManager* rtcp_manager)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    // Already opened?
    if (std::find(logical_output_ports_.begin(), logical_output_ports_.end(), port) == logical_output_ports_.end())
    {
        if (port == 0)
        {
            logError(RTPS, "Trying to open logical port 0.");
        } // But let's continue...

        if (std::find(pending_logical_output_ports_.begin(), pending_logical_output_ports_.end(), port)
                == pending_logical_output_ports_.end()) // Check isn't enqueued already
        {
            pending_logical_output_ports_.emplace_back(port);
            if (connection_established())
            {
                scopedLock.unlock();
                TCPTransactionId id = rtcp_manager->sendOpenLogicalPortRequest(this, port);
                scopedLock.lock();
                negotiating_logical_ports_[id] = port;
            }
        }
    }

}

void TCPChannelResource::send_pending_open_logical_ports(RTCPMessageManager* rtcp_manager)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    if (!pending_logical_output_ports_.empty())
    {
        for (uint16_t port : pending_logical_output_ports_)
        {
            TCPTransactionId id = rtcp_manager->sendOpenLogicalPortRequest(this, port);
            negotiating_logical_ports_[id] = port;
            eClock::my_sleep(100);
        }
    }
}

void TCPChannelResource::add_logical_port_response(
        const TCPTransactionId &id,
        bool success,
        RTCPMessageManager* rtcp_manager)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    auto it = negotiating_logical_ports_.find(id);
    if (it != negotiating_logical_ports_.end())
    {
        uint16_t port = it->second;
        auto portIt = std::find(pending_logical_output_ports_.begin(), pending_logical_output_ports_.end(), port);
        negotiating_logical_ports_.erase(it);
        if (portIt != pending_logical_output_ports_.end())
        {
            pending_logical_output_ports_.erase(portIt);
            if (success)
            {
                logical_output_ports_.push_back(port);
                logInfo(RTCP, "OpenedLogicalPort: " << port);
            }
            else
            {
                scopedLock.unlock();
                prepare_send_check_logical_ports_req(port, rtcp_manager);
            }
        }
        else
        {
            logWarning(RTCP, "Received add_logical_port_response for port " << port
                << ", but it wasn't found in pending list.");
        }
    }
    else
    {
        logWarning(RTCP, "Received add_logical_port_response, but the transaction id wasn't registered " <<
            "(maybe removed" << " while negotiating?).");
    }
}

void TCPChannelResource::prepare_send_check_logical_ports_req(
        uint16_t closedPort,
        RTCPMessageManager* rtcp_manager)
{
    std::vector<uint16_t> candidatePorts;
    uint16_t base_port = GetBaseAutoPort(closedPort); // The first failed port
    uint16_t max_port = closedPort + parent_->GetMaxLogicalPort();

    for (uint16_t p = base_port;
        p <= closedPort + (parent_->GetLogicalPortRange()
            * parent_->GetLogicalPortIncrement());
        p += parent_->GetLogicalPortIncrement())
    {
        // Don't add ports just tested and already pendings
        if (p <= max_port && p != closedPort)
        {
            std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
            auto pendingIt = std::find(pending_logical_output_ports_.begin(), pending_logical_output_ports_.end(), p);
            if (pendingIt == pending_logical_output_ports_.end())
            {
                candidatePorts.emplace_back(p);
            }
        }
    }

    if (candidatePorts.empty()) // No more available ports!
    {
        logError(RTCP, "Cannot find an available logical port.");
    }
    else
    {
        TCPTransactionId id = rtcp_manager->sendCheckLogicalPortsRequest(this, candidatePorts);
        std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
        last_checked_logical_port_[id] = candidatePorts.back();
    }
}

void TCPChannelResource::process_check_logical_ports_response(
        const TCPTransactionId &transactionId,
        const std::vector<uint16_t> &availablePorts,
        RTCPMessageManager* rtcp_manager)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    auto it = last_checked_logical_port_.find(transactionId);
    if (it != last_checked_logical_port_.end())
    {
        uint16_t lastPort = it->second;
        last_checked_logical_port_.erase(it);
        scopedLock.unlock();
        if (availablePorts.empty())
        {
            prepare_send_check_logical_ports_req(lastPort, rtcp_manager);
        }
        else
        {
            add_logical_port(availablePorts.front(), rtcp_manager);
        }
    }
    else
    {
        logWarning(RTCP, "Received process_check_logical_ports_response without sending a Request.");
    }
}

void TCPChannelResource::set_logical_port_pending(uint16_t port)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    auto it = std::find(logical_output_ports_.begin(), logical_output_ports_.end(), port);
    if (it != logical_output_ports_.end())
    {
        pending_logical_output_ports_.push_back(port);
        logical_output_ports_.erase(it);
    }
}

bool TCPChannelResource::remove_logical_port(uint16_t port)
{
    std::unique_lock<std::recursive_mutex> scopedLock(pending_logical_mutex_);
    if (!is_logical_port_added(port))
        return false;

    auto it = std::remove(logical_output_ports_.begin(), logical_output_ports_.end(), port);
    logical_output_ports_.erase(it, logical_output_ports_.end());
    it = std::remove(pending_logical_output_ports_.begin(), pending_logical_output_ports_.end(), port);
    pending_logical_output_ports_.erase(it, pending_logical_output_ports_.end());
    return true;
}

bool TCPChannelResource::check_keep_alive_expired() const
{
    if (waiting_for_keep_alive_ > std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(0)))
    {
        std::chrono::time_point<std::chrono::system_clock> time_now = std::chrono::system_clock::now();
        return waiting_for_keep_alive_ <= time_now;
    }
    return false;
}

void TCPChannelResource::update_keep_alive_timeout(
        const std::chrono::time_point<std::chrono::system_clock>& timeout)
{
    if (waiting_for_keep_alive_ == std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(0))
            || timeout == std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(0)))
    {
        waiting_for_keep_alive_ = timeout;
    }
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
