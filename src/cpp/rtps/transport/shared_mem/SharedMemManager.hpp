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

#ifndef _FASTDDS_SHAREDMEM_MANAGER_H_
#define _FASTDDS_SHAREDMEM_MANAGER_H_

#include <atomic>
#include <list>
#include <unordered_map>

#include <rtps/transport/shared_mem/SharedMemGlobal.hpp>

#include <utils/shared_memory/RobustSharedLock.hpp>
#include <utils/shared_memory/SharedMemWatchdog.hpp>

namespace eprosima {
namespace fastdds {
namespace rtps {

using Log = fastdds::dds::Log;

/**
 *  Provides functionality for the application to:
 *
 *  Open shared-memory ports.
 *  Create shared memory segments.
 */
class SharedMemManager :
    public std::enable_shared_from_this<SharedMemManager>
{
private:

    struct BufferNode
    {
        struct Status
        {
            // When buffers are enqueued in a port this validity_id is copied to the BufferDescriptor in the port.
            // If the sender process needs to recover the buffer by force, just increment this validity_id, the
            // receiver process must check this validity_id vs the validity_id in the BufferDescriptor, if not equal
            // the buffer has been invalidated by the sender.
            uint64_t validity_id : 24;
            // This counter is incremented each time the buffer is enqueued in a port, an decremented when pop
            // from the port to be processed
            uint64_t enqueued_count : 20;
            // When listener processes start processing this buffer increments the processing_count. This way
            // the sender can know whether the buffer is been processed or is just only enqueued in some ports.
            uint64_t processing_count : 20;
        };

        std::atomic<Status> status;
        uint32_t data_size;
        SharedMemSegment::Offset data_offset;

        /**
         * Atomically invalidates a buffer.
         */
        inline void invalidate_buffer()
        {
            auto s = status.load(std::memory_order_relaxed);
            while (!status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id + 1, (uint64_t)0u, (uint64_t)0u },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }
        }

        /**
         * Atomically invalidates a buffer, only, when the buffer is valid for the caller.
         * @return true when succeeded, false when the buffer was invalid for the caller.
         */
        inline bool invalidate_buffer(
                uint32_t listener_validity_id)
        {
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id + 1, (uint64_t)0u, (uint64_t)0u },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically invalidates a buffer, only, if the buffer is not being processed.
         * @return true when succeeded, false otherwise.
         */
        bool invalidate_if_not_processing()
        {
            auto s = status.load(std::memory_order_relaxed);
            // If the buffer is not beeing processed by any listener => is invalidated
            while (s.processing_count == 0 &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id + 1, (uint64_t)0u, (uint64_t)0u},
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }
            logWarning(RTPS_TRANSPORT_SHM, "Buffer is being invalidated, segment_size may be insufficient");
            return (s.processing_count == 0);
        }

        /**
         * @return true if listener_validity_id == current buffer validity_id.
         */
        inline bool is_valid(
                uint32_t listener_validity_id) const
        {
            return (status.load(std::memory_order_relaxed).validity_id == listener_validity_id);
        }

        /**
         * Atomically decrease enqueued count & increase the buffer processing counts, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool dec_enqueued_inc_processing_counts(
                uint32_t listener_validity_id)
        {
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count - 1, (uint64_t)s.processing_count + 1 },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically increase the buffer processing count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool inc_processing_count(
                uint32_t listener_validity_id)
        {
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count, (uint64_t)s.processing_count + 1 },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically increase the buffer enqueued count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool inc_enqueued_count(
                uint32_t listener_validity_id)
        {
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count + 1, (uint64_t)s.processing_count },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically decrease the buffer enqueued count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool dec_enqueued_count(
                uint32_t listener_validity_id)
        {
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count - 1, (uint64_t)s.processing_count },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        inline bool is_not_referenced() const
        {
            auto s = status.load(std::memory_order_relaxed);
            return (s.enqueued_count == 0) && (s.processing_count == 0);
        }

        /**
         * Atomically decrease the buffer processing count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool dec_processing_count(
                uint32_t listener_validity_id)
        {
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count, (uint64_t)s.processing_count - 1 },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

    };

    SharedMemManager(
            const std::string& domain_name)
        : segments_mem_(0)
        , global_segment_(domain_name)
        , watch_task_(SegmentWrapper::WatchTask::get())
    {
        static_assert(std::alignment_of<BufferNode>::value % 8 == 0, "SharedMemManager::BufferNode bad alignment");

        if (domain_name.length() > SharedMemGlobal::MAX_DOMAIN_NAME_LENGTH)
        {
            throw std::runtime_error(
                      domain_name +
                      " too long for domain name (max " +
                      std::to_string(SharedMemGlobal::MAX_DOMAIN_NAME_LENGTH) +
                      " characters");
        }

        per_allocation_extra_size_ =
                SharedMemSegment::compute_per_allocation_extra_size(std::alignment_of<BufferNode>::value,
                        domain_name);
    }

public:

    static std::shared_ptr<SharedMemManager> create(
            const std::string& domain_name)
    {
        return std::shared_ptr<SharedMemManager>(new SharedMemManager(domain_name));
    }

    ~SharedMemManager()
    {
        remove_segments_from_watch();
    }

    class Buffer
    {
    protected:

        virtual ~Buffer() = default;

    public:

        virtual void* data() = 0;
        virtual uint32_t size() = 0;
    };

    class SharedMemBuffer : public Buffer
    {
    public:

        SharedMemBuffer(
                std::shared_ptr<SharedMemSegment>& segment,
                const SharedMemSegment::Id& segment_id,
                BufferNode* buffer_node,
                uint32_t original_validity_id)
            : segment_(segment)
            , segment_id_(segment_id)
            , buffer_node_(buffer_node)
            , original_validity_id_(original_validity_id)
        {
            data_ = segment_->get_address_from_offset(buffer_node_->data_offset);
        }

        ~SharedMemBuffer() override
        {
            buffer_node_->dec_processing_count(original_validity_id_);
        }

        void* data() override
        {
            return data_;
        }

        uint32_t size() override
        {
            return buffer_node_->data_size;
        }

        SharedMemSegment::Offset node_offset() const
        {
            return segment_->get_offset_from_address(buffer_node_);
        }

        SharedMemSegment::Id segment_id() const
        {
            return segment_id_;
        }

        inline uint32_t validity_id() const
        {
            return original_validity_id_;
        }

        void inc_enqueued_count(
                uint32_t validity_id)
        {
            buffer_node_->inc_enqueued_count(validity_id);
        }

        void dec_enqueued_count(
                uint32_t validity_id)
        {
            buffer_node_->dec_enqueued_count(validity_id);
        }

    private:

        std::shared_ptr<SharedMemSegment> segment_;
        SharedMemSegment::Id segment_id_;
        BufferNode* buffer_node_;
        void* data_;
        uint32_t original_validity_id_;
    };

    /**
     * Handle a shared-memory segment
     * Allows buffer allocation / deallocation
     */
    class Segment
    {
    public:

        Segment(
                uint32_t size,
                uint32_t payload_size,
                uint32_t max_allocations,
                const std::string& domain_name)
            : segment_id_()
            , overflows_count_(0)
        {
            generate_segment_id_and_name(domain_name);

            SharedMemSegment::remove(segment_name_.c_str());

            try
            {
                segment_ = std::unique_ptr<SharedMemSegment>(
                    new SharedMemSegment(boost::interprocess::create_only, segment_name_.c_str(), size));
            }
            catch (const std::exception& e)
            {
                logError(RTPS_TRANSPORT_SHM, "Failed to create segment " << segment_name_
                                                                         << ": " << e.what());

                throw;
            }

            free_bytes_ = payload_size;

            // Alloc the buffer nodes
            auto buffers_nodes = segment_->get().construct<BufferNode>
                        (boost::interprocess::anonymous_instance)[max_allocations]();

            // All buffer nodes are free
            for (uint32_t i = 0; i < max_allocations; i++)
            {
                buffers_nodes[i].status.exchange({0, 0, 0});
                buffers_nodes[i].data_size = 0;
                buffers_nodes[i].data_offset = 0;
                free_buffers_.push_back(&buffers_nodes[i]);
            }
        }

        ~Segment()
        {
            segment_.reset();

            // After remove(), remote processes with the segment open will still have the memory block mapped,
            // but that block is unlinked to the namespace, so next trials to open a segment called segment_name_
            // will fail.
            // The memory block will be freed, by the OS, when last handle is closed.
            SharedMemSegment::remove(segment_name_.c_str());

            if (overflows_count_)
            {
                logWarning(RTPS_TRANSPORT_SHM,
                        "Segment " << segment_id_.to_string().c_str()
                                   << " closed. It had " << "overflows_count "
                                   << overflows_count_);
            }
        }

        SharedMemSegment::Id id()
        {
            return segment_id_;
        }

        std::shared_ptr<Buffer> alloc_buffer(
                uint32_t size,
                const std::chrono::steady_clock::time_point& max_blocking_time_point)
        {
            (void)max_blocking_time_point;

            std::lock_guard<std::mutex> lock(alloc_mutex_);

            if (!recover_buffers(size))
            {
                throw std::runtime_error("allocation overflow");
            }

            void* data = nullptr;
            BufferNode* buffer_node = nullptr;
            std::shared_ptr<SharedMemBuffer> new_buffer;

            try
            {
                buffer_node = pop_free_node();

                data = segment_->get().allocate(size);
                free_bytes_ -= size;

                buffer_node->data_offset = segment_->get_offset_from_address(data);
                buffer_node->data_size = size;

                auto validity_id = buffer_node->status.load(std::memory_order_relaxed).validity_id;

                // TODO(Adolfo) : Dynamic allocation. Use foonathan to convert it to static allocation
                new_buffer = std::make_shared<SharedMemBuffer>(segment_, segment_id_, buffer_node,
                                static_cast<uint32_t>(validity_id));

                if (new_buffer)
                {
                    buffer_node->inc_processing_count(static_cast<uint32_t>(validity_id));
                }
                else
                {
                    throw std::runtime_error("alloc_buffer: out of memory");
                }

                // TODO(Adolfo) : Dynamic allocation. Use foonathan to convert it to static allocation
                allocated_buffers_.push_back(buffer_node);
            }
            catch (const std::exception&)
            {
                if (buffer_node)
                {
                    if (data)
                    {
                        release_buffer(buffer_node);
                    }

                    free_buffers_.push_back(buffer_node);
                }

                overflows_count_++;

                throw;
            }

            return new_buffer;
        }

        uint64_t mem_size()
        {
            return segment_->mem_size();
        }

    private:

        std::string segment_name_;

        std::unique_ptr<RobustExclusiveLock> segment_name_lock_;

        // TODO(Adolfo) : Dynamic allocations. Use foonathan to convert it to static allocation
        std::list<BufferNode*> free_buffers_;
        std::list<BufferNode*> allocated_buffers_;

        std::mutex alloc_mutex_;
        std::shared_ptr<SharedMemSegment> segment_;
        SharedMemSegment::Id segment_id_;
        uint64_t overflows_count_;

        uint32_t free_bytes_;

        void generate_segment_id_and_name(
                const std::string& domain_name)
        {
            static constexpr uint32_t MAX_COLLISIONS = 16;
            uint32_t collisions_count = MAX_COLLISIONS;

            do
            {
                // No collisions are most probable
                segment_id_.generate();
                segment_name_ = domain_name + "_" + segment_id_.to_string();

                try
                {
                    // Lock exclusive the segment name while the segment is alive
                    segment_name_lock_ =
                            std::unique_ptr<RobustExclusiveLock>(new RobustExclusiveLock(segment_name_ + "_el"));
                }
                catch (const std::exception&)
                {
                    collisions_count--;
                }
            } while (collisions_count > 0 && nullptr == segment_name_lock_);

            if (nullptr == segment_name_lock_)
            {
                throw std::runtime_error("error: couldn't generate segment_name");
            }
        }

        inline BufferNode* pop_free_node()
        {
            if (free_buffers_.empty())
            {
                throw std::runtime_error("BufferNodes overflow");
            }

            auto node = free_buffers_.back();
            free_buffers_.pop_back();
            return node;
        }

        void release_buffer(
                BufferNode* buffer_node)
        {
            segment_->get().deallocate(
                segment_->get_address_from_offset(buffer_node->data_offset));

            free_bytes_ += buffer_node->data_size;
        }

        /**
         * Recover unreferenced buffers and, in case of overflow, also recovers the oldest buffers not being
         * processed by any listener (until enough free bytes is gathered to solve the overflow).
         * @return true if at least required_data_size bytes are free after the recovery, false otherwise.
         */
        bool recover_buffers(
                uint32_t required_data_size)
        {
            auto it = allocated_buffers_.begin();
            while (it != allocated_buffers_.end())
            {
                // There is enough space to allocate the buffer
                if (free_bytes_ >= required_data_size)
                {
                    if ((*it)->is_not_referenced())
                    {
                        (*it)->invalidate_buffer();

                        release_buffer(*it);

                        free_buffers_.push_back(*it);
                        it = allocated_buffers_.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }
                else // No enough space, try to recover oldest not processing buffers
                {
                    // Buffer is not being processed by any listener
                    if ((*it)->invalidate_if_not_processing())
                    {
                        release_buffer(*it);

                        free_buffers_.push_back(*it);
                        it = allocated_buffers_.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }
            }

            // We may have enough memory but no free buffers
            it = allocated_buffers_.begin();
            while (free_buffers_.empty() && it != allocated_buffers_.end())
            {
                // Buffer is not beign processed by any listener
                if ((*it)->invalidate_if_not_processing())
                {
                    release_buffer(*it);

                    free_buffers_.push_back(*it);
                    it = allocated_buffers_.erase(it);
                }
                else
                {
                    it++;
                }
            }

            return free_bytes_ >= required_data_size;
        }

    }; // Segment

    class Port;

    /**
     * Listen to descriptors pushed to a port.
     * Provides an interface to wait and access to the data referenced by the descriptors
     */
    class Listener
    {
    public:

        Listener(
                SharedMemManager* shared_mem_manager,
                std::shared_ptr<SharedMemGlobal::Port> port)
            : global_port_(port)
            , shared_mem_manager_(shared_mem_manager)
            , is_closed_(false)
        {
            global_listener_ = global_port_->create_listener(&listener_index_);
        }

        ~Listener()
        {
            if (global_port_)
            {
                try
                {
                    global_port_->unregister_listener(&global_listener_, listener_index_);
                }
                catch (const std::exception& e)
                {
                    logWarning(RTPS_TRANSPORT_SHM, e.what());
                }
            }
        }

        Listener& operator = (
                Listener&& other)
        {
            global_listener_ = std::move(other.global_listener_);
            other.global_listener_.reset();
            global_port_ = other.global_port_;
            other.global_port_.reset();
            shared_mem_manager_ = other.shared_mem_manager_;
            is_closed_.exchange(other.is_closed_);

            return *this;
        }

        /**
         * Extract the first buffer enqueued in the port.
         * If the queue is empty, blocks until a buffer is pushed
         * to the port.
         * @return A shared_ptr to the buffer, this shared_ptr can be nullptr if the
         * wait was interrupted because errors or close operations.
         * @remark Multithread not supported.
         */
        std::shared_ptr<Buffer> pop()
        {
            std::shared_ptr<Buffer> buffer_ref;
            bool is_buffer_valid = false;

            try
            {
                while (!is_buffer_valid)
                {
                    bool was_cell_freed;

                    SharedMemGlobal::PortCell* head_cell = nullptr;
                    buffer_ref.reset();

                    while ( !is_closed_.load() && nullptr == (head_cell = global_listener_->head()))
                    {
                        // Wait until there's data to pop
                        global_port_->wait_pop(*global_listener_, is_closed_, listener_index_);
                    }

                    if (!head_cell)
                    {
                        return nullptr;
                    }

                    if (!global_port_->is_port_ok())
                    {
                        throw std::runtime_error("");
                    }

                    SharedMemGlobal::BufferDescriptor buffer_descriptor = head_cell->data();

                    auto segment = shared_mem_manager_->find_segment(buffer_descriptor.source_segment_id);
                    auto buffer_node =
                            static_cast<BufferNode*>(segment->get_address_from_offset(buffer_descriptor.
                                    buffer_node_offset));

                    // TODO(Adolfo) : Dynamic allocation. Use foonathan to convert it to static allocation
                    buffer_ref = std::make_shared<SharedMemBuffer>(segment, buffer_descriptor.source_segment_id,
                                    buffer_node, buffer_descriptor.validity_id);

                    // If the cell has been read by all listeners
                    global_port_->pop(*global_listener_, was_cell_freed);

                    if (buffer_ref)
                    {
                        global_port_->listener_processing_start(listener_index_, buffer_descriptor);
                        if (was_cell_freed)
                        {
                            // Atomically increase processing & decrease enqueued
                            is_buffer_valid = buffer_node->dec_enqueued_inc_processing_counts(
                                buffer_descriptor.validity_id);
                        }
                        else
                        {
                            is_buffer_valid = buffer_node->inc_processing_count(buffer_descriptor.validity_id);
                        }
                    }
                    else
                    {
                        if (was_cell_freed)
                        {
                            buffer_node->dec_enqueued_count(buffer_descriptor.validity_id);
                        }

                        throw std::runtime_error("pop() : out of memory");
                    }
                }
            }
            catch (const std::exception& e)
            {
                if (global_port_->is_port_ok())
                {
                    throw;
                }
                else
                {
                    logWarning(RTPS_TRANSPORT_SHM,
                            "SHM Listener on port " << global_port_->port_id() << " failure: " << e.what());

                    regenerate_port();
                }
            }

            return buffer_ref;
        }

        void stop_processing_buffer()
        {
            global_port_->listener_processing_stop(listener_index_);
        }

        void regenerate_port()
        {
            auto new_port = shared_mem_manager_->regenerate_port(global_port_, global_port_->open_mode());

            auto new_listener = new_port->create_listener();

            *this = std::move(*new_listener);
        }

        /**
         * Unblock a thread blocked in pop() call, not allowing pop() to block again.
         * @throw std::exception on error
         */
        void close()
        {
            // Just in case a thread is blocked in pop() function
            global_port_->close_listener(&is_closed_);
        }

    private:

        std::shared_ptr<SharedMemGlobal::Port> global_port_;

        std::unique_ptr<SharedMemGlobal::Listener> global_listener_;
        uint32_t listener_index_;

        SharedMemManager* shared_mem_manager_;

        std::atomic<bool> is_closed_;

    }; // Listener

    /**
     * Allows to push buffers and create listeners of a shared-memory port,
     */
    class Port
    {
    public:

        Port(
                SharedMemManager* shared_mem_manager,
                std::shared_ptr<SharedMemGlobal::Port> port,
                SharedMemGlobal::Port::OpenMode open_mode)
            : shared_mem_manager_(shared_mem_manager)
            , global_port_(port)
            , open_mode_(open_mode)
        {
        }

        Port& operator = (
                Port&& other)
        {
            shared_mem_manager_ = other.shared_mem_manager_;
            open_mode_ = other.open_mode_;
            global_port_ = other.global_port_;
            other.global_port_.reset();

            return *this;
        }

        /**
         * Try to enqueue a buffer in the port.
         * @returns false If the port's queue is full so buffer couldn't be enqueued.
         */
        bool try_push(
                const std::shared_ptr<Buffer>& buffer)
        {
            assert(std::dynamic_pointer_cast<SharedMemBuffer>(buffer));

            SharedMemBuffer* shared_mem_buffer = std::static_pointer_cast<SharedMemBuffer>(buffer).get();

            bool ret;
            bool are_listeners_active = false;
            auto validity_id = shared_mem_buffer->validity_id();

            shared_mem_buffer->inc_enqueued_count(validity_id);

            try
            {
                ret = global_port_->try_push( {shared_mem_buffer->segment_id(), shared_mem_buffer->node_offset(),
                                               validity_id},
                                &are_listeners_active);

                if (!are_listeners_active)
                {
                    shared_mem_buffer->dec_enqueued_count(validity_id);
                }
            }
            catch (std::exception& e)
            {
                shared_mem_buffer->dec_enqueued_count(validity_id);

                if (!global_port_->is_port_ok())
                {
                    logWarning(RTPS_TRANSPORT_SHM, "SHM Port " << global_port_->port_id() << " failure: "
                                                               << e.what());

                    regenerate_port();
                    ret = false;
                }
                else
                {
                    throw;
                }
            }

            return ret;
        }

        /**
         * @brief Unlock buffers being processed by the port if the port is frozen.
         *
         * If the port is zombie, finds all the buffers that were being processed by a listener
         * and decrements their processing count, so that they are not kept locked forever
         */
        void recover_blocked_processing()
        {
            SharedMemGlobal::BufferDescriptor buffer_descriptor;
            if (SharedMemGlobal::Port::is_zombie(global_port_->address_id(), global_port_->port_id(),
                    shared_mem_manager_->global_segment()->domain_name()))
            {
                while (global_port_->get_and_remove_blocked_processing(buffer_descriptor))
                {
                    auto segment = shared_mem_manager_->find_segment(buffer_descriptor.source_segment_id);
                    if (!segment)
                    {
                        // If the segment is gone, nothing to do
                        continue;
                    }
                    auto buffer_node =
                            static_cast<BufferNode*>(segment->get_address_from_offset(buffer_descriptor.
                                    buffer_node_offset));
                    buffer_node->dec_processing_count(buffer_descriptor.validity_id);
                }
            }
        }

        std::shared_ptr<Listener> create_listener()
        {
            return std::make_shared<Listener>(shared_mem_manager_, global_port_);
        }

    private:

        void regenerate_port()
        {
            recover_blocked_processing();
            auto new_port = shared_mem_manager_->regenerate_port(global_port_, open_mode_);

            *this = std::move(*new_port);
        }

        SharedMemManager* shared_mem_manager_;

        std::shared_ptr<SharedMemGlobal::Port> global_port_;

        SharedMemGlobal::Port::OpenMode open_mode_;

    }; // Port

    /**
     * Creates a shared-memory segment
     * @param size size of the segment
     * @param max_buffers maximum, at a time, allocated buffers
     * @return A shared_ptr to the segment
     */
    std::shared_ptr<Segment> create_segment(
            uint32_t size,
            uint32_t max_allocations)
    {
        return std::make_shared<Segment>(size + segment_allocation_extra_size(max_allocations), size, max_allocations,
                       global_segment_.domain_name());
    }

    /**
     * Computes the segment's extra size needed to store allocator internal structures
     * @param in max_allocations The maximum buffer allocations supported.
     * @return the extra size in bytes.
     */
    uint32_t segment_allocation_extra_size(
            uint32_t max_allocations) const
    {
        // Every buffer allocation of 'n-bytes', consumes an extra 'per_allocation_extra_size_' bytes.
        // This is due to the allocator internal structures (also residing in the shared-memory segment)
        // used to manage the allocation algorithm.
        // So with an estimation of 'max_allocations' user buffers, the total segment extra size is computed.
        uint32_t allocation_extra_size = (max_allocations * sizeof(BufferNode)) + per_allocation_extra_size_ +
                max_allocations * per_allocation_extra_size_;

        return allocation_extra_size;
    }

    /**
     * Checks if a port can be open for writing.
     * @param port_id The identifier of the port to check.
     * @return whether the port can be written or not.
     */
    bool can_write_to_port(
            uint32_t address_id,
            uint32_t port_id) const
    {
        return global_segment_.can_write_to_port(address_id, port_id);
    }

    std::shared_ptr<Port> open_port(
            uint32_t address_id,
            uint32_t port_id,
            uint32_t max_descriptors,
            uint32_t healthy_check_timeout_ms,
            SharedMemGlobal::Port::OpenMode open_mode = SharedMemGlobal::Port::OpenMode::ReadShared)
    {
        return std::make_shared<Port>(this,
                       global_segment_.open_port(address_id, port_id, max_descriptors,
                       healthy_check_timeout_ms, open_mode),
                       open_mode);
    }

    /**
     * Remove a port from the system.
     */
    void remove_port(
            uint32_t address_id,
            uint32_t port_id)
    {
        global_segment_.remove_port(address_id, port_id);
    }

    /**
     * @return Pointer to the underlying global segment. The pointer is only valid
     * while this SharedMemManager is alive.
     */
    SharedMemGlobal* global_segment()
    {
        return &global_segment_;
    }

    /**
     * @return The total mem size, in bytes, used by remote mapped segments.
     */
    uint64_t segments_mem()
    {
        std::lock_guard<std::mutex> lock(ids_segments_mutex_);

        return segments_mem_;
    }

private:

    std::shared_ptr<Port> regenerate_port(
            std::shared_ptr<SharedMemGlobal::Port> port,
            SharedMemGlobal::Port::OpenMode open_mode)
    {
        return std::make_shared<Port>(this, global_segment_.regenerate_port(port, open_mode), open_mode);
    }

    /**
     * Controls life-cycle of a remote segment
     */
    class SegmentWrapper
    {
    public:

        SegmentWrapper()
        {
        }

        SegmentWrapper(
                std::weak_ptr<SharedMemManager> shared_mem_manager,
                std::shared_ptr<SharedMemSegment> segment_,
                SharedMemSegment::Id segment_id,
                const std::string& segment_name)
            : shared_mem_manager_(shared_mem_manager)
            , segment_(segment_)
            , segment_id_(segment_id)
            , segment_name_(segment_name)
        {
            lock_file_name_ = segment_name + "_el";
            update_alive_time(std::chrono::steady_clock::now());
        }

        std::shared_ptr<SharedMemSegment> segment()
        {
            return segment_;
        }

        void update_alive_time(
                const std::chrono::steady_clock::time_point& time)
        {
            last_alive_check_time_.store(time.time_since_epoch().count());
        }

        /**
         * Singleton task, for SharedMemWatchdog, that periodically checks opened segments
         * to garbage collect those closed by the origin
         */
        class WatchTask : public SharedMemWatchdog::Task
        {
        public:

            static std::shared_ptr<WatchTask>& get()
            {
                static std::shared_ptr<WatchTask> watch_task_instance(new WatchTask());
                return watch_task_instance;
            }

            void add_segment(
                    std::shared_ptr<SegmentWrapper> segment)
            {
                // Add added segments to the watched set
                std::lock_guard<std::mutex> lock(to_add_remove_mutex_);

                to_add_.push_back(segment);
            }

            void remove_segment(
                    std::shared_ptr<SegmentWrapper> segment)
            {
                // Add added segments to the watched set
                std::lock_guard<std::mutex> lock(to_add_remove_mutex_);

                to_remove_.push_back(segment);
            }

            virtual ~WatchTask()
            {
                shared_mem_watchdog_->remove_task(this);
            }

        private:

            std::unordered_map<std::shared_ptr<SegmentWrapper>, uint32_t> watched_segments_;
            std::unordered_map<std::shared_ptr<SegmentWrapper>, uint32_t>::iterator watched_it_;

            std::mutex to_add_remove_mutex_;
            std::vector<std::shared_ptr<SegmentWrapper>> to_add_;
            std::vector<std::shared_ptr<SegmentWrapper>> to_remove_;

            // Keep a reference to the SharedMemWatchdog so that it is not destroyed until this instance is destroyed
            std::shared_ptr<SharedMemWatchdog> shared_mem_watchdog_;

            WatchTask()
                : watched_it_(watched_segments_.end())
                , shared_mem_watchdog_(SharedMemWatchdog::get())
            {
                shared_mem_watchdog_->add_task(this);
            }

            void update_watched_segments()
            {
                // Add / remove segments to the watched map
                std::lock_guard<std::mutex> lock(to_add_remove_mutex_);

                for (auto& segment : to_add_)
                {
                    auto segment_it = watched_segments_.find(segment);
                    if (segment_it != watched_segments_.end())
                    {
                        // The segment already exists, just increase the references
                        (*segment_it).second++;
                    }
                    else // New segment
                    {
                        watched_segments_.insert({segment, 1});
                    }
                }

                to_add_.clear();

                for (auto& segment : to_remove_)
                {
                    auto segment_it = watched_segments_.find(segment);
                    if (segment_it != watched_segments_.end())
                    {
                        (*segment_it).second--;

                        if ((*segment_it).second == 0)
                        {
                            watched_segments_.erase(segment_it);
                        }
                    }
                }

                to_remove_.clear();
            }

            void run()
            {
                constexpr uint32_t MAX_CHECKS_PER_BATCH {100};
                constexpr std::chrono::milliseconds PER_BATCH_SLEEP_TIME {10};

                auto now = std::chrono::steady_clock::now();

                // Segments check was completed in the last run
                if (watched_it_ == watched_segments_.end())
                {
                    // Add / remove requested segments
                    update_watched_segments();
                    watched_it_ = watched_segments_.begin();
                }

                auto now_t = std::chrono::steady_clock::now();
                // Maximum time for checking half the watchdog period
                auto limit_t = now_t + SharedMemWatchdog::period() / 2;
                uint32_t batch_count = 0;

                while (watched_it_ != watched_segments_.end() && now_t < limit_t)
                {
                    auto& segment = (*watched_it_).first;
                    // The segment has not been check for much time...
                    if (segment->alive_check_timeout(now))
                    {
                        if (!(*watched_it_).first->check_alive())
                        {
                            watched_it_ = watched_segments_.erase(watched_it_);
                        }
                        else
                        {
                            watched_it_++;
                        }
                    }
                    else
                    {
                        watched_it_++;
                    }

                    // Every batch a sleep is performed to avoid high resources consumption
                    if (++batch_count ==  MAX_CHECKS_PER_BATCH)
                    {
                        batch_count = 0;
                        std::this_thread::sleep_for(PER_BATCH_SLEEP_TIME);
                        now_t = std::chrono::steady_clock::now();
                    }
                }
            }

        };

    private:

        std::weak_ptr<SharedMemManager> shared_mem_manager_;
        std::shared_ptr<SharedMemSegment> segment_;
        SharedMemSegment::Id segment_id_;
        std::string segment_name_;
        std::string lock_file_name_;
        std::atomic<std::chrono::steady_clock::time_point::rep> last_alive_check_time_;

        static constexpr uint32_t ALIVE_CHECK_TIMEOUT_SECS {5};

        bool check_alive()
        {
            if (!RobustExclusiveLock::is_locked(lock_file_name_))
            {
                // The segment is not locked so the origin is no longer active
                close_and_remove();

                return false;
            }

            update_alive_time(std::chrono::steady_clock::now());

            return true;
        }

        bool alive_check_timeout(
                const std::chrono::steady_clock::time_point& now) const
        {
            std::chrono::steady_clock::time_point last_check_time(
                std::chrono::nanoseconds(last_alive_check_time_.load()));

            return std::chrono::duration_cast<std::chrono::seconds>(now - last_check_time).count()
                   >= ALIVE_CHECK_TIMEOUT_SECS;
        }

        void close_and_remove()
        {
            // Remove from the namespace (just in case the origin didn't do it)
            SharedMemSegment::remove(segment_name_.c_str());

            if (auto shared_mem_manager = shared_mem_manager_.lock())
            {
                shared_mem_manager->release_segment(segment_id_);
            }
        }

    };

    uint32_t per_allocation_extra_size_;

    std::unordered_map<SharedMemSegment::Id::type, std::shared_ptr<SegmentWrapper>,
            std::hash<SharedMemSegment::Id::type>> ids_segments_;
    std::mutex ids_segments_mutex_;
    uint64_t segments_mem_;

    SharedMemGlobal global_segment_;

    // Keep a reference to the WatchTask so that it is not destroyed until all Manger instances are destroyed
    std::shared_ptr<SegmentWrapper::WatchTask> watch_task_;

    std::shared_ptr<SharedMemSegment> find_segment(
            SharedMemSegment::Id id)
    {
        std::shared_ptr<SharedMemSegment> segment;
        std::lock_guard<std::mutex> lock(ids_segments_mutex_);

        auto segment_it = ids_segments_.find(id.get());

        if (segment_it != ids_segments_.end())
        {
            segment = (*segment_it).second->segment();
        }
        else // Is a new segment
        {
            auto segment_name = global_segment_.domain_name() + "_" + id.to_string();
            segment = std::make_shared<SharedMemSegment>(boost::interprocess::open_only, segment_name);
            auto segment_wrapper = std::make_shared<SegmentWrapper>(shared_from_this(), segment, id, segment_name);

            ids_segments_[id.get()] = segment_wrapper;
            segments_mem_ += segment->mem_size();

            SegmentWrapper::WatchTask::get()->add_segment(segment_wrapper);
        }

        return segment;
    }

    void release_segment(
            SharedMemSegment::Id id)
    {
        std::lock_guard<std::mutex> lock(ids_segments_mutex_);

        auto segment_it = ids_segments_.find(id.get());

        if (segment_it != ids_segments_.end())
        {
            segments_mem_ -= (*segment_it).second->segment()->mem_size();
            ids_segments_.erase(segment_it);
        }
    }

    /**
     * Remove all the opened remote segments from the watchdog.
     * This function should be called before destroying the SharedMemManager.
     * Because ids_segments_ is about to be clear, all the segments will be released.
     */
    void remove_segments_from_watch()
    {
        std::lock_guard<std::mutex> lock(ids_segments_mutex_);

        for (auto segment : ids_segments_)
        {
            SegmentWrapper::WatchTask::get()->remove_segment(segment.second);
        }
    }

};

} // namespace rtps
} // namespace fastdds
} // namespace eprosima

#endif // _FASTDDS_SHAREDMEM_MANAGER_H_