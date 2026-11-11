// heap.hpp -- P1's min-heap + lazy deletion, connected to P3's EventSet interface.
// Algorithms are P1's (from minheap.cpp). Changes made only to connect it (marked "P3 change"):
//   * removed main(), "using namespace std" and <bits/stdc++.h> (that header does not exist on Mac/clang)
//   * event struct got two extra fields (type, packet) so the simulator gets its full event back
//   * next_live_event() returns the whole event instead of only its id
//   * cancel_event() returns false if the event is already cancelled
//   * added live_size() and a purge counter
#pragma once
#include <vector>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include "event_set.hpp"

namespace heapimpl {

struct event {
    long long timestamp;
    long long sequence;
    long long event_id;
    int type;      // P3 change
    int packet;    // P3 change
};

inline bool earlier(const event& a, const event& b) {
    if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
    return a.sequence < b.sequence;
}

class minheap {
    std::vector<event> heap;
    long long tot_events = 0;

    void siftup(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (earlier(heap[i], heap[p])) { std::swap(heap[p], heap[i]); i = p; }
            else break;
        }
    }
    void siftdown(int i) {
        int s = (int)heap.size();
        while (true) {
            int prior = i;
            if (2 * i + 1 < s && earlier(heap[2 * i + 1], heap[prior])) prior = 2 * i + 1;
            if (2 * i + 2 < s && earlier(heap[2 * i + 2], heap[prior])) prior = 2 * i + 2;
            if (prior == i) break;
            std::swap(heap[prior], heap[i]);
            i = prior;
        }
    }
public:
    void insert(const event& a) { heap.push_back(a); ++tot_events; siftup((int)heap.size() - 1); }
    event deleteMin() {
        if (heap.empty()) throw std::runtime_error("heap is empty");
        event result = heap[0];
        if (heap.size() == 1) { heap.pop_back(); --tot_events; return result; }
        heap[0] = heap.back();
        heap.pop_back();
        --tot_events;
        siftdown(0);
        return result;
    }
    const event& peek() const { if (heap.empty()) throw std::runtime_error("heap is empty"); return heap[0]; }
    bool empty() const { return heap.empty(); }
    long long size() const { return tot_events; }
    std::vector<event> get_events() const { return heap; }
    void build_heap(const std::vector<event>& a) {
        heap = a;
        tot_events = (long long)heap.size();
        for (int i = (int)heap.size() / 2 - 1; i >= 0; i--) siftdown(i);
    }
};

class id_set {
    std::vector<std::vector<long long>> buckets = std::vector<std::vector<long long>>(100000);
    long long item_count = 0;
public:
    long long size() { return item_count; }
    bool search(const long long& b) {
        for (long long num : buckets[b % 100000]) if (num == b) return true;
        return false;
    }
    void insert(const long long& a) {
        if (!search(a)) { buckets[a % 100000].push_back(a); ++item_count; }
    }
    void erase(const long long& c) {
        int s = (int)buckets[c % 100000].size();
        for (int i = 0; i < s; i++) {
            if (buckets[c % 100000][i] == c) {
                std::swap(buckets[c % 100000][i], buckets[c % 100000][s - 1]);
                buckets[c % 100000].pop_back();
                --item_count;
                return;
            }
        }
    }
    void clear() { for (auto& bucket : buckets) bucket.clear(); item_count = 0; }
};

class scheduler {
    minheap heap;
    id_set cancelled;
    id_set in_heap;   // IDs currently in the heap

    void purge() {
        ++purges;
        std::vector<event> old_events = heap.get_events();
        std::vector<event> live_events;
        in_heap.clear();
        for (const event& e : old_events) {
            if (!cancelled.search(e.event_id)) { live_events.push_back(e); in_heap.insert(e.event_id); }
        }
        heap.build_heap(live_events);
        cancelled.clear();
    }
public:
    long long purges = 0;   // P3 change

    bool add_event(const event& a) {
        if (in_heap.search(a.event_id)) return false;
        heap.insert(a);
        in_heap.insert(a.event_id);
        return true;
    }
    bool cancel_event(const long long& a) {
        if (!in_heap.search(a) || cancelled.search(a)) return false;   // P3 change: 2nd test
        cancelled.insert(a);
        long long live_events = heap.size() - cancelled.size();
        if (cancelled.size() > live_events) purge();
        return true;
    }
    std::optional<event> next_live_event() {          // P3 change: returns the event
        while (!heap.empty()) {
            event result = heap.deleteMin();
            in_heap.erase(result.event_id);
            if (cancelled.search(result.event_id)) { cancelled.erase(result.event_id); continue; }
            return result;
        }
        return std::nullopt;
    }
    long long live_size() { return heap.size() - cancelled.size(); }   // P3 change
    long long physical_size() { return heap.size(); }                  // P3 change
};

} // namespace heapimpl

// The plug that connects P1's scheduler to the simulator.
class HeapEventSet : public EventSet {
    heapimpl::scheduler s;
public:
    void insert(const Event& e) override {
        s.add_event(heapimpl::event{e.time, (long long)e.seq, (long long)e.seq, e.type, e.packet}); // id = seq (unique)
    }
    bool deleteMin(Event& out) override {
        auto r = s.next_live_event();
        if (!r) return false;
        out.time = r->timestamp; out.seq = (std::uint64_t)r->sequence; out.type = r->type; out.packet = r->packet;
        return true;
    }
    bool cancel(const Event& e) override { return s.cancel_event((long long)e.seq); }
    std::size_t size() const override { return (std::size_t)const_cast<heapimpl::scheduler&>(s).live_size(); }
    const char* name() const override { return "heap"; }
    long long purges() const { return s.purges; }
    long long physicalSize() { return s.physical_size(); }
};
