// Fermat primality test.
// Primary author: CJ

#include <stdexcept>

#include "modarith.hpp"
#include "primality.hpp"

bool fermat(u64 n, TestParams& p) {
    // TODO(CJ): for p.rounds random bases a in [2, n-2], reject if
    // powmod(a, n - 1, n) != 1. Use p.rng for base selection.
    (void)n;
    (void)p;
    throw std::logic_error("fermat: not implemented");
}
