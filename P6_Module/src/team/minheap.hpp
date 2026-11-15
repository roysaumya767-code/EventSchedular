#pragma once
#include <algorithm>
#include <vector>
#include <optional>
#include <stdexcept>
namespace team_heap {
using std::vector; using std::swap; using std::optional; using std::nullopt; using std::runtime_error;
struct event {
    double timestamp;
    long long sequence;
    long long event_id;
};


bool earlier(const event& a, const event& b) {
    if (a.timestamp != b.timestamp) {
        return a.timestamp < b.timestamp;
    }
    return a.sequence < b.sequence;
}


class minheap {
private:
    vector<event> heap;
    long long int tot_events = 0;

    void siftup(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;

            if (earlier(heap[i], heap[p])) {
                swap(heap[p], heap[i]);
                i = p;
            }
            else {
                break;
            }
        }
    }

    void siftdown(int i) {
        int s = heap.size();

        while (true) {
            int prior = i;

            if (2 * i + 1 < s &&
                earlier(heap[2 * i + 1], heap[prior])) {
                prior = 2 * i + 1;
            }

            if (2 * i + 2 < s &&
                earlier(heap[2 * i + 2], heap[prior])) {
                prior = 2 * i + 2;
            }

            if (prior == i) {
                break;
            }

            swap(heap[prior], heap[i]);
            i = prior;
        }
    }

public:

    void insert(const event& a) {
        heap.push_back(a);
        ++tot_events;
        siftup(heap.size() - 1);
    }

    event deleteMin() {
        if (heap.empty()) {
            throw runtime_error("heap is empty");
        }

        event result = heap[0];

        if (heap.size() == 1) {
            heap.pop_back();
            --tot_events;
            return result;
        }

        heap[0] = heap.back();
        heap.pop_back();

        --tot_events;

        siftdown(0);

        return result;
    }

    const event& peek() const {
        if (heap.empty()) {
            throw runtime_error("heap is empty");
        }

        return heap[0];
    }

    bool empty() const {
        return heap.empty();
    }

    long long int size() const {
        return tot_events;
    }

    const vector<event>& get_events() const {
        return heap;
    }

    size_t allocated_bytes() const { return heap.capacity()*sizeof(event); }

    void build_heap(const vector<event>& a) {
        heap = a;
        tot_events = heap.size();

        for (int i = heap.size() / 2 - 1; i >= 0; i--) {
            siftdown(i);
        }
    }
};


class id_set {
private:
    vector<vector<long long int>> buckets =
        vector<vector<long long int>>(100000);

    long long int item_count = 0;

public:

    long long int size() const {
        return item_count;
    }

    bool search(const long long int& b) const {
        for (long long int num : buckets[b % 100000]) {
            if (num == b) {
                return true;
            }
        }

        return false;
    }

    void insert(const long long int& a) {
        if (!search(a)) {
            buckets[a % 100000].push_back(a);
            ++item_count;
        }
    }

    void erase(const long long int& c) {
        int s = buckets[c % 100000].size();

        for (int i = 0; i < s; i++) {
            if (buckets[c % 100000][i] == c) {

                swap(buckets[c % 100000][i],
                     buckets[c % 100000][s - 1]);

                buckets[c % 100000].pop_back();

                --item_count;
                return;
            }
        }
    }

    size_t allocated_bytes() const {
        size_t bytes=buckets.capacity()*sizeof(vector<long long>);
        for(const auto& b:buckets) bytes+=b.capacity()*sizeof(long long);
        return bytes;
    }

    void clear() {
        for (auto& bucket : buckets) {
            bucket.clear();
        }

        item_count = 0;
    }
};


class scheduler {
private:
    minheap heap;
    id_set cancelled;
    id_set in_heap;   // IDs currently in the heap

    long long purge_count=0, purge_input=0;

    void enforce_purge() {
        if(cancelled.size()>heap.size()-cancelled.size()) purge();
    }

    void purge() {
        ++purge_count;
        purge_input+=heap.size();

        vector<event> old_events = heap.get_events();
        vector<event> live_events;

        in_heap.clear();

        for (const event& e : old_events) {

            if (!cancelled.search(e.event_id)) {
                live_events.push_back(e);
                in_heap.insert(e.event_id);
            }
        }

        heap.build_heap(live_events);

        cancelled.clear();
    }

public:
    size_t live_size() const { return heap.size()-cancelled.size(); }
    size_t physical_size() const { return heap.size(); }
    long long purges() const { return purge_count; }
    long long purge_entries() const { return purge_input; }
    size_t allocated_bytes() const {
        return sizeof(*this)+heap.allocated_bytes()+cancelled.allocated_bytes()+in_heap.allocated_bytes();
    }
    bool validate() const {
        const auto& es=heap.get_events();
        if(cancelled.size()>static_cast<long long>(live_size()) || in_heap.size()!=heap.size()) return false;
        size_t c=0;
        for(size_t i=0;i<es.size();++i) {
            if(!in_heap.search(es[i].event_id)) return false;
            if(cancelled.search(es[i].event_id)) ++c;
            if(i && earlier(es[i],es[(i-1)/2])) return false;
        }
        return c==static_cast<size_t>(cancelled.size());
    }

    bool add_event(const event& a) {

        if (a.event_id<0 || in_heap.search(a.event_id)) {
            return false;
        }

        heap.insert(a);
        in_heap.insert(a.event_id);
        return true;
    }


    bool cancel_event(const long long int& a) {

        if (a<0 || !in_heap.search(a) || cancelled.search(a)) {
            return false;
        }

        cancelled.insert(a);

        long long int live_events = heap.size() - cancelled.size();

        if (cancelled.size() > live_events) {
            purge();
        }

        return true;
    }


    optional<event> next_live_event() {

        while (!heap.empty()) {

            event result = heap.deleteMin();
            in_heap.erase(result.event_id);

            if (cancelled.search(result.event_id)) {

                cancelled.erase(result.event_id);

                continue;
            }

            enforce_purge();
            return result;
        }

        return nullopt;
    }
};



}
