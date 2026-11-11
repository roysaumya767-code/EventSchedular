// rng.hpp -- our own random number generator (splitmix64).
// We do NOT use std::mt19937 / std::uniform_int_distribution, because their results
// can differ between compilers. Ours gives the same numbers on every computer.
#pragma once
#include <cstdint>

class Rng {
    std::uint64_t state;
public:
    explicit Rng(std::uint64_t seed) : state(seed) {}
    std::uint64_t next() {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    // whole number in [lo, hi]
    long long range(long long lo, long long hi) {
        return lo + (long long)(next() % (std::uint64_t)(hi - lo + 1));
    }
    // true with probability permille/1000
    bool chance(int permille) { return (int)(next() % 1000) < permille; }
};
