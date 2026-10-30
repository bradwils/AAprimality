// Miller-Rabin primality test.
// Primary author: Brad

#include <stdexcept>

#include "modarith.hpp"
#include "primality.hpp"

bool miller_rabin(u64 n, TestParams& p) {
    // TODO(Brad): write n - 1 = 2^s * d, then for p.rounds random bases check
    // the squaring chain. Use p.rng for base selection.
    (void)n;
    (void)p;
    throw std::logic_error("miller_rabin: not implemented");
}
