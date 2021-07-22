#include <fastdds/rtps/writer/LocatorSelectorSender.hpp>
#include <fastdds/rtps/writer/RTPSWriter.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

bool LocatorSelectorSender::send(
        CDRMessage_t* message,
        std::chrono::steady_clock::time_point max_blocking_time_point) const
{
    return writer.send_nts(message, *this, max_blocking_time_point);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
