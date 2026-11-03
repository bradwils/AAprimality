// Miller-Rabin primality test.
// Primary author: Brad
//
// For an odd composite, a random base is a witness with probability at least 3/4,
// so k rounds give error <= 4^-k. Unlike Fermat, no class of composites has a high error rate.

#include <random>

#include "modarith.hpp"
#include "primality.hpp"

// True if a is a witness that n is composite, where n - 1 = 2^s * d with d odd.
// For prime n the chain a^d, a^(2d), ..., a^(n-1) either starts at 1 or reaches
// n - 1, because 1 and n - 1 are the only square roots of 1.
static bool is_witness(u64 a, u64 n, u64 d, int s) {
    u64 x = powmod(a, d, n);
    if (x == 1 || x == n - 1) return false;
    for (int r = 1; r < s; r++) {
        x = mulmod(x, x, n);
        if (x == n - 1) return false;
        if (x == 1) return true; // non-trivial square root of 1
    }
    return true;
}

bool miller_rabin(u64 n, TestParams& p) {
    p.rounds_used = 0;
    if (n < 2) return false;
    if (n < 4) return true;
    if ((n & 1) == 0) return false;

    u64 d = n - 1;
    int s = 0;
    while ((d & 1) == 0) {
        d >>= 1;
        s++;
    }

    std::uniform_int_distribution<u64> pick(2, n - 2);
    for (int i = 1; i <= p.rounds; i++) {
        p.rounds_used = i;
        if (is_witness(pick(p.rng), n, d, s)) return false;
    }
    return true;
}
