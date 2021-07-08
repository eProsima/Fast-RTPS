// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <fastrtps/utils/Semaphore.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/log/Log.h>
#include <MockReceiverResource.h>
#include <SharedMemGlobalMock.hpp>
#include "../../../src/cpp/rtps/transport/shared_mem/SharedMemSenderResource.hpp"
#include "../../../src/cpp/rtps/transport/shared_mem/SharedMemManager.hpp"
#include "../../../src/cpp/rtps/transport/shared_mem/SharedMemGlobal.hpp"
#include "../../../src/cpp/rtps/transport/shared_mem/SHMLocator.hpp"
#include "../../../src/cpp/rtps/transport/shared_mem/MultiProducerConsumerRingBuffer.hpp"

#include <string>
#include <fstream>
#include <streambuf>
#include <memory>
#include <gtest/gtest.h>
#include <thread>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;
using namespace eprosima::fastdds;
using namespace eprosima::fastdds::rtps;

#if defined(_WIN32)
#define GET_PID _getpid
#else
#define GET_PID getpid
#include <sys/statvfs.h>
#endif // if defined(_WIN32)

static uint16_t g_default_port = 0;
static uint16_t g_output_port = 0;
static uint16_t g_input_port = 0;

uint16_t get_port(
        uint16_t offset)
{
    uint16_t port = static_cast<uint16_t>(GET_PID());

    if (offset > port)
    {
        port += offset;
    }

    return port;
}

class SHMTransportTests : public ::testing::Test
{
public:

    SHMTransportTests()
    {
        eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Kind::Info);
    }

    ~SHMTransportTests()
    {
        eprosima::fastdds::dds::Log::Flush();
        eprosima::fastdds::dds::Log::KillThread();
    }

    SharedMemTransportDescriptor descriptor;
};

class SHMCondition : public ::testing::Test
{
public:

    SHMCondition()
    {
        eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Kind::Info);
    }

    ~SHMCondition()
    {
        eprosima::fastdds::dds::Log::Flush();
        eprosima::fastdds::dds::Log::KillThread();
    }

    static constexpr uint32_t TEST_MAX_NOTIFICATIONS = 3;

    void wait_test(
            int id,
            SharedMemSegment::mutex& mutex,
            SharedMemSegment::condition_variable& cv,
            bool* condition,
            std::atomic<uint32_t>& notifications_counter,
            boost::posix_time::milliseconds time_out)
    {
        std::thread thread_wait([&]
                {
                    std::unique_lock<SharedMemSegment::mutex> lock(mutex);

                    while (notifications_counter.load() < TEST_MAX_NOTIFICATIONS)
                    {
                        boost::system_time const timeout =
                        boost::get_system_time() + time_out;

                        std::cout << "P" << id << " waiting..." << std::endl;

                        if (cv.timed_wait(lock, timeout, [&]
                        {
                            return *condition;
                        }))
                        {
                            notifications_counter.fetch_add(1);
                            *condition = false;

                            std::cout << "P" << id << " " << notifications_counter.load()
                                      << " notifications received." << std::endl;
                        }
                        else
                        {
                            std::cout << "P" << id << " wait timeout!" << std::endl;
                        }
                    }
                });

        std::this_thread::sleep_for(std::chrono::seconds(1));

        thread_wait.join();
    }

};


class SHMRingBuffer : public ::testing::Test
{
protected:

    struct MyData
    {
        uint32_t thread_number;
        uint32_t counter;
    };

    std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>::Cell[]> cells_;
    std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>> ring_buffer_;
    uint32_t buffer_size_;

    SHMRingBuffer()
        : buffer_size_((std::max)((unsigned int) 4, std::thread::hardware_concurrency()))
    {
    }

    void SetUp() override
    {
        cells_ = std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>::Cell[]>(
            new MultiProducerConsumerRingBuffer<MyData>::Cell[buffer_size_]);

        ring_buffer_ =
                std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>>(new MultiProducerConsumerRingBuffer<MyData>(
                            cells_.get(), buffer_size_));
    }

    void TearDown() override
    {
        ring_buffer_.reset();
        cells_.reset();
    }

};

class SHMRingBufferMultiThread
    :   public SHMRingBuffer,
    public testing::WithParamInterface<std::tuple<uint32_t, uint32_t, uint32_t>>
{

};

TEST_F(SHMRingBuffer, test_read_write_bounds)
{
    auto listener = ring_buffer_->register_listener();

    for (uint32_t i = 0; i < buffer_size_; i++)
    {
        ring_buffer_->push({0, i});
    }

    ASSERT_THROW(ring_buffer_->push({0, (std::numeric_limits<uint32_t>::max)()}), std::exception);

    for (uint32_t i = 0; i < buffer_size_; i++)
    {
        EXPECT_EQ(listener->head()->data().counter, i);
        listener->pop();
    }

    ASSERT_THROW(listener->pop(), std::exception);
}

TEST_F(SHMRingBuffer, circular_pointer)
{
    uint32_t r = 0;
    uint32_t w = 0;
    uint32_t i = 0;

    auto listener = ring_buffer_->register_listener();

    // Buffer full
    for (; i < buffer_size_; i++)
    {
        ring_buffer_->push({0, w++});
    }

    i = (i % buffer_size_);
    ASSERT_EQ(i, 0u);

    // Another cicle
    for (; i < buffer_size_; i++)
    {
        ASSERT_EQ(listener->head()->data().counter, r++);
        listener->pop();
        ring_buffer_->push({0, w++});
    }

    i = (i % buffer_size_);
    ASSERT_EQ(i, 0u);

    // Flush the buffer
    for (; i < buffer_size_; i++)
    {
        ASSERT_EQ(listener->head()->data().counter, r++);
        listener->pop();
    }

    // Is empty
    ASSERT_THROW(listener->pop(), std::exception);
}

TEST_F(SHMRingBuffer, one_listener_reads_all)
{
    auto listener1 = ring_buffer_->register_listener();
    auto listener2 = ring_buffer_->register_listener();

    for (uint32_t i = 0; i < buffer_size_; i++)
    {
        ring_buffer_->push({0, i});
    }

    for (uint32_t i = 0; i < buffer_size_; i++)
    {
        listener1->pop();
    }

    ASSERT_EQ(listener1->head(), nullptr);
}

TEST_F(SHMRingBuffer, copy)
{
    std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>> ring_buffer;

    std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>::Cell[]> cells
        = std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>::Cell[]>(
                new MultiProducerConsumerRingBuffer<MyData>::Cell[2]);

    ring_buffer =
            std::unique_ptr<MultiProducerConsumerRingBuffer<MyData>>(new MultiProducerConsumerRingBuffer<MyData>(
                        cells_.get(), 2));

    auto listener = ring_buffer->register_listener();

    std::vector<const MyData*> enqueued_data;

    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(0u, enqueued_data.size());

    ring_buffer->push({ 0, 0 });
    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(1u, enqueued_data.size());
    enqueued_data.clear();

    ring_buffer->push({ 0, 1 });
    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(2u, enqueued_data.size());

    ASSERT_EQ(0u, enqueued_data[0]->counter);
    ASSERT_EQ(1u, enqueued_data[1]->counter);

    listener->pop();

    enqueued_data.clear();
    ring_buffer->push({ 0, 2 });
    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(2u, enqueued_data.size());

    ASSERT_EQ(1u, enqueued_data[0]->counter);
    ASSERT_EQ(2u, enqueued_data[1]->counter);

    listener->pop();

    enqueued_data.clear();
    ring_buffer->push({ 0, 3 });
    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(2u, enqueued_data.size());

    ASSERT_EQ(2u, enqueued_data[0]->counter);
    ASSERT_EQ(3u, enqueued_data[1]->counter);

    listener->pop();
    enqueued_data.clear();
    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(1u, enqueued_data.size());

    listener->pop();
    enqueued_data.clear();
    ring_buffer->copy(&enqueued_data);
    ASSERT_EQ(0u, enqueued_data.size());
}

TEST_F(SHMRingBuffer, listeners_register_unregister)
{
    // 0 Must be discarted because no listeners
    ring_buffer_->push({0, 0});

    auto listener1 = ring_buffer_->register_listener();
    // 1 Must be only read by listener1
    ring_buffer_->push({0, 1});

    auto listener2 = ring_buffer_->register_listener();
    // 2 Must be read by listener1 and listener 2
    ring_buffer_->push({0, 2});

    // 3
    ring_buffer_->push({0, 3});

    ASSERT_EQ(listener1->head()->data().counter, 1u);
    ASSERT_EQ(listener2->head()->data().counter, 2u);

    listener1->pop(); // 1:1
    ASSERT_EQ(listener1->head()->data().counter, 2u);

    listener2->pop(); // 2:2

    // Listener 1 must decrease ref_counter of 2 and 3 in its destructor
    listener1.reset();

    // 4
    ring_buffer_->push({0, 4});

    ASSERT_EQ(listener2->head()->data().counter, 3u);
    listener2->pop(); // 3

    ASSERT_EQ(listener2->head()->data().counter, 4u);
    listener2->pop();
}

TEST_P(SHMRingBufferMultiThread, multiple_writers_listeners)
{
    const uint32_t elements_to_push = buffer_size_ * std::get<1>(GetParam());
    std::vector<std::thread> threads;
    std::atomic<uint32_t> ready_listeners;
    ready_listeners.store(0);

    uint32_t num_listeners_writters = std::get<0>(GetParam());
    uint32_t num_register_unregister = std::get<2>(GetParam());

    for (uint32_t i = 0; i < num_listeners_writters; i++)
    {
        // Listeners
        threads.emplace_back(
            std::thread(
                [&]()
                {
                    std::vector<uint32_t> read_counters(num_listeners_writters, (std::numeric_limits<uint32_t>::max)());
                    MultiProducerConsumerRingBuffer<MyData>::Cell* cell;

                    auto listener = ring_buffer_->register_listener();
                    ready_listeners.fetch_add(1);

                    do
                    {
                        // poll until there's data
                        while (nullptr == (cell = listener->head()))
                        {
                        }

                        ASSERT_EQ(++read_counters[cell->data().thread_number], cell->data().counter);

                        listener->pop();

                    } while (cell->data().counter != elements_to_push - 1);

                }));
    }

    // Wait until all listeners ready
    while (ready_listeners.load(std::memory_order_relaxed) != num_listeners_writters)
    {
        std::this_thread::yield();
    }

    for (uint32_t i = 0; i < num_listeners_writters; i++)
    {
        // Writers
        threads.emplace_back(
            std::thread(
                [&](uint32_t thread_number)
                {
                    for (uint32_t c = 0; c < elements_to_push; c++)
                    {
                        bool success = false;
                        while (!success)
                        {
                            try
                            {
                                ring_buffer_->push({thread_number, c});
                                success = true;
                            }
                            catch (const std::exception&)
                            {
                            }
                        }
                    }
                }, i));
    }

    for (uint32_t i = 0; i < num_register_unregister; i++)
    {
        // Register-Unregister
        threads.emplace_back(
            std::thread(
                [&]()
                {
                    auto listener = ring_buffer_->register_listener();

                    // Reads two times the size values
                    for (uint32_t i = 0; i < buffer_size_ * 2; i++)
                    {
                        // poll until there's data
                        while (!listener->head())
                        {
                        }
                        listener->pop();
                    }

                    // Unregister
                    listener.reset();

                }));
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

TEST_F(SHMCondition, wait_notify)
{
    SharedMemSegment::condition_variable cv;
    SharedMemSegment::mutex mutex;
    bool condition = false;

    std::thread thread_wait([&]
            {
                std::unique_lock<SharedMemSegment::mutex> lock(mutex);
                cv.wait(lock, [&]
                {
                    return condition;
                });
            });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        std::lock_guard<SharedMemSegment::mutex> lock(mutex);
        condition = true;
    }

    cv.notify_one();

    thread_wait.join();
}

// POSIX glibc 2.25 conditions are not robust when used with interprocess shared memory:
// https://sourceware.org/bugzilla/show_bug.cgi?id=21422
// FastRTPS > v1.10.0 SHM has implemented robust conditions to solve issue #1144
// This is the correspoding regresion test
#ifndef _MSC_VER
TEST_F(SHMCondition, robust_condition_fix_glibc_deadlock)
{
    struct SharedSegment
    {
        std::atomic<uint32_t> notify_count[2];
        SharedMemSegment::mutex mutex;
        SharedMemSegment::condition_variable cv;
        bool condition;
    };

    auto p1 = fork();
    if (p1 == 0)
    {
        auto p2 = fork();
        if (p2 == 0)
        {
            printf("p2 go!\n");
            //P2
            boost::interprocess::shared_memory_object::remove("robust_condition_fix_test");
            boost::interprocess::managed_shared_memory shm(boost::interprocess::create_only,
                    "robust_condition_fix_test", 1024 + sizeof(SharedSegment));

            auto shared_segment = shm.construct<SharedSegment>("shared_segment")();
            shared_segment->notify_count[0].exchange(0);
            shared_segment->notify_count[1].exchange(0);

            wait_test(2, shared_segment->mutex,
                    shared_segment->cv,
                    &shared_segment->condition,
                    shared_segment->notify_count[1],
                    boost::posix_time::milliseconds(60 * 1000));
        }
        else //P1
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("p1 go!\n");

            // Kill p2
            ASSERT_EQ(0, system((std::string("kill -9 ") + std::to_string(p2)).c_str()));
            printf("p2 killed!\n");

            boost::interprocess::managed_shared_memory shm(boost::interprocess::open_only,
                    "robust_condition_fix_test");

            auto shared_segment = shm.find<SharedSegment>("shared_segment").first;

            // P2 died without notifications
            ASSERT_EQ(shared_segment->notify_count[1].load(), 0u);

            wait_test(1, shared_segment->mutex,
                    shared_segment->cv,
                    &shared_segment->condition,
                    shared_segment->notify_count[0],
                    boost::posix_time::milliseconds(500));
        }
    }
    else // P0
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("p0 go!\n");

        boost::interprocess::managed_shared_memory shm(boost::interprocess::open_only,
                "robust_condition_fix_test");

        auto shared_segment = shm.find<SharedSegment>("shared_segment").first;

        for (uint32_t i = 0; i < TEST_MAX_NOTIFICATIONS; i++)
        {
            {
                std::unique_lock<boost::interprocess::interprocess_mutex> lock(shared_segment->mutex);
                shared_segment->condition = true;
            }
            shared_segment->cv.notify_all();

            // Wait until P1 receives the last notification
            while (shared_segment->notify_count[0].load() < i + 1)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}
#endif // ifndef _MSC_VER

TEST_F(SHMCondition, max_listeners)
{
    SharedMemSegment::condition_variable cv;
    SharedMemSegment::mutex mutex;
    bool condition = false;

    std::cout << "sizeof(SharedMemSegment::condition_variable) = "
              << sizeof(SharedMemSegment::condition_variable)
              << std::endl;

    static constexpr uint32_t max_test_listeners = 1024;
    std::vector<std::thread> threads;

    std::atomic<uint32_t> waiting_threads(0);
    std::atomic<uint32_t> wait_exception(0);
    std::atomic<uint32_t> wait_ok(0);

    for (uint32_t i = 0; i < max_test_listeners; i++)
    {
        threads.emplace_back([&]
                {
                    std::unique_lock<SharedMemSegment::mutex> lock(mutex);
                    waiting_threads.fetch_add(1);
                    try
                    {
                        cv.wait(lock, [&]
                        {
                            return condition;
                        });

                        wait_ok.fetch_add(1);
                    }
                    catch (const std::exception&)
                    {
                        wait_exception.fetch_add(1);
                    }
                });
    }

    std::cout << threads.size() << " listeners spawned." << std::endl;

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (waiting_threads.load() + wait_exception.load() < threads.size());

    std::cout << waiting_threads.load() << " waiting. " << wait_exception.load() << " failed." << std::endl;

    {
        std::unique_lock<SharedMemSegment::mutex> lock(mutex);
        condition = true;
    }
    cv.notify_all();

    std::cout << "all notified" << std::endl;

    for (auto& thread : threads)
    {
        thread.join();
    }

    std::cout << "all joined!" << std::endl;

    ASSERT_EQ(wait_ok.load(), waiting_threads.load() - wait_exception.load());
    ASSERT_GT(wait_exception.load(), 0u);
}

TEST_F(SHMCondition, fifo_policy)
{
    SharedMemSegment::condition_variable cv;
    SharedMemSegment::mutex mutex;
    bool condition = false;

    std::vector<std::thread> threads;
    std::vector<uint32_t> exit_order;

    auto wait_lambda = [&](uint32_t id, const boost::posix_time::ptime& end_time_point)
            {
                std::unique_lock<SharedMemSegment::mutex> lock(mutex);
                ASSERT_NO_THROW(cv.timed_wait(lock, end_time_point, [&]
                        {
                            return condition;
                        }));
                exit_order.push_back(id);
            };

    // Check notify is FIFO
    // Three elements remove the intermediate
    auto now = boost::posix_time::microsec_clock::universal_time();
    threads.emplace_back(std::thread(wait_lambda, 0, now + boost::posix_time::seconds(3600)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    threads.emplace_back(std::thread(wait_lambda, 1, now + boost::posix_time::seconds(3600)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    threads.emplace_back(std::thread(wait_lambda, 2, now + boost::posix_time::seconds(3600)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::unique_lock<SharedMemSegment::mutex> lock(mutex);
        condition = true;
    }


    ASSERT_EQ(exit_order.size(), 0u);
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(exit_order.size(), 1u);
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(exit_order.size(), 2u);
    cv.notify_one();

    for (auto& thread : threads)
    {
        thread.join();
    }

    ASSERT_EQ(exit_order, std::vector<uint32_t>({0, 1, 2}));
    threads.clear();
    exit_order.clear();
}


TEST_F(SHMTransportTests, opening_and_closing_input_channel)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t genericInputChannelLocator;
    genericInputChannelLocator.kind = LOCATOR_KIND_SHM;
    genericInputChannelLocator.port = g_input_port; // listen port

    // Then
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.OpenInputChannel(genericInputChannelLocator, nullptr, 0x00FF));
    ASSERT_TRUE  (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.CloseInputChannel(genericInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.CloseInputChannel(genericInputChannelLocator));
}

TEST_F(SHMTransportTests, closing_input_channel_leaves_other_channels_unclosed)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t genericInputChannelLocator;
    genericInputChannelLocator.kind = LOCATOR_KIND_SHM;
    genericInputChannelLocator.port = g_input_port; // listen port

    Locator_t otherInputChannelLocator;
    otherInputChannelLocator.kind = LOCATOR_KIND_SHM;
    otherInputChannelLocator.port = g_input_port + 1; // listen port

    // Then
    ASSERT_TRUE  (transportUnderTest.OpenInputChannel(genericInputChannelLocator, nullptr, 0x00FF));
    ASSERT_TRUE  (transportUnderTest.OpenInputChannel(otherInputChannelLocator, nullptr, 0x00FF));
    ASSERT_TRUE  (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.IsInputChannelOpen(otherInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.CloseInputChannel(genericInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.IsInputChannelOpen(otherInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.CloseInputChannel(genericInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.IsInputChannelOpen(otherInputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.CloseInputChannel(otherInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(genericInputChannelLocator));
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(otherInputChannelLocator));
}

TEST_F(SHMTransportTests, RemoteToMainLocal_returns_input_locator)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t remote_locator;
    remote_locator.kind = LOCATOR_KIND_SHM;
    remote_locator.port = g_default_port;

    // When
    Locator_t mainLocalLocator = transportUnderTest.RemoteToMainLocal(remote_locator);

    // Then
    ASSERT_EQ(mainLocalLocator, remote_locator);
}

TEST_F(SHMTransportTests, transform_remote_locator_returns_input_locator)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    // Check with multicast
    {
        // As transform_remote_locator checks if the locator is accessible, it should be previously open
        Locator_t remote_locator = SHMLocator::create_locator(g_default_port, SHMLocator::Type::MULTICAST);
        ASSERT_TRUE(transportUnderTest.OpenInputChannel(remote_locator, nullptr, 0xFF));

        // Then
        Locator_t otherLocator;
        ASSERT_TRUE(transportUnderTest.transform_remote_locator(remote_locator, otherLocator));
        ASSERT_EQ(otherLocator, remote_locator);

        ASSERT_TRUE(transportUnderTest.CloseInputChannel(remote_locator));
    }

    // Check with unicast
    {
        // As transform_remote_locator checks if the locator is accessible, it should be previously open
        Locator_t remote_locator = SHMLocator::create_locator(g_default_port, SHMLocator::Type::UNICAST);
        ASSERT_TRUE(transportUnderTest.OpenInputChannel(remote_locator, nullptr, 0xFF));

        // Then
        Locator_t otherLocator;
        ASSERT_TRUE(transportUnderTest.transform_remote_locator(remote_locator, otherLocator));
        ASSERT_EQ(otherLocator, remote_locator);

        ASSERT_TRUE(transportUnderTest.CloseInputChannel(remote_locator));
    }
}

TEST_F(SHMTransportTests, transform_remote_locator_fails_on_non_open_channel)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    // Check with multicast
    {
        // Given
        Locator_t remote_locator = SHMLocator::create_locator(g_default_port, SHMLocator::Type::MULTICAST);

        // Then
        Locator_t otherLocator;
        ASSERT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, otherLocator));
    }

    // Check with unicast
    {
        // Given
        Locator_t remote_locator = SHMLocator::create_locator(g_default_port, SHMLocator::Type::UNICAST);

        // Then
        Locator_t otherLocator;
        ASSERT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, otherLocator));
    }
}

void transform_remote_locator_failures(
        SharedMemTransport& transportUnderTest,
        SHMLocator::Type locator_type)
{
    // Given
    Locator_t correct_locator = SHMLocator::create_locator(g_default_port, locator_type);
    ASSERT_TRUE(transportUnderTest.OpenInputChannel(correct_locator, nullptr, 0xFF));

    Locator_t other_locator;
    Locator_t remote_locator;

    // Check wrong kind
    remote_locator = correct_locator;
    remote_locator.kind = LOCATOR_KIND_INVALID;
    EXPECT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, other_locator));

    // Check wrong address
    remote_locator = correct_locator;
    LOCATOR_ADDRESS_INVALID(remote_locator.address);
    EXPECT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, other_locator));

    // Check wrong host
    remote_locator = correct_locator;
    remote_locator.address[1]++;
    EXPECT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, other_locator));

    // Check wrong user
    remote_locator = correct_locator;
    remote_locator.address[4]++;
    EXPECT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, other_locator));

    // Check wrong pid
    remote_locator = correct_locator;
    remote_locator.address[8]++;
    EXPECT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, other_locator));

    // Check wrong port
    remote_locator = correct_locator;
    remote_locator.port += 1000;
    EXPECT_FALSE(transportUnderTest.transform_remote_locator(remote_locator, other_locator));

    ASSERT_TRUE(transportUnderTest.CloseInputChannel(correct_locator));
}

TEST_F(SHMTransportTests, transform_remote_locator_failures)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    // Check with multicast
    transform_remote_locator_failures(transportUnderTest, SHMLocator::Type::MULTICAST);

    // Check with unicast
    transform_remote_locator_failures(transportUnderTest, SHMLocator::Type::UNICAST);
}

TEST_F(SHMTransportTests, all_shared_mem_locators_are_local)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t shared_mem_locator;
    shared_mem_locator.kind = LOCATOR_KIND_SHM;
    shared_mem_locator.port = g_default_port;

    // Then
    ASSERT_TRUE(transportUnderTest.is_local_locator(shared_mem_locator));
}

TEST_F(SHMTransportTests, match_if_port_and_address_matches)
{
    // Given
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t locatorAlpha;
    locatorAlpha.kind = LOCATOR_KIND_SHM;
    locatorAlpha.port = g_default_port;

    Locator_t locatorBeta;
    locatorBeta.kind = LOCATOR_KIND_SHM;
    locatorBeta.port = g_default_port;

    // Then
    ASSERT_TRUE(transportUnderTest.DoInputLocatorsMatch(locatorAlpha, locatorBeta));

    locatorBeta.port = g_default_port + 1;
    // Then
    ASSERT_FALSE(transportUnderTest.DoInputLocatorsMatch(locatorAlpha, locatorBeta));
}

TEST_F(SHMTransportTests, send_and_receive_between_ports)
{
    SharedMemTransport transportUnderTest(descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t unicastLocator;
    unicastLocator.kind = LOCATOR_KIND_SHM;
    unicastLocator.port = g_default_port;

    Locator_t outputChannelLocator;
    outputChannelLocator.kind = LOCATOR_KIND_SHM;
    outputChannelLocator.port = g_default_port + 1;

    Semaphore sem;
    MockReceiverResource receiver(transportUnderTest, unicastLocator);
    MockMessageReceiver* msg_recv = dynamic_cast<MockMessageReceiver*>(receiver.CreateMessageReceiver());

    eprosima::fastrtps::rtps::SendResourceList send_resource_list;
    ASSERT_TRUE(transportUnderTest.OpenOutputChannel(send_resource_list, outputChannelLocator));
    ASSERT_FALSE(send_resource_list.empty());
    ASSERT_TRUE(transportUnderTest.IsInputChannelOpen(unicastLocator));
    octet message[5] = { 'H', 'e', 'l', 'l', 'o' };

    std::function<void()> recCallback = [&]()
            {
                EXPECT_EQ(memcmp(message, msg_recv->data, 5), 0);
                sem.post();
            };
    msg_recv->setCallback(recCallback);

    LocatorList locator_list;
    locator_list.push_back(unicastLocator);

    auto sendThreadFunction = [&]()
            {
                Locators locators_begin(locator_list.begin());
                Locators locators_end(locator_list.end());

                EXPECT_TRUE(send_resource_list.at(0)->send(message, 5, &locators_begin, &locators_end,
                        (std::chrono::steady_clock::now() + std::chrono::microseconds(100))));
            };

    std::unique_ptr<std::thread> sender_thread;
    sender_thread.reset(new std::thread(sendThreadFunction));

    sem.wait();
    sender_thread->join();
}

TEST_F(SHMTransportTests, port_and_segment_overflow_discard)
{
    SharedMemTransportDescriptor my_descriptor;

    my_descriptor.segment_size(16);
    my_descriptor.max_message_size(16);
    my_descriptor.port_queue_capacity(4);

    SharedMemTransport transportUnderTest(my_descriptor);
    ASSERT_TRUE(transportUnderTest.init());

    Locator_t unicastLocator;
    unicastLocator.kind = LOCATOR_KIND_SHM;
    unicastLocator.port = g_default_port;

    Semaphore sem;
    MockReceiverResource receiver(transportUnderTest, unicastLocator);
    MockMessageReceiver* msg_recv = dynamic_cast<MockMessageReceiver*>(receiver.CreateMessageReceiver());

    bool is_first_message_received = false;
    std::function<void()> recCallback = [&]()
            {
                is_first_message_received = true;
                sem.wait();
            };
    msg_recv->setCallback(recCallback);

    Locator_t outputChannelLocator;
    outputChannelLocator.kind = LOCATOR_KIND_SHM;
    outputChannelLocator.port = g_default_port + 1;

    eprosima::fastrtps::rtps::SendResourceList send_resource_list;
    ASSERT_TRUE(transportUnderTest.OpenOutputChannel(send_resource_list, outputChannelLocator));
    ASSERT_FALSE(send_resource_list.empty());
    octet message[4] = { 'H', 'e', 'l', 'l'};

    LocatorList locator_list;
    locator_list.push_back(unicastLocator);

    {
        Locators locators_begin(locator_list.begin());
        Locators locators_end(locator_list.end());
        // Internally the segment is bigger than "my_descriptor.segment_size" so a bigger buffer is tried
        // to cause segment overflow
        octet message_big[4096] = { 'H', 'e', 'l', 'l'};

        EXPECT_TRUE(send_resource_list.at(0)->send(message_big, sizeof(message_big), &locators_begin, &locators_end,
                (std::chrono::steady_clock::now() + std::chrono::microseconds(100))));
    }

    // At least 4 msgs of 4 bytes are allowed
    for (int i = 0; i < 4; i++)
    {
        Locators locators_begin(locator_list.begin());
        Locators locators_end(locator_list.end());

        // At least 4 msgs of 4 bytes are allowed
        EXPECT_TRUE(send_resource_list.at(0)->send(message, sizeof(message), &locators_begin, &locators_end,
                (std::chrono::steady_clock::now() + std::chrono::microseconds(100))));
    }

    // Wait until the receiver get the first message
    while (!is_first_message_received)
    {
        std::this_thread::yield();
    }

    // The receiver has poped a message so now 3 messages are in the
    // port's queue

    // Push a 4th should go well
    {
        Locators locators_begin(locator_list.begin());
        Locators locators_end(locator_list.end());

        EXPECT_TRUE(send_resource_list.at(0)->send(message, sizeof(message), &locators_begin, &locators_end,
                (std::chrono::steady_clock::now() + std::chrono::microseconds(100))));
    }

    // Push a 5th will not cause overflow
    {
        Locators locators_begin(locator_list.begin());
        Locators locators_end(locator_list.end());

        EXPECT_TRUE(send_resource_list.at(0)->send(message, sizeof(message), &locators_begin, &locators_end,
                (std::chrono::steady_clock::now() + std::chrono::microseconds(100))));
    }

    sem.disable();
}

TEST_F(SHMTransportTests, port_mutex_deadlock_recover)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);
    SharedMemGlobal* shared_mem_global = shared_mem_manager->global_segment();
    MockPortSharedMemGlobal port_mocker;

    port_mocker.remove_port_mutex(domain_name, 0);

    auto global_port = shared_mem_global->open_port(0, 0, 1, 1000);

    Semaphore sem_lock_done;
    Semaphore sem_end_thread_locker;
    std::thread thread_locker([&]
            {
                // lock has to be done in another thread because
                // boost::inteprocess_named_mutex and  interprocess_mutex are recursive in Win32
                auto port_mutex = port_mocker.get_port_mutex(domain_name, 0);
                ASSERT_TRUE(port_mutex->try_lock());
                sem_lock_done.post();
                sem_end_thread_locker.wait();
            }
            );

    sem_lock_done.wait();

    auto port_mutex = port_mocker.get_port_mutex(domain_name, 0);
    ASSERT_FALSE(port_mutex->try_lock());

    auto global_port2 = shared_mem_global->open_port(0, 0, 1, 1000);

    ASSERT_NO_THROW(global_port2->healthy_check());

    sem_end_thread_locker.post();
    thread_locker.join();
}

TEST_F(SHMTransportTests, port_lock_read_exclusive)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);

    shared_mem_manager->remove_port(0, 0);

    auto port = shared_mem_manager->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::ReadExclusive);
    ASSERT_THROW(shared_mem_manager->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::ReadExclusive),
            std::exception);

    port.reset();
    port = shared_mem_manager->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::ReadExclusive);
}

TEST_F(SHMTransportTests, robust_exclusive_lock)
{
    const std::string lock_name = "robust_exclusive_lock_test1_el";

    RobustExclusiveLock::remove(lock_name.c_str());

    // Lock
    auto el1 = std::make_shared<RobustExclusiveLock>(lock_name);

    // A second lock fail
    ASSERT_THROW(auto tmp = std::make_shared<RobustExclusiveLock>(lock_name), std::exception);
    ASSERT_TRUE(RobustExclusiveLock::is_locked(lock_name));

    // Remove lock
    el1.reset();
    ASSERT_FALSE(RobustExclusiveLock::is_locked(lock_name));

    bool was_created;
    el1 = std::make_shared<RobustExclusiveLock>(lock_name, &was_created);
    // The resource did not exits
    ASSERT_TRUE(was_created);

    el1.reset();
    // Has been already deleted
    ASSERT_FALSE(RobustExclusiveLock::remove(lock_name.c_str()));

    // Create a fake file
    FILE* f = fopen(SharedDir::get_file_path(lock_name).c_str(), "w+");
    ASSERT_TRUE(f != nullptr);
    fclose(f);

    el1 = std::make_shared<RobustExclusiveLock>(lock_name, &was_created);
    ASSERT_FALSE(was_created);
}

TEST_F(SHMTransportTests, robust_shared_lock)
{
    const std::string lock_name = "robust_shared_lock_test1_sl";

    RobustSharedLock::remove(lock_name.c_str());

    ASSERT_FALSE(RobustSharedLock::is_locked(lock_name));

    auto sl1 = std::make_shared<RobustSharedLock>(lock_name);
    ASSERT_TRUE(RobustSharedLock::is_locked(lock_name));

    auto sl2 = std::make_shared<RobustSharedLock>(lock_name);

    sl1.reset();
    // sl2 holds the lock
    ASSERT_TRUE(RobustSharedLock::is_locked(lock_name));

    bool was_lock_created;
    bool was_lock_released;
    auto new_lock = std::make_shared<RobustSharedLock>(lock_name, &was_lock_created, &was_lock_released);
    // sl2 holds the lock so the object exists
    ASSERT_FALSE(was_lock_created);
    ASSERT_FALSE(was_lock_released);

    sl2.reset();
    // still locked by new_lock
    ASSERT_TRUE(RobustSharedLock::is_locked(lock_name));

    new_lock.reset();
    // not locked
    ASSERT_FALSE(RobustSharedLock::is_locked(lock_name));

    // and has been removed
    ASSERT_FALSE(RobustSharedLock::remove(lock_name.c_str()));

    new_lock = std::make_shared<RobustSharedLock>(lock_name, &was_lock_created, &was_lock_released);
    // A new object was been created
    ASSERT_TRUE(was_lock_created);
    ASSERT_TRUE(was_lock_released);

    sl1 = std::make_shared<RobustSharedLock>(lock_name);
    ASSERT_TRUE(RobustSharedLock::is_locked(lock_name));

    new_lock.reset();
    new_lock = std::make_shared<RobustSharedLock>(lock_name, &was_lock_created, &was_lock_released);
    // sl1 holds the lock
    ASSERT_FALSE(was_lock_created);
    ASSERT_FALSE(was_lock_released);

    sl1.reset();
    new_lock.reset();

    // The resource has been removed
    ASSERT_FALSE(RobustSharedLock::remove(lock_name.c_str()));

    // Create a fake file
    FILE* f = fopen(SharedDir::get_file_path(lock_name).c_str(), "w+");
    ASSERT_TRUE(f != nullptr);
    fclose(f);

    new_lock = std::make_shared<RobustSharedLock>(lock_name, &was_lock_created, &was_lock_released);
    // sl1 holds the lock
    ASSERT_FALSE(was_lock_created);
    ASSERT_TRUE(was_lock_released);

    new_lock.reset();
    // The resource has been removed
    ASSERT_FALSE(RobustSharedLock::remove(lock_name.c_str()));
}

// This regression test asserts that shared-memory mapped files cannot exceed file-system bounds.
// The original implementation in boost::interprocess, for posix systems, use ftruncate() to set the
// file size. ftruncate() do not check available size in the file system, so SIGBUS error can occur
// when reading / writing in the mapped mem.
TEST_F(SHMTransportTests, memory_bounds)
{
    const std::string domain_name("SHMTests");
    auto shared_mem_manager = SharedMemManager::create(domain_name);
    auto shm_path = SharedDir::get_file_path("");

    constexpr uint32_t max_system_size = (std::numeric_limits<uint32_t>::max)() - 1024u;

    uint64_t max_file_system_free_size;
    uint64_t physical_mem_size;

#ifndef _MSC_VER // Posix is assumed
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    physical_mem_size = pages * page_size;

    struct statvfs stat_info;
    ASSERT_TRUE(statvfs(shm_path.c_str(), &stat_info) == 0);
    max_file_system_free_size = stat_info.f_bavail * stat_info.f_bsize;

#else // Windows

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    physical_mem_size = status.ullTotalPhys;

    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;

    ASSERT_TRUE(GetDiskFreeSpaceEx(shm_path.c_str(), &FreeBytesAvailableToCaller, &TotalNumberOfBytes,
            &TotalNumberOfFreeBytes));

    max_file_system_free_size = FreeBytesAvailableToCaller.QuadPart;

#endif // ifndef _MSC_VER

    uint64_t mem_size = (std::min)(max_file_system_free_size, physical_mem_size);

    uint32_t allocations;
    uint32_t allocation_size;
    uint64_t free_size60 = static_cast<uint64_t>(mem_size * 0.6);
    uint32_t extra_allocation = 0u;

    // The /dev/shm file system is < physical memory => cannot support allocation beyond 100%
    // Is the default configuration in Linux systems
    bool over_system_limit = (max_file_system_free_size < physical_mem_size);

#if defined(_WIN32) && !defined(_WIN64)
    // Win32 processes have a maximum shared mem allowed of 2GB.
    // The following limit will make a single allocation of 1.3GB on each phase,
    // making the second one throw an exception as expected
    if (free_size60 > max_system_size / 3u)
    {
        free_size60 = max_system_size / 3u;
        over_system_limit = true;
    }
#else
    if (free_size60 > max_system_size)
    {
        allocations = static_cast<uint32_t>(free_size60 / max_system_size);
        allocation_size = max_system_size;
        extra_allocation = free_size60 % max_system_size;
    }
    else
#endif // Win 32 bits
    {
        allocation_size = static_cast<uint32_t>(free_size60);
        allocations = 1u;
    }

    std::cout << "Gathered system information:" << std::endl;
    std::cout << "    - max_file_system_free_size: " << max_file_system_free_size << std::endl;
    std::cout << "    - physical_mem_size:         " << physical_mem_size << std::endl;
    std::cout << "    - free_size60:               " << free_size60 << std::endl;
    std::cout << "    - allocations:               " << allocations << std::endl;
    std::cout << "    - allocation_size:           " << allocation_size << std::endl;
    std::cout << "    - extra_allocation:          " << extra_allocation << std::endl;

    std::vector<std::shared_ptr<SharedMemManager::Segment>> segments;

    auto zero_mem = [](
        const std::shared_ptr<SharedMemManager::Segment>& segment,
        uint32_t size)
            {
                auto buf = segment->alloc_buffer(size,
                                std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
                ASSERT_TRUE(buf != nullptr);
                memset(buf->data(), 0, buf->size());
                buf.reset();
            };

    auto allocation_phase = [&]()
            {
                for (uint32_t i = 0; i < allocations; i++)
                {
                    std::cout << "    - Creating segment of " << allocation_size << " bytes" << std::endl;
                    segments.push_back(shared_mem_manager->create_segment(allocation_size, 1));
                    std::cout << "        - Created. Fill content with 0's" << std::endl;
                    zero_mem(segments.back(), allocation_size);
                    std::cout << "        - Done" << std::endl;
                }
                if (extra_allocation)
                {
                    std::cout << "    - Creating segment of " << extra_allocation << " bytes" << std::endl;
                    segments.push_back(shared_mem_manager->create_segment(extra_allocation, 1));
                    std::cout << "        - Created. Fill content with 0's" << std::endl;
                    zero_mem(segments.back(), extra_allocation);
                    std::cout << "        - Done" << std::endl;
                }
            };

    // Fist allocation must succeed
    std::cout << "First phase:" << std::endl;
    allocation_phase();

    std::cout << "Second phase:" << std::endl;
    if (over_system_limit)
    {
        // Second allocation expected to fail
        EXPECT_THROW(allocation_phase(), std::exception);
    }
    else // Allocation beyond 100% physical mem may or may not be supported by virtual-mem and page file
    {
        try
        {
            allocation_phase();
        }
        catch (const std::exception&)
        {
        }
    }
}

TEST_F(SHMTransportTests, port_listener_dead_recover)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);
    SharedMemGlobal* shared_mem_global = shared_mem_manager->global_segment();

    uint32_t listener1_index;
    auto port1 = shared_mem_global->open_port(0, 0, 1, 1000);
    auto listener1 = port1->create_listener(&listener1_index);

    auto listener2 = shared_mem_manager->open_port(0, 0, 1, 1000)->create_listener();

    std::atomic<uint32_t> thread_listener2_state(0);
    std::thread thread_listener2([&]
            {
                // lock has to be done in another thread because
                // boost::inteprocess_named_mutex and  interprocess_mutex are recursive in Win32
                auto buf = listener2->pop();
                ASSERT_TRUE(*static_cast<uint8_t*>(buf->data()) == 1);

                thread_listener2_state = 1;

                buf = listener2->pop();
                // The pop is broken by port regeneration
                ASSERT_TRUE(buf == nullptr);

                thread_listener2_state = 2;

                buf = listener2->pop();
                // 2 is received in the new regenerated port
                ASSERT_TRUE(*static_cast<uint8_t*>(buf->data()) == 2);

                thread_listener2_state = 3;
            }
            );

    auto port_sender = shared_mem_manager->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::Write);
    auto segment = shared_mem_manager->create_segment(1024, 16);
    auto buf = segment->alloc_buffer(1, std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
    ASSERT_TRUE(buf != nullptr);
    memset(buf->data(), 0, buf->size());
    *static_cast<uint8_t*>(buf->data()) = 1u;
    ASSERT_TRUE(port_sender->try_push(buf));

    // Wait until message received
    while (thread_listener2_state.load() < 1u)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    MockPortSharedMemGlobal port_mocker;
    std::atomic_bool is_listener1_closed(false);
    std::thread thread_listener1([&]
            {
                // Deadlock the listener.
                port_mocker.wait_pop_deadlock(*port1, *listener1, is_listener1_closed, listener1_index);
            }
            );

    // Wait until port is regenerated
    while (thread_listener2_state.load() < 2u)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    *static_cast<uint8_t*>(buf->data()) = 2u;
    // This push must fail because port is not OK
    ASSERT_FALSE(port_sender->try_push(buf));

    // This push must success because port was regenerated in the last try_push call.
    ASSERT_TRUE(port_sender->try_push(buf));

    // Wait until port is regenerated
    while (thread_listener2_state.load() < 3u)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    thread_listener2.join();

    // Unblocks thread_listener1
    port_mocker.unblock_wait_pop(*port1, is_listener1_closed);

    thread_listener1.join();
}

TEST_F(SHMTransportTests, empty_cv_mutex_deadlocked_try_push)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);
    SharedMemGlobal* shared_mem_global = shared_mem_manager->global_segment();
    MockPortSharedMemGlobal port_mocker;

    auto global_port = shared_mem_global->open_port(0, 0, 1, 1000);

    Semaphore sem_lock_done;
    Semaphore sem_end_thread_locker;
    std::thread thread_locker([&]
            {
                // lock has to be done in another thread because
                // boost::inteprocess_named_mutex and  interprocess_mutex are recursive in Win32
                ASSERT_TRUE(port_mocker.lock_empty_cv_mutex(*global_port));
                sem_lock_done.post();
                sem_end_thread_locker.wait();
            }
            );

    sem_lock_done.wait();

    ASSERT_FALSE(port_mocker.lock_empty_cv_mutex(*global_port));

    bool listerner_active;
    SharedMemSegment::Id random_id;
    random_id.generate();
    SharedMemGlobal::BufferDescriptor foo = {random_id, 0, 0};
    ASSERT_THROW(global_port->try_push(foo, &listerner_active), std::exception);

    ASSERT_THROW(global_port->healthy_check(), std::exception);

    sem_end_thread_locker.post();
    thread_locker.join();
}

TEST_F(SHMTransportTests, dead_listener_sender_port_recover)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);
    SharedMemGlobal* shared_mem_global = shared_mem_manager->global_segment();

    shared_mem_global->remove_port(0, 0);
    auto deadlocked_port = shared_mem_global->open_port(0, 0, 1, 1000);
    uint32_t listener_index;
    auto deadlocked_listener = deadlocked_port->create_listener(&listener_index);

    // Simulates a deadlocked wait_pop
    std::atomic_bool is_listener_closed(false);
    std::thread thread_wait_deadlock([&]
            {
                MockPortSharedMemGlobal port_mocker;
                port_mocker.wait_pop_deadlock(*deadlocked_port, *deadlocked_listener,
                is_listener_closed, listener_index);
                (void)port_mocker; // Removes an inexplicable warning when compiling with VC(v140 toolset)
            });

    // Assert the thread is waiting
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Open the deadlocked port
    auto port = shared_mem_global->open_port(0, 0, 1, 1000);
    auto listener = port->create_listener(&listener_index);
    bool listerners_active;
    SharedMemSegment::Id random_id;
    random_id.generate();
    SharedMemGlobal::BufferDescriptor foo = {random_id, 0, 0};
    ASSERT_TRUE(port->try_push(foo, &listerners_active));
    ASSERT_TRUE(listerners_active);
    ASSERT_TRUE(listener->head() != nullptr);
    ASSERT_TRUE(listener->head()->data().source_segment_id == random_id);
    ASSERT_TRUE(listener->pop());

    deadlocked_port->close_listener(&is_listener_closed);
    thread_wait_deadlock.join();
}

TEST_F(SHMTransportTests, port_not_ok_listener_recover)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);
    SharedMemGlobal* shared_mem_global = shared_mem_manager->global_segment();

    shared_mem_global->remove_port(0, 0);
    auto read_port = shared_mem_manager->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::ReadExclusive);
    auto listener = read_port->create_listener();

    std::atomic<uint32_t> stage(0u);

    // Simulates a deadlocked wait_pop
    std::thread thread_listener([&]
            {
                auto buff = listener->pop();
                // The pop is broken by port regeneration
                ASSERT_TRUE(buff == nullptr);
                stage.exchange(1u);
                buff = listener->pop();
                ASSERT_TRUE(*static_cast<uint8_t*>(buff->data()) == 6);
            });

    // Open the deadlocked port
    auto port = shared_mem_global->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::Write);
    auto managed_port = shared_mem_manager->open_port(0, 0, 1, 1000, SharedMemGlobal::Port::OpenMode::Write);
    auto data_segment = shared_mem_manager->create_segment(1, 1);

    MockPortSharedMemGlobal port_mocker;
    port_mocker.set_port_not_ok(*port);
    (void)port_mocker; // Removes an inexplicable warning when compiling with VC(v140 toolset)

    while (stage.load() != 1u)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto buffer = data_segment->alloc_buffer(1, std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
    *static_cast<uint8_t*>(buffer->data()) = 6;
    // Fail because port regeneration
    ASSERT_FALSE(managed_port->try_push(buffer));
    ASSERT_TRUE(managed_port->try_push(buffer));

    thread_listener.join();
}

TEST_F(SHMTransportTests, buffer_recover)
{
    const std::string domain_name("SHMTests");

    auto shared_mem_manager = SharedMemManager::create(domain_name);

    auto segment = shared_mem_manager->create_segment(3, 3);

    shared_mem_manager->remove_port(0, 1);
    auto pub_sub1_write = shared_mem_manager->open_port(0, 1, 8, 1000, SharedMemGlobal::Port::OpenMode::Write);

    shared_mem_manager->remove_port(0, 2);
    auto pub_sub2_write = shared_mem_manager->open_port(0, 2, 8, 1000, SharedMemGlobal::Port::OpenMode::Write);

    auto sub1_read = shared_mem_manager->open_port(0, 1, 8, 1000, SharedMemGlobal::Port::OpenMode::ReadExclusive);

    auto sub2_read = shared_mem_manager->open_port(0, 2, 8, 1000, SharedMemGlobal::Port::OpenMode::ReadExclusive);

    bool exit_listeners = false;

    uint32_t listener1_sleep_ms = 400u;
    uint32_t listener2_sleep_ms = 100u;

    auto listener1 = sub1_read->create_listener();

    std::atomic<uint32_t> listener1_recv_count(0);
    auto thread_listener1 = std::thread( [&]
                    {
                        while (!exit_listeners)
                        {
                            auto buffer = listener1->pop();

                            if (buffer)
                            {
                                listener1_recv_count.fetch_add(1);
                                // This is a slow listener
                                std::this_thread::sleep_for(std::chrono::milliseconds(listener1_sleep_ms));
                                buffer.reset();
                            }
                        }
                    });

    auto listener2 = sub2_read->create_listener();
    std::atomic<uint32_t> listener2_recv_count(0u);
    SharedMemSegment::condition_variable received_cv;
    SharedMemSegment::mutex received_mutex;
    auto thread_listener2 = std::thread( [&]
                    {
                        while (!exit_listeners)
                        {
                            auto buffer = listener2->pop();

                            if (buffer)
                            {
                                {
                                    std::lock_guard<SharedMemSegment::mutex> lock(received_mutex);
                                    listener2_recv_count.fetch_add(1);
                                }
                                std::this_thread::sleep_for(std::chrono::milliseconds(listener2_sleep_ms));
                                buffer.reset();
                                received_cv.notify_one();
                            }
                        }
                    });

    // Test 1 (without port overflow)
    uint32_t send_counter = 0u;
    while (listener1_recv_count.load() < 16u)
    {
        {
            // The segment should never overflow
            auto buf = segment->alloc_buffer(1, std::chrono::steady_clock::time_point());

            ASSERT_EQ(true, pub_sub1_write->try_push(buf));
            ASSERT_EQ(true, pub_sub2_write->try_push(buf));
        }

        {
            std::unique_lock<SharedMemSegment::mutex> lock(received_mutex);
            send_counter++;
            // Wait until listener2 (the fast listener) receives the buffer
            received_cv.wait(lock, [&]
                    {
                        return send_counter == listener2_recv_count.load();
                    });
        }
    }

    // The slow listener is 4 times slower than the fast one
    ASSERT_LT(listener1_recv_count.load() * 3, listener2_recv_count.load());
    ASSERT_GT(listener1_recv_count.load(), listener2_recv_count.load() / 5);
    std::cout << "Test1:"
              << " Listener1_recv_count " << listener1_recv_count.load()
              << " Listener2_recv_count " << listener2_recv_count.load()
              << std::endl;

    // Test 2 (with port overflow)
    listener2_sleep_ms = 0u;
    send_counter = 0u;
    listener1_recv_count.exchange(0u);
    listener2_recv_count.exchange(0u);
    uint32_t port_overflows1 = 0u;
    uint32_t port_overflows2 = 0u;
    while (listener1_recv_count.load() < 16u)
    {
        {
            // The segment should never overflow
            auto buf = segment->alloc_buffer(1u, std::chrono::steady_clock::time_point());

            if (!pub_sub1_write->try_push(buf))
            {
                port_overflows1++;
            }

            if (!pub_sub2_write->try_push(buf))
            {
                port_overflows2++;
            }
        }

        send_counter++;
    }

    std::cout << "Test2:"
              << " port_overflows1 " << port_overflows1
              << " port_overflows2 " << port_overflows2
              << " send_counter " << send_counter
              << " listener1_recv_count " << listener1_recv_count.load()
              << " listener2_recv_count " << listener2_recv_count.load()
              << std::endl;

    ASSERT_GT(port_overflows1, 0u);
    ASSERT_LT(port_overflows2, port_overflows1);
    ASSERT_LT(listener1_recv_count.load(), listener2_recv_count.load());
    ASSERT_GT(send_counter, listener2_recv_count.load());

    exit_listeners = true;

    {
        auto buf = segment->alloc_buffer(1u, std::chrono::steady_clock::time_point());
        ASSERT_EQ(true, pub_sub1_write->try_push(buf));
        ASSERT_EQ(true, pub_sub2_write->try_push(buf));
    }

    thread_listener1.join();
    thread_listener2.join();
}

TEST_F(SHMTransportTests, remote_segments_free)
{
    const std::string domain_name("SHMTests");
    uint32_t num_participants = 100;

    std::vector<std::shared_ptr<SharedMemManager>> managers;
    std::vector<std::shared_ptr<SharedMemManager::Port>> ports;
    std::vector<std::shared_ptr<SharedMemManager::Segment>> segments;
    std::vector<std::shared_ptr<SharedMemManager::Listener>> listeners;

    std::cout << "Creating " << num_participants << " SharedMemManagers & respective segments..." << std::endl;

    // Create participants
    for (uint32_t i = 0; i < num_participants; i++)
    {
        managers.push_back(SharedMemManager::create(domain_name));
        segments.push_back(managers.back()->create_segment(16u, 1u));
        ports.push_back(managers.back()->open_port(0, i, num_participants, 1000));
        listeners.push_back(ports.back()->create_listener());
    }

    uint64_t segment_mem_size = segments[0]->mem_size();

    std::cout << "segment_mem_size = " << segment_mem_size << std::endl;
    std::cout << "Each participant send a message to the others " << segment_mem_size << std::endl;

    // Each participant send a message to the others
    for (uint32_t i = 0; i < num_participants; i++)
    {
        auto buf = segments[i]->alloc_buffer(8, std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
        memset(buf->data(), 0, buf->size());

        for (uint32_t j = 0; j < num_participants; j++)
        {
            if (j != i)
            {
                ASSERT_TRUE(ports[j]->try_push(buf));
                ASSERT_TRUE(listeners[j]->pop() != nullptr);
            }
        }
    }

    uint64_t remote_mem_size = segment_mem_size * (num_participants - 1);

    // Each manager has references to all segments from other managers
    for (uint32_t i = 0; i < num_participants; i++)
    {
        ASSERT_EQ(remote_mem_size, managers[i]->segments_mem());
    }

    std::cout << "Each manager has opened all others remote segments. per manager remote opened mem_size = "
              << remote_mem_size << std::endl
              << " Total remote segments in this process = "
              << (num_participants - 1) * num_participants
              << std::endl;

    // Release all segments except participant 0
    for (uint32_t i = 0; i < num_participants; i++)
    {
        segments[i].reset();
    }

    std::cout << "All segments released " << std::endl;

    auto last_mem_size = remote_mem_size;

    // Wait until manager0, detect releases segments
    uint64_t mem_size;
    uint32_t num_releases_batches = 0u;
    while ((mem_size = managers[0]->segments_mem()) != 0u)
    {
        if (mem_size != last_mem_size)
        {
            std::cout << "manager[0] release detected. remote opened mem_size = " << mem_size << std::endl;
            last_mem_size = mem_size;
            num_releases_batches++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Assuming num_participants is big enough for the watchdog to complete the all releases in one period,
    // several releases batches should be observed.
    ASSERT_GT(num_releases_batches, 1u);

    std::cout << "Wait for all managers release all remote segments..." << std::endl;

    // Wait until all managers have released remote segments
    uint64_t total_mem_in_use = 1;
    while (total_mem_in_use)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        total_mem_in_use = 0;

        for (uint32_t i = 0; i < num_participants; i++)
        {
            total_mem_in_use += managers[i]->segments_mem();
        }
    }
}

/*TEST_F(SHMTransportTests, simple_latency)
   {
    int num_samples = 1000;
    char data[16] = { "" };

    std::thread thread_subscriber([&]
        {
            SharedMemManager shared_mem_manager("SHMTransportTests");
            auto port_pub_to_sub = shared_mem_manager.open_port(0, 64, 1000);
            auto port_sub_to_pub = shared_mem_manager.open_port(1, 64, 1000);
            auto listener_sub = port_pub_to_sub->create_listener();

            auto segment = shared_mem_manager.create_segment(sizeof(data)*64,64);
            int i = num_samples;

            do
            {
                auto recv_sample = listener_sub->pop();

                auto sample_to_send = segment->alloc_buffer(sizeof(data));
                memcpy(sample_to_send->data(), data, sizeof(data));
                ASSERT_TRUE(port_sub_to_pub->try_push(sample_to_send));
            } while (--i);
        });

    std::thread thread_publisher([&]
        {
            SharedMemManager shared_mem_manager("SHMTransportTests");
            auto port_pub_to_sub = shared_mem_manager.open_port(0, 64, 1000);
            auto port_sub_to_pub = shared_mem_manager.open_port(1, 64, 1000);
            auto listener_pub = port_sub_to_pub->create_listener();

            auto segment = shared_mem_manager.create_segment(sizeof(data) * 64, 64);
            int i = num_samples;

            std::chrono::high_resolution_clock::rep total_times = 0;
            std::chrono::high_resolution_clock::rep min_time = (std::numeric_limits<std::chrono::high_resolution_clock::rep>::max)();
            std::chrono::high_resolution_clock::rep max_time = (std::numeric_limits<std::chrono::high_resolution_clock::rep>::min)();

            while (i--)
            {
                auto t0 = std::chrono::high_resolution_clock::now();

                auto sample_to_send = segment->alloc_buffer(sizeof(data));
                memcpy(sample_to_send->data(), data, sizeof(data));
                ASSERT_TRUE(port_pub_to_sub->try_push(sample_to_send));
                sample_to_send.reset();

                auto recv_sample = listener_pub->pop();

                auto t1 = std::chrono::high_resolution_clock::now();

                auto t = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                if (t < min_time)
                {
                    min_time = t;
                }
                if (t > max_time)
                {
                    max_time = t;
                }
                total_times += t;
            }

            printf("LatencyTest for %d samples. Avg = %.3f(us) Min = %.3f(us) Max = %.3f(us)\n", num_samples, total_times / (num_samples*1000.0), min_time/1000.0, max_time/1000.0);
        });

    thread_subscriber.join();
    thread_publisher.join();
   }*/

/*TEST_F(SHMTransportTests, simple_latency2)
   {
    int num_samples = 1000;
    octet data[16] = { "" };

    Locator_t sub_locator;
    sub_locator.kind = LOCATOR_KIND_SHM;
    sub_locator.port = 0;

    Locator_t pub_locator;
    pub_locator.kind = LOCATOR_KIND_SHM;
    pub_locator.port = 1;

    SharedMemTransportDescriptor my_descriptor;

    std::thread thread_subscriber([&]
        {
            SharedMemTransport transport(my_descriptor);
            ASSERT_TRUE(transport.init());

            Semaphore sem;
            MockReceiverResource receiver(transport, sub_locator);
            MockMessageReceiver* msg_recv = dynamic_cast<MockMessageReceiver*>(receiver.CreateMessageReceiver());

            int samples_to_receive = num_samples;

            LocatorList send_locators_list;
            send_locators_list.push_back(pub_locator);

            eprosima::fastrtps::rtps::SendResourceList send_resource_list;
            ASSERT_TRUE(transport.OpenOutputChannel(send_resource_list, pub_locator));

            std::function<void()> sub_callback = [&]()
            {
                Locators locators_begin(send_locators_list.begin());
                Locators locators_end(send_locators_list.end());

                EXPECT_TRUE(send_resource_list.at(0)->send(data, sizeof(data), &locators_begin, &locators_end,
                    (std::chrono::steady_clock::now() + std::chrono::milliseconds(100))));

                if (--samples_to_receive == 0)
                {
                    sem.post();
                }
            };

            msg_recv->setCallback(sub_callback);

            sem.wait();
        });

    std::thread thread_publisher([&]
        {
            SharedMemTransport transport(my_descriptor);
            ASSERT_TRUE(transport.init());

            Semaphore sem;
            MockReceiverResource receiver(transport, pub_locator);
            MockMessageReceiver* msg_recv = dynamic_cast<MockMessageReceiver*>(receiver.CreateMessageReceiver());

            int samples_sent = 0;

            LocatorList send_locators_list;
            send_locators_list.push_back(sub_locator);

            eprosima::fastrtps::rtps::SendResourceList send_resource_list;
            ASSERT_TRUE(transport.OpenOutputChannel(send_resource_list, sub_locator));

            std::chrono::high_resolution_clock::rep total_times = 0;
            std::chrono::high_resolution_clock::rep min_time = (std::numeric_limits<std::chrono::high_resolution_clock::rep>::max)();
            std::chrono::high_resolution_clock::rep max_time = (std::numeric_limits<std::chrono::high_resolution_clock::rep>::min)();

            auto t0 = std::chrono::high_resolution_clock::now();

            Locators locators_begin(send_locators_list.begin());
            Locators locators_end(send_locators_list.end());

            EXPECT_TRUE(send_resource_list.at(0)->send(data, sizeof(data), &locators_begin, &locators_end,
                (std::chrono::steady_clock::now() + std::chrono::milliseconds(100))));

            std::function<void()> pub_callback = [&]()
            {
                if (++samples_sent < num_samples)
                {
                    auto t1 = std::chrono::high_resolution_clock::now();

                    auto t = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                    if (t < min_time)
                    {
                        min_time = t;
                    }
                    if (t > max_time)
                    {
                        max_time = t;
                    }
                    total_times += t;

                    t0 = std::chrono::high_resolution_clock::now();

                    Locators locators_begin(send_locators_list.begin());
                    Locators locators_end(send_locators_list.end());

                    EXPECT_TRUE(send_resource_list.at(0)->send(data, sizeof(data), &locators_begin, &locators_end,
                        (std::chrono::steady_clock::now() + std::chrono::milliseconds(100))));
                }
                else
                {
                    sem.post();
                }
            };

            msg_recv->setCallback(pub_callback);

            sem.wait();

            printf("LatencyTest for %d samples. Avg = %.3f(us) Min = %.3f(us) Max = %.3f(us)\n", num_samples, total_times / (num_samples * 1000.0), min_time / 1000.0, max_time / 1000.0);
        });

    thread_subscriber.join();
    thread_publisher.join();
   }*/

/*TEST_F(SHMTransportTests, simple_throughput)
   {
    const size_t sample_size = 1024;
    int num_samples_per_batch = 100000;

    std::atomic<int> samples_received(0);

    octet sample_data[sample_size];
    memset(sample_data, 0, sizeof(sample_data));

    Locator_t sub_locator;
    sub_locator.kind = LOCATOR_KIND_SHM;
    sub_locator.port = 0;

    Locator_t pub_locator;
    pub_locator.kind = LOCATOR_KIND_SHM;
    pub_locator.port = 1;

    SharedMemTransportDescriptor my_descriptor;

    my_descriptor.port_queue_capacity = num_samples_per_batch;
    my_descriptor.segment_size = sample_size * num_samples_per_batch;

    // Subscriber

    SharedMemTransport sub_transport(my_descriptor);
    ASSERT_TRUE(sub_transport.init());

    MockReceiverResource sub_receiver(sub_transport, sub_locator);
    MockMessageReceiver* sub_msg_recv = dynamic_cast<MockMessageReceiver*>(sub_receiver.CreateMessageReceiver());

    std::function<void()> sub_callback = [&]()
        {
            samples_received.fetch_add(1);
        };

    sub_msg_recv->setCallback(sub_callback);

    // Publisher

    SharedMemTransport pub_transport(my_descriptor);
    ASSERT_TRUE(pub_transport.init());

    LocatorList send_locators_list;
    send_locators_list.push_back(sub_locator);

    eprosima::fastrtps::rtps::SendResourceList send_resource_list;
    ASSERT_TRUE(pub_transport.OpenOutputChannel(send_resource_list, sub_locator));

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int i=0; i<num_samples_per_batch; i++)
    {
        Locators locators_begin(send_locators_list.begin());
        Locators locators_end(send_locators_list.end());

        EXPECT_TRUE(send_resource_list.at(0)->send(sample_data, sizeof(sample_data), &locators_begin, &locators_end,
                (std::chrono::steady_clock::now() + std::chrono::milliseconds(100))));
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    auto real_samples_received = samples_received.load();
    printf("Samples [sent,received] [%d,%d] send_time_per_sample %.3f(us)\n"
        , num_samples_per_batch
        , real_samples_received
        , std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count() / (num_samples_per_batch*1000.0));
   }*/

// This test is linux only
/*TEST_F(SHMTransportTests, simple_throughput_inter)
   {
    const size_t sample_size = 1024;
    int num_samples_per_batch = 100000;

    std::atomic<int> samples_received(0);

    octet sample_data[sample_size];
    memset(sample_data, 0, sizeof(sample_data));

    Locator_t sub_locator;
    sub_locator.kind = LOCATOR_KIND_SHM;
    sub_locator.port = 0;

    Locator_t pub_locator;
    pub_locator.kind = LOCATOR_KIND_SHM;
    pub_locator.port = 1;

    SharedMemTransportDescriptor my_descriptor;

    my_descriptor.port_queue_capacity = num_samples_per_batch;
    my_descriptor.segment_size = sample_size * num_samples_per_batch;

    // Child
    if(fork() == 0)
    {
        Semaphore sem_end_subscriber;

        // Subscriber
        SharedMemTransport sub_transport(my_descriptor);
        ASSERT_TRUE(sub_transport.init());

        MockReceiverResource sub_receiver(sub_transport, sub_locator);
        MockMessageReceiver* sub_msg_recv = dynamic_cast<MockMessageReceiver*>(sub_receiver.CreateMessageReceiver());

        std::function<void()> sub_callback = [&]()
            {
                if(samples_received.fetch_add(1)+1 == num_samples_per_batch)
                {
                    sem_end_subscriber.post();
                }
            };

        sub_msg_recv->setCallback(sub_callback);

        sem_end_subscriber.wait();

        printf("Samples [received] [%d]\n", samples_received.load());
    }
    else
    {
        // Give time to the subscriber to listen
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Publisher
        SharedMemTransport pub_transport(my_descriptor);
        ASSERT_TRUE(pub_transport.init());

        LocatorList send_locators_list;
        send_locators_list.push_back(sub_locator);

        SendResourceList send_resource_list;
        ASSERT_TRUE(pub_transport.OpenOutputChannel(send_resource_list, sub_locator));

        auto t0 = std::chrono::high_resolution_clock::now();

        for (int i=0; i<num_samples_per_batch; i++)
        {
            Locators locators_begin(send_locators_list.begin());
            Locators locators_end(send_locators_list.end());

            EXPECT_TRUE(send_resource_list.at(0)->send(sample_data, sizeof(sample_data), &locators_begin, &locators_end,
                    (std::chrono::steady_clock::now() + std::chrono::milliseconds(100))));
        }

        auto t1 = std::chrono::high_resolution_clock::now();

        printf("Samples [sent] [%d] send_time_per_sample %.3f(us)\n"
            , num_samples_per_batch
            , std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count() / (num_samples_per_batch*1000.0));
    }
   }*/

/*
 #ifdef INSTANTIATE_TEST_SUITE_P
 #define GTEST_INSTANTIATE_TEST_MACRO(x, y, z) INSTANTIATE_TEST_SUITE_P(x, y, z)
 #else
 #define GTEST_INSTANTIATE_TEST_MACRO(x, y, z) INSTANTIATE_TEST_CASE_P(x, y, z, )
 #endif // ifdef INSTANTIATE_TEST_SUITE_P

   GTEST_INSTANTIATE_TEST_MACRO(
    SHMTransportTests,
    SHMRingBufferMultiThread,
    testing::Values(
        std::make_tuple(
            (std::max)((unsigned int)1, std::thread::hardware_concurrency()/2), 100000, 0),
        std::make_tuple(
            (std::max)((unsigned int)1, std::thread::hardware_concurrency())*2, 100,0),
        std::make_tuple(
            (std::max)((unsigned int)1, std::thread::hardware_concurrency()/2), 100000, std::thread::hardware_concurrency()/2),
        std::make_tuple(
            (std::max)((unsigned int)1, std::thread::hardware_concurrency())*2, 100,std::thread::hardware_concurrency())
    )
   );*/

TEST_F(SHMTransportTests, dump_file)
{
    std::string log_file = "shm_transport_dump.txt";
    std::remove(log_file.c_str());

    {
        SharedMemTransportDescriptor shm_descriptor;

        shm_descriptor.rtps_dump_file(log_file);

        SharedMemTransport transportUnderTest(shm_descriptor);
        ASSERT_TRUE(transportUnderTest.init());

        Locator_t unicastLocator;
        unicastLocator.kind = LOCATOR_KIND_SHM;
        unicastLocator.port = g_default_port;

        Locator_t outputChannelLocator;
        outputChannelLocator.kind = LOCATOR_KIND_SHM;
        outputChannelLocator.port = g_default_port + 1;

        Semaphore sem;
        MockReceiverResource receiver(transportUnderTest, unicastLocator);
        MockMessageReceiver* msg_recv = dynamic_cast<MockMessageReceiver*>(receiver.CreateMessageReceiver());

        eprosima::fastrtps::rtps::SendResourceList send_resource_list;
        ASSERT_TRUE(transportUnderTest.OpenOutputChannel(send_resource_list, outputChannelLocator));
        ASSERT_FALSE(send_resource_list.empty());
        ASSERT_TRUE(transportUnderTest.IsInputChannelOpen(unicastLocator));
        octet message[5] = { 'H', 'e', 'l', 'l', 'o' };

        std::function<void()> recCallback = [&]()
                {
                    EXPECT_EQ(memcmp(message, msg_recv->data, 5), 0);
                    sem.post();
                };
        msg_recv->setCallback(recCallback);

        LocatorList locator_list;
        locator_list.push_back(unicastLocator);

        auto sendThreadFunction = [&]()
                {
                    Locators locators_begin(locator_list.begin());
                    Locators locators_end(locator_list.end());

                    EXPECT_TRUE(send_resource_list.at(0)->send(message, 5, &locators_begin, &locators_end,
                            (std::chrono::steady_clock::now() + std::chrono::microseconds(1000))));
                };

        std::unique_ptr<std::thread> sender_thread;
        sender_thread.reset(new std::thread(sendThreadFunction));

        sem.wait();
        sender_thread->join();
    }

    {
        std::ifstream dump_file(log_file.c_str());
        std::string dump_text((std::istreambuf_iterator<char>(dump_file)),
                std::istreambuf_iterator<char>());

        ASSERT_EQ(dump_text.length(), 312u);
        ASSERT_EQ(dump_text.c_str()[308], '6');
        ASSERT_EQ(dump_text.c_str()[309], 'f');
        ASSERT_EQ(dump_text.c_str()[310], 10);
        ASSERT_EQ(dump_text.c_str()[311], 10);
    }

    std::remove(log_file.c_str());
}

int main(
        int argc,
        char** argv)
{
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Info);
    g_default_port = get_port(4000);
    g_output_port = get_port(5000);
    g_input_port = get_port(5010);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
