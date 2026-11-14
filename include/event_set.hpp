// event_set.hpp  --  P3 (Simulation Architect)
// The shared interface for all event set structures.
#pragma once
#include <cstddef>
#include "event.hpp"

class EventSet {
public:
    virtual ~EventSet() {}
    virtual void        insert(const Event& e) = 0;      // add an event
    virtual bool        deleteMin(Event& out) = 0;       // remove the smallest (time,seq); false if empty
    virtual bool        cancel(const Event& e) = 0;      // remove this exact event; false if not present
    virtual std::size_t size() const = 0;                // number of LIVE (not cancelled) events
    virtual const char* name() const = 0;

    // successor(t, out): finds the first event with time >= t (smallest (time, seq) where time >= t).
    // Returns true and writes to out if found; returns false if no such event exists.
    virtual bool successor(long long t, Event& out) {
        (void)t; (void)out;
        return false;
    }
};
