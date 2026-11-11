// event.hpp  --  P3 (Simulation Architect)
// An Event is one thing that will happen at a certain moment in the simulation.
#pragma once
#include <cstdint>

enum EventType : int { SEND = 0, ACK_ARRIVE = 1, TIMER = 2 };

struct Event {
    long long     time;    // WHEN it happens (whole number of "ticks", never a decimal)
    std::uint64_t seq;     // sequence number: 0,1,2,3... given in the order events are CREATED
    int           type;    // SEND, ACK_ARRIVE or TIMER
    int           packet;  // which packet this event is about
};

// THE ordering rule of the whole project (this is what D3 is about):
// compare time first; if the times are equal, the smaller sequence number goes first.
// Because every seq is different, two different events are NEVER "equal".
inline bool operator<(const Event& a, const Event& b) {
    if (a.time != b.time) return a.time < b.time;
    return a.seq < b.seq;
}
