// simple_sets.hpp  --  TEMPORARY stand-ins so P3 can test the simulator
// before P1 (heap), P2 (AVL) and P4 (oracle) finish. Do NOT use in the final benchmarks.
#pragma once
#include <vector>
#include <set>
#include <algorithm>
#include "event_set.hpp"

// 1) Sorted vector, kept from biggest to smallest so the smallest is at the END (cheap to remove).
class SortedVectorSet : public EventSet {
    std::vector<Event> v;   // descending order
    static bool bigger(const Event& a, const Event& b) { return b < a; }
public:
    void insert(const Event& e) override {
        auto pos = std::lower_bound(v.begin(), v.end(), e, bigger);  // binary search
        v.insert(pos, e);
    }
    bool deleteMin(Event& out) override {
        if (v.empty()) return false;
        out = v.back(); v.pop_back(); return true;
    }
    bool cancel(const Event& e) override {
        auto pos = std::lower_bound(v.begin(), v.end(), e, bigger);
        if (pos == v.end() || e < *pos || *pos < e) return false;
        v.erase(pos); return true;
    }
    std::size_t size() const override { return v.size(); }
    const char* name() const override { return "sortedvec"; }
};

// 2) A library balanced tree (std::set). Only used to prove two DIFFERENT structures
//    give identical simulator output.
class StdSetEventSet : public EventSet {
    std::set<Event> s;
public:
    void insert(const Event& e) override { s.insert(e); }
    bool deleteMin(Event& out) override {
        if (s.empty()) return false;
        out = *s.begin(); s.erase(s.begin()); return true;
    }
    bool cancel(const Event& e) override { return s.erase(e) > 0; }
    std::size_t size() const override { return s.size(); }
    const char* name() const override { return "stdset"; }
};
