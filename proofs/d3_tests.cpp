// d3_tests.cpp -- P3 validation for D3. Compile: g++ -std=c++17 -Iinclude tests/d3_tests.cpp src/simulator.cpp -o d3_tests
#include <cstdio>
#include <memory>
#include "simulator.hpp"
#include "simple_sets.hpp"
#include "heap.hpp"
#include "avl.hpp"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; } else std::printf("ok:   %s\n", msg); } while (0)

int main() {
    // Test 1: 5 events with the SAME time come out in creation (seq) order.
    {
        SortedVectorSet s; StdSetEventSet t;
        for (std::uint64_t q : {3ULL, 1ULL, 4ULL, 0ULL, 2ULL}) {
            Event e; e.time = 50; e.seq = q; e.type = TIMER; e.packet = 0; s.insert(e); t.insert(e);
        }
        bool good = true; Event a{}, b{};
        for (std::uint64_t want = 0; want < 5; want++) {
            s.deleteMin(a); t.deleteMin(b);
            if (a.seq != want || b.seq != want) good = false;
        }
        CHECK(good, "same-time events leave in sequence order (both structures)");
    }
    // Test 2: smaller time always first, even if its seq is bigger.
    {
        SortedVectorSet s; Event a{}, x{};
        a.time = 20; a.seq = 0; a.type = TIMER; a.packet = 0; s.insert(a);
        x.time = 10; x.seq = 9; x.type = TIMER; x.packet = 0; s.insert(x);
        Event out{}; s.deleteMin(out);
        CHECK(out.time == 10 && out.seq == 9, "smaller time wins even with larger seq");
    }
    // Test 3: same seed, two runs, same structure => identical fingerprint.
    SimConfig cfg; cfg.maxEvents = 200000;
    SimResult r1, r2, r3;
    { SortedVectorSet s; Simulator m(cfg); r1 = m.run(s); }
    { SortedVectorSet s; Simulator m(cfg); r2 = m.run(s); }
    CHECK(r1.traceHash == r2.traceHash && r1.processed == r2.processed, "same seed + same structure => identical trace");
    // Test 4: two DIFFERENT structures => identical trace.
    { StdSetEventSet s; Simulator m(cfg); r3 = m.run(s); }
    CHECK(r1.traceHash == r3.traceHash && r1.processed == r3.processed && r1.cancelled == r3.cancelled,
          "different structures => identical trace");
    // Test 4b: P1 heap and P2 AVL give the same trace as the sorted vector (this is the real D3 claim).
    SimResult rh, ra;
    { HeapEventSet s; Simulator m(cfg); rh = m.run(s); }
    { AvlEventSet  s; Simulator m(cfg); ra = m.run(s); }
    CHECK(rh.traceHash == r1.traceHash && rh.processed == r1.processed && rh.cancelled == r1.cancelled, "heap trace == sorted vector trace");
    CHECK(ra.traceHash == r1.traceHash && ra.processed == r1.processed && ra.cancelled == r1.cancelled, "AVL trace == sorted vector trace");
    // Test 5: a different seed gives a different trace (the fingerprint is really sensitive).
    SimConfig cfg2 = cfg; cfg2.seed = 999;
    SimResult r4; { SortedVectorSet s; Simulator m(cfg2); r4 = m.run(s); }
    CHECK(r4.traceHash != r1.traceHash, "different seed => different trace");
    // Test 6: workload is cancellation-heavy.
    CHECK(r1.cancelled * 2 > r1.scheduled, "more than half of all events were cancelled");

    // Test 7: ack = 0 (no ACK ever comes) must still finish, because of the retry limit.
    SimConfig cfg0; cfg0.maxEvents = 2000; cfg0.ackPermille = 0;
    SimResult r5; { SortedVectorSet s; Simulator m(cfg0); r5 = m.run(s); }
    CHECK(r5.processed > 0 && r5.retransmissions > 0, "ack = 0 terminates (retry limit works)");

    std::printf("%s\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return failures ? 1 : 0;
}
