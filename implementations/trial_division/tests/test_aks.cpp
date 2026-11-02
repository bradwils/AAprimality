// Correctness tests for AKS. Oracle = trial division (deliberately NOT Miller-Rabin or
// Fermat, so the oracle is independent of the other algorithms in the project).
#include <cstdio>
#include <string>
#include <vector>
#include "aks.hpp"

static bool trial_division(uint64_t n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0) return false;
    for (uint64_t d = 3; d * d <= n; d += 2)
        if (n % d == 0) return false;
    return true;
}

static int failures = 0;
static void check(uint64_t n, const char* label) {
    bool got = aks::is_prime(n), want = trial_division(n);
    if (got != want) {
        std::printf("FAIL [%s] n=%llu aks=%d expected=%d\n", label, (unsigned long long)n, got, want);
        ++failures;
    }
}

int main(int argc, char** argv) {
    const bool slow = argc > 1 && std::string(argv[1]) == "--slow";
    // 1. Exhaustive on small range (3000 by default; 20000 with --slow)
    const uint64_t exhaustive_max = slow ? 20000 : 3000;
    for (uint64_t n = 0; n <= exhaustive_max; ++n) check(n, "exhaustive");

    // 2. Carmichael numbers (fool the Fermat test; AKS must call them composite)
    for (uint64_t n : {561ULL, 1105ULL, 1729ULL, 2465ULL, 2821ULL, 6601ULL, 8911ULL, 41041ULL,
                       825265ULL, 321197185ULL, 5394826801ULL})
        check(n, "carmichael");

    // 3. Perfect powers (step 1) and prime powers
    for (uint64_t n : {4ULL, 9ULL, 25ULL, 49ULL, 121ULL, 1024ULL, 3125ULL, 65536ULL,
                       9739ULL * 9739ULL, 1291ULL * 1291ULL * 1291ULL, 2147483648ULL})
        check(n, "perfect_power");

    // 4. Known primes. Fast tier only; large primes (minutes each) run with --slow.
    for (uint64_t n : {65537ULL, 1000003ULL}) check(n, "prime");
    if (slow) {
        // 2^24-3, Mersenne prime 2^31-1, and 2^32-5 (largest 32-bit prime)
        for (uint64_t n : {16777213ULL, 2147483647ULL, 4294967291ULL}) check(n, "prime_slow");
    }

    // 5. Semiprimes with close factors (hard case for trial division, not for AKS)
    for (uint64_t n : {1009ULL * 1013ULL, 10007ULL * 10009ULL, 65521ULL * 65537ULL})
        check(n, "semiprime");

    // 6. 64-bit path (n >= 2^32 uses the slow mulmod branch); keep to a few composites
    //    that exit early so the test stays fast.
    check(4294967297ULL, "64bit_composite");          // 2^32+1 = 641 * 6700417
    check(18446744073709551615ULL, "64bit_composite"); // 2^64-1

    if (failures == 0) std::printf("ALL AKS TESTS PASSED\n");
    return failures ? 1 : 0;
}
