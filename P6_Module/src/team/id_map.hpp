#pragma once
#include "../event_set.hpp"
#include <stdexcept>
namespace p6 {
// Custom chained hash map for AVL cancellation by ID. No library map/hash map.
class IdMap {
    std::vector<std::vector<Event>> buckets=std::vector<std::vector<Event>>(64);
    size_t count=0;
    static uint64_t hash(uint64_t x) {
        x+=0x9e3779b97f4a7c15ULL;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
        x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
    void grow() {
        std::vector<std::vector<Event>> next(buckets.size()*2);
        for(const auto& b:buckets) for(auto e:b) next[hash(e.id)%next.size()].push_back(e);
        buckets.swap(next);
    }
public:
    bool find(uint64_t id,Event& result) const {
        for(auto e:buckets[hash(id)%buckets.size()]) if(e.id==id) {result=e;return true;}
        return false;
    }
    void insert(Event e) {
        Event ignored;
        if(find(e.id,ignored)) throw std::runtime_error("duplicate ID");
        if(count>=buckets.size()) grow();
        buckets[hash(e.id)%buckets.size()].push_back(e);++count;
    }
    bool erase(uint64_t id) {
        auto& b=buckets[hash(id)%buckets.size()];
        for(size_t i=0;i<b.size();++i) if(b[i].id==id) {b[i]=b.back();b.pop_back();--count;return true;}
        return false;
    }
    size_t size() const {return count;}
    size_t backing_bytes() const {
        size_t bytes=buckets.capacity()*sizeof(std::vector<Event>);
        for(const auto& b:buckets) bytes+=b.capacity()*sizeof(Event);
        return bytes;
    }
};
}
