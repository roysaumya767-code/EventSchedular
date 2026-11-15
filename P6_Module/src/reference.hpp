#pragma once
#include "event_set.hpp"
#include <map>
#include <set>
#include <stdexcept>
namespace p6 {
// Harness demonstration only: this is NOT P1's heap or P2's AVL.
class Reference final : public EventSet {
    std::set<Event> events;
    std::map<uint64_t, Event> ids;
public:
    void insert(Event e) override {
        if(ids.count(e.id)) throw std::runtime_error("duplicate ID");
        events.insert(e); ids.emplace(e.id,e);
    }
    Event delete_min() override {
        if(events.empty()) throw std::runtime_error("empty queue");
        Event e=*events.begin(); events.erase(events.begin()); ids.erase(e.id); return e;
    }
    bool cancel(uint64_t id) override {
        auto it=ids.find(id); if(it==ids.end()) return false;
        events.erase(it->second); ids.erase(it); return true;
    }
    size_t size() const override { return events.size(); }
    Metrics metrics() const override { Metrics m; m.physical=events.size(); return m; }
};
}
