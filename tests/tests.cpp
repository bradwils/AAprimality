// Correctness tests for every algorithm. The expected results never come from the
// code being tested: they come from a sieve, hand-checked lists, and
// data/instances.csv (labelled by a separate Python prime check in scripts/gen_data.py).
//
// Results from randomized tests can only be wrong in one direction, so the result
// for a prime must always be prime. At 20 rounds (error <= 4^-20) the Miller-Rabin
// result for every composite must also be composite. Prime results from Fermat on
// composites are expected; they are counted and printed, not treated as failures.
//
// Run from the repo root:  build/tests [--slow]

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "instances.hpp"
#include "modarith.hpp"
#include "primality.hpp"

namespace {

int checks = 0, failures = 0;

#define CHECK(cond, ...)                          \
    do {                                          \
        checks++;                                 \
        if (!(cond)) {                            \
            failures++;                           \
            std::printf("    FAIL: ");            \
            std::printf(__VA_ARGS__);             \
            std::printf("\n");                    \
        }                                         \
    } while (0)

const int K_SURE = 20;
const u64 SEED = 20261102;

std::vector<bool> sieve(u64 limit) {
    std::vector<bool> is_p(limit + 1, true);
    is_p[0] = is_p[1] = false;
    for (u64 i = 2; i * i <= limit; i++)
        if (is_p[i])
            for (u64 j = i * i; j <= limit; j += i) is_p[j] = false;
    return is_p;
}

// Runs the algorithm on n and checks the result. A prime result from Fermat on a
// composite is recorded instead of failing.
struct Runner {
    const Algorithm& a;
    std::mt19937_64 rng;
    std::vector<u64> fermat_false_primes;

    void expect(u64 n, bool prime, int k = K_SURE) {
        TestParams p{k, rng};
        bool got = a.fn(n, p);
        if (!prime && got && std::strcmp(a.name, "fermat") == 0) {
            fermat_false_primes.push_back(n);
            return;
        }
        CHECK(got == prime, "%s returned %s for %llu", a.name, got ? "prime" : "composite", (unsigned long long)n);
    }
};

void test_modarith() {
    std::printf("[mulmod and powmod]\n");
    const u64 M = UINT64_MAX;
    CHECK(mulmod(M - 1, M - 1, M) == 1, "(n-1)^2 mod n with n = 2^64-1");
    CHECK(mulmod(M - 1, 2, M) == M - 2, "overflowing product");
    CHECK(powmod(2, 64, M) == 1, "2^64 mod (2^64-1)");
    CHECK(powmod(3, 200, 1000000007ULL) == 136318165ULL, "3^200 mod 1e9+7 (from Python pow)");
    bool all = true;
    for (u64 n = 2; n < 60; n++)
        for (u64 b = 0; b < n; b++)
            for (u64 e = 0, slow = 1 % n; e < 40; e++, slow = slow * b % n)
                all = all && powmod(b, e, n) == slow;
    CHECK(all, "powmod matches repeated multiplication for n < 60");
}

void test_algorithm(const Algorithm& a, const std::vector<Instance>& inst, bool slow) {
    std::printf("[%s]\n", a.name);
    Runner run{a, std::mt19937_64(SEED), {}};

    for (u64 n : {0, 1, 4, 9, 25}) run.expect(n, false);
    for (u64 n : {2, 3, 5, 7}) run.expect(n, true);

    // AKS is slow, so a smaller exhaustive range is used for it
    const u64 limit = a.randomized ? 200000 : slow ? 20000 : 3000;
    auto is_p = sieve(limit);
    for (u64 n = 0; n <= limit; n++) run.expect(n, is_p[n], is_p[n] ? 1 : K_SURE);
    std::printf("    every n up to %llu against a sieve\n", (unsigned long long)limit);

    std::vector<u64> primes = {65537, 1000003};
    if (a.randomized || slow) primes.insert(primes.end(), {16777213ULL, 2147483647ULL, 4294967291ULL});
    if (a.randomized) primes.insert(primes.end(), {2305843009213693951ULL, 9223372036854775783ULL, 18446744073709551557ULL});
    for (u64 n : primes) run.expect(n, true, DEFAULT_ROUNDS);

    std::vector<u64> composites = {
        // Carmichael numbers
        561, 1105, 1729, 2465, 2821, 6601, 8911, 10585, 15841, 29341, 41041, 46657, 52633,
        62745, 63973, 75361, 101101, 115921, 126217, 162401, 825265, 321197185, 5394826801ULL,
        // perfect powers
        1024, 3125, 65536, 9739ULL * 9739, 1291ULL * 1291 * 1291, 2147483648ULL, 18446744030759878681ULL,
        // close semiprimes
        1009ULL * 1013, 10007ULL * 10009, 65521ULL * 65537,
        // composites with a small factor, exercising the 64-bit path
        4294967297ULL, UINT64_MAX,
        // strong pseudoprimes to the first prime bases
        2047, 1373653, 25326001, 3215031751ULL,
    };
    if (a.randomized)  // skipped for AKS: with no small factor, step 5 would run at 40+ bits, which is slow
        composites.insert(composites.end(), {4294967291ULL * 4294967279ULL, 1000000007ULL * 998244353ULL,
                                             2152302898747ULL, 3474749660383ULL, 341550071728321ULL,
                                             3825123056546413051ULL});
    for (u64 n : composites) run.expect(n, false);

    // Labelled instances. For AKS, only the first 10 of each family and size up to 16 bits are checked.
    std::map<std::pair<std::string, int>, int> taken;
    long used = 0;
    for (const Instance& i : inst) {
        if (!a.randomized && (i.bits > 16 || taken[{i.family, i.bits}]++ >= 10)) continue;
        run.expect(i.n, i.prime);
        used++;
    }
    std::printf("    %ld labelled instances from data/instances.csv\n", used);

    if (a.randomized) {
        std::mt19937_64 rng(SEED);
        TestParams p{DEFAULT_ROUNDS, rng};
        a.fn(1000003, p);
        CHECK(p.rounds_used == DEFAULT_ROUNDS, "%s: every round runs on a prime", a.name);
        a.fn(9, p);
        CHECK(p.rounds_used == 1, "%s: 9 is rejected in round 1", a.name);
        a.fn(10, p);
        CHECK(p.rounds_used == 0, "%s: no rounds run for even n", a.name);
    }

    if (!run.fermat_false_primes.empty()) {
        std::printf("    returned prime for %zu composites (expected for Fermat), e.g.", run.fermat_false_primes.size());
        for (size_t i = 0; i < run.fermat_false_primes.size() && i < 8; i++) std::printf(" %llu", (unsigned long long)run.fermat_false_primes[i]);
        std::printf("\n");
    }
}

}  // namespace

int main(int argc, char** argv) {
    const bool slow = argc > 1 && std::strcmp(argv[1], "--slow") == 0;
    std::vector<Instance> inst;
    try {
        inst = load_instances();
    } catch (const std::exception& e) {
        std::printf("note: %s; skipping data set checks\n", e.what());
    }

    test_modarith();
    for (const Algorithm& a : ALGORITHMS) test_algorithm(a, inst, slow);

    std::printf("\n%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
