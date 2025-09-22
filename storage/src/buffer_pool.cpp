#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <atomic>
#include "third_party/ConcurrentHashMap.h"
#include "storage/disk.hpp"


struct PageId {
    std::string file_name;
    uint64_t page_id;
    
    bool operator==(const PageId& other) const {
        return file_name == other.file_name && page_id == other.page_id;
    }
    
    bool operator<(const PageId& other) const {
        if (file_name != other.file_name) {
            return file_name < other.file_name;
        }
        return page_id < other.page_id;
    }
};


struct PageIdHash {
    std::size_t operator()(const PageId& pid) const {
        std::size_t h1 = std::hash<std::string>{}(pid.file_name);
        std::size_t h2 = std::hash<uint64_t>{}(pid.page_id);
        return h1 ^ (h2 << 1);
    }
};


struct Frame {
    std::vector<uint8_t> data;
    PageId page_id;
    std::shared_mutex page_mutex;
    std::atomic<bool> is_dirty{false};
    std::atomic<int> pin_count{0};
    
    Frame() = default;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
};


class BufferPoolManager {
private:
    static constexpr size_t DEFAULT_BUFFER_POOL_SIZE = 1000;
    std::vector<std::unique_ptr<Frame>> frames_;
    size_t pool_size_;
    ConcurrentHashMap<PageId, size_t, PageIdHash> page_table_;
    std::vector<size_t> free_frames_;
    mutable std::mutex free_frames_mutex_;
    std::shared_mutex buffer_pool_mutex_;
    
    BufferPoolManager(size_t pool_size = DEFAULT_BUFFER_POOL_SIZE) 
        : pool_size_(pool_size) {
        frames_.reserve(pool_size_);
        free_frames_.reserve(pool_size_);
        for (size_t i = 0; i < pool_size_; ++i) {
            frames_.emplace_back(std::make_unique<Frame>());
            free_frames_.push_back(i);
        }
    }

    std::optional<size_t> get_free_frame() {
        std::lock_guard<std::mutex> lock(free_frames_mutex_);
        if (free_frames_.empty()) {
            return std::nullopt;
        }
        size_t frame_idx = free_frames_.back();
        free_frames_.pop_back();
        return frame_idx;
    }
    

    void return_free_frame(size_t frame_idx) {
        std::lock_guard<std::mutex> lock(free_frames_mutex_);
        free_frames_.push_back(frame_idx);
    }
    

    void load_page_to_frame(const PageId& page_id, size_t frame_idx) {
        auto& frame = frames_[frame_idx];
        frame->data = read_page(page_id.file_name, page_id.page_id);
        frame->page_id = page_id;
        frame->is_dirty.store(false);
        frame->pin_count.store(1);
    }
    
    void flush_page(size_t frame_idx) {
        auto& frame = frames_[frame_idx];
        if (frame->is_dirty.load()) {
            write_page(frame->page_id.file_name, frame->page_id.page_id, frame->data);
            frame->is_dirty.store(false);
        }
    }

public:
    // Get singleton instance
    static BufferPoolManager& get_instance() {
        static BufferPoolManager instance;
        return instance;
    }
    
    BufferPoolManager(const BufferPoolManager&) = delete;
    BufferPoolManager& operator=(const BufferPoolManager&) = delete;
    

    std::pair<std::vector<uint8_t>*, std::shared_lock<std::shared_mutex>> fetch_page_read(
        const std::string& file_name, uint64_t page_id) {
        return fetch_page_internal(file_name, page_id, false);
    }
    
    std::pair<std::vector<uint8_t>*, std::unique_lock<std::shared_mutex>> fetch_page_write(
        const std::string& file_name, uint64_t page_id) {
        auto result = fetch_page_internal(file_name, page_id, true);
        return {result.first, std::unique_lock<std::shared_mutex>(
            frames_[get_frame_index(PageId{file_name, page_id})]->page_mutex, std::adopt_lock)};
    }
    
    template<typename LockType>
    std::pair<std::vector<uint8_t>*, LockType> fetch_page(
        const std::string& file_name, uint64_t page_id, bool is_write) {
        if (is_write) {
            auto result = fetch_page_write(file_name, page_id);
            return {result.first, std::move(result.second)};
        } else {
            auto result = fetch_page_read(file_name, page_id);
            return {result.first, std::move(result.second)};
        }
    }
    
    bool unpin_page(const std::string& file_name, uint64_t page_id, bool is_dirty = false) {
        PageId pid{file_name, page_id};
        
        auto frame_idx_opt = page_table_.get(pid);
        if (!frame_idx_opt) {
            return false;
        }
        
        size_t frame_idx = *frame_idx_opt;
        auto& frame = frames_[frame_idx];
        
        if (is_dirty) {
            frame->is_dirty.store(true);
        }
        
        int old_pin_count = frame->pin_count.fetch_sub(1);
        return old_pin_count > 0;
    }
    
    bool flush_page(const std::string& file_name, uint64_t page_id) {
        PageId pid{file_name, page_id};
        
        auto frame_idx_opt = page_table_.get(pid);
        if (!frame_idx_opt) {
            return false;
        }
        
        size_t frame_idx = *frame_idx_opt;
        auto& frame = frames_[frame_idx];
        
        std::unique_lock<std::shared_mutex> lock(frame->page_mutex);
        flush_page(frame_idx);
        return true;
    }
    

    void flush_all_pages() {
        std::shared_lock<std::shared_mutex> pool_lock(buffer_pool_mutex_);
        
        for (size_t i = 0; i < pool_size_; ++i) {
            auto& frame = frames_[i];
            if (frame->pin_count.load() > 0) {
                std::unique_lock<std::shared_mutex> page_lock(frame->page_mutex);
                flush_page(i);
            }
        }
    }
    

    struct PoolStats {
        size_t total_frames;
        size_t free_frames;
        size_t pinned_frames;
        size_t dirty_frames;
    };
    
    PoolStats get_stats() const {
        PoolStats stats;
        stats.total_frames = pool_size_;
        
        {
            std::lock_guard<std::mutex> lock(free_frames_mutex_);
            stats.free_frames = free_frames_.size();
        }
        
        stats.pinned_frames = 0;
        stats.dirty_frames = 0;
        
        for (const auto& frame : frames_) {
            if (frame->pin_count.load() > 0) {
                stats.pinned_frames++;
            }
            if (frame->is_dirty.load()) {
                stats.dirty_frames++;
            }
        }
        
        return stats;
    }

private:

    std::pair<std::vector<uint8_t>*, std::shared_lock<std::shared_mutex>> fetch_page_internal(
        const std::string& file_name, uint64_t page_id, bool is_write) {
        
        PageId pid{file_name, page_id};
        auto frame_idx_opt = page_table_.get(pid);
        
        if (frame_idx_opt) {
            size_t frame_idx = *frame_idx_opt;
            auto& frame = frames_[frame_idx];
            frame->pin_count.fetch_add(1);
            if (is_write) {
                frame->page_mutex.lock();
            } else {
                frame->page_mutex.lock_shared();
            }
            return {&frame->data, std::shared_lock<std::shared_mutex>(frame->page_mutex, std::adopt_lock)};
        }
        
        std::unique_lock<std::shared_mutex> pool_lock(buffer_pool_mutex_);
        frame_idx_opt = page_table_.get(pid);
        if (frame_idx_opt) {
            pool_lock.unlock();
            size_t frame_idx = *frame_idx_opt;
            auto& frame = frames_[frame_idx];
            frame->pin_count.fetch_add(1);
            
            if (is_write) {
                frame->page_mutex.lock();
            } else {
                frame->page_mutex.lock_shared();
            }
            
            return {&frame->data, std::shared_lock<std::shared_mutex>(frame->page_mutex, std::adopt_lock)};
        }
        
        auto free_frame_idx = get_free_frame();
        if (!free_frame_idx) {
            throw std::runtime_error("No free frames available in buffer pool");
        }
        
        size_t frame_idx = *free_frame_idx;
        load_page_to_frame(pid, frame_idx);
        page_table_.insert(pid, frame_idx);
        auto& frame = frames_[frame_idx];
        if (is_write) {
            frame->page_mutex.lock();
        } else {
            frame->page_mutex.lock_shared();
        }
        return {&frame->data, std::shared_lock<std::shared_mutex>(frame->page_mutex, std::adopt_lock)};
    }
    

    size_t get_frame_index(const PageId& pid) const {
        auto frame_idx_opt = page_table_.get(pid);
        if (!frame_idx_opt) {
            throw std::runtime_error("Page not found in buffer pool");
        }
        return *frame_idx_opt;
    }
};


inline std::pair<std::vector<uint8_t>*, std::shared_lock<std::shared_mutex>> 
fetch_page_read(const std::string& file_name, uint64_t page_id) {
    return BufferPoolManager::get_instance().fetch_page_read(file_name, page_id);
}

inline std::pair<std::vector<uint8_t>*, std::unique_lock<std::shared_mutex>> 
fetch_page_write(const std::string& file_name, uint64_t page_id) {
    return BufferPoolManager::get_instance().fetch_page_write(file_name, page_id);
}

inline bool unpin_page(const std::string& file_name, uint64_t page_id, bool is_dirty = false) {
    return BufferPoolManager::get_instance().unpin_page(file_name, page_id, is_dirty);
}

inline bool flush_page(const std::string& file_name, uint64_t page_id) {
    return BufferPoolManager::get_instance().flush_page(file_name, page_id);
}

// Example usage:
/*
int main() {
    // Fetch a page for reading
    {
        auto [page_data, read_lock] = fetch_page_read("test.db", 1);
        // Use page_data for reading
        // Lock is automatically released when read_lock goes out of scope
    }
    unpin_page("test.db", 1);
    
    // Fetch a page for writing
    {
        auto [page_data, write_lock] = fetch_page_write("test.db", 2);
        // Modify page_data
        // Lock is automatically released when write_lock goes out of scope
    }
    unpin_page("test.db", 2, true); // Mark as dirty
    
    // Flush specific page
    flush_page("test.db", 2);
    
    // Get buffer pool statistics
    auto stats = BufferPoolManager::get_instance().get_stats();
    
    return 0;
}
*/