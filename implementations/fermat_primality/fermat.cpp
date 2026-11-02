#include <random>

#include "modarith.hpp"
#include "primality.hpp"

bool fermat(u64 n, TestParams& p) {
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
    for (int i = 0; i < p.rounds; i++) {
        if (powmod(pick(p.rng), n - 1, n) != 1) {
            return false;
        }
    }
    return true;
}
