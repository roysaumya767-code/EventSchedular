#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace p6 {
struct Event { double time; uint64_t id; };
inline bool operator<(Event a, Event b) { return a.time < b.time || (a.time == b.time && a.id < b.id); }
inline bool operator==(Event a, Event b) { return a.time == b.time && a.id == b.id; }
// -1 means unavailable, never an invented zero. Counters are lifetime totals.
struct Metrics {
    long long physical=-1, bytes=-1, purges=-1, purged_entries=-1;
    long long rotations=-1, height=-1;
};
struct EventSet {
    virtual ~EventSet() = default;
    virtual void insert(Event)=0;
    virtual Event delete_min()=0;
    virtual bool cancel(uint64_t)=0;
    virtual size_t size() const=0;
    virtual bool validate_structure() { return true; }
    virtual Metrics metrics() const { return {}; }
};
struct Backend { std::string name; std::function<std::unique_ptr<EventSet>()> make; };
}
