// avl.hpp -- Structure B: AVL Tree keyed by (time, sequence number)
// Supports insert, deleteMin, true cancellation (node deletion), and successor(t).
// Definition: successor(t) returns the first event with time >= t (smallest (time, seq)).
#pragma once
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include "../../include/event.hpp"
#include "../../include/event_set.hpp"

namespace structB {

struct Node {
    Event event;
    int h = 0;
    Node* left = nullptr;
    Node* right = nullptr;
};

class AvlTree {
private:
    Node* root_ = nullptr;
    std::size_t size_ = 0;
    std::uint64_t left_rotations_ = 0;
    std::uint64_t right_rotations_ = 0;

    static inline int ht(Node* x) { return x ? x->h : -1; }
    static inline void upd(Node* x) {
        if (x) {
            int hl = ht(x->left);
            int hr = ht(x->right);
            x->h = 1 + (hl > hr ? hl : hr);
        }
    }
    static inline int bf(Node* x) { return x ? (ht(x->left) - ht(x->right)) : 0; }

    Node* rotR(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        x->right = y;
        upd(y);
        upd(x);
        right_rotations_++;
        return x;
    }

    Node* rotL(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        y->left = x;
        upd(x);
        upd(y);
        left_rotations_++;
        return y;
    }

    Node* rebalance(Node* z) {
        if (!z) return nullptr;
        upd(z);
        int b = bf(z);
        if (b > 1) { // Left-heavy
            if (bf(z->left) < 0) { // LR double rotation
                z->left = rotL(z->left);
            }
            return rotR(z); // LL single rotation
        }
        if (b < -1) { // Right-heavy
            if (bf(z->right) > 0) { // RL double rotation
                z->right = rotR(z->right);
            }
            return rotL(z); // RR single rotation
        }
        return z;
    }

    Node* insertInternal(Node* x, const Event& k, bool& inserted) {
        if (!x) {
            inserted = true;
            return new Node{k, 0, nullptr, nullptr};
        }
        if (k < x->event) {
            x->left = insertInternal(x->left, k, inserted);
        } else if (x->event < k) {
            x->right = insertInternal(x->right, k, inserted);
        } else {
            inserted = false; // duplicate key (same time and seq)
            return x;
        }
        return rebalance(x);
    }

    Node* eraseInternal(Node* x, const Event& k, bool& erased) {
        if (!x) {
            erased = false;
            return nullptr;
        }
        if (k < x->event) {
            x->left = eraseInternal(x->left, k, erased);
        } else if (x->event < k) {
            x->right = eraseInternal(x->right, k, erased);
        } else {
            erased = true;
            if (!x->left) {
                Node* r = x->right;
                delete x;
                return r;
            }
            if (!x->right) {
                Node* l = x->left;
                delete x;
                return l;
            }
            // Node has two children: replace with in-order successor (smallest in right subtree)
            Node* s = x->right;
            while (s->left) s = s->left;
            x->event = s->event;
            x->right = eraseInternal(x->right, s->event, erased);
        }
        return rebalance(x);
    }

    void clearInternal(Node* x) {
        if (!x) return;
        clearInternal(x->left);
        clearInternal(x->right);
        delete x;
    }

    bool validateInternal(Node* x, const Event* low, const Event* high) const {
        if (!x) return true;
        if (low && !(*low < x->event)) return false;
        if (high && !(x->event < *high)) return false;

        int b = bf(x);
        if (b < -1 || b > 1) return false;

        int expected_h = 1 + std::max(ht(x->left), ht(x->right));
        if (x->h != expected_h) return false;

        return validateInternal(x->left, low, &x->event) &&
               validateInternal(x->right, &x->event, high);
    }

public:
    AvlTree() = default;

    ~AvlTree() {
        clear();
    }

    AvlTree(const AvlTree&) = delete;
    AvlTree& operator=(const AvlTree&) = delete;

    AvlTree(AvlTree&& other) noexcept
        : root_(other.root_),
          size_(other.size_),
          left_rotations_(other.left_rotations_),
          right_rotations_(other.right_rotations_) {
        other.root_ = nullptr;
        other.size_ = 0;
        other.left_rotations_ = 0;
        other.right_rotations_ = 0;
    }

    AvlTree& operator=(AvlTree&& other) noexcept {
        if (this != &other) {
            clear();
            root_ = other.root_;
            size_ = other.size_;
            left_rotations_ = other.left_rotations_;
            right_rotations_ = other.right_rotations_;
            other.root_ = nullptr;
            other.size_ = 0;
            other.left_rotations_ = 0;
            other.right_rotations_ = 0;
        }
        return *this;
    }

    void clear() {
        clearInternal(root_);
        root_ = nullptr;
        size_ = 0;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int height() const { return ht(root_); }

    std::uint64_t leftRotations() const { return left_rotations_; }
    std::uint64_t rightRotations() const { return right_rotations_; }
    std::uint64_t totalRotations() const { return left_rotations_ + right_rotations_; }
    void resetRotationCounters() {
        left_rotations_ = 0;
        right_rotations_ = 0;
    }

    bool insert(const Event& e) {
        bool inserted = false;
        root_ = insertInternal(root_, e, inserted);
        if (inserted) size_++;
        return inserted;
    }

    bool erase(const Event& e) {
        bool erased = false;
        root_ = eraseInternal(root_, e, erased);
        if (erased) size_--;
        return erased;
    }

    bool deleteMin(Event& out) {
        if (!root_) return false;
        Node* curr = root_;
        while (curr->left) curr = curr->left;
        out = curr->event;
        bool erased = false;
        root_ = eraseInternal(root_, out, erased);
        if (erased) size_--;
        return erased;
    }

    // successor(t, out): finds the first event with time >= t (smallest (time, seq))
    bool successor(long long t, Event& out) const {
        Node* curr = root_;
        const Event* best = nullptr;
        while (curr) {
            if (curr->event.time >= t) {
                best = &(curr->event);
                curr = curr->left; // look for an even smaller (earlier) event >= t
            } else {
                curr = curr->right;
            }
        }
        if (best) {
            out = *best;
            return true;
        }
        return false;
    }

    bool validate() const {
        return validateInternal(root_, nullptr, nullptr);
    }
};

class AvlEventSet : public EventSet {
private:
    AvlTree tree_;

public:
    void insert(const Event& e) override {
        tree_.insert(e);
    }

    bool deleteMin(Event& out) override {
        return tree_.deleteMin(out);
    }

    bool cancel(const Event& e) override {
        return tree_.erase(e);
    }

    std::size_t size() const override {
        return tree_.size();
    }

    const char* name() const override { return "avl"; }

    bool successor(long long t, Event& out) override {
        return tree_.successor(t, out);
    }

    bool validate() const {
        return tree_.validate();
    }

    int height() const {
        return tree_.height();
    }

    std::uint64_t leftRotations() const { return tree_.leftRotations(); }
    std::uint64_t rightRotations() const { return tree_.rightRotations(); }
    std::uint64_t totalRotations() const { return tree_.totalRotations(); }
    void resetRotationCounters() { tree_.resetRotationCounters(); }
};

} // namespace structB
