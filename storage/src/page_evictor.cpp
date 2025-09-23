#ifndef CLOCK_EVICTOR_H
#define CLOCK_EVICTOR_H

#include <unordered_map>
#include <functional>

typedef int page_id_t;
typedef std::function<void(page_id_t)> evict_page_callback_t;

struct ClockFrame {
    page_id_t page_id;
    int reference_bit;
    int valid;
};

class ClockEvictor {
private:
    struct ClockFrame* frames;
    std::unordered_map<page_id_t, int> page_to_frame;
    int capacity;
    int clock_hand;
    evict_page_callback_t flush_page;
    int find_empty_frame();
    int find_victim_frame();

public:
    ClockEvictor(int capacity, evict_page_callback_t flush_callback);
    ~ClockEvictor();
    

    void update_access(page_id_t page_id);
    page_id_t evict();
    
    int add_page(page_id_t page_id);
    void remove_page(page_id_t page_id);
    
    int get_frame_count() const;
    int is_full() const;

};


ClockEvictor::ClockEvictor(int capacity, evict_page_callback_t flush_callback) 
    : capacity(capacity), clock_hand(0), flush_page(flush_callback) {
    frames = new ClockFrame[capacity];
    
    for (int i = 0; i < capacity; i++) {
        frames[i].page_id = -1;
        frames[i].reference_bit = 0;
        frames[i].valid = 0;
    }
}

ClockEvictor::~ClockEvictor() {
    delete[] frames;
}

void ClockEvictor::update_access(page_id_t page_id) {
    auto it = page_to_frame.find(page_id);
    if (it != page_to_frame.end()) {
        int frame_idx = it->second;
        frames[frame_idx].reference_bit = 1;
    }
}

page_id_t ClockEvictor::evict() {
    if (page_to_frame.empty()) {
        return -1;
    }
    int victim_frame = find_victim_frame();
    if (victim_frame == -1) {
        return -1;
    }
    page_id_t evicted_page = frames[victim_frame].page_id;
    if (flush_page) {
        flush_page(evicted_page);
    }
    page_to_frame.erase(evicted_page);
    frames[victim_frame].page_id = -1;
    frames[victim_frame].reference_bit = 0;
    frames[victim_frame].valid = 0;
    return evicted_page;
}

int ClockEvictor::add_page(page_id_t page_id) {
    if (page_to_frame.find(page_id) != page_to_frame.end()) {
        return 0;
    }
    int frame_idx = find_empty_frame();
    if (frame_idx == -1) {
        return -1;
    }
    frames[frame_idx].page_id = page_id;
    frames[frame_idx].reference_bit = 1;
    frames[frame_idx].valid = 1;
    page_to_frame[page_id] = frame_idx;
    return 1;
}

void ClockEvictor::remove_page(page_id_t page_id) {
    auto it = page_to_frame.find(page_id);
    if (it != page_to_frame.end()) {
        int frame_idx = it->second;
        frames[frame_idx].page_id = -1;
        frames[frame_idx].reference_bit = 0;
        frames[frame_idx].valid = 0;
        page_to_frame.erase(it);
    }
}

int ClockEvictor::get_frame_count() const {
    return page_to_frame.size();
}

int ClockEvictor::is_full() const {
    return page_to_frame.size() >= capacity;
}

int ClockEvictor::find_empty_frame() {
    for (int i = 0; i < capacity; i++) {
        if (!frames[i].valid) {
            return i;
        }
    }
    return -1;
}

int ClockEvictor::find_victim_frame() {
    int start_hand = clock_hand;
    while (1) {
        if (frames[clock_hand].valid) {
            if (frames[clock_hand].reference_bit == 0) {
                int victim = clock_hand;
                clock_hand = (clock_hand + 1) % capacity;
                return victim;
            } else {
                frames[clock_hand].reference_bit = 0;
            }
        }
        clock_hand = (clock_hand + 1) % capacity;
        if (clock_hand == start_hand) {
            break;
        }
    }
    return -1;
}

#endif