// Miller-Rabin primality test
// Primary author: Brad
//
// Guarantee: probabilistic. Never says "prime" for a composite that a
// witness exposes. Each random base catches a composite with probability
// at least 3/4, so k rounds give error <= 4^-k for any odd composite n.
// No known class of inputs defeats it (unlike Fermat and Carmichael numbers).
//
// Single file. Build the command line tool with:
//   clang++ -std=c++17 -O2 -o mr algorithm.cpp
// To reuse the algorithm from another file (tests, bench, the group harness):
//   #define MR_NO_MAIN
//   #include "algorithm.cpp"

#include <cstdint>
#include <random>

// Rounds used when the caller does not pick one. 4^-9 = 3.8e-6, so a
// "probably prime" answer is wrong less than 1 time in 260,000 on any input.
// (k = 8 gives 1.5e-5, which just misses five nines.)
const int DEFAULT_K = 9;

// (a * b) mod n without overflow.
// a and b are below n < 2^64, so a * b can need up to 128 bits. The 128-bit
// intermediate holds it exactly, then % n brings it back down to 64 bits.
uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t n) {
    return (unsigned __int128)a * b % n;
}

// base^exp mod n by repeated squaring.
// Walk the bits of exp from lowest to highest. base is squared every step so
// it always holds base^(2^i). When bit i of exp is set, that power is
// multiplied into the result.
uint64_t pow_mod(uint64_t base, uint64_t exp, uint64_t n) {
    uint64_t result = 1;
    base %= n;
    while (exp > 0) {
        if (exp & 1) result = mul_mod(result, base, n);
        base = mul_mod(base, base, n);
        exp >>= 1;
    }
    return result;
}

// One round. Returns true if a is a witness that n is composite.
// n - 1 = 2^s * d with d odd. The chain a^d, a^(2d), a^(4d), ..., a^(2^s d)
// ends at a^(n-1). If n is prime the chain either starts at 1 or hits n-1
// somewhere before the end, because 1 and n-1 are the only square roots of 1.
bool is_witness(uint64_t a, uint64_t n, uint64_t d, int s) {
    uint64_t x = pow_mod(a, d, n);
    if (x == 1 || x == n - 1) return false;
    for (int r = 1; r < s; r++) {
        x = mul_mod(x, x, n);
        if (x == n - 1) return false;  // reached -1, this base is fooled
        if (x == 1) return true;       // squared to 1 from something other
                                       // than -1: a non-trivial root of 1
    }
    return true;  // chain ended without ever hitting -1
}

// Returns true if n is probably prime after k random rounds.
// A false answer is always right. A true answer is wrong with probability
// at most 4^-k when n is composite. If rounds_used is given it is set to the
// round that rejected n (1-based), or to k if n survived every round.
bool is_probable_prime(uint64_t n, int k, std::mt19937_64& rng, int* rounds_used = nullptr) {
    if (rounds_used) *rounds_used = 0;
    if (n < 2) return false;
    if (n < 4) return true;        // 2 and 3
    if ((n & 1) == 0) return false;

    // write n - 1 as 2^s * d with d odd
    uint64_t d = n - 1;
    int s = 0;
    while ((d & 1) == 0) {
        d >>= 1;
        s++;
    }

    std::uniform_int_distribution<uint64_t> pick(2, n - 2);
    for (int i = 1; i <= k; i++) {
        if (rounds_used) *rounds_used = i;
        if (is_witness(pick(rng), n, d, s)) return false;
    }
    return true;
}

#ifndef MR_NO_MAIN
#include <cstdio>
#include <cstdlib>

// usage: mr <n> [k] [seed]
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <n> [k] [seed]\n", argv[0]);
        return 1;
    }

    uint64_t n = std::strtoull(argv[1], nullptr, 10);
    int k = argc > 2 ? std::atoi(argv[2]) : DEFAULT_K;
    uint64_t seed = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : std::random_device{}();
    std::mt19937_64 rng(seed);

    std::printf("%s\n", is_probable_prime(n, k, rng) ? "probably prime" : "composite");
    return 0;
}
#endif
