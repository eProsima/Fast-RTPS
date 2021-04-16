// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/statistics/dds/domain/DomainParticipant.hpp>
#include <fastdds/statistics/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/statistics/topic_names.hpp>
#include <fastrtps/types/TypesBase.h>

#include "../../logging/mock/MockConsumer.h"

using eprosima::fastrtps::types::ReturnCode_t;

namespace eprosima {
namespace fastdds {
namespace statistics {
namespace dds {

class StatisticsDomainParticipantMockTests : public ::testing::Test
{
public:

    void helper_block_for_at_least_entries(
            uint32_t amount)
    {
        std::unique_lock<std::mutex> lck(*mutex_);
        mock_consumer_->cv().wait(lck, [this, amount]
                {
                    return mock_consumer_->ConsumedEntriesSize_nts() >= amount;
                });
    }

    eprosima::fastdds::dds::MockConsumer* mock_consumer_;

    mutable std::mutex* mutex_;

protected:

    void SetUp() override
    {
        mutex_ = new std::mutex();
    }

    void TearDown() override
    {
        delete mutex_;
        mutex_ = nullptr;
    }

};

class DomainParticipantTest : public DomainParticipant
{
public:

    eprosima::fastdds::dds::Publisher* get_builtin_publisher() const
    {
        return builtin_publisher_;
    }
};

/**
 * This test checks that enable_statistics_datawriter fails returning RETCODE_ERROR when create_datawriter fails
 * returning a nullptr.
 * 1. Create participant
 * 2. Mock create_datawriter so it returns nullptr
 * 3. Call enable_statistics_datawriter and check it fails.
 * 4. Capture log error entry
 */
TEST_F(StatisticsDomainParticipantMockTests, EnableStatisticsDataWriterFailureCreateDataWriter)
{
#ifdef FASTDDS_STATISTICS
    mock_consumer_ = new eprosima::fastdds::dds::MockConsumer();

    eprosima::fastdds::dds::Log::RegisterConsumer(std::unique_ptr<eprosima::fastdds::dds::LogConsumer>(mock_consumer_));
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Error);
    eprosima::fastdds::dds::Log::SetCategoryFilter(std::regex("(STATISTICS_DOMAIN_PARTICIPANT)"));
    eprosima::fastdds::dds::Log::SetErrorStringFilter(std::regex("(DataWriter creation has failed)"));

    // 1. Create DomainParticipant
    eprosima::fastdds::dds::DomainParticipant* participant =
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->
                    create_participant(0, eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);
    ASSERT_NE(participant, nullptr);

    DomainParticipant* statistics_participant =
            eprosima::fastdds::statistics::dds::DomainParticipant::narrow(participant);
    ASSERT_NE(statistics_participant, nullptr);

    // 2. Mock create_datawriter
    DomainParticipantTest* statistics_participant_test = static_cast<DomainParticipantTest*>(statistics_participant);
    eprosima::fastdds::dds::Publisher* builtin_pub = statistics_participant_test->get_builtin_publisher();
    EXPECT_CALL(*builtin_pub, create_datawriter(testing::_, testing::_)).WillOnce(testing::Return(nullptr));

    // 3. enable_statistics_datawriter
    EXPECT_EQ(ReturnCode_t::RETCODE_ERROR, statistics_participant->enable_statistics_datawriter(HEARTBEAT_COUNT_TOPIC,
        STATISTICS_DATAWRITER_QOS));

    // 4. Check log error entry
    helper_block_for_at_least_entries(1);
    auto consumed_entries = mock_consumer_->ConsumedEntries();
    EXPECT_EQ(consumed_entries.size(), 1u);
#endif // FASTDDS_STATISTICS
}

} // dds
} // statistics
} // fastdds
} // eprosima

int main(
        int argc,
        char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
