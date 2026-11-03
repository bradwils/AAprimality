// Fermat primality test.
// Primary author: CJ

#include <random>

#include "modarith.hpp"
#include "primality.hpp"

bool fermat(u64 n, TestParams& p) {
    p.rounds_used = 0;
    if (n < 2) {
        return false;
    }
    if (n < 4) {
        return true;
    }
    if ((n & 1) == 0) {
        return false;
    }

    std::uniform_int_distribution<u64> pick(2, n - 2);
    for (int i = 1; i <= p.rounds; i++) {
        p.rounds_used = i;
        if (powmod(pick(p.rng), n - 1, n) != 1) {
            return false;
        }
    }
    return true;
}

// Well that was simple