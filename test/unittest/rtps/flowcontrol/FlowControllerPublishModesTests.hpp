#ifndef _UNITTEST_RTPS_FLOWCONTROL_FLOWCONTROLLERPUBLISHMODESTESTS_HPP_
#define _UNITTEST_RTPS_FLOWCONTROL_FLOWCONTROLLERPUBLISHMODESTESTS_HPP_

#include <rtps/flowcontrol/FlowControllerImpl.hpp>
#include <fastdds/rtps/flowcontrol/FlowControllerDescriptor.hpp>

#include <gtest/gtest.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {

std::ostream& operator <<(
        std::ostream& output,
        const RTPSWriter& writer);
std::ostream& operator <<(
        std::ostream& output,
        const CacheChange_t* change);

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

template<typename T>
class FlowControllerPublishModes :  public testing::Test
{
protected:

    void TearDown() override
    {
        changes_delivered.clear();
        current_bytes_processed = 0;
    }

    void wait_changes_was_delivered(
            size_t number_of_changes)
    {
        std::unique_lock<std::mutex> lock(changes_delivered_mutex);
        number_changes_delivered_cv.wait(lock, [&]()
                {
                    return number_of_changes == changes_delivered.size();
                });
    }

    std::thread::id last_thread_delivering_sample;

    std::vector<eprosima::fastrtps::rtps::CacheChange_t*> changes_delivered;

    std::mutex changes_delivered_mutex;

    std::condition_variable number_changes_delivered_cv;

    uint32_t current_bytes_processed = 0;
};

using Schedulers = ::testing::Types<eprosima::fastdds::rtps::FlowControllerFifoSchedule,
                eprosima::fastdds::rtps::FlowControllerRoundRobinSchedule,
                eprosima::fastdds::rtps::FlowControllerHighPrioritySchedule,
                eprosima::fastdds::rtps::FlowControllerPriorityWithReservationSchedule>;

TYPED_TEST_SUITE(FlowControllerPublishModes, Schedulers, );

#define INIT_CACHE_CHANGE(change, writer, seq) \
    change.writerGUID = writer.getGuid(); \
    change.writer_info.previous = nullptr; \
    change.writer_info.next = nullptr; \
    change.sequenceNumber.low = uint32_t(seq); \
    change.serializedPayload.length = 10000;

#endif // _UNITTEST_RTPS_FLOWCONTROL_FLOWCONTROLLERPUBLISHMODESTESTS_HPP_
