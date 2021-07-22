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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define TEST_FRIENDS \
    FRIEND_TEST(WriterProxyTests, MissingChangesUpdate); \
    FRIEND_TEST(WriterProxyTests, LostChangesUpdate); \
    FRIEND_TEST(WriterProxyTests, ReceivedChangeSet); \
    FRIEND_TEST(WriterProxyTests, IrrelevantChangeSet);

#include <rtps/reader/WriterProxy.h>
#include <rtps/participant/RTPSParticipantImpl.h>
#include <fastrtps/rtps/reader/RTPSReader.h>
#include <fastrtps/rtps/reader/StatefulReader.h>
#include <fastrtps/rtps/builtin/data/WriterProxyData.h>
#include <fastrtps/rtps/resources/TimedEvent.h>

#include <rtps/reader/WriterProxy.cpp>

// Make SequenceNumberSet_t compatible with GMock macros

namespace testing {
namespace internal {
using namespace eprosima::fastrtps::rtps;

template<>
bool AnyEq::operator ()(
        const SequenceNumberSet_t& a,
        const SequenceNumberSet_t& b) const
{
    // remember that using SequenceNumberSet_t = BitmapRange<SequenceNumber_t, SequenceNumberDiff, 256>;
    // see test\unittest\utils\BitmapRangeTests.cpp method TestResult::Check

    if (a.empty() && b.empty())
    {
        return true;
    }

    if (a.base() == b.base())
    {
        uint32_t num_bits[2];
        uint32_t num_longs[2];
        SequenceNumberSet_t::bitmap_type bitmap[2];

        a.bitmap_get(num_bits[0], bitmap[0], num_longs[0]);
        b.bitmap_get(num_bits[1], bitmap[1], num_longs[1]);

        if (num_bits[0] != num_bits[1] || num_longs[0] != num_longs[1])
        {
            return false;
        }
        return std::equal(bitmap[0].cbegin(), bitmap[0].cbegin() + num_longs[0], bitmap[1].cbegin());
    }
    else
    {
        bool equal = true;

        a.for_each([&b, &equal](const SequenceNumber_t& e)
                {
                    equal &= b.is_set(e);
                });

        if (!equal)
        {
            return false;
        }

        b.for_each([&a, &equal](const SequenceNumber_t& e)
                {
                    equal &= a.is_set(e);
                });

        return equal;
    }
}

} // namespace internal
} // namespace testing

namespace eprosima {
namespace fastrtps {
namespace rtps {

TEST(WriterProxyTests, MissingChangesUpdate)
{
    using ::testing::ReturnRef;
    using ::testing::Eq;
    using ::testing::Ref;
    using ::testing::Const;

    WriterProxyData wattr( 4u, 1u );
    StatefulReader readerMock; // avoid annoying uninteresting call warnings

    // Testing the Timed events are properly configured
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr, SequenceNumber_t());

    // 1. Simulate initial acknack
    SequenceNumberSet_t t1(SequenceNumber_t(0, 0));
    EXPECT_CALL(readerMock, simp_send_acknack(t1)).Times(1u);
    wproxy.perform_initial_ack_nack();

    // 2. Simulate Writer initial HEARTBEAT if there is a single sample in writer's history
    // According to RTPS Wire spec 8.3.7.5.3 firstSN.value and lastSN.value cannot be 0 or negative
    // and lastSN.value < firstSN.value
    bool assert_liveliness = false;
    uint32_t heartbeat_count = 1;
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 1),
        SequenceNumber_t(0, 1),
        false,
        false,
        false,
        assert_liveliness );

    SequenceNumberSet_t t2 (SequenceNumber_t(0, 1));
    t2.add(SequenceNumber_t(0, 1));
    ASSERT_THAT(t2, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(), wproxy.available_changes_max());
    ASSERT_EQ(0u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 1)));
    ASSERT_EQ(1u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 2)));

    // 3. Simulate reception of a HEARTBEAT after two more samples are added to the writer's history
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 1),
        SequenceNumber_t(0, 3),
        false,
        false,
        false,
        assert_liveliness);

    t2.add(SequenceNumber_t(0, 2));
    t2.add(SequenceNumber_t(0, 3));
    ASSERT_THAT(t2, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(), wproxy.available_changes_max());
    ASSERT_EQ(0u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 1)));
    ASSERT_EQ(1u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 2)));
    ASSERT_EQ(2u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 3)));
    ASSERT_EQ(3u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)));

    // 4. Simulate reception of a DATA(6).
    wproxy.received_change_set(SequenceNumber_t(0, 6));

    // According to the RTPS standard, sequence numbers 4 and 5 would be unknown,
    // but henceforth we don't differentiate between unknown and missing
    t2.add(SequenceNumber_t(0, 4));
    t2.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t2, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(), wproxy.available_changes_max());
    ASSERT_EQ(4u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 5)));
    ASSERT_EQ(5u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 6)));
    ASSERT_EQ(5u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)));

    // 5. Simulate reception of a HEARTBEAT(1,6)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 1),
        SequenceNumber_t(0, 6),
        false,
        false,
        false,
        assert_liveliness);

    ASSERT_THAT(t2, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(), wproxy.available_changes_max());
    ASSERT_EQ(4u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 5)));
    ASSERT_EQ(5u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 6)));
    ASSERT_EQ(5u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)));

    // 5. Simulate reception of a HEARTBEAT(1,7)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 1),
        SequenceNumber_t(0, 7),
        false,
        false,
        false,
        assert_liveliness);

    t2.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t2, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(), wproxy.available_changes_max());
    ASSERT_EQ(4u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 5)));
    ASSERT_EQ(5u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 6)));
    ASSERT_EQ(5u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)));
    ASSERT_EQ(6u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 8)));

    // 6. Simulate reception of all missing DATA
    wproxy.received_change_set(SequenceNumber_t(0, 1));
    wproxy.received_change_set(SequenceNumber_t(0, 2));
    wproxy.received_change_set(SequenceNumber_t(0, 3));
    wproxy.received_change_set(SequenceNumber_t(0, 4));
    wproxy.received_change_set(SequenceNumber_t(0, 5));

    SequenceNumberSet_t t6(SequenceNumber_t(0, 7));
    t6.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t6, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 6), wproxy.available_changes_max());
    ASSERT_EQ(0u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)));
    ASSERT_EQ(1u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 8)));

    // 7. Simulate reception of a faulty HEARTBEAT with a lower last sequence number (4)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 1),
        SequenceNumber_t(0, 4),
        false,
        false,
        false,
        assert_liveliness );

    ASSERT_THAT(t6, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 6), wproxy.available_changes_max());

    // 8. Simulate reception of DATA(8) and DATA(10)
    wproxy.received_change_set(SequenceNumber_t(0, 8));
    wproxy.received_change_set( SequenceNumber_t(0, 10));

    // According to the RTPS standard, sequence numbers 7 and 9 would be unknown,
    // but henceforth we don't differentiate between unknown and missing
    t6.add(SequenceNumber_t(0, 7));
    t6.add(SequenceNumber_t(0, 9));
    ASSERT_THAT(t6, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 6), wproxy.available_changes_max());
    ASSERT_EQ(4u, wproxy.number_of_changes_from_writer());
    ASSERT_EQ(1u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)));
    ASSERT_EQ(2u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 11)));

    // 9. Simulate reception of HEARTBEAT(1,10)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t( 0, 1 ),
        SequenceNumber_t( 0, 10 ),
        false,
        false,
        false,
        assert_liveliness );

    t6.add(SequenceNumber_t(0, 6));
    t6.add(SequenceNumber_t(0, 7));
    t6.add(SequenceNumber_t(0, 9));
    ASSERT_THAT(t6, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 6), wproxy.available_changes_max());

}

TEST(WriterProxyTests, LostChangesUpdate)
{
    WriterProxyData wattr(4u, 1u);
    StatefulReader readerMock;
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr, SequenceNumber_t());

    // 1. Simulate reception of a HEARTBEAT(3,3)
    uint32_t heartbeat_count = 1;
    bool assert_liveliness = false;
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 3),
        SequenceNumber_t(0, 3),
        false,
        false,
        false,
        assert_liveliness);

    SequenceNumberSet_t t1(SequenceNumber_t(0, 3));
    t1.add(SequenceNumber_t(0, 3));
    ASSERT_THAT(t1, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 2), wproxy.available_changes_max());
    ASSERT_EQ(1u, wproxy.number_of_changes_from_writer());
    ASSERT_EQ(1u, wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)));

    // 2. Simulate reception of a DATA(5) follow by a HEARTBEAT(5,5)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.received_change_set(SequenceNumber_t(0, 5));
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 5),
        SequenceNumber_t(0, 5),
        false,
        false,
        false,
        assert_liveliness);

    ASSERT_THAT( SequenceNumberSet_t(SequenceNumber_t( 0, 6)), wproxy.missing_changes());
    ASSERT_EQ( SequenceNumber_t( 0, 5 ), wproxy.available_changes_max());
    ASSERT_EQ( 0u, wproxy.number_of_changes_from_writer());
    ASSERT_EQ( 0u, wproxy.unknown_missing_changes_up_to( SequenceNumber_t( 0, 5 )));

    // 3. Simulate reception of a faulty HEARTBEAT with a lower first sequence number (4)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 4),
        SequenceNumber_t(0, 5),
        false,
        false,
        false,
        assert_liveliness);

    ASSERT_THAT(SequenceNumberSet_t( SequenceNumber_t(0, 6)), wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 5), wproxy.available_changes_max());
    ASSERT_EQ(0u, wproxy.unknown_missing_changes_up_to( SequenceNumber_t(0, 5)));

    // 4. Simulate reception of a DATA(7)
    // According to the RTPS standard, sequence number 5 would be missing and 6 would be unknown,
    // but henceforth we don't differentiate between unknown and missing thus we add 6 to the SequenceNumberSet_t
    wproxy.received_change_set( SequenceNumber_t(0, 7));

    SequenceNumberSet_t t4(SequenceNumber_t(0, 6));
    t4.add(SequenceNumber_t(0, 6));
    ASSERT_THAT(t4, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 5), wproxy.available_changes_max());
    ASSERT_EQ(2u, wproxy.number_of_changes_from_writer());
    ASSERT_EQ(1u, wproxy.unknown_missing_changes_up_to( SequenceNumber_t(0, 8)));

    // 5. Simulate reception of a HEARTBEAT(8,8)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 8),
        SequenceNumber_t(0, 8),
        false,
        false,
        false,
        assert_liveliness);

    SequenceNumberSet_t t5(SequenceNumber_t(0, 8));
    t5.add(SequenceNumber_t(0, 8));
    ASSERT_THAT(t5, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 7), wproxy.available_changes_max());

    // 6. Simulate reception of a HEARTBEAT(10,10)
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 10),
        SequenceNumber_t(0, 10),
        false,
        false,
        false,
        assert_liveliness);

    SequenceNumberSet_t t6(SequenceNumber_t(0, 10));
    t6.add(SequenceNumber_t(0, 10));
    ASSERT_THAT(t6, wproxy.missing_changes());
    ASSERT_EQ(SequenceNumber_t(0, 9), wproxy.available_changes_max());
}

TEST(WriterProxyTests, ReceivedChangeSet)
{
    WriterProxyData wattr(4u, 1u);
    StatefulReader readerMock;
    WriterProxy wproxy(&readerMock,
            RemoteLocatorsAllocationAttributes(),
            ResourceLimitedContainerConfig());

    /// Tests that initial acknack timed event is updated with new interval
    /// Tests that heartbeat response timed event is updated with new interval
    /// Tests that initial acknack timed event is started

    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr, SequenceNumber_t());

    // 1. Writer proxy receives DATA with sequence number 3
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 3));

    // According to the RTPS standard, sequence numbers 1 and 2 would be
    // unknown, but henceforth we don't differentiate between unknown and missing
    SequenceNumberSet_t t1(SequenceNumber_t(0, 1));
    t1.add(SequenceNumber_t(0, 1));
    t1.add(SequenceNumber_t(0, 2));
    ASSERT_THAT(t1, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);

    // 2. Writer proxy receives DATA with sequence number 6
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 6));

    // According to the RTPS standard, sequence numbers 1,2, 4 and 5 would be
    // unknown, but henceforth we don't differentiate between unknown and missing
    t1.add(SequenceNumber_t(0, 4));
    t1.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t1, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 4u);

    // 3. Writer proxy receives DATA with sequence number 2
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be RECEIVED
    // Sequence number 3 should be RECEIVED
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 2));

    // According to the RTPS standard, sequence numbers 1, 4 and 5 would be
    // unknown, but henceforth we don't differentiate between unknown and missing
    SequenceNumberSet_t t3(SequenceNumber_t(0, 1));
    t3.add(SequenceNumber_t(0, 1));
    t3.add(SequenceNumber_t(0, 4));
    t3.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t3, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 3u);

    // 4. Writer proxy receives DATA with sequence number 1
    // Sequence number 1 should be RECEIVED
    // Sequence number 2 should be RECEIVED
    // Sequence number 3 should be RECEIVED
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 1));

    // According to the RTPS standard, sequence numbers 4 and 5 would be
    // unknown, but henceforth we don't differentiate between unknown and missing
    SequenceNumberSet_t t4(SequenceNumber_t(0, 4));
    t4.add(SequenceNumber_t(0, 4));
    t4.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t4, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);

    // 5. Simulate reception of a HEARTBEAT(4,6)
    // Sequence number 1 should be LOST
    // Sequence number 2 should be LOST
    // Sequence number 3 should be LOST
    // Sequence number 4 should be MISSING
    // Sequence number 5 should be MISSING
    // Sequence number 6 should be RECEIVED
    bool assert_liveliness = false;
    uint32_t heartbeat_count = 1;
    EXPECT_CALL(*wproxy.heartbeat_response_, restart_timer()).Times(1u);
    wproxy.process_heartbeat(
        heartbeat_count++,
        SequenceNumber_t(0, 4),
        SequenceNumber_t(0, 6),
        false,
        false,
        false,
        assert_liveliness);

    ASSERT_THAT(t4, wproxy.missing_changes());
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);

    // 6. Writer proxy receives DATA with sequence number 8
    // Sequence number 4 should be MISSING
    // Sequence number 5 should be MISSING
    // Sequence number 6 should be RECEIVED
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 8));

    // According to the RTPS standard, sequence numbers 4 and 5 should be MISSING and 7 should be UNKNOWN
    // but henceforth we don't differentiate between UNKNOWN and MISSING
    t4.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t4, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 5u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 3u);

    // 7. Writer proxy receives DATA with sequence number 4
    // Sequence number 4 should be RECEIVED
    // Sequence number 5 should be MISSING
    // Sequence number 6 should be RECEIVED
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 4));

    // According to the RTPS standard, sequence number 5 should be MISSING and 7 should be UNKNOWN
    // but henceforth we don't differentiate between UNKNOWN and MISSING
    SequenceNumberSet_t t7(SequenceNumber_t(0, 5));
    t7.add(SequenceNumber_t(0, 5));
    t7.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t7, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 4u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 2u);

    // 8. Writer proxy receives DATA with sequence number 5
    // Sequence number 5 should be RECEIVED
    // Sequence number 6 should be RECEIVED
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED
    wproxy.received_change_set(SequenceNumber_t(0, 5));

    // According to the RTPS standard, sequence number 7 should be UNKNOWN
    // but henceforth we don't differentiate between UNKNOWN and MISSING
    SequenceNumberSet_t t8(SequenceNumber_t(0, 7));
    t8.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t8, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 1u);

    // 9. Writer proxy receives DATA with sequence sequence number 7
    // No changes from writer
    wproxy.received_change_set(SequenceNumber_t(0, 7));

    ASSERT_THAT(SequenceNumberSet_t(), wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 0u);
}

TEST(WriterProxyTests, IrrelevantChangeSet)
{
    WriterProxyData wattr(4u, 1u);
    StatefulReader readerMock;
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr, SequenceNumber_t());

    // 1. Simulate reception of a GAP message for sequence number 3.
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 3));

    // According to the RTPS standard, sequence numbers 1 and 2 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    SequenceNumberSet_t t1(SequenceNumber_t(0, 1));
    t1.add(SequenceNumber_t(0, 1));
    t1.add(SequenceNumber_t(0, 2));
    ASSERT_THAT(t1, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);

    // 2. Simulate reception of a GAP message for sequence number 6.
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED with is_relevant = false
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 6));

    // According to the RTPS standard, sequence numbers 1, 2, 4 and 5 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    t1.add(SequenceNumber_t(0, 4));
    t1.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t1, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 4u);

    // 3. Simulate reception of a GAP message for sequence number 2.
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be RECEIVED with is_relevant = false
    // Sequence number 3 should be RECEIVED with is_relevant = false
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 2));

    // According to the RTPS standard, sequence numbers 1, 4 and 5 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    SequenceNumberSet_t t3(SequenceNumber_t(0, 1));
    t3.add(SequenceNumber_t(0, 1));
    t3.add(SequenceNumber_t(0, 4));
    t3.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t3, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 3u);

    // 4. Simulate reception of a GAP message for sequence number 1.
    // Sequence numbers 1, 2, and 3 should be removed from writer proxy
    // Sequence number 1 should be RECEIVED with is_relevant = false
    // Sequence number 2 should be RECEIVED with is_relevant = false
    // Sequence number 3 should be RECEIVED with is_relevant = false
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 1));

    // According to the RTPS standard, sequence numbers 4 and 5 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    SequenceNumberSet_t t4(SequenceNumber_t(0, 4));
    t4.add(SequenceNumber_t(0, 4));
    t4.add(SequenceNumber_t(0, 5));
    ASSERT_THAT(t4, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);

    // 5. Simulate reception of a GAP message for sequence number 8.
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED with is_relevant = false
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 8));

    // According to the RTPS standard, sequence numbers 4, 5 and 7 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    t4.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t4, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 5u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 3u);

    // 6. Simulate reception of a GAP message for sequence number 4.
    // Sequence number 4 should be RECEIVED with is_relevant = false
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED with is_relevant = false
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 4));

    // According to the RTPS standard, sequence numbers 5 and 7 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    SequenceNumberSet_t t6(SequenceNumber_t(0, 1));
    t6.add(SequenceNumber_t(0, 5));
    t6.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t6, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 4u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 2u);

    // 7. Simulate reception of a GAP message for sequence number 5.
    // Sequence number 5 should be RECEIVED with is_relevant = false
    // Sequence number 6 should be RECEIVED with is_relevant = false
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED with is_relevant = false
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 5));

    // According to the RTPS standard, sequence number 7 should be UNKNOWN,
    // but henceforth we don't differentiate between UNKNOWN and MISSING.
    SequenceNumberSet_t t7(SequenceNumber_t(0, 7));
    t7.add(SequenceNumber_t(0, 7));
    ASSERT_THAT(t7, wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 2u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), true);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 1u);

    // 8. Simulate reception of a GAP message for sequence number 7.
    // All sequence numbers received, no changes from writer
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 7));

    ASSERT_THAT(SequenceNumberSet_t(), wproxy.missing_changes());
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 0u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), false);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 0u);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

int main(
        int argc,
        char** argv)
{
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
