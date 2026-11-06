#include <iostream>
#include <bits/stdc++.h>
using namespace std;


struct event {
    long long timestamp;
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

    vector<event> get_events() const {
        return heap;
    }

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

    long long int size() {
        return item_count;
    }

    bool search(const long long int& b) {
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

    void purge() {

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

    bool add_event(const event& a) {

        if (in_heap.search(a.event_id)) {
            return false;
        }

        heap.insert(a);
        in_heap.insert(a.event_id);
        return true;
    }


    bool cancel_event(const long long int& a) {

        if (!in_heap.search(a)) {
            return false;
        }

        cancelled.insert(a);

        long long int live_events = heap.size() - cancelled.size();

        if (cancelled.size() > live_events) {
            purge();
        }

        return true;
    }


    optional<long long int> next_live_event() {

        while (!heap.empty()) {

            event result = heap.deleteMin();
            in_heap.erase(result.event_id);

            if (cancelled.search(result.event_id)) {

                cancelled.erase(result.event_id);

                continue;
            }

            return result.event_id;
        }

        return nullopt;
    }
};


int main() {

    scheduler s;

    s.add_event({10, 1, 101});
    s.add_event({20, 2, 102});
    s.add_event({30, 3, 103});
    s.add_event({40, 4, 104});
    s.add_event({50, 5, 105});

    s.cancel_event(101);
    s.cancel_event(102);
    s.cancel_event(103);

    while (true) {

        optional<long long int> result = s.next_live_event();

        if (!result.has_value()) {
            break;
        }

        cout << "Next live event: " << result.value() << endl;
    }

    return 0;
}
