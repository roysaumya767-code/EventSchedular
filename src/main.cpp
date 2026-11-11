// main.cpp -- P3. Usage: ./sim --structure sortedvec|stdset|heap|avl [--events N] [--seed S] [--ack permille]
// When P1 and P2 finish, add "heap" and "avl" to makeSet() below (2 lines each).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "simulator.hpp"
#include "simple_sets.hpp"
#include "heap.hpp"   // P1
#include "avl.hpp"    // P2

static std::unique_ptr<EventSet> makeSet(const char* name) {
    if (!std::strcmp(name, "sortedvec")) return std::unique_ptr<EventSet>(new SortedVectorSet());
    if (!std::strcmp(name, "stdset"))    return std::unique_ptr<EventSet>(new StdSetEventSet());
    if (!std::strcmp(name, "heap"))      return std::unique_ptr<EventSet>(new HeapEventSet());
    if (!std::strcmp(name, "avl"))       return std::unique_ptr<EventSet>(new AvlEventSet());
    return nullptr;
}

int main(int argc, char** argv) {
    SimConfig cfg; const char* structure = "sortedvec";
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!std::strcmp(argv[i], "--structure")) structure = argv[i + 1];
        else if (!std::strcmp(argv[i], "--events")) cfg.maxEvents = std::atoll(argv[i + 1]);
        else if (!std::strcmp(argv[i], "--seed"))   cfg.seed = std::strtoull(argv[i + 1], 0, 10);
        else if (!std::strcmp(argv[i], "--ack"))    cfg.ackPermille = std::atoi(argv[i + 1]);
        else { std::fprintf(stderr, "unknown option %s\n", argv[i]); return 1; }
    }
    auto set = makeSet(structure);
    if (!set) { std::fprintf(stderr, "unknown structure %s\n", structure); return 1; }
    Simulator sim(cfg);
    SimResult r = sim.run(*set);
    // Everything below is IDENTICAL for every structure (we do not print the structure name here,
    // so you can compare the outputs of two runs with diff).
    std::printf("processed=%llu scheduled=%llu cancelled=%llu retransmissions=%llu maxLive=%llu lastTime=%lld\n",
        (unsigned long long)r.processed, (unsigned long long)r.scheduled, (unsigned long long)r.cancelled,
        (unsigned long long)r.retransmissions, (unsigned long long)r.maxLive, r.lastTime);
    std::printf("cancelled%%=%.1f\n", 100.0 * r.cancelled / r.scheduled);
    std::printf("traceHash=%016llx\n", (unsigned long long)r.traceHash);
    return 0;
}
