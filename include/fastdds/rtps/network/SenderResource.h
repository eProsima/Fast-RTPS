// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _FASTDDS_RTPS_SENDER_RESOURCE_H
#define _FASTDDS_RTPS_SENDER_RESOURCE_H

#include <functional>
#include <chrono>

#include <fastdds/rtps/common/Locator.h>
#include <fastdds/rtps/network/NetworkBuffer.hpp>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
 * RAII object that encapsulates the Send operation over one chanel in an unknown transport.
 * A Sender resource is always univocally associated to a transport channel; the
 * act of constructing a Sender Resource opens the channel and its destruction
 * closes it.
 * @ingroup NETWORK_MODULE
 */
class SenderResource
{
public:

    using NetworkBuffer = eprosima::fastdds::rtps::NetworkBuffer;

    /* The maximum number of buffers that will be sent on a single send call.
     * We are currently supporting 3 buffers, due to the way RTPSMessageGroup works.
     * Only the last DATA / DATA_FRAG on the datagram may be back-referenced, so there will be at most 3 buffers used:
     * - Buffer with RTPS header + other submessages + DATA / DATA_FRAG header
     * - Buffer pointing to history cache payload
     * - Buffer with padding of up to 3 bytes
     */
    static constexpr size_t max_required_buffers = 3;

    /**
     * Sends to a destination locator, through the channel managed by this resource.
     * @param data Pointer to the contiguous data buffer.
     * @param data_size Number of bytes in @c data to send.
     * @param destination_locators_begin destination endpoint Locators iterator begin.
     * @param destination_locators_end destination endpoint Locators iterator end.
     * @param max_blocking_time_point If transport supports it then it will use it as maximum blocking time.
     * @return Success of the send operation.
     */
    bool send(
            const octet* data,
            uint32_t data_size,
            LocatorsIterator* destination_locators_begin,
            LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point& max_blocking_time_point)
    {
        if (send_lambda_ && !send_buffers_lambda_)
        {
            logWarning(RTPS, "The usage of send_lambda_ on SenderResource has been deprecated."
                    << std::endl << "Please implement send_buffers_lambda_ instead.");
            return send_lambda_(data, data_size,
                           destination_locators_begin, destination_locators_end, max_blocking_time_point);
        }

        NetworkBuffer buf{ data, data_size };
        return send(&buf, 1, data_size,
                       destination_locators_begin, destination_locators_end, max_blocking_time_point);
    }

    /**
     * Sends to a destination locator, through the channel managed by this resource.
     * @param buffers Array of buffers to gather.
     * @param num_buffers Number of elements on @c buffers.
     * @param total_bytes Total size of the raw data. Should be equal to the sum of the @c length field of all
     * buffers.
     * @param destination_locators_begin destination endpoint Locators iterator begin.
     * @param destination_locators_end destination endpoint Locators iterator end.
     * @param max_blocking_time_point If transport supports it then it will use it as maximum blocking time.
     * @return Success of the send operation.
     */
    bool send(
            const NetworkBuffer* buffers,
            size_t num_buffers,
            uint32_t total_bytes,
            LocatorsIterator* destination_locators_begin,
            LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point& max_blocking_time_point)
    {
        if (send_buffers_lambda_)
        {
            return send_buffers_lambda_(buffers, num_buffers, total_bytes,
                           destination_locators_begin, destination_locators_end, max_blocking_time_point);
        }

        if (send_lambda_)
        {
            send_lambda_ = nullptr;
            logError(RTPS, "The usage of send_lambda_ on SenderResource has been deprecated."
                    << std::endl << "Please implement send_buffers_lambda_ instead.");
        }

        return false;
    }

    /**
     * Resources can only be transfered through move semantics. Copy, assignment, and
     * construction outside of the factory are forbidden.
     */
    SenderResource(
            SenderResource&& rValueResource)
        : transport_kind_(rValueResource.transport_kind_)
    {
        clean_up.swap(rValueResource.clean_up);
        send_lambda_.swap(rValueResource.send_lambda_);
        send_buffers_lambda_.swap(rValueResource.send_buffers_lambda_);
    }

    virtual ~SenderResource() = default;

    int32_t kind() const
    {
        return transport_kind_;
    }

    /**
     * Add locators representing the local endpoints managed by this sender resource.
     *
     * @param [in,out] locators  List where locators will be added.
     */
    virtual void add_locators_to_list(
            LocatorList_t& locators) const
    {
        (void)locators;
    }

protected:

    SenderResource(
            int32_t transport_kind)
        : transport_kind_(transport_kind)
    {
    }

    int32_t transport_kind_;

    std::function<void()> clean_up;
    std::function<bool(
                const octet* data,
                uint32_t data_size,
                LocatorsIterator* destination_locators_begin,
                LocatorsIterator* destination_locators_end,
                const std::chrono::steady_clock::time_point&)> send_lambda_;
    std::function<bool(
                const NetworkBuffer* buffers,
                size_t num_buffers,
                uint32_t total_bytes,
                LocatorsIterator* destination_locators_begin,
                LocatorsIterator* destination_locators_end,
                const std::chrono::steady_clock::time_point&)> send_buffers_lambda_;

private:

    SenderResource() = delete;
    SenderResource(
            const SenderResource&) = delete;
    SenderResource& operator =(
            const SenderResource&) = delete;
};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif /* _FASTDDS_RTPS_SENDER_RESOURCE_H */
