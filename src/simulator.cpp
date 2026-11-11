// simulator.cpp -- P3 (Simulation Architect)
#include "simulator.hpp"
#include <cstdio>
#include <cstdlib>

static std::uint64_t fnv(std::uint64_t h, std::uint64_t x) {   // FNV-1a style fingerprint
    for (int i = 0; i < 8; i++) { h ^= (x & 0xFF); h *= 1099511628211ULL; x >>= 8; }
    return h;
}

// THE ONLY place where a sequence number is created: seq = global counter, then counter++.
Event Simulator::schedule(long long time, int type, int packet) {
    Event e; e.time = time; e.seq = nextSeq++; e.type = type; e.packet = packet;
    es->insert(e);
    res.scheduled++;
    if (es->size() > res.maxLive) res.maxLive = es->size();
    return e;
}

// Send (or re-send) a packet: maybe an ACK will come back, and arm the timers.
void Simulator::sendPacket(long long now, int p) {
    if (rng.chance(cfg.ackPermille))
        schedule(now + rng.range(1, cfg.maxAckDelay), ACK_ARRIVE, p);
    for (int i = 0; i < cfg.timersPerPacket; i++)
        packets[p].timers.push_back(schedule(now + (long long)cfg.rto * (i + 1), TIMER, p));
}

void Simulator::cancelTimers(int p) {
    for (const Event& t : packets[p].timers) {
        if (!es->cancel(t)) { std::fprintf(stderr, "BUG: cancel failed\n"); std::exit(1); }
        res.cancelled++;
    }
    packets[p].timers.clear();
}

void Simulator::handle(const Event& e) {
    PacketState& ps = packets[e.packet];
    if (e.type == SEND) {
        sendPacket(e.time, e.packet);
        if ((long long)res.scheduled < cfg.maxEvents) {            // start the next packet?
            int np = nextPacket++;
            packets.push_back(PacketState());
            schedule(e.time + rng.range(1, cfg.maxSendGap), SEND, np);
        }
    } else if (e.type == ACK_ARRIVE) {
        if (!ps.acked) { ps.acked = true; cancelTimers(e.packet); } // ACK came: timers no longer needed
    } else { // TIMER fired: this timer is gone from the set; remove it from our list
        for (size_t i = 0; i < ps.timers.size(); i++)
            if (ps.timers[i].seq == e.seq) { ps.timers.erase(ps.timers.begin() + i); break; }
        if (!ps.acked) {              // packet still unacknowledged -> retransmit
            cancelTimers(e.packet);   // the other timers of the old attempt are obsolete
            res.retransmissions++;
            sendPacket(e.time, e.packet);
        }
    }
}

SimResult Simulator::run(EventSet& set) {
    es = &set;
    packets.push_back(PacketState());
    nextPacket = 1;
    schedule(0, SEND, 0);

    std::uint64_t h = 1469598103934665603ULL;
    bool first = true; Event prev; prev.time = 0; prev.seq = 0;
    Event e;
    while (es->deleteMin(e)) {
        // D3 invariant, checked on EVERY event: (time,seq) must strictly increase.
        if (!first && !(prev < e)) {
            std::fprintf(stderr, "ORDER VIOLATION: (%lld,%llu) after (%lld,%llu)\n",
                         e.time, (unsigned long long)e.seq, prev.time, (unsigned long long)prev.seq);
            std::exit(1);
        }
        first = false; prev = e;
        h = fnv(h, (std::uint64_t)e.time); h = fnv(h, e.seq);
        h = fnv(h, (std::uint64_t)e.type); h = fnv(h, (std::uint64_t)e.packet);
        res.processed++; res.lastTime = e.time;
        handle(e);
    }
    res.traceHash = h;
    return res;
}
