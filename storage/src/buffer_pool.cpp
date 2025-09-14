#pragma once

#include <variant>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include "third_party/ConcurrentHashMap.h"

struct Page {
    uint64_t page_id;
    bool is_dirty;
    int pin_count;
    std::vector<uint8_t> data;
    Page(uint64_t page_id, size_t page_size);
};

class Replacer {
public:
    virtual ~Replacer() = default;
    virtual void RecordAccess(Page* frame) = 0;
    virtual std::optional<Page*> Victim() = 0;
    virtual void Pin(Page* frame) = 0;
    virtual void Unpin(Page* frame) = 0;
};


class ClockReplacer : public Replacer {
public:
    ClockReplacer(size_t pool_size);
    ~ClockReplacer() override;

    void RecordAccess(Page* frame) override;
    std::optional<Page*> Victim() override;
    void Pin(Page* frame) override;
    void Unpin(Page* frame) override;

private:
    struct ClockFrame {
        Page* page;
        bool reference_bit;
    };

    std::vector<ClockFrame> clock_frames_;
    size_t clock_hand_;
    size_t pool_size_;
    std::unordered_map<Page*, size_t> page_to_index_;
};

class BufferPoolManager;

class PageHandle {
public:
    PageHandle(Page* page, std::mutex& page_mutex, BufferPoolManager* bpm, bool is_write);
    PageHandle(PageHandle&& other) noexcept;
    PageHandle& operator=(PageHandle&& other) noexcept;
    PageHandle(const PageHandle&) = delete;
    PageHandle& operator=(const PageHandle&) = delete;
    ~PageHandle();
    Page* operator->() const { return page_; }
    Page& operator*() const { return *page_; }
    bool IsValid() const { return page_ != nullptr; }
private:
    Page* page_ = nullptr;
     std::variant<std::shared_lock<std::shared_mutex>, std::unique_lock<std::shared_mutex>> lock_;
    BufferPoolManager* bpm_ = nullptr;
    bool is_write_ = false;
    void Unpin();
};


class BufferPoolManager {
public:
    BufferPoolManager();
    ~BufferPoolManager();
    PageHandle FetchPage(uint64_t page_id, bool isWrite);
    bool UnpinPage(uint64_t page_id, bool is_dirty);

private:
    std::unique_ptr<Replacer> replacer_;
    ConcurrentHashMap<uint64_t, Page*> page_table_;
    ConcurrentHashMap<uint64_t, std::mutex> page_lock_table_;
    bool FlushPage(uint64_t page_id);
};


class BufferPoolSingleton {
public:
    static BufferPoolManager& GetInstance() {
        static BufferPoolManager instance;
        return instance;
    }

    BufferPoolSingleton() = delete;
    BufferPoolSingleton(const BufferPoolSingleton&) = delete;
    BufferPoolSingleton& operator=(const BufferPoolSingleton&) = delete;
};