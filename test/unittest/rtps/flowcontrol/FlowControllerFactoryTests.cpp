#include <rtps/flowcontrol/FlowControllerFactory.hpp>
#include <rtps/flowcontrol//FlowControllerImpl.hpp>

#include <gtest/gtest.h>

using namespace eprosima::fastdds::rtps;

TEST(FlowControllerFactory, get_default_flow_controllers)
{
    FlowControllerFactory factory;
    FlowController* flow_controller = nullptr;

    // Initialize factory.
    factory.init(nullptr);

    eprosima::fastrtps::rtps::WriterAttributes besteffort_sync_attributes;
    besteffort_sync_attributes.endpoint.reliabilityKind = eprosima::fastrtps::rtps::BEST_EFFORT;
    eprosima::fastrtps::rtps::WriterAttributes reliable_sync_attributes;
    eprosima::fastrtps::rtps::WriterAttributes besteffort_async_attributes;
    besteffort_async_attributes.endpoint.reliabilityKind = eprosima::fastrtps::rtps::BEST_EFFORT;
    besteffort_async_attributes.mode = eprosima::fastrtps::rtps::ASYNCHRONOUS_WRITER;
    eprosima::fastrtps::rtps::WriterAttributes reliable_async_attributes;
    reliable_async_attributes.mode = eprosima::fastrtps::rtps::ASYNCHRONOUS_WRITER;


    // Retrieve PureSyncFlowController.
    flow_controller = factory.retrieve_flow_controller(FASTDDS_FLOW_CONTROLLER_DEFAULT, besteffort_sync_attributes);
    FlowControllerImpl<FlowControllerPureSyncPublishMode,
            FlowControllerFifoSchedule>* pure_sync = dynamic_cast<FlowControllerImpl<FlowControllerPureSyncPublishMode,
                    FlowControllerFifoSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != pure_sync);

    // Retrieve SyncFlowController.
    flow_controller = factory.retrieve_flow_controller(FASTDDS_FLOW_CONTROLLER_DEFAULT, reliable_sync_attributes);
    FlowControllerImpl<FlowControllerSyncPublishMode,
            FlowControllerFifoSchedule>* sync = dynamic_cast<FlowControllerImpl<FlowControllerSyncPublishMode,
                    FlowControllerFifoSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != sync);

    // Retrieve AsyncFlowController.
    flow_controller = factory.retrieve_flow_controller(FASTDDS_FLOW_CONTROLLER_DEFAULT, besteffort_async_attributes);
    FlowControllerImpl<FlowControllerAsyncPublishMode,
            FlowControllerFifoSchedule>* async = dynamic_cast<FlowControllerImpl<FlowControllerAsyncPublishMode,
                    FlowControllerFifoSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async);
    flow_controller = factory.retrieve_flow_controller(FASTDDS_FLOW_CONTROLLER_DEFAULT, reliable_async_attributes);
    async = dynamic_cast<FlowControllerImpl<FlowControllerAsyncPublishMode,
                    FlowControllerFifoSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async);

}

TEST(FlowControllerFactory, register_flow_controllers)
{
    FlowControllerFactory factory;
    FlowController* flow_controller = nullptr;
    FlowControllerDescriptor flow_controller_descr;
    eprosima::fastrtps::rtps::WriterAttributes writer_attributes;

    // Initialize factory.
    factory.init(nullptr);

    // AsyncFlowController with Fifo scheduler
    const char* async_fifo = "AsyncFlowControllerFifo";
    flow_controller_descr.name = async_fifo;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::FIFO;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_fifo, writer_attributes);
    FlowControllerImpl<FlowControllerAsyncPublishMode,
            FlowControllerFifoSchedule>* async_fifo_flow = dynamic_cast<FlowControllerImpl<FlowControllerAsyncPublishMode,
                    FlowControllerFifoSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_fifo_flow);

    const char* async_robin = "AsyncFlowControllerRobin";
    flow_controller_descr.name = async_robin;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::ROUND_ROBIN;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_robin, writer_attributes);
    FlowControllerImpl<FlowControllerAsyncPublishMode,
            FlowControllerRoundRobinSchedule>* async_robin_flow = dynamic_cast<FlowControllerImpl<FlowControllerAsyncPublishMode,
                    FlowControllerRoundRobinSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_robin_flow);

    const char* async_high = "AsyncFlowControllerHigh";
    flow_controller_descr.name = async_high;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::HIGH_PRIORITY;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_high, writer_attributes);
    FlowControllerImpl<FlowControllerAsyncPublishMode,
            FlowControllerHighPrioritySchedule>* async_high_flow = dynamic_cast<FlowControllerImpl<FlowControllerAsyncPublishMode,
                    FlowControllerHighPrioritySchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_high_flow);

    const char* async_reserv = "AsyncFlowControllerReservation";
    flow_controller_descr.name = async_reserv;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::PRIORITY_WITH_RESERVATION;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_reserv, writer_attributes);
    FlowControllerImpl<FlowControllerAsyncPublishMode,
            FlowControllerPriorityWithReservationSchedule>* async_reserv_flow = dynamic_cast<FlowControllerImpl<FlowControllerAsyncPublishMode,
                    FlowControllerPriorityWithReservationSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_reserv_flow);

    flow_controller_descr.max_bytes_per_period = 1;
    flow_controller_descr.period_ms = 1;

    // AsyncLimitedFlowController with Fifo scheduler
    const char* async_limited_fifo = "AsyncLimitedFlowControllerFifo";
    flow_controller_descr.name = async_limited_fifo;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::FIFO;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_limited_fifo, writer_attributes);
    FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
            FlowControllerFifoSchedule>* async_limited_fifo_flow = dynamic_cast<FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
                    FlowControllerFifoSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_limited_fifo_flow);

    const char* async_limited_robin = "AsyncLimitedFlowControllerRobin";
    flow_controller_descr.name = async_limited_robin;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::ROUND_ROBIN;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_limited_robin, writer_attributes);
    FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
            FlowControllerRoundRobinSchedule>* async_limited_robin_flow = dynamic_cast<FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
                    FlowControllerRoundRobinSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_limited_robin_flow);

    const char* async_limited_high = "AsyncLimitedFlowControllerHigh";
    flow_controller_descr.name = async_limited_high;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::HIGH_PRIORITY;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_limited_high, writer_attributes);
    FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
            FlowControllerHighPrioritySchedule>* async_limited_high_flow = dynamic_cast<FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
                    FlowControllerHighPrioritySchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_limited_high_flow);

    const char* async_limited_reserv = "AsyncLimitedFlowControllerReservation";
    flow_controller_descr.name = async_limited_reserv;
    flow_controller_descr.scheduler = FlowControllerSchedulerPolicy::PRIORITY_WITH_RESERVATION;
    factory.register_flow_controller(flow_controller_descr);
    flow_controller = factory.retrieve_flow_controller(async_limited_reserv, writer_attributes);
    FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
            FlowControllerPriorityWithReservationSchedule>* async_limited_reserv_flow = dynamic_cast<FlowControllerImpl<FlowControllerLimitedAsyncPublishMode,
                    FlowControllerPriorityWithReservationSchedule>*>(flow_controller);
    ASSERT_TRUE(nullptr != async_limited_reserv_flow);
}

int main(
        int argc,
        char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
