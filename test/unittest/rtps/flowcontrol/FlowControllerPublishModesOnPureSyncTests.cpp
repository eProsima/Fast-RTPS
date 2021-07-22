#include "FlowControllerPublishModesTests.hpp"

using namespace eprosima::fastdds::rtps;
using namespace testing;

TYPED_TEST(FlowControllerPublishModes, pure_sync_publish_mode)
{
    FlowControllerDescriptor flow_controller_descr;
    FlowControllerImpl<FlowControllerPureSyncPublishMode, TypeParam> pure_sync(nullptr,
            &flow_controller_descr);
    pure_sync.init();

    // Initialize callback to get info.
    auto send_functor = [&](
        eprosima::fastrtps::rtps::CacheChange_t*,
        eprosima::fastrtps::rtps::RTPSMessageGroup&,
        eprosima::fastrtps::rtps::LocatorSelectorSender&,
        const std::chrono::time_point<std::chrono::steady_clock>&)
            {
                this->last_thread_delivering_sample = std::this_thread::get_id();
            };


    // Instantiate writers.
    eprosima::fastrtps::rtps::RTPSWriter writer1;
    eprosima::fastrtps::rtps::RTPSWriter writer2;

    // Register writers.
    pure_sync.register_writer(&writer1);

    eprosima::fastrtps::rtps::CacheChange_t change_writer1;
    INIT_CACHE_CHANGE(change_writer1, writer1, 1);

    eprosima::fastrtps::rtps::CacheChange_t change_writer2;
    INIT_CACHE_CHANGE(change_writer2, writer2, 1);

    // Testing add_new_sample. Writer will be able to deliver it.
    EXPECT_CALL(writer1,
            deliver_sample_nts(&change_writer1, _, Ref(writer1.general_locator_selector_), _)).
            WillOnce(DoAll(send_functor, Return(eprosima::fastrtps::rtps::DeliveryRetCode::DELIVERED)));
    writer1.getMutex().lock();
    ASSERT_TRUE(pure_sync.add_new_sample(&writer1, &change_writer1,
            std::chrono::steady_clock::now() + std::chrono::hours(24)));
    writer1.getMutex().unlock();
    EXPECT_EQ(std::this_thread::get_id(), this->last_thread_delivering_sample);

    EXPECT_CALL(writer2,
            deliver_sample_nts(&change_writer2, _, Ref(writer2.general_locator_selector_), _)).
            WillOnce(DoAll(send_functor, Return(eprosima::fastrtps::rtps::DeliveryRetCode::DELIVERED)));
    writer2.getMutex().lock();
    ASSERT_TRUE(pure_sync.add_new_sample(&writer2, &change_writer2,
            std::chrono::steady_clock::now() + std::chrono::hours(24)));
    writer2.getMutex().unlock();
    EXPECT_EQ(std::this_thread::get_id(), this->last_thread_delivering_sample);

    // Testing add_new_sample. Writer will not be able to deliver it.
    EXPECT_CALL(writer1,
            deliver_sample_nts(&change_writer1, _, Ref(writer1.general_locator_selector_), _)).
            WillOnce(DoAll(send_functor, Return(eprosima::fastrtps::rtps::DeliveryRetCode::NOT_DELIVERED)));
    writer1.getMutex().lock();
    ASSERT_FALSE(pure_sync.add_new_sample(&writer1, &change_writer1,
            std::chrono::steady_clock::now() + std::chrono::hours(24)));
    writer1.getMutex().unlock();
    EXPECT_EQ(std::this_thread::get_id(), this->last_thread_delivering_sample);

    EXPECT_CALL(writer2,
            deliver_sample_nts(&change_writer2, _, Ref(writer2.general_locator_selector_), _)).
            WillOnce(DoAll(send_functor, Return(eprosima::fastrtps::rtps::DeliveryRetCode::NOT_DELIVERED)));
    writer2.getMutex().lock();
    ASSERT_FALSE(pure_sync.add_new_sample(&writer2, &change_writer2,
            std::chrono::steady_clock::now() + std::chrono::hours(24)));
    writer2.getMutex().unlock();
    EXPECT_EQ(std::this_thread::get_id(), this->last_thread_delivering_sample);

    // Testing add_old_sample. Writers never be called.
    writer1.getMutex().lock();
    ASSERT_FALSE(pure_sync.add_old_sample(&writer1, &change_writer1));
    writer1.getMutex().unlock();
    writer2.getMutex().lock();
    ASSERT_FALSE(pure_sync.add_old_sample(&writer2, &change_writer2));
    writer2.getMutex().unlock();

    pure_sync.unregister_writer(&writer1);
}
