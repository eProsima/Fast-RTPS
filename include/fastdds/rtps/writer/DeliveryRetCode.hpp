#ifndef _FASTDDS_RTPS_WRITER_DELIVERYRETCODE_HPP_
#define _FASTDDS_RTPS_WRITER_DELIVERYRETCODE_HPP_

#include <cstdint>

namespace eprosima {
namespace fastrtps {
namespace rtps {

enum class DeliveryRetCode : uint32_t
{
    DELIVERED,
    NOT_DELIVERED,
    EXCEEDED_LIMIT
};

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

#endif // _FASTDDS_RTPS_WRITER_DELIVERYRETCODE_HPP_
