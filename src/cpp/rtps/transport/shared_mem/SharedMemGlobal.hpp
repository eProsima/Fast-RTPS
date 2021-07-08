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

#ifndef _FASTDDS_SHAREDMEM_GLOBAL_H_
#define _FASTDDS_SHAREDMEM_GLOBAL_H_

#include <vector>
#include <mutex>
#include <memory>

#include <utils/shared_memory/SharedMemSegment.hpp>
#include <utils/shared_memory/RobustExclusiveLock.hpp>
#include <utils/shared_memory/RobustSharedLock.hpp>
#include <utils/shared_memory/SharedMemWatchdog.hpp>

#include <rtps/transport/shared_mem/MultiProducerConsumerRingBuffer.hpp>

#define THREADID "(ID:" << std::this_thread::get_id() << ") "

namespace eprosima {
namespace fastdds {
namespace rtps {

/**
 * This class defines the global resources for shared-memory communication.
 * Mainly the shared-memory ports and its operations.
 */
class SharedMemGlobal
{
public:

    struct BufferDescriptor;

    // Long names for SHM files could cause problems on some platforms
    static constexpr uint32_t MAX_DOMAIN_NAME_LENGTH = 16;

    SharedMemGlobal(
            const std::string& domain_name)
        : domain_name_(domain_name)
    {
        if (domain_name.length() > MAX_DOMAIN_NAME_LENGTH)
        {
            throw std::runtime_error(
                      domain_name +
                      " too long for domain name (max " +
                      std::to_string(MAX_DOMAIN_NAME_LENGTH) +
                      " characters");
        }
    }

    /**
     * Identifies a data buffer given its segment_id (shared-memory segment global_name)
     * and offset inside the segment
     */
    struct BufferDescriptor
    {
        SharedMemSegment::Id source_segment_id;
        SharedMemSegment::Offset buffer_node_offset;
        uint32_t validity_id;
    };

    typedef MultiProducerConsumerRingBuffer<BufferDescriptor>::Listener Listener;
    typedef MultiProducerConsumerRingBuffer<BufferDescriptor>::Cell PortCell;

    static const uint32_t CURRENT_ABI_VERSION = 5;

    struct PortNode
    {
        alignas(8) std::atomic<std::chrono::high_resolution_clock::rep> last_listeners_status_check_time_ms;
        alignas(8) std::atomic<uint32_t> ref_counter;

        SharedMemSegment::Offset buffer;
        SharedMemSegment::Offset buffer_node;

        uint32_t port_id;
        uint32_t num_listeners;
        uint32_t healthy_check_timeout_ms;
        uint32_t port_wait_timeout_ms;
        uint32_t max_buffer_descriptors;
        uint32_t waiting_count;

        uint32_t is_port_ok : 1;
        uint32_t is_opened_read_exclusive : 1;
        uint32_t is_opened_for_reading : 1;
        uint32_t pad : 29;

        UUID<8> uuid;

        SharedMemSegment::condition_variable empty_cv;
        SharedMemSegment::mutex empty_cv_mutex;

        // Number of listeners this port supports
        static constexpr size_t LISTENERS_STATUS_SIZE = 1024;

        // Status of each listener
        struct ListenerStatus
        {
            // True if this slot is taken by an active listener
            uint8_t is_in_use               : 1;

            // True if the listener is waiting for a notification of new message
            uint8_t is_waiting              : 1;

            // True if the listener has taken a new message and is still processing it
            uint8_t is_processing           : 1;

            // Break bit packing, next field will start in a new byte
            uint8_t                         : 0;

            // These are used to detect listeners frozen while waiting for a notification
            uint8_t counter                 : 4;
            uint8_t last_verified_counter   : 4;

            // Descriptor of the message the listener is processing
            // Valid only if is_processing is true.
            BufferDescriptor descriptor;
        };
        ListenerStatus listeners_status[LISTENERS_STATUS_SIZE];

        char domain_name[MAX_DOMAIN_NAME_LENGTH + 1];
    };

    /**
     * A shared-memory port is a communication channel where data can be written / read.
     * A port has a port_id and a global name derived from the port_id and the domain.
     * System processes can open a port by knowing its name.
     */
    class Port
    {
        friend class MockPortSharedMemGlobal;

    private:

        std::shared_ptr<SharedMemSegment> port_segment_;

        PortNode* node_;

        uint32_t address_id_;

        std::unique_ptr<MultiProducerConsumerRingBuffer<BufferDescriptor>> buffer_;

        uint64_t overflows_count_;

        std::unique_ptr<RobustExclusiveLock> read_exclusive_lock_;
        std::unique_ptr<RobustSharedLock> read_shared_lock_;

        inline void notify_unicast(
                bool was_buffer_empty_before_push)
        {
            if (was_buffer_empty_before_push)
            {
                node_->empty_cv.notify_one();
            }
        }

        inline void notify_multicast()
        {
            node_->empty_cv.notify_all();
        }

        /**
         * Singleton task, for SharedMemWatchdog, that periodically checks all opened ports
         * to verify if some listener is dead.
         */
        class WatchTask : public SharedMemWatchdog::Task
        {
        public:

            struct PortContext
            {
                std::shared_ptr<SharedMemSegment> port_segment;
                PortNode* node;
                MultiProducerConsumerRingBuffer<BufferDescriptor>* buffer;
            };

            static const std::shared_ptr<WatchTask>& get()
            {
                static std::shared_ptr<WatchTask> watch_task_instance(new WatchTask());
                return watch_task_instance;
            }

            /**
             * Called by the Port constructor, adds a port to the watching list.
             */
            void add_port(
                    std::shared_ptr<PortContext>&& port)
            {
                std::lock_guard<std::mutex> lock(watched_ports_mutex_);
                watched_ports_.push_back(port);
            }

            /**
             * Called by the Port destructor, removes a port from the watching list.
             */
            void remove_port(
                    PortNode* port_node)
            {
                std::lock_guard<std::mutex> lock(watched_ports_mutex_);

                auto it = watched_ports_.begin();

                while (it != watched_ports_.end())
                {
                    if ((*it)->node == port_node)
                    {
                        watched_ports_.erase(it);
                        break;
                    }

                    ++it;
                }

            }

            virtual ~WatchTask()
            {
                shared_mem_watchdog_->remove_task(this);
            }

        private:

            std::vector<std::shared_ptr<PortContext>> watched_ports_;
            std::mutex watched_ports_mutex_;

            // Keep a reference to the SharedMemWatchdog so that it is not destroyed until this instance is destroyed
            std::shared_ptr<SharedMemWatchdog> shared_mem_watchdog_;

            WatchTask()
                : shared_mem_watchdog_(SharedMemWatchdog::get())
            {
                shared_mem_watchdog_->add_task(this);
            }

            bool update_status_all_listeners(
                    PortNode* port_node)
            {
                uint32_t listeners_found = 0;

                for (uint32_t i = 0; i < PortNode::LISTENERS_STATUS_SIZE; i++)
                {
                    auto& status = port_node->listeners_status[i];

                    if (status.is_in_use)
                    {
                        listeners_found++;
                        // Check only currently waiting listeners
                        if (status.is_waiting)
                        {
                            if (status.counter != status.last_verified_counter)
                            {
                                status.last_verified_counter = status.counter;
                            }
                            else             // Counter is freeze => this listener is blocked!!!
                            {
                                return false;
                            }
                        }

                        if (listeners_found == port_node->num_listeners)
                        {
                            break;
                        }
                    }
                }

                port_node->last_listeners_status_check_time_ms.exchange(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count());

                return listeners_found == port_node->num_listeners;
            }

            void run()
            {
                auto now = std::chrono::high_resolution_clock::now();

                auto timeout_elapsed = [](
                    std::chrono::high_resolution_clock::time_point& now,
                    const PortContext& port_context)
                        {
                            return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
                                   - port_context.node->last_listeners_status_check_time_ms.load()
                                   > port_context.node->healthy_check_timeout_ms;
                        };

                std::lock_guard<std::mutex> lock(watched_ports_mutex_);

                auto port_it =  watched_ports_.begin();
                while (port_it != watched_ports_.end())
                {
                    // If more than 'healthy_check_timeout_ms' milliseconds elapsed since last check.
                    // Most probably has not, so the check is done without locking empty_cv_mutex.
                    if (timeout_elapsed(now, *(*port_it)))
                    {
                        try
                        {
                            std::unique_lock<SharedMemSegment::mutex> lock_port((*port_it)->node->empty_cv_mutex);
                            // Check again, there can be races before locking the mutex.
                            if (timeout_elapsed(now, *(*port_it)))
                            {
                                if (!update_status_all_listeners((*port_it)->node))
                                {
                                    (*port_it)->node->is_port_ok = false;
                                }
                            }

                            ++port_it;
                        }
                        catch (std::exception& e)
                        {
                            (*port_it)->node->is_port_ok = false;

                            logWarning(RTPS_TRANSPORT_SHM, "Port " << (*port_it)->node->port_id
                                                                   << ": " << e.what());

                            // Remove the port from watch
                            port_it = watched_ports_.erase(port_it);
                        }
                    }
                    else
                    {
                        ++port_it;
                    }
                }
            }

        };

        bool check_status_all_listeners() const
        {
            uint32_t listeners_found = 0;

            for (uint32_t i = 0; i < PortNode::LISTENERS_STATUS_SIZE; i++)
            {
                auto& status = node_->listeners_status[i];

                if (status.is_in_use)
                {
                    listeners_found++;
                    // Check only currently waiting listeners
                    if (status.is_waiting)
                    {
                        if (status.counter == status.last_verified_counter)
                        {
                            return false;
                        }
                    }
                }
            }

            return (listeners_found == node_->num_listeners);
        }

        // Keep a reference to the WatchTask so that it is not destroyed until the last Port instance is destroyed
        std::shared_ptr<WatchTask> watch_task_;

    public:

        /**
         * Defines open sharing mode of a shared-memory port:
         *
         * ReadShared (multiple listeners / multiple writers): Once a port is opened ReadShared
         * it cannot be opened ReadExclusive.
         *
         * ReadExclusive (one listener / multipler writers): Once a port is opened ReadExclusive
         * it cannot be opened ReadShared.
         *
         * Write (multiple writers): A port can always be opened for writing.
         */
        enum class OpenMode
        {
            ReadShared,
            ReadExclusive,
            Write
        };

        static std::string open_mode_to_string(
                OpenMode open_mode)
        {
            switch (open_mode)
            {
                case OpenMode::ReadShared: return "ReadShared";
                case OpenMode::ReadExclusive: return "ReadExclusive";
                case OpenMode::Write: return "Write";
            }

            return "";
        }

        Port(
                std::shared_ptr<SharedMemSegment>&& port_segment,
                PortNode* node,
                uint32_t address_id,
                std::unique_ptr<RobustExclusiveLock>&& read_exclusive_lock = std::unique_ptr<RobustExclusiveLock>())
            : port_segment_(std::move(port_segment))
            , node_(node)
            , address_id_(address_id)
            , overflows_count_(0)
            , read_exclusive_lock_(std::move(read_exclusive_lock))
            , watch_task_(WatchTask::get())
        {
            auto buffer_base = static_cast<MultiProducerConsumerRingBuffer<BufferDescriptor>::Cell*>(
                port_segment_->get_address_from_offset(node_->buffer));

            auto buffer_node = static_cast<MultiProducerConsumerRingBuffer<BufferDescriptor>::Node*>(
                port_segment_->get_address_from_offset(node_->buffer_node));

            buffer_ = std::unique_ptr<MultiProducerConsumerRingBuffer<BufferDescriptor>>(
                new MultiProducerConsumerRingBuffer<BufferDescriptor>(buffer_base, buffer_node));

            node_->ref_counter.fetch_add(1);

            auto port_context = std::make_shared<Port::WatchTask::PortContext>();
            *port_context = {port_segment_, node_, buffer_.get()};
            Port::WatchTask::get()->add_port(std::move(port_context));
        }

        ~Port()
        {
            Port::WatchTask::get()->remove_port(node_);

            if (node_->ref_counter.fetch_sub(1) == 1)
            {
                auto segment_name = port_segment_->name();

                try
                {
                    // This check avoid locking port_mutex when the port is not OK, also avoid
                    // recursive lock of port_mutex in create_port()
                    if (node_->is_port_ok)
                    {
                        std::unique_ptr<SharedMemSegment::named_mutex> port_mutex =
                                SharedMemSegment::try_open_and_lock_named_mutex(segment_name + "_mutex");

                        std::unique_lock<SharedMemSegment::named_mutex> port_lock(*port_mutex, std::adopt_lock);

                        // Check again to ensure nobody has re-opened the port while we were locking port_mutex
                        if (node_->ref_counter.load(std::memory_order_relaxed) == 0
                                && is_port_ok())
                        {
                            node_->is_port_ok = false;
                            node_ = nullptr;
                            port_segment_.reset();

                            SharedMemSegment::remove(segment_name.c_str());
                            SharedMemSegment::named_mutex::remove((segment_name + "_mutex").c_str());
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    if (node_)
                    {
                        node_->is_port_ok = false;
                    }

                    logWarning(RTPS_TRANSPORT_SHM, THREADID << segment_name.c_str()
                                                            << e.what());
                }
            }
        }

        /**
         * Try to enqueue a buffer descriptor in the port.
         * If the port queue is full returns inmediatelly with false value.
         * @param[in] buffer_descriptor buffer descriptor to be enqueued
         * @param[out] listeners_active false if no active listeners => buffer not enqueued
         * @return false in overflow case, true otherwise.
         */
        bool try_push(
                const BufferDescriptor& buffer_descriptor,
                bool* listeners_active)
        {
            std::unique_lock<SharedMemSegment::mutex> lock_empty(node_->empty_cv_mutex);

            if (!node_->is_port_ok)
            {
                throw std::runtime_error("the port is marked as not ok!");
            }

            try
            {
                bool was_opened_as_unicast_port = node_->is_opened_read_exclusive;
                bool was_buffer_empty_before_push = buffer_->is_buffer_empty();
                bool was_someone_listening = (node_->waiting_count > 0);

                *listeners_active = buffer_->push(buffer_descriptor);

                lock_empty.unlock();

                if (was_someone_listening)
                {
                    if (was_opened_as_unicast_port)
                    {
                        notify_unicast(was_buffer_empty_before_push);
                    }
                    else
                    {
                        notify_multicast();
                    }
                }

                return true;
            }
            catch (const std::exception&)
            {
                lock_empty.unlock();
                overflows_count_++;
            }
            return false;
        }

        /**
         * Waits while the port is empty and listener is not closed
         * @param[in] listener reference to the listener that will wait for an incoming buffer descriptor.
         * @param[in] is_listener_closed this reference can become true in the middle of the waiting process,
         * @param[in] listener_index to update the port's listener_status,
         * if that happens wait is aborted.
         */
        void wait_pop(
                Listener& listener,
                const std::atomic<bool>& is_listener_closed,
                uint32_t listener_index)
        {
            try
            {
                std::unique_lock<SharedMemSegment::mutex> lock(node_->empty_cv_mutex);

                if (!node_->is_port_ok)
                {
                    throw std::runtime_error("port marked as not ok");
                }

                auto& status = node_->listeners_status[listener_index];
                // Update this listener status
                status.is_waiting = 1;
                status.counter = status.last_verified_counter + 1;
                node_->waiting_count++;

                do
                {
                    boost::system_time const timeout =
                            boost::get_system_time() + boost::posix_time::milliseconds(node_->port_wait_timeout_ms);

                    if (node_->empty_cv.timed_wait(lock, timeout, [&]
                            {
                                return is_listener_closed.load() || listener.head() != nullptr;
                            }))
                    {
                        break; // Codition met, Break the while
                    }
                    else // Timeout
                    {
                        if (!node_->is_port_ok)
                        {
                            throw std::runtime_error("port marked as not ok");
                        }

                        status.counter = status.last_verified_counter + 1;
                    }
                } while (1);

                node_->waiting_count--;
                status.is_waiting = 0;

            }
            catch (const std::exception&)
            {
                node_->is_port_ok = false;
                throw;
            }
        }

        inline bool is_port_ok() const
        {
            return node_->is_port_ok;
        }

        inline uint32_t address_id() const
        {
            return address_id_;
        }

        inline uint32_t port_id() const
        {
            return node_->port_id;
        }

        inline OpenMode open_mode() const
        {
            if (node_->is_opened_for_reading)
            {
                return node_->is_opened_read_exclusive ? OpenMode::ReadExclusive : OpenMode::ReadShared;
            }

            return OpenMode::Write;
        }

        inline uint32_t healthy_check_timeout_ms() const
        {
            return node_->healthy_check_timeout_ms;
        }

        inline uint32_t max_buffer_descriptors() const
        {
            return node_->max_buffer_descriptors;
        }

        /**
         * Set the caller's 'is_closed' flag (protecting empty_cv_mutex) and
         * forces wake-up all listeners on this port.
         * This function is used when destroying a listener waiting for messages
         * in the port.
         * @param is_listener_closed pointer to the atomic is_closed flag of the Listener object.
         * @throw std::exception on error
         */
        void close_listener(
                std::atomic<bool>* is_listener_closed)
        {
            {
                std::lock_guard<SharedMemSegment::mutex> lock(node_->empty_cv_mutex);
                is_listener_closed->exchange(true);
            }

            node_->empty_cv.notify_all();
        }

        /**
         * Removes the head buffer-descriptor from the listener's queue
         * @param [in] listener reference to the listener that will pop the buffer descriptor.
         * @param [out] was_cell_freed is true if the port's cell is freed because all listeners has poped the cell
         * @throw std::runtime_error if buffer is empty
         */
        void pop(
                Listener& listener,
                bool& was_cell_freed)
        {
            was_cell_freed = listener.pop();
        }

        /**
         * Register a new listener
         * The new listener's read pointer is equal to the ring-buffer write pointer at the registering moment.
         * @param [out] listener_index pointer to where the index of the listener is returned. This index is
         * used to reference the elements from the listeners_status array.
         * @return A shared_ptr to the listener.
         * The listener will be unregistered when shared_ptr is destroyed.
         * @throw std::exception on error
         */
        std::unique_ptr<Listener> create_listener(
                uint32_t* listener_index)
        {
            std::unique_ptr<Listener> listener;

            std::lock_guard<SharedMemSegment::mutex> lock(node_->empty_cv_mutex);

            uint32_t i;
            // Find a free listener_status
            for (i = 0; i < PortNode::LISTENERS_STATUS_SIZE; i++)
            {
                if (!node_->listeners_status[i].is_in_use)
                {
                    break;
                }
            }

            if (i < PortNode::LISTENERS_STATUS_SIZE)
            {
                *listener_index = i;
                node_->listeners_status[i].is_in_use = true;
                node_->listeners_status[i].is_processing = false;
                node_->num_listeners++;
                listener = buffer_->register_listener();
            }
            else
            {
                throw std::runtime_error("max listeners reached");
            }

            return listener;
        }

        /**
         * Remove the listener from the port, the cells still not processed by the listener are freed
         * @throw std::exception on failure
         */
        void unregister_listener(
                std::unique_ptr<Listener>* listener,
                uint32_t listener_index)
        {
            try
            {
                std::lock_guard<SharedMemSegment::mutex> lock(node_->empty_cv_mutex);

                (*listener).reset();
                node_->num_listeners--;
                node_->listeners_status[listener_index].is_in_use = false;
                node_->listeners_status[listener_index].is_processing = false;
            }
            catch (const std::exception&)
            {
                node_->is_port_ok = false;

                (*listener).reset();

                throw;
            }
        }

        /**
         * @brief Look for a listener processing a buffer and return the buffer descriptor being processed
         *
         * Iterates over all the listeners, and upon finding the first one that is marked as \c is_processing:
         *  - Marks it as not processing
         *  - Copies the buffer descriptor that was being processed in the provided buffer
         *  - Finishes iterating
         *
         * The intention of this method is to locate all buffer descriptors that are being processed
         * and apply necessary operations on these buffers when the port is found to be zombie.
         * This method should be called iteratively until the return value is false
         * (there is no processing listener remaining).
         * The calling method has the chance to apply necessary actions on the buffer descriptor.
         *
         * \pre Current port is zombie, as reported by is_zombie()
         *
         * @param[OUT] buffer_descriptor Descriptor of the buffer that was being processed by the first processing listener
         * @return True if there was at least one processing listener. False if not.
         */
        bool get_and_remove_blocked_processing(
                BufferDescriptor& buffer_descriptor)
        {
            std::lock_guard<SharedMemSegment::mutex> lock(node_->empty_cv_mutex);
            for (uint32_t i = 0; i < PortNode::LISTENERS_STATUS_SIZE; i++)
            {
                if (node_->listeners_status[i].is_in_use &&
                        node_->listeners_status[i].is_processing)
                {
                    buffer_descriptor = node_->listeners_status[i].descriptor;
                    listener_processing_stop(i);
                    return true;
                }
            }
            return false;
        }

        /**
         * Marks the listener as processing a buffer
         * @param listener_index The index of the listener as returned by create_listener.
         * @param buffer_descriptor The descriptor of the buffer that the listener is processing
         */
        void listener_processing_start(
                uint32_t listener_index,
                const BufferDescriptor& buffer_descriptor)
        {
            node_->listeners_status[listener_index].descriptor = buffer_descriptor;
            node_->listeners_status[listener_index].is_processing = true;
        }

        /**
         * Marks the listener as finished processing a buffer
         * @param listener_index The index of the listener as returned by create_listener.
         */
        void listener_processing_stop(
                uint32_t listener_index)
        {
            node_->listeners_status[listener_index].is_processing = false;
        }

        /**
         * Performs a check on the opened port.
         * When a process crashes with a port opened the port can be left inoperative.
         * @throw std::exception if the port is inoperative.
         */
        void healthy_check()
        {
            if (!node_->is_port_ok)
            {
                throw std::runtime_error("port is marked as not ok");
            }

            auto t0 = std::chrono::high_resolution_clock::now();

            // If in any moment during the timeout all waiting listeners are OK
            // then the port is OK
            bool is_check_ok = false;
            while ( !is_check_ok &&
                    std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::high_resolution_clock::now() - t0).count() < node_->healthy_check_timeout_ms)
            {
                {
                    std::unique_lock<SharedMemSegment::mutex> lock(node_->empty_cv_mutex);
                    is_check_ok = check_status_all_listeners();

                    if (!node_->is_port_ok)
                    {
                        throw std::runtime_error("port marked as not ok");
                    }
                }

                if (!is_check_ok)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(node_->port_wait_timeout_ms));
                }
            }

            if (!is_check_ok || !node_->is_port_ok)
            {
                node_->is_port_ok = false;
                throw std::runtime_error("healthy_check failed");
            }
        }

        void lock_read_exclusive()
        {
            std::string lock_name = get_port_segment_name(node_->domain_name, address_id_, node_->port_id) + "_el";
            read_exclusive_lock_ = std::unique_ptr<RobustExclusiveLock>(new RobustExclusiveLock(lock_name));
        }

        void lock_read_shared()
        {
            std::string lock_name = get_port_segment_name(node_->domain_name, address_id_, node_->port_id) + "_sl";
            read_shared_lock_ = std::unique_ptr<RobustSharedLock>(new RobustSharedLock(lock_name));
        }

        void unlock_read_locks()
        {
            read_exclusive_lock_.reset();
            read_shared_lock_.reset();
        }

        static std::unique_ptr<RobustExclusiveLock> lock_read_exclusive(
                uint32_t address_id,
                uint32_t port_id,
                const std::string& domain_name)
        {
            std::string lock_name = get_port_segment_name(domain_name, address_id, port_id) + "_el";
            return std::unique_ptr<RobustExclusiveLock>(new RobustExclusiveLock(lock_name));
        }

        static bool is_zombie(
                uint32_t address_id,
                uint32_t port_id,
                const std::string& domain_name)
        {
            bool was_lock_created;
            std::string lock_name;

            try
            {
                // An exclusive port is zombie when it has a "_el" file & the file is not locked
                lock_name = get_port_segment_name(domain_name, address_id, port_id) + "_el";
                RobustExclusiveLock zombie_test(lock_name, &was_lock_created);
                // Lock acquired, did the file exist before the acquire?
                if (!was_lock_created)
                {
                    // Yes, is zombie
                    return true;
                }
            }
            catch (const std::exception&)
            {
                // Resource locked => not zombie.
            }

            try
            {
                // A shared port is zombie when it has a "_sl" file & the file is not locked
                lock_name = get_port_segment_name(domain_name, address_id, port_id) + "_sl";
                bool was_lock_released;
                RobustSharedLock zombie_test(lock_name, &was_lock_created, &was_lock_released);
                // Lock acquired, did the file exist and was release before the acquire?
                if (!was_lock_created && was_lock_released)
                {
                    // Yes, is zombie
                    return true;
                }
            }
            catch (const std::exception&)
            {
                // Resource locked => not zombie.
            }

            return false;
        }

    }; // Port

    /**
     * Checks if a port can be open for writing.
     * @param port_id The identifier of the port to check.
     * @return whether the port can be written or not.
     */
    bool can_write_to_port(
            uint32_t address_id,
            uint32_t port_id) const
    {
        try
        {
            auto port_segment_name = get_port_segment_name(address_id, port_id);

            std::unique_ptr<SharedMemSegment::named_mutex> port_mutex =
                    SharedMemSegment::open_or_create_and_lock_named_mutex(port_segment_name + "_mutex");

            std::unique_lock<SharedMemSegment::named_mutex> port_lock(*port_mutex, std::adopt_lock);

            if (Port::is_zombie(address_id, port_id, domain_name_))
            {
                return false;
            }

            // Try to open
            auto port_segment = std::shared_ptr<SharedMemSegment>(
                new SharedMemSegment(boost::interprocess::open_only, port_segment_name.c_str()));

            if (!port_segment->check_sanity())
            {
                return false;
            }

            auto port_node = port_segment->get().find<PortNode>(
                ("port_node_abi" + std::to_string(CURRENT_ABI_VERSION)).c_str()).first;

            if (port_node)
            {
                return true;
            }
        }
        catch (std::exception&)
        {
        }

        return false;
    }

    /**
     * Open a shared-memory port. If the port doesn't exist in the system a port with port_id is created,
     * otherwise the existing port is opened.
     *
     * @param [in] port_id Identifies the port
     * @param [in] max_buffer_descriptors Capacity of the port (only used if the port is created)
     * @param [in] healthy_check_timeout_ms Timeout for healthy check test
     * @param [in] open_mode Can be ReadShared, ReadExclusive or Write (see Port::OpenMode enum).
     *
     * @return A shared_ptr to the new port
     * @return nullptr if the open_mode is ReadExclusive and the port_id is already opened.
     * @remarks This function performs a test to validate whether the existing port is OK, if the test
     * goes wrong the existing port is removed from shared-memory and a new port is created.
     */
    std::shared_ptr<Port> open_port(
            uint32_t address_id,
            uint32_t port_id,
            uint32_t max_buffer_descriptors,
            uint32_t healthy_check_timeout_ms,
            Port::OpenMode open_mode = Port::OpenMode::ReadShared)
    {
        return open_port_internal(address_id, port_id, max_buffer_descriptors, healthy_check_timeout_ms,
                       open_mode, nullptr);
    }

    /**
     * Delete the port and open a new one with the same ID.
     * It is used when a port has been marked as not OK.
     * @return A shared_ptr to the new port
     * @throw std::exception on error
     */
    std::shared_ptr<Port> regenerate_port(
            std::shared_ptr<Port> port,
            SharedMemGlobal::Port::OpenMode open_mode)
    {
        return open_port_internal(
            port->address_id(),
            port->port_id(),
            port->max_buffer_descriptors(),
            port->healthy_check_timeout_ms(),
            open_mode,
            port);
    }

    /**
     * Remove a port from the system.
     */
    void remove_port(
            uint32_t address_id,
            uint32_t port_id)
    {
        auto port_segment_name = get_port_segment_name(address_id, port_id);
        SharedMemSegment::remove(port_segment_name.c_str());
    }

    std::string domain_name()
    {
        return domain_name_;
    }

private:

    std::string domain_name_;

    std::string get_port_segment_name(
            uint32_t address_id,
            uint32_t port_id) const
    {
        return address_id ?
               domain_name_ + "_port_" + std::to_string(address_id) + "_" + std::to_string(port_id) :
               domain_name_ + "_port" + std::to_string(port_id);
    }

    static std::string get_port_segment_name(
            const std::string& domain_name,
            uint32_t address_id,
            uint32_t port_id)
    {
        return address_id ?
               domain_name + "_port_" + std::to_string(address_id) + "_" + std::to_string(port_id) :
               domain_name + "_port" + std::to_string(port_id);
    }

    std::shared_ptr<Port> open_port_internal(
            uint32_t address_id,
            uint32_t port_id,
            uint32_t max_buffer_descriptors,
            uint32_t healthy_check_timeout_ms,
            Port::OpenMode open_mode,
            std::shared_ptr<Port> regenerating_port)
    {
        std::string err_reason;
        std::shared_ptr<Port> port;

        auto port_segment_name = get_port_segment_name(address_id, port_id);

        logInfo(RTPS_TRANSPORT_SHM, THREADID << "Opening "
                                             << port_segment_name);

        std::unique_ptr<SharedMemSegment::named_mutex> port_mutex =
                SharedMemSegment::open_or_create_and_lock_named_mutex(port_segment_name + "_mutex");

        std::unique_lock<SharedMemSegment::named_mutex> port_lock(*port_mutex, std::adopt_lock);

        if (regenerating_port)
        {
            try
            {
                regenerating_port->unlock_read_locks();
            }
            catch (std::exception& e)
            {
                logError(RTPS_TRANSPORT_SHM, THREADID << "Port "
                                                      << port_id << " failed unlock_read_locks " << e.what());
            }
        }

        try
        {
            if (Port::is_zombie(address_id, port_id, domain_name_))
            {
                logWarning(RTPS_TRANSPORT_SHM, THREADID << "Port "
                                                        << port_id << " Zombie. Reset the port");

                SharedMemSegment::remove(port_segment_name.c_str());

                throw std::runtime_error("zombie port");
            }

            // Try to open
            auto port_segment = std::shared_ptr<SharedMemSegment>(
                new SharedMemSegment(boost::interprocess::open_only, port_segment_name.c_str()));

            SharedMemGlobal::PortNode* port_node;

            try
            {
                if (!port_segment->check_sanity())
                {
                    throw std::runtime_error("check_sanity failed");
                }

                port_node = port_segment->get().find<PortNode>(
                    ("port_node_abi" + std::to_string(CURRENT_ABI_VERSION)).c_str()).first;

                if (port_node)
                {
                    port = std::make_shared<Port>(std::move(port_segment), port_node, address_id);
                }
                else
                {
                    throw std::runtime_error("port_abi not compatible");
                }
            }
            catch (std::exception&)
            {
                logWarning(RTPS_TRANSPORT_SHM, THREADID << "Port "
                                                        << port_id << " Couldn't find port_node ");

                SharedMemSegment::remove(port_segment_name.c_str());

                logWarning(RTPS_TRANSPORT_SHM, THREADID << "Port "
                                                        << port_id << " Removed.");

                throw;
            }

            try
            {
                if (open_mode == Port::OpenMode::ReadExclusive)
                {
                    try
                    {
                        port->lock_read_exclusive();
                    }
                    catch (const std::exception&)
                    {
                        std::stringstream ss;

                        ss << port_node->port_id << " (" << port_node->uuid.to_string() <<
                            ") because is ReadExclusive locked";

                        err_reason = ss.str();
                        port.reset();
                    }
                }

                if (port)
                {
                    port->healthy_check();

                    port_node->is_opened_read_exclusive |= (open_mode == Port::OpenMode::ReadExclusive);
                    port_node->is_opened_for_reading |= (open_mode != Port::OpenMode::Write);

                    logInfo(RTPS_TRANSPORT_SHM,
                            THREADID << "Port " << port_node->port_id << " (" << port_node->uuid.to_string() <<
                            ") Opened" << Port::open_mode_to_string(open_mode));
                }
            }
            catch (std::exception&)
            {
                port->unlock_read_locks();

                port_node->is_port_ok = false;

                auto port_uuid = port_node->uuid.to_string();

                logWarning(RTPS_TRANSPORT_SHM, THREADID << "Existing Port "
                                                        << port_id << " (" << port_uuid << ") NOT Healthy.");

                SharedMemSegment::remove(port_segment_name.c_str());

                logWarning(RTPS_TRANSPORT_SHM, THREADID << "Port "
                                                        << port_id << " (" << port_uuid << ") Removed.");

                throw;
            }
        }
        catch (std::exception&)
        {
            // Doesn't exist => create it
            // The segment will contain the node, the buffer and the internal allocator structures (512bytes estimated)
            uint32_t extra = 512;
            uint32_t segment_size = sizeof(PortNode) + sizeof(PortCell) * max_buffer_descriptors;

            std::unique_ptr<SharedMemSegment> port_segment;

            try
            {
                port_segment = std::unique_ptr<SharedMemSegment>(
                    new SharedMemSegment(boost::interprocess::create_only, port_segment_name.c_str(),
                    segment_size + extra));
            }
            catch (std::exception& e)
            {
                logWarning(RTPS_TRANSPORT_SHM, "Failed to create port segment " << port_segment_name
                                                                                << ": " << e.what());
            }

            if (port_segment)
            {
                try
                {
                    // Memset the whole segment to zero in order to force physical map of the buffer
                    auto payload = port_segment->get().allocate(segment_size);
                    memset(payload, 0, segment_size);
                    port_segment->get().deallocate(payload);

                    port = init_port(address_id, port_id, port_segment, max_buffer_descriptors, open_mode,
                                    healthy_check_timeout_ms);
                }
                catch (std::exception& e)
                {
                    SharedMemSegment::remove(port_segment_name.c_str());

                    logError(RTPS_TRANSPORT_SHM, "Failed init_port " << port_segment_name
                                                                     << ": " << e.what());

                    throw;
                }
            }
        }

        if (port == nullptr)
        {
            throw std::runtime_error("Couldn't open port " + err_reason);
        }

        return port;
    }

    std::shared_ptr<Port> init_port(
            uint32_t address_id,
            uint32_t port_id,
            std::unique_ptr<SharedMemSegment>& segment,
            uint32_t max_buffer_descriptors,
            Port::OpenMode open_mode,
            uint32_t healthy_check_timeout_ms)
    {
        std::shared_ptr<Port> port;
        PortNode* port_node = nullptr;
        MultiProducerConsumerRingBuffer<BufferDescriptor>::Node* buffer_node = nullptr;

        std::unique_ptr<RobustExclusiveLock> lock_read_exclusive;
        if (open_mode == Port::OpenMode::ReadExclusive)
        {
            lock_read_exclusive = Port::lock_read_exclusive(address_id, port_id, domain_name_);
        }

        // Port node allocation
        port_node =
                segment->get().construct<PortNode>(("port_node_abi" +
                        std::to_string(CURRENT_ABI_VERSION)).c_str())();
        port_node->is_port_ok = false;
        port_node->port_id = port_id;
        UUID<8>::generate(port_node->uuid);
        port_node->waiting_count = 0;
        port_node->is_opened_read_exclusive = (open_mode == Port::OpenMode::ReadExclusive);
        port_node->is_opened_for_reading = (open_mode != Port::OpenMode::Write);
        port_node->num_listeners = 0;
        port_node->healthy_check_timeout_ms = healthy_check_timeout_ms;
        port_node->last_listeners_status_check_time_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        port_node->port_wait_timeout_ms = healthy_check_timeout_ms / 3;
        port_node->max_buffer_descriptors = max_buffer_descriptors;
        memset(port_node->listeners_status, 0, sizeof(port_node->listeners_status));
#ifdef _MSC_VER
        strncpy_s(port_node->domain_name, sizeof(port_node->domain_name),
                domain_name_.c_str(), sizeof(port_node->domain_name) - 1);
#else
        strncpy(port_node->domain_name, domain_name_.c_str(), sizeof(port_node->domain_name) - 1);
#endif // ifdef _MSC_VER
        port_node->domain_name[sizeof(port_node->domain_name) - 1] = 0;

        // Buffer cells allocation
        auto buffer = segment->get().construct<MultiProducerConsumerRingBuffer<BufferDescriptor>::Cell>(
            boost::interprocess::anonymous_instance)[max_buffer_descriptors]();
        port_node->buffer = segment->get_offset_from_address(buffer);

        // Buffer node allocation
        buffer_node = segment->get().construct<MultiProducerConsumerRingBuffer<BufferDescriptor>::Node>(
            boost::interprocess::anonymous_instance)();

        MultiProducerConsumerRingBuffer<BufferDescriptor>::init_node(buffer_node, max_buffer_descriptors);

        port_node->buffer_node = segment->get_offset_from_address(buffer_node);

        port_node->is_port_ok = true;
        port = std::make_shared<Port>(std::move(segment), port_node, address_id, std::move(lock_read_exclusive));

        if (open_mode == Port::OpenMode::ReadShared)
        {
            port->lock_read_shared();
        }

        logInfo(RTPS_TRANSPORT_SHM, THREADID << "Port "
                                             << port_node->port_id << " (" << port_node->uuid.to_string()
                                             << Port::open_mode_to_string(open_mode) << ") Created.");

        return port;
    }

};

} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_SHAREDMEM_GLOBAL_H_