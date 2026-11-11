// avl.hpp -- P2's AVL tree, connected to P3's EventSet interface.
// Algorithms are P2's (from avl.cpp). Changes made only to connect it (marked "P3 change"):
//   * removed main(), "using namespace std"
//   * Event: "string type" became "int type" + "int packet" (strings copied on every comparison = slow)
//   * comparator takes const references (no copying)
//   * leftRotations is now really counted in rotL (it was never increased before)
//   * insert/erase report whether they did something (needed for size() and cancel() results)
#pragma once
#include <algorithm>
#include <cstdlib>
#include "event_set.hpp"

namespace avlimpl {

long long leftRotations = 0;
long long rightRotations = 0;
bool insertedNew = false;   // P3 change
bool erasedOne = false;     // P3 change

struct Event {
    long long time;
    long long sequence;
    long long id;
    int type;      // P3 change
    int packet;    // P3 change
};

struct Node { Event event; int h; Node *left, *right; };

inline int ht(Node* x) { return x ? x->h : -1; }
inline void upd(Node* x) { x->h = 1 + std::max(ht(x->left), ht(x->right)); }

inline Node* rotR(Node* y) {
    Node* x = y->left; y->left = x->right; x->right = y;
    upd(y); upd(x); rightRotations++;
    return x;
}
inline Node* rotL(Node* x) {
    Node* y = x->right; x->right = y->left; y->left = x;
    upd(x); upd(y); leftRotations++;   // P3 change: counter added
    return y;
}
inline int bf(Node* x) { return ht(x->left) - ht(x->right); }

inline Node* rebalance(Node* z) {
    upd(z);
    if (bf(z) > 1) {
        if (bf(z->left) < 0) z->left = rotL(z->left);
        return rotR(z);
    }
    if (bf(z) < -1) {
        if (bf(z->right) > 0) z->right = rotR(z->right);
        return rotL(z);
    }
    return z;
}

inline bool comparator(const Event& a, const Event& b) {   // P3 change: const&
    if (a.time != b.time) return a.time < b.time;
    return a.sequence < b.sequence;
}

inline Node* insert(Node* x, const Event& k) {
    if (!x) { insertedNew = true; return new Node{k, 0, nullptr, nullptr}; }
    if (comparator(k, x->event)) x->left = insert(x->left, k);
    else if (comparator(x->event, k)) x->right = insert(x->right, k);
    else return x;   // no duplicates
    return rebalance(x);
}

inline Node* erase(Node* x, const Event& k) {
    if (x == nullptr) return nullptr;
    if (comparator(k, x->event)) x->left = erase(x->left, k);
    else if (comparator(x->event, k)) x->right = erase(x->right, k);
    else {
        erasedOne = true;   // P3 change
        if (x->left == nullptr) { Node* r = x->right; delete x; return r; }
        if (x->right == nullptr) { Node* l = x->left; delete x; return l; }
        Node* s = x->right;
        while (s->left) s = s->left;
        x->event = s->event;
        x->right = erase(x->right, s->event);
    }
    return rebalance(x);
}

inline Node* getmin(Node* x) {
    if (!x) return x;
    if (x->left) return getmin(x->left);
    return x;
}

inline bool deleteMin(Node*& x, Event& result) {
    if (x == nullptr) return false;
    Node* y = getmin(x);
    result = y->event;
    x = erase(x, y->event);
    return true;
}

inline Node* successor(Node* x, long long t) {
    Node* best = nullptr;
    while (x != nullptr) {
        if (x->event.time >= t) { best = x; x = x->left; }
        else x = x->right;
    }
    return best;
}

inline bool validation(Node* x, Event* low, Event* high) {
    if (!x) return true;
    if (low != nullptr && !(comparator(*low, x->event))) return false;
    if (high != nullptr && !(comparator(x->event, *high))) return false;
    int m = bf(x);
    if (std::abs(m) > 1) return false;
    int htm = 1 + std::max(ht(x->left), ht(x->right));
    if (x->h != htm) return false;
    return validation(x->left, low, &x->event) && validation(x->right, &x->event, high);
}
inline bool validateAvl(Node* x) { return validation(x, nullptr, nullptr); }
inline void clearTree(Node* x) { if (!x) return; clearTree(x->left); clearTree(x->right); delete x; }

} // namespace avlimpl

// The plug that connects P2's AVL tree to the simulator.
class AvlEventSet : public EventSet {
    avlimpl::Node* root = nullptr;
    std::size_t count = 0;
    static avlimpl::Event conv(const ::Event& e) {
        return avlimpl::Event{e.time, (long long)e.seq, (long long)e.seq, e.type, e.packet};
    }
public:
    ~AvlEventSet() { avlimpl::clearTree(root); }
    void insert(const ::Event& e) override {
        avlimpl::insertedNew = false;
        root = avlimpl::insert(root, conv(e));
        if (avlimpl::insertedNew) ++count;
    }
    bool deleteMin(::Event& out) override {
        avlimpl::Event r;
        if (!avlimpl::deleteMin(root, r)) return false;
        --count;
        out.time = r.time; out.seq = (std::uint64_t)r.sequence; out.type = r.type; out.packet = r.packet;
        return true;
    }
    bool cancel(const ::Event& e) override {
        avlimpl::erasedOne = false;
        root = avlimpl::erase(root, conv(e));
        if (avlimpl::erasedOne) { --count; return true; }
        return false;
    }
    std::size_t size() const override { return count; }
    const char* name() const override { return "avl"; }
    bool validate() const { return avlimpl::validateAvl(root); }   // P2's checker
    int height() const { return avlimpl::ht(root); }
};
