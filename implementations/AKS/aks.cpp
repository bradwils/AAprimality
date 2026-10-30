// AKS primality test (deterministic, polynomial time).
// Primary author: Ben

#include <stdexcept>

#include "modarith.hpp"
#include "primality.hpp"

bool aks(u64 n, TestParams& p) {
    // TODO(Ben): perfect-power check, find smallest r with ord_r(n) > log2(n)^2,
    // gcd checks, then verify (X + a)^n == X^n + a mod (X^r - 1, n).
    // Deterministic: p.rounds and p.rng are unused.
    (void)n;
    (void)p;
    throw std::logic_error("aks: not implemented");
}
