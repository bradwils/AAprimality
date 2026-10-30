// Shared interface for all primality tests.
#pragma once

#include <cstdint>
#include <random>

using u64 = std::uint64_t;

struct TestParams {
    int rounds; // number of random bases; ignored by deterministic tests
    std::mt19937_64& rng; // seeded by main so randomized runs are reproducible
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
