// Shared interface for all primality tests.
#pragma once

#include <cstdint>
#include <random>

using u64 = std::uint64_t;

struct TestParams {
    int rounds; // number of random bases; unused by deterministic tests
    std::mt19937_64& rng; // seeded by the caller so randomized runs are reproducible
    int rounds_used = 0; // set by randomized tests: the round in which n was found composite, or rounds if none
};

// Returns true if n is (probably) prime.
using PrimalityFn = bool (*)(u64 n, TestParams& p);

bool fermat(u64 n, TestParams& p); // implementations/fermat_primality
bool miller_rabin(u64 n, TestParams& p); // implementations/miller_rabin
bool aks(u64 n, TestParams& p); // implementations/AKS

struct Algorithm {
    const char* name;
    const char* author;
    bool randomized;
    PrimalityFn fn;
};

inline constexpr Algorithm ALGORITHMS[] = {
    {"fermat", "CJ",   true,  fermat},
    {"miller-rabin", "Brad", true,  miller_rabin},
    {"aks", "Ben",  false, aks},
};

inline constexpr int DEFAULT_ROUNDS = 9; // 4^-9 < 4e-6 error bound for Miller-Rabin
