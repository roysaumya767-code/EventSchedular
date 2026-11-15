#pragma once
#include "event_set.hpp"
#include "team/avl.hpp"
#include "team/minheap.hpp"
#include "team/id_map.hpp"
#include <limits>
#include <cmath>
namespace p6 {
inline long long checked_id(Event e) {
    if(e.id>static_cast<uint64_t>(std::numeric_limits<long long>::max()) || !std::isfinite(e.time) || e.time<0)
        throw std::runtime_error("invalid event");
    return static_cast<long long>(e.id);
}
class HeapAdapter final : public EventSet {
    team_heap::scheduler queue;
public:
    void insert(Event e) override {
        auto id=checked_id(e);
        if(!queue.add_event({e.time,id,id})) throw std::runtime_error("duplicate heap ID");
    }
    Event delete_min() override {
        auto e=queue.next_live_event();
        if(!e) throw std::runtime_error("empty heap");
        return {e->timestamp,static_cast<uint64_t>(e->event_id)};
    }
    bool cancel(uint64_t id) override {
        if(id>static_cast<uint64_t>(std::numeric_limits<long long>::max())) return false;
        return queue.cancel_event(static_cast<long long>(id));
    }
    size_t size() const override {return queue.live_size();}
    bool validate_structure() override {return queue.validate();}
    Metrics metrics() const override {
        Metrics m;m.physical=queue.physical_size();
        m.bytes=queue.allocated_bytes()+sizeof(*this)-sizeof(queue);
        m.purges=queue.purges();m.purged_entries=queue.purge_entries();return m;
    }
};
class AVLAdapter final : public EventSet {
    team_avl::Core tree;
    team_avl::Node* root=nullptr;
    IdMap ids;
    static team_avl::Event convert(Event e) {auto id=checked_id(e);return {e.time,id,id,""};}
    bool check_index(team_avl::Node* n) const {
        if(!n)return true;
        Event e;
        return ids.find(n->event.id,e) && e.time==n->event.time && e.id==static_cast<uint64_t>(n->event.sequence)
            && check_index(n->left) && check_index(n->right);
    }
public:
    AVLAdapter()=default;
    AVLAdapter(const AVLAdapter&)=delete;
    AVLAdapter& operator=(const AVLAdapter&)=delete;
    ~AVLAdapter() override {tree.clearTree(root);}
    void insert(Event e) override {auto v=convert(e);ids.insert(e);root=tree.insert(root,v);}
    Event delete_min() override {
        team_avl::Event e;
        if(!tree.deleteMin(root,e)) throw std::runtime_error("empty AVL");
        if(!ids.erase(e.id)) throw std::runtime_error("AVL ID index missing");
        return {e.time,static_cast<uint64_t>(e.id)};
    }
    bool cancel(uint64_t id) override {
        Event e;if(!ids.find(id,e))return false;
        root=tree.cancel(root,convert(e));ids.erase(id);return true;
    }
    size_t size() const override {return ids.size();}
    bool validate_structure() override {return tree.validateAvl(root) && check_index(root);}
    Metrics metrics() const override {
        Metrics m;m.physical=ids.size();m.bytes=sizeof(*this)+ids.size()*sizeof(team_avl::Node)+ids.backing_bytes();
        m.rotations=tree.leftRotations+tree.rightRotations;m.height=root ? root->h : -1;return m;
    }
};
inline std::vector<Backend> team_backends() {
    return {{"heap",[]{return std::make_unique<HeapAdapter>();}},
            {"avl",[]{return std::make_unique<AVLAdapter>();}}};
}
}
