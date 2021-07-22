#include "FlowControllerPublishModesTests.hpp"

using namespace testing;

namespace eprosima {
namespace fastrtps {
namespace rtps {

std::ostream& operator <<(
        std::ostream& output,
        const RTPSWriter& writer)
{
    return output << "Writer" << writer.getGuid().entityId.value[3];
}

std::ostream& operator <<(
        std::ostream& output,
        const CacheChange_t* change)
{
    return output << "change_writer" << uint16_t(change->writerGUID.entityId.value[3]) << "_" << change->sequenceNumber;
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

int main(
        int argc,
        char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
