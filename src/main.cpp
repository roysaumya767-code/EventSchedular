// main.cpp -- P3. Usage:
//   ./sim --structure sortedvec|stdset|heap|avl [--events N] [--seed S] [--ack permille]
//         [--timers K] [--rto T] [--maxdelay D] [--gap G] [--retries R]
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

// Read a whole number from text; stop the program with a clear message if the text is not a number in [lo, hi].
static long long parseNum(const char* opt, const char* text, long long lo, long long hi) {
    char* end = nullptr;
    long long v = std::strtoll(text, &end, 10);
    if (end == text || *end != '\0' || v < lo || v > hi) {
        std::fprintf(stderr, "bad value '%s' for %s (allowed: %lld..%lld)\n", text, opt, lo, hi);
        std::exit(1);
    }
    return v;
}

int main(int argc, char** argv) {
    SimConfig cfg; const char* structure = "sortedvec";
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) { std::fprintf(stderr, "option %s needs a value\n", argv[i]); return 1; }
        const char* opt = argv[i]; const char* val = argv[i + 1];
        if      (!std::strcmp(opt, "--structure")) structure = val;
        else if (!std::strcmp(opt, "--events"))   cfg.maxEvents       = parseNum(opt, val, 1, 1000000000LL);
        else if (!std::strcmp(opt, "--seed"))     cfg.seed            = (std::uint64_t)parseNum(opt, val, 0, 9000000000000000000LL);
        else if (!std::strcmp(opt, "--ack"))      cfg.ackPermille     = (int)parseNum(opt, val, 0, 1000);
        else if (!std::strcmp(opt, "--timers"))   cfg.timersPerPacket = (int)parseNum(opt, val, 1, 1000);
        else if (!std::strcmp(opt, "--rto"))      cfg.rto             = (int)parseNum(opt, val, 1, 1000000);
        else if (!std::strcmp(opt, "--maxdelay")) cfg.maxAckDelay     = (int)parseNum(opt, val, 1, 1000000);
        else if (!std::strcmp(opt, "--gap"))      cfg.maxSendGap      = (int)parseNum(opt, val, 1, 1000000);
        else if (!std::strcmp(opt, "--retries"))  cfg.maxRetries      = (int)parseNum(opt, val, 0, 1000000);
        else { std::fprintf(stderr, "unknown option %s\n", opt); return 1; }
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
