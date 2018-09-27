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

#include <asio.hpp>
#include <fastrtps/transport/TCPChannelResource.h>
#include <fastrtps/transport/TCPTransportInterface.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/utils/eClock.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

TCPChannelResource::TCPChannelResource(TCPTransportInterface* parent, eProsimaTCPSocket& socket, const Locator_t& locator)
    : ChannelResource()
    , mParent (parent)
    , mLocator(locator)
    , m_inputSocket(false)
    , mWaitingForKeepAlive(false)
    , mPendingLogicalPort(0)
    , mNegotiatingLogicalPort(0)
    , mCheckingLogicalPort(0)
    , mNegotiationSemaphore(0)
    , mSocket(moveSocket(socket))
    , mConnectionStatus(eConnectionStatus::eDisconnected)
    //, mLogicalConnections(0)
{
}

TCPChannelResource::TCPChannelResource(TCPTransportInterface* parent, eProsimaTCPSocket& socket)
    : ChannelResource()
    , mParent(parent)
    , mLocator()
    , m_inputSocket(true)
    , mWaitingForKeepAlive(false)
    , mPendingLogicalPort(0)
    , mNegotiatingLogicalPort(0)
    , mCheckingLogicalPort(0)
	, mNegotiationSemaphore(0)
	, mSocket(moveSocket(socket))
    , mConnectionStatus(eConnectionStatus::eWaitingForBind)
    //, mLogicalConnections(0)
{
}

TCPChannelResource::~TCPChannelResource()
{
	mNegotiationSemaphore.disable();

    Clear();

    if (mRTCPThread != nullptr)
    {
        mRTCPThread->join();
        delete(mRTCPThread);
        mRTCPThread = nullptr;
    }
}

void TCPChannelResource::Disable()
{
	mNegotiationSemaphore.disable();

	ChannelResource::Disable();
}

void TCPChannelResource::Connect()
{
    auto type = mParent->GetProtocolType();
    auto endpoint = mParent->GenerateLocalEndpoint(mLocator, IPLocator::getPhysicalPort(mLocator));
    getSocketPtr(mSocket)->open(type);
    getSocketPtr(mSocket)->async_connect(endpoint, std::bind(&TCPChannelResource::SocketConnected, this,
        std::placeholders::_1));
}

void TCPChannelResource::SocketConnected(const asio::error_code& error)
{
    if (error.value())
    {
        getSocketPtr(mSocket)->close();
        eClock::my_sleep(100);
        Connect();
    }
    else
    {
        if (mConnectionStatus == eConnectionStatus::eDisconnected)
        {
            mConnectionStatus = eConnectionStatus::eConnected;
            mParent->SocketConnected(this);
        }
    }
}

bool TCPChannelResource::IsLogicalPortOpened(uint16_t port)
{
	std::unique_lock<std::recursive_mutex> scopedLock(mPendingLogicalMutex);
	return std::find(mLogicalOutputPorts.begin(), mLogicalOutputPorts.end(), port) != mLogicalOutputPorts.end();
}

bool TCPChannelResource::IsLogicalPortAdded(uint16_t port)
{
	std::unique_lock<std::recursive_mutex> scopedLock(mPendingLogicalMutex);
	return std::find(mLogicalOutputPorts.begin(), mLogicalOutputPorts.end(), port) != mLogicalOutputPorts.end()
           || std::find(mPendingLogicalOutputPorts.begin(), mPendingLogicalOutputPorts.end(), port)
                != mPendingLogicalOutputPorts.end();
}

std::thread* TCPChannelResource::ReleaseRTCPThread()
{
    std::thread* outThread = mRTCPThread;
    mRTCPThread = nullptr;
    return outThread;
}

void TCPChannelResource::fillLogicalPorts(std::vector<Locator_t>& outVector)
{
	std::unique_lock<std::recursive_mutex> scopedLock(mPendingLogicalMutex);
    Locator_t temp = mLocator;
    for (uint16_t port : mPendingLogicalOutputPorts)
    {
        IPLocator::setLogicalPort(temp, port);
        outVector.emplace_back(temp);
    }
    for (uint16_t port : mLogicalOutputPorts)
    {
        IPLocator::setLogicalPort(temp, port);
        outVector.emplace_back(temp);
    }
}

uint32_t TCPChannelResource::GetMsgSize() const
{
	return m_rec_msg.max_size;
}

void TCPChannelResource::EnqueueLogicalPort(uint16_t port)
{
	std::unique_lock<std::recursive_mutex> scopedLock(mPendingLogicalMutex);
    if (std::find(mPendingLogicalOutputPorts.begin(), mPendingLogicalOutputPorts.end(), port)
        == mPendingLogicalOutputPorts.end()
        && std::find(mLogicalOutputPorts.begin(), mLogicalOutputPorts.end(), port) == mLogicalOutputPorts.end())
    {
        if (port == 0)
        {
            logError(RTPS, "Trying to enqueue logical port 0.");
        }
        mPendingLogicalOutputPorts.emplace_back(port);
    }
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
