// Benchmarks and error measurements for algorithm.cpp (Miller-Rabin).
// Build and run from this folder:
//   clang++ -std=c++17 -O2 -o build/bench bench.cpp
//   build/bench all            (or one of: liars time_bits time_k families error diag)
//
// Reads data/instances.csv, writes CSVs into results/.
// Instance loading and generation are never inside a timed region.

#define MR_NO_MAIN
#include "algorithm.cpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;
static double now_ns() {
    return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}
static volatile uint64_t sink;  // stops the compiler deleting the calls

static const int REPS = 21;           // timing repetitions per point, median reported
static const uint64_t SEED = 20261102;
static const uint64_t LIAR_MAX = 20000;

struct Inst { std::string family; int bits; uint64_t n; };
static std::vector<Inst> all_inst;
static std::string out_dir = "results";

static void load(const char* path) {
    FILE* f = std::fopen(path, "r");
    if (!f) { std::fprintf(stderr, "cannot open %s (run gen_data.py first)\n", path); std::exit(1); }
    char line[1024];
    std::fgets(line, sizeof line, f);
    while (std::fgets(line, sizeof line, f)) {
        char fam[64];
        int bits;
        unsigned long long n;
        if (std::sscanf(line, "%63[^,],%d,%llu", fam, &bits, &n) == 3) all_inst.push_back({fam, bits, n});
    }
    std::fclose(f);
}

static std::vector<uint64_t> select(const std::string& fam, int lo, int hi) {
    std::vector<uint64_t> v;
    for (const Inst& i : all_inst)
        if (i.family == fam && i.bits >= lo && i.bits <= hi) v.push_back(i.n);
    return v;
}

static double pct(std::vector<double> v, double p) {
    std::sort(v.begin(), v.end());
    return v[(size_t)(p * (v.size() - 1))];
}

static FILE* open_out(const char* name) {
    std::string path = out_dir + "/" + name;
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", path.c_str()); std::exit(1); }
    return f;
}

static uint64_t gcd(uint64_t a, uint64_t b) { while (b) { uint64_t t = a % b; a = b; b = t; } return a; }

// ---------------------------------------------------------------------------
// liars: for every odd composite n up to LIAR_MAX, try EVERY base a in [2, n-2]
// and count how many fail to expose n. Gives the exact per-round error.
// The Fermat column counts bases with a^(n-1) = 1 (the whole of a Fermat round).
static void exp_liars() {
    std::printf("liars: exhaustive bases for odd composites up to %llu\n", (unsigned long long)LIAR_MAX);
    FILE* f = open_out("liars.csv");
    std::fprintf(f, "n,factors,distinct_primes,mr_liars,fermat_liars,coprime_bases,mr_liar_fraction,fermat_liar_fraction,carmichael\n");
    double worst = 0; uint64_t worst_n = 0; int carm = 0;
    for (uint64_t n = 9; n <= LIAR_MAX; n += 2) {
        std::string fs; int omega = 0, total = 0;
        uint64_t m = n;
        for (uint64_t p = 3; p * p <= m; p += 2) {
            if (m % p) continue;
            omega++;
            while (m % p == 0) { m /= p; total++; fs += (fs.empty() ? "" : "*") + std::to_string(p); }
        }
        if (m > 1) { omega++; total++; fs += (fs.empty() ? "" : "*") + std::to_string(m); }
        if (total == 1) continue;  // prime
        uint64_t d = n - 1; int s = 0;
        while ((d & 1) == 0) { d >>= 1; s++; }
        uint64_t mr = 0, fe = 0, cop = 0;
        for (uint64_t a = 2; a <= n - 2; a++) {
            if (!is_witness(a, n, d, s)) mr++;
            if (pow_mod(a, n - 1, n) == 1) fe++;
            if (gcd(a, n) == 1) cop++;
        }
        bool carmichael = (fe == cop);
        double mf = (double)mr / (double)(n - 3), ff = (double)fe / (double)(n - 3);
        if (mf > worst) { worst = mf; worst_n = n; }
        if (carmichael) carm++;
        std::fprintf(f, "%llu,%s,%d,%llu,%llu,%llu,%.6f,%.6f,%d\n", (unsigned long long)n, fs.c_str(), omega,
                     (unsigned long long)mr, (unsigned long long)fe, (unsigned long long)cop, mf, ff, carmichael ? 1 : 0);
    }
    std::fclose(f);
    std::printf("  worst Miller-Rabin liar share %.4f at n = %llu; Carmichael numbers in range: %d\n",
                worst, (unsigned long long)worst_n, carm);
}

// ---------------------------------------------------------------------------
// time_bits: ns per call against bit length, k = 1 and k = 9, for primes,
// semiprimes and random odd composites, plus the cost of one a^(n-1) mod n
// (the whole of a Fermat round) on the same primes.
static double batch_ns(const std::vector<uint64_t>& v, int k, bool fermat_core, uint64_t seed) {
    std::mt19937_64 rng(seed);
    double t0 = now_ns();
    uint64_t acc = 0;
    for (uint64_t n : v) {
        if (fermat_core) {
            std::uniform_int_distribution<uint64_t> pick(2, n - 2);
            acc += pow_mod(pick(rng), n - 1, n);
        } else {
            acc += is_probable_prime(n, k, rng);
        }
    }
    double t1 = now_ns();
    sink = acc;
    return (t1 - t0) / (double)v.size();
}

static void time_row(FILE* f, const char* fam, int bits, int k, const std::vector<uint64_t>& v, bool fermat_core) {
    batch_ns(v, k, fermat_core, 1);  // warm-up
    std::vector<double> t;
    for (int r = 0; r < REPS; r++) t.push_back(batch_ns(v, k, fermat_core, 1000 + r));
    std::fprintf(f, "%s,%d,%d,%zu,%d,%.1f,%.1f,%.1f,%.1f,%.1f\n", fam, bits, k, v.size(), REPS,
                 pct(t, 0.5), pct(t, 0.0), pct(t, 1.0), pct(t, 0.25), pct(t, 0.75));
}

static void exp_time_bits() {
    std::printf("time_bits\n");
    FILE* f = open_out("time_bits.csv");
    std::fprintf(f, "family,bits,k,instances,reps,median_ns_per_call,min_ns,max_ns,q1_ns,q3_ns\n");
    for (int bits = 8; bits <= 64; bits += 4) {
        for (const char* fam : {"prime", "semiprime", "rand_composite"}) {
            auto v = select(fam, bits, bits);
            for (int k : {1, 9}) time_row(f, fam, bits, k, v, false);
        }
        time_row(f, "fermat_core_prime", bits, 1, select("prime", bits, bits), true);
    }
    std::fclose(f);
}

// time_k: ns per call against k at 64 bits.
static void exp_time_k() {
    std::printf("time_k\n");
    FILE* f = open_out("time_k.csv");
    std::fprintf(f, "family,bits,k,instances,reps,median_ns_per_call,min_ns,max_ns,q1_ns,q3_ns\n");
    for (const char* fam : {"prime", "semiprime", "rand_composite"}) {
        auto v = select(fam, 64, 64);
        for (int k = 1; k <= 20; k++) time_row(f, fam, 64, k, v, false);
    }
    std::fclose(f);
}

// ---------------------------------------------------------------------------
// families: per-call timing and rounds used at k = 9 for inputs of about 60 to
// 64 bits. One clock read pair per call, so the numbers include the timer's own cost.
struct Subset { const char* name; const char* family; int lo, hi; };

static void exp_families() {
    std::printf("families\n");
    const Subset sets[] = {
        {"prime_64", "prime", 64, 64},
        {"rand_composite_64", "rand_composite", 64, 64},
        {"semiprime_64", "semiprime", 64, 64},
        {"carmichael_60plus", "carmichael", 60, 64},
        {"hard_60plus", "hard", 60, 64},
    };
    const int K = DEFAULT_K, CALLS = 200000;
    FILE* f = open_out("families.csv");
    FILE* h = open_out("rounds_hist.csv");
    std::fprintf(f, "subset,instances,calls,k,mean_rounds,mean_ns,p50_ns,p90_ns,p99_ns,p999_ns,max_ns,timer_tick_ns\n");
    std::fprintf(h, "subset,rounds,count,share\n");

    // the clock only moves in ticks: smallest non-zero step between two reads
    double overhead = 1e9;
    for (int i = 0; i < 1000000; i++) {
        double a = now_ns(), b = now_ns();
        if (b - a > 0 && b - a < overhead) overhead = b - a;
    }

    for (const Subset& s : sets) {
        auto v = select(s.family, s.lo, s.hi);
        if (v.empty()) { std::printf("  (skipping %s: no instances)\n", s.name); continue; }
        std::mt19937_64 rng(SEED);
        for (int i = 0; i < 5000; i++) { int r; sink = is_probable_prime(v[i % v.size()], K, rng, &r); }
        std::vector<double> t; t.reserve(CALLS);
        std::vector<long> hist(K + 1, 0);
        double rsum = 0;
        for (int i = 0; i < CALLS; i++) {
            uint64_t n = v[(size_t)i % v.size()];
            int r;
            double t0 = now_ns();
            bool res = is_probable_prime(n, K, rng, &r);
            double t1 = now_ns();
            sink = res;
            t.push_back(t1 - t0);
            hist[r]++; rsum += r;
        }
        double mean = 0; for (double x : t) mean += x; mean /= t.size();
        std::fprintf(f, "%s,%zu,%d,%d,%.4f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", s.name, v.size(), CALLS, K,
                     rsum / CALLS, mean, pct(t, 0.5), pct(t, 0.9), pct(t, 0.99), pct(t, 0.999), pct(t, 1.0), overhead);
        for (int r = 0; r <= K; r++)
            if (hist[r]) std::fprintf(h, "%s,%d,%ld,%.6f\n", s.name, r, hist[r], (double)hist[r] / CALLS);
        std::printf("  %-20s mean rounds %.3f, mean %.0f ns, p99 %.0f ns\n", s.name, rsum / CALLS, mean, pct(t, 0.99));
    }
    std::fclose(f);
    std::fclose(h);
}

// ---------------------------------------------------------------------------
// error: one run per composite family with a huge k records the round that
// first rejected n. The error rate at any k is the share of calls that took
// more than k rounds, so all k come from the same run.
static void exp_error() {
    std::printf("error\n");
    const int KMAX = 30, KREPORT = 12;
    struct E { const char* name; const char* family; int lo, hi; long calls; };
    const E sets[] = {
        {"rand_composite_64", "rand_composite", 64, 64, 2000000},
        {"semiprime_64", "semiprime", 64, 64, 2000000},
        {"carmichael_all", "carmichael", 1, 64, 2000000},
        {"carmichael_60plus", "carmichael", 60, 64, 2000000},
        {"spsp2", "spsp2", 1, 64, 2000000},
        {"spsp_multi", "spsp_multi", 1, 64, 2000000},
        {"hard_all", "hard", 1, 64, 20000000},
    };
    FILE* f = open_out("error_vs_k.csv");
    std::fprintf(f, "subset,instances,k,calls,errors,rate,bound_4_pow_minus_k\n");
    for (const E& e : sets) {
        auto v = select(e.family, e.lo, e.hi);
        if (v.empty()) { std::printf("  (skipping %s: no instances)\n", e.name); continue; }
        std::mt19937_64 rng(SEED + 7);
        std::vector<long> first(KMAX + 2, 0);  // first[r] = calls rejected in round r; first[KMAX+1] = survived all
        for (long i = 0; i < e.calls; i++) {
            int r;
            bool res = is_probable_prime(v[(size_t)i % v.size()], KMAX, rng, &r);
            first[res ? KMAX + 1 : r]++;
        }
        long surv_above = 0;
        std::vector<long> above(KMAX + 2, 0);
        for (int k = KMAX; k >= 0; k--) { surv_above += first[k + 1]; above[k] = surv_above; }
        for (int k = 1; k <= KREPORT; k++)
            std::fprintf(f, "%s,%zu,%d,%ld,%ld,%.3e,%.3e\n", e.name, v.size(), k, e.calls, above[k],
                         (double)above[k] / e.calls, std::pow(4.0, -k));
        std::printf("  %-20s k=1 error %.4f, k=3 %.2e, k=9 %.2e (%ld of %ld)\n", e.name,
                    (double)above[1] / e.calls, (double)above[3] / e.calls, (double)above[9] / e.calls, above[9], e.calls);
    }
    std::fclose(f);

    // per-number k = 1 error for every Carmichael number
    FILE* c = open_out("carmichael_k1.csv");
    std::fprintf(c, "n,bits,trials,k1_error\n");
    const int TRIALS = 3000;
    double sum = 0, worst = 0; size_t cnt = 0;
    std::mt19937_64 rng(SEED + 11);
    for (const Inst& i : all_inst) {
        if (i.family != "carmichael") continue;
        int survived = 0;
        for (int t = 0; t < TRIALS; t++) survived += is_probable_prime(i.n, 1, rng);
        double rate = (double)survived / TRIALS;
        std::fprintf(c, "%llu,%d,%d,%.5f\n", (unsigned long long)i.n, i.bits, TRIALS, rate);
        sum += rate; cnt++; if (rate > worst) worst = rate;
    }
    std::fclose(c);
    std::printf("  Carmichael k=1 error: mean %.4f over %zu numbers, worst %.4f\n", sum / cnt, cnt, worst);
}

// ---------------------------------------------------------------------------
// diag: follow-up checks added after the first full run, to explain two
// results that did not match the predictions.
//  - same_instance / per_instance: is the spread of call times on 64-bit
//    primes caused by the instances or by the machine?
//  - mulmod_chain: ns per dependent mul_mod around the 32-bit boundary.
static void exp_diag() {
    std::printf("diag\n");
    FILE* f = open_out("diag.csv");
    std::fprintf(f, "test,param,value_ns\n");
    auto v = select("prime", 64, 64);
    std::mt19937_64 rng(SEED + 3);
    const int K = DEFAULT_K, CALLS = 200000;

    std::vector<double> t;
    for (int i = 0; i < CALLS; i++) {
        double t0 = now_ns();
        bool r = is_probable_prime(v[0], K, rng);
        double t1 = now_ns();
        sink = r;
        t.push_back(t1 - t0);
    }
    for (double p : {0.5, 0.9, 0.99, 0.999})
        std::fprintf(f, "same_instance,p%g,%.1f\n", p * 100, pct(t, p));

    // Two passes over the same primes (forward, then backward), 200 calls each.
    // If the instance matters, the two medians for one prime should agree.
    auto pass = [&](bool forward) {
        std::vector<double> med(v.size());
        for (size_t j = 0; j < v.size(); j++) {
            size_t idx = forward ? j : v.size() - 1 - j;
            std::vector<double> u;
            for (int i = 0; i < 200; i++) {
                double t0 = now_ns();
                bool r = is_probable_prime(v[idx], K, rng);
                double t1 = now_ns();
                sink = r;
                u.push_back(t1 - t0);
            }
            med[idx] = pct(u, 0.5);
        }
        return med;
    };
    std::vector<double> medA = pass(true), medB = pass(false);
    for (double p : {0.01, 0.5, 0.99})
        std::fprintf(f, "per_instance_median_forward,p%g,%.1f\n", p * 100, pct(medA, p));
    FILE* g2 = open_out("diag_instances.csv");
    std::fprintf(g2, "n,s,bits_of_d,popcount_of_d,median_forward_ns,median_backward_ns\n");
    for (size_t j = 0; j < v.size(); j++) {
        uint64_t d = v[j] - 1; int s = 0;
        while ((d & 1) == 0) { d >>= 1; s++; }
        std::fprintf(g2, "%llu,%d,%d,%d,%.1f,%.1f\n", (unsigned long long)v[j], s, 64 - __builtin_clzll(d),
                     __builtin_popcountll(d), medA[j], medB[j]);
    }
    std::fclose(g2);

    // one k = 1 call per prime, bit length by bit length, to see where the step is
    for (int bits = 28; bits <= 40; bits++) {
        std::mt19937_64 g(bits * 77);
        std::vector<uint64_t> ps;
        while (ps.size() < 1000) {
            uint64_t c = (g() >> (64 - bits)) | (1ULL << (bits - 1)) | 1;
            if (is_probable_prime(c, 20, g)) ps.push_back(c);
        }
        batch_ns(ps, 1, false, 1);
        std::vector<double> tt;
        for (int r = 0; r < REPS; r++) tt.push_back(batch_ns(ps, 1, false, 1000 + r));
        std::fprintf(f, "prime_k1_by_bits,%d,%.1f\n", bits, pct(tt, 0.5));
    }

    for (int bits = 28; bits <= 40; bits++) {
        std::mt19937_64 g(bits);
        uint64_t n = (g() >> (64 - bits)) | (1ULL << (bits - 1)) | 1;
        uint64_t x = 2 + g() % 1000;
        const long N = 20000000;
        double t0 = now_ns();
        for (long i = 0; i < N; i++) x = mul_mod(x, x + 1, n);
        double t1 = now_ns();
        sink = x;
        std::fprintf(f, "mulmod_chain,%d,%.2f\n", bits, (t1 - t0) / N);
    }
    std::fclose(f);
}

int main(int argc, char** argv) {
    std::string exp = argc > 1 ? argv[1] : "all";
    if (argc > 2) out_dir = argv[2];
    load("data/instances.csv");
    std::printf("loaded %zu instances\n", all_inst.size());
    bool all = exp == "all";
    if (all || exp == "liars") exp_liars();
    if (all || exp == "time_bits") exp_time_bits();
    if (all || exp == "time_k") exp_time_k();
    if (all || exp == "families") exp_families();
    if (all || exp == "error") exp_error();
    if (all || exp == "diag") exp_diag();
    return 0;
}
