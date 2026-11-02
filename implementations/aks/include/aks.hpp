// AKS deterministic primality test (Agrawal-Kayal-Saxena, 2002).
// Primary author: Ben (see implementations/aks/BEN)
#pragma once
#include <cstdint>
#include <string>

namespace aks {

// Which step of the algorithm decided the answer. Useful for analysis/benchmarks.
enum class Verdict {
    TooSmall,        // n < 2
    PerfectPower,    // step 1: n = a^b, b >= 2          -> composite
    FactorFound,     // step 3: 1 < gcd(a, n) < n         -> composite
    SmallPrime,      // step 4: n <= r                    -> prime
    PolyMismatch,    // step 5: (X+a)^n != X^n + a        -> composite
    PolyAllPassed    // step 5 passed for all a           -> prime
};

struct Stats {
    Verdict  verdict = Verdict::TooSmall;
    uint64_t r = 0;                 // multiplicative-order parameter found in step 2
    uint64_t a_limit = 0;           // upper bound on a in step 5
    uint64_t a_checked = 0;         // how many values of a were actually tested in step 5
    uint64_t poly_squarings = 0;    // O(r^2) polynomial squarings performed
    uint64_t poly_linear_mults = 0; // O(r) multiplications by (X + a)
    double   t_step1_s = 0, t_step2_s = 0, t_step3_s = 0, t_step5_s = 0;
};

// Returns true iff n is prime. Deterministic, no randomness. Works for any 64-bit n
// (but running time is only practical up to roughly 2^32 with this implementation; see README).
bool is_prime(uint64_t n, Stats* stats = nullptr);

const char* verdict_name(Verdict v);

}  // namespace aks
