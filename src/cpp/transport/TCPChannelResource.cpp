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
#include <fastrtps/utils/IPLocator.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

TCPChannelResource::TCPChannelResource(eProsimaTCPSocket& socket, Locator_t& locator, bool outputLocator, bool inputSocket)
    : ChannelResource()
    , mLocator(locator)
    , m_inputSocket(inputSocket)
    , mWaitingForKeepAlive(false)
    , mPendingLogicalPort(0)
    , mNegotiatingLogicalPort(0)
    , mCheckingLogicalPort(0)
	, mNegotiationSemaphore(0)
	, mSocket(moveSocket(socket))
    , mConnectionStatus(eConnectionStatus::eDisconnected)
    //, mLogicalConnections(0)
{
    mReadMutex = new std::recursive_mutex();
    mWriteMutex = new std::recursive_mutex();
    if (outputLocator)
    {
        mPendingLogicalOutputPorts.emplace_back(IPLocator::getLogicalPort(locator));
        logInfo(RTCP, "Bound output locator (physical: " << IPLocator::getPhysicalPort(locator) << "; logical: " << IPLocator::getLogicalPort(locator) << ")");
    }
    else
    {
        mLogicalOutputPorts.emplace_back(IPLocator::getLogicalPort(locator));
    }
}

TCPChannelResource::TCPChannelResource(eProsimaTCPSocket& socket, Locator_t& locator, bool outputLocator, bool inputSocket,
    uint32_t maxMsgSize)
    : ChannelResource(maxMsgSize)
    , mLocator(locator)
    , m_inputSocket(inputSocket)
    , mWaitingForKeepAlive(false)
    , mPendingLogicalPort(0)
    , mNegotiatingLogicalPort(0)
    , mCheckingLogicalPort(0)
    , mSocket(moveSocket(socket))
    , mConnectionStatus(eConnectionStatus::eDisconnected)
    //, mLogicalConnections(0)
{
    mReadMutex = new std::recursive_mutex();
    mWriteMutex = new std::recursive_mutex();
    if (outputLocator)
    {
        mPendingLogicalOutputPorts.emplace_back(IPLocator::getLogicalPort(locator));
        logInfo(RTCP, "Bound output locator (physical: " << IPLocator::getPhysicalPort(locator) << "; logical: " << IPLocator::getLogicalPort(locator) << ")");
    }
    else
    {
        mLogicalOutputPorts.emplace_back(IPLocator::getLogicalPort(locator));
    }
}

TCPChannelResource::~TCPChannelResource()
{
	mNegotiationSemaphore.disable();

    Clear();

    if (mReadMutex != nullptr)
    {
        delete mReadMutex;
    }
    if (mWriteMutex != nullptr)
    {
        delete mWriteMutex;
    }

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

bool TCPChannelResource::IsLogicalPortOpened(uint16_t port)
{
	std::unique_lock<std::recursive_mutex> scopedLock(mPendingLogicalMutex);
	return std::find(mLogicalOutputPorts.begin(), mLogicalOutputPorts.end(), port) != mLogicalOutputPorts.end();
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
