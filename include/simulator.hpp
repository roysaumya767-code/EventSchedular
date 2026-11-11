// simulator.hpp  --  P3 (Simulation Architect)
// A sender transmits packets. Each send arms several timers (retransmission timer,
// keep-alive, etc.). When the ACK arrives, all timers of that packet are CANCELLED.
// So most scheduled events end up cancelled -- exactly the workload the project needs.
#pragma once
#include <cstdint>
#include <vector>
#include "event_set.hpp"
#include "rng.hpp"

struct SimConfig {
    std::uint64_t seed            = 12345;
    long long     maxEvents       = 1000000; // stop starting NEW packets after this many events were scheduled
    int           timersPerPacket = 4;       // timers armed per send (all cancelled when the ACK arrives)
    int           ackPermille     = 950;     // chance (out of 1000) that an ACK comes back at all
    int           rto             = 100;     // base timeout (ticks)
    int           maxAckDelay     = 120;     // ACK delay is random in [1, maxAckDelay]
    int           maxSendGap      = 3;       // gap between two new packets is random in [1, maxSendGap]
    int           maxRetries      = 20;      // give up on a packet after this many retransmissions (stops endless loops, e.g. ack = 0)
};

struct SimResult {
    std::uint64_t processed       = 0;  // events taken out and handled
    std::uint64_t scheduled       = 0;  // events ever inserted
    std::uint64_t cancelled       = 0;  // events cancelled
    std::uint64_t retransmissions = 0;
    std::uint64_t maxLive         = 0;  // biggest number of live events at once
    std::uint64_t traceHash       = 0;  // fingerprint of the ENTIRE processing order
    long long     lastTime        = 0;
};

class Simulator {
public:
    explicit Simulator(const SimConfig& c) : cfg(c), rng(c.seed) {}
    SimResult run(EventSet& set);   // the same code runs for ANY structure
private:
    struct PacketState { bool acked = false; int retries = 0; std::vector<Event> timers; };

    SimConfig cfg;
    Rng rng;
    EventSet* es = nullptr;
    SimResult res;
    std::uint64_t nextSeq = 0;
    int nextPacket = 0;
    std::vector<PacketState> packets;

    Event schedule(long long time, int type, int packet);
    void  sendPacket(long long now, int packet);
    void  cancelTimers(int packet);
    void  handle(const Event& e);
};
