// event_set.hpp  --  P3 (Simulation Architect)
// The "plug socket". The simulator only talks to THIS interface.
// P1 writes HeapEventSet, P2 writes AvlEventSet, P4 writes SortedArrayEventSet.
// Each of them must inherit from EventSet and fill in the functions below.
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
};
