// Tests for algorithm.cpp (Miller-Rabin).
// Build and run from this folder:
//   clang++ -std=c++17 -O2 -o build/tests tests.cpp && build/tests
//
// Everything here checks answers against ground truth that does not come from
// the code under test: a sieve, hand-checked lists, or data/instances.csv
// (labelled by gen_data.py, which uses its own Python oracle).

#define MR_NO_MAIN
#include "algorithm.cpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        checks++;                                                     \
        if (!(cond)) {                                                \
            failures++;                                               \
            std::printf("    FAIL line %d: ", __LINE__);              \
            std::printf(__VA_ARGS__);                                 \
            std::printf("\n");                                        \
        }                                                             \
    } while (0)

static void section(const char* name) { std::printf("[%s]\n", name); }

// k used when a test needs "composite" answers to be reliable. 4^-20 = 9e-13.
static const int K_SURE = 20;

static std::vector<bool> sieve(uint64_t limit) {
    std::vector<bool> is_p(limit + 1, true);
    is_p[0] = false;
    if (limit >= 1) is_p[1] = false;
    for (uint64_t i = 2; i * i <= limit; i++)
        if (is_p[i])
            for (uint64_t j = i * i; j <= limit; j += i) is_p[j] = false;
    return is_p;
}

static void test_edge_cases() {
    section("edge cases");
    std::mt19937_64 rng(1);
    CHECK(!is_probable_prime(0, 9, rng), "0 is not prime");
    CHECK(!is_probable_prime(1, 9, rng), "1 is not prime");
    CHECK(is_probable_prime(2, 9, rng), "2 is prime");
    CHECK(is_probable_prime(3, 9, rng), "3 is prime");
    CHECK(!is_probable_prime(4, 9, rng), "4 is composite");
    CHECK(is_probable_prime(5, 9, rng), "5 is prime");
    CHECK(!is_probable_prime(9, K_SURE, rng), "9 is composite");
    CHECK(!is_probable_prime(25, K_SURE, rng), "25 is composite");
    CHECK(!is_probable_prime(UINT64_MAX, K_SURE, rng), "2^64-1 is composite");
    CHECK(is_probable_prime(18446744073709551557ULL, K_SURE, rng), "largest prime below 2^64");
    CHECK(is_probable_prime(7, 0, rng), "k = 0 runs no rounds, so odd n > 3 is accepted");
}

static void test_helpers() {
    section("mul_mod and pow_mod");
    CHECK(mul_mod(UINT64_MAX - 1, UINT64_MAX - 1, UINT64_MAX) == 1, "(n-1)^2 mod n = 1 with n = 2^64-1");
    CHECK(mul_mod(UINT64_MAX - 1, 2, UINT64_MAX) == UINT64_MAX - 2, "overflowing product");
    CHECK(pow_mod(2, 64, UINT64_MAX) == 1, "2^64 mod (2^64-1) = 1");
    CHECK(pow_mod(5, 0, 13) == 1, "x^0 = 1");
    CHECK(pow_mod(3, 200, 1000000007ULL) == 136318165ULL, "3^200 mod 1e9+7 (value from Python pow)");
    bool all = true;
    for (uint64_t n = 2; n < 60 && all; n++)
        for (uint64_t b = 0; b < n && all; b++) {
            uint64_t slow = 1 % n;
            for (uint64_t e = 0; e < 40; e++) {
                if (pow_mod(b, e, n) != slow) { all = false; break; }
                slow = slow * b % n;
            }
        }
    CHECK(all, "pow_mod matches naive repeated multiplication for n < 60");
}

static void test_exhaustive_small() {
    section("every n below 200000 against a sieve");
    const uint64_t LIMIT = 200000;
    auto is_p = sieve(LIMIT);
    std::mt19937_64 rng(2);
    long primes = 0, composites = 0, false_neg = 0, false_pos = 0;
    for (uint64_t n = 0; n <= LIMIT; n++) {
        if (is_p[n]) {
            primes++;
            // one round is enough to test the one-sided guarantee
            if (!is_probable_prime(n, 1, rng)) false_neg++;
        } else {
            composites++;
            if (is_probable_prime(n, K_SURE, rng)) false_pos++;
        }
    }
    std::printf("    %ld primes, %ld composites\n", primes, composites);
    CHECK(false_neg == 0, "a prime was rejected %ld times (must never happen)", false_neg);
    CHECK(false_pos == 0, "a composite survived %d rounds %ld times", K_SURE, false_pos);
}

static void test_known_numbers() {
    section("known primes, composites, Carmichael, strong pseudoprimes");
    std::mt19937_64 rng(3);
    const uint64_t primes[] = {2147483647ULL, 4294967291ULL, 2305843009213693951ULL,
                               9223372036854775783ULL, 18446744073709551557ULL};
    for (uint64_t p : primes) CHECK(is_probable_prime(p, DEFAULT_K, rng), "%llu is prime", (unsigned long long)p);

    const uint64_t composites[] = {
        18446744030759878681ULL,                    // 4294967291^2, largest prime squared below 2^64
        (uint64_t)4294967291ULL * 4294967279ULL,    // two primes just under 2^32
        4294967297ULL,                              // 2^32 + 1 = 641 * 6700417
        1000000007ULL * 998244353ULL,               // two well known primes
    };
    for (uint64_t c : composites) CHECK(!is_probable_prime(c, K_SURE, rng), "%llu is composite", (unsigned long long)c);

    const uint64_t carmichael[] = {561, 1105, 1729, 2465, 2821, 6601, 8911, 10585, 15841, 29341,
                                   41041, 46657, 52633, 62745, 63973, 75361, 101101, 115921,
                                   126217, 162401};
    for (uint64_t c : carmichael) CHECK(!is_probable_prime(c, K_SURE, rng), "Carmichael %llu", (unsigned long long)c);

    // Each is a strong pseudoprime to several fixed bases. Random bases still catch them.
    const uint64_t spsp[] = {2047, 1373653, 25326001, 3215031751ULL, 2152302898747ULL,
                             3474749660383ULL, 341550071728321ULL, 3825123056546413051ULL};
    for (uint64_t c : spsp) CHECK(!is_probable_prime(c, K_SURE, rng), "strong pseudoprime %llu", (unsigned long long)c);
}

static void test_witness_bound() {
    section("share of bases that fool one round stays below 1/4");
    double worst = 0;
    uint64_t worst_n = 0;
    for (uint64_t n = 9; n <= 5000; n += 2) {
        uint64_t d = n - 1;
        int s = 0;
        while ((d & 1) == 0) { d >>= 1; s++; }
        // skip primes using trial division
        bool prime = true;
        for (uint64_t p = 3; p * p <= n; p += 2) if (n % p == 0) { prime = false; break; }
        if (prime) {
            // no base may ever be a witness against a prime
            for (uint64_t a = 2; a <= n - 2; a++)
                if (is_witness(a, n, d, s)) { CHECK(false, "base %llu is a witness against prime %llu", (unsigned long long)a, (unsigned long long)n); return; }
            continue;
        }
        uint64_t liars = 0;
        for (uint64_t a = 2; a <= n - 2; a++)
            if (!is_witness(a, n, d, s)) liars++;
        double frac = (double)liars / (double)(n - 3);
        if (frac > worst) { worst = frac; worst_n = n; }
    }
    std::printf("    worst odd composite up to 5000: n = %llu, liar share %.4f\n", (unsigned long long)worst_n, worst);
    CHECK(worst < 0.25, "liar share %.4f is not below 1/4", worst);
}

static void test_rounds_used() {
    section("rounds_used reporting");
    std::mt19937_64 rng(4);
    int r = -1;
    CHECK(is_probable_prime(1000000007ULL, 9, rng, &r) && r == 9, "prime uses all 9 rounds (got %d)", r);
    CHECK(!is_probable_prime(1000000008ULL, 9, rng, &r) && r == 0, "even n is rejected before any round (got %d)", r);
    CHECK(!is_probable_prime(1000000007ULL * 3, 9, rng, &r) && r >= 1 && r <= 9, "composite rejected in 1..9 rounds (got %d)", r);

    std::mt19937_64 a(99), b(99);
    bool same = true;
    for (int i = 0; i < 1000; i++) {
        int ra, rb;
        bool xa = is_probable_prime(561 + 2 * i, 3, a, &ra);
        bool xb = is_probable_prime(561 + 2 * i, 3, b, &rb);
        if (xa != xb || ra != rb) same = false;
    }
    CHECK(same, "same seed gives the same answers and round counts");
}

struct Row { std::string family; int bits; uint64_t n; bool prime; };

static std::vector<Row> load(const char* path) {
    std::vector<Row> rows;
    FILE* f = std::fopen(path, "r");
    if (!f) return rows;
    char line[1024];
    std::fgets(line, sizeof line, f);  // header
    while (std::fgets(line, sizeof line, f)) {
        char fam[64], label[16];
        int bits;
        unsigned long long n;
        if (std::sscanf(line, "%63[^,],%d,%llu,%15[^,\n]", fam, &bits, &n, label) == 4)
            rows.push_back({fam, bits, n, std::strcmp(label, "prime") == 0});
    }
    std::fclose(f);
    return rows;
}

static void test_dataset(const char* path) {
    section("data/instances.csv labels");
    auto rows = load(path);
    CHECK(!rows.empty(), "could not read %s (run gen_data.py first)", path);
    std::mt19937_64 rng(5);
    long false_neg = 0, false_pos = 0, hi_bits = 0;
    for (const Row& r : rows) {
        bool got = is_probable_prime(r.n, r.prime ? DEFAULT_K : K_SURE, rng);
        if (r.prime && !got) false_neg++;
        if (!r.prime && got) {
            false_pos++;
            std::printf("    survived: %s %llu\n", r.family.c_str(), (unsigned long long)r.n);
        }
        if (r.bits >= 60) hi_bits++;
    }
    std::printf("    %zu instances checked, %ld at 60 bits or more\n", rows.size(), hi_bits);
    CHECK(false_neg == 0, "%ld primes rejected", false_neg);
    CHECK(false_pos == 0, "%ld composites survived %d rounds", false_pos, K_SURE);
}

int main(int argc, char** argv) {
    const char* data = argc > 1 ? argv[1] : "data/instances.csv";
    test_edge_cases();
    test_helpers();
    test_exhaustive_small();
    test_known_numbers();
    test_witness_bound();
    test_rounds_used();
    test_dataset(data);
    std::printf("\n%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
