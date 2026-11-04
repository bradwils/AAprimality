// Experiments comparing Fermat, Miller-Rabin and AKS on the shared data set.
// Run from the repo root after scripts/gen_data.py:
//
//   build/bench [all|time_bits|time_k|tails|error|carmichael|liars|aks_steps|aks_32bit]
//               [--max-seconds S] [--calls N]
//
// Each experiment writes results/<name>.csv. The same instances and seeds are
// used for every algorithm, and the timers wrap only the calls being measured.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "aks.hpp"
#include "instances.hpp"
#include "modarith.hpp"
#include "primality.hpp"

namespace {

const u64 SEED = 20261102;
const int K = DEFAULT_ROUNDS;
double max_seconds = 2.0; // larger sizes are skipped once one call takes longer than this
long calls = 1000000; // calls per subset in the error experiment
volatile u64 sink; // prevents the compiler from removing calls whose result is unused
std::vector<Instance> all;

struct Subset { const char* name; const char* family; int lo, hi; };

double now_ns() {
    return std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

double pct(std::vector<double> v, double p) {
    std::sort(v.begin(), v.end());
    return v[(size_t)(p * (v.size() - 1))];
}

FILE* open_out(const char* name, const char* header) {
    std::string path = std::string("results/") + name;
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) { std::fprintf(stderr, "cannot write %s (run from the repo root)\n", path.c_str()); std::exit(1); }
    std::fprintf(f, "%s\n", header);
    std::printf("%s\n", name);
    return f;
}

const Algorithm& algo(const char* name) {
    for (const Algorithm& a : ALGORITHMS)
        if (std::strcmp(a.name, name) == 0) return a;
    std::abort();
}

const Algorithm* RANDOMIZED[] = {&algo("fermat"), &algo("miller-rabin")};

struct Cell { int instances = 0, reps = 0, wrong = 0; double median = 0, lo = 0, hi = 0; };

// Returns ns per call. Each repeat times a whole batch of instances, and the
// median repeat is kept. A first, untimed pass checks every result and sets the
// batch: every instance and 21 repeats for fast algorithms, or as many instances
// as fit in 0.2 s and 3 repeats for slow ones.
Cell time_cell(const Algorithm& a, const std::vector<Instance>& v, int k) {
    std::mt19937_64 rng(SEED);
    TestParams p{k, rng};
    Cell c;
    double start = now_ns();
    while (c.instances < (int)v.size() && (c.instances == 0 || now_ns() - start < 2e8)) {
        c.wrong += a.fn(v[c.instances].n, p) != v[c.instances].prime;
        c.instances++;
    }
    c.reps = now_ns() - start < 1e8 ? 21 : 3;
    std::vector<double> t;
    for (int r = 0; r < c.reps; r++) {
        u64 acc = 0;
        double t0 = now_ns();
        for (int i = 0; i < c.instances; i++) acc += a.fn(v[i].n, p);
        t.push_back((now_ns() - t0) / c.instances);
        sink = acc;
    }
    c.median = pct(t, 0.5), c.lo = pct(t, 0), c.hi = pct(t, 1);
    return c;
}

void write_cell(FILE* f, const Algorithm& a, const char* fam, int bits, int k, const Cell& c) {
    std::fprintf(f, "%s,%s,%d,%d,%d,%d,%d,%.1f,%.1f,%.1f\n", a.name, fam, bits, k, c.instances, c.reps, c.wrong,
                 c.median, c.lo, c.hi);
}

#define CELL_HEADER "algorithm,family,bits,k,instances,reps,wrong,median_ns,min_ns,max_ns"

// Time per call at each bit length, for every algorithm (k = 9 for Fermat and Miller-Rabin).
// In this and the next two experiments the algorithms are timed back to back for
// each input set, so any drift in clock speed or temperature affects all of them.
void time_bits() {
    FILE* f = open_out("time_bits.csv", CELL_HEADER);
    for (const char* fam : {"prime", "semiprime", "rand_composite", "prime_square"}) {
        bool stopped[std::size(ALGORITHMS)] = {};
        for (int bits = 8; bits <= 64; bits += 4) {
            auto v = of_family(all, fam, bits, bits);
            if (v.empty()) continue;
            for (size_t i = 0; i < std::size(ALGORITHMS); i++) {
                const Algorithm& a = ALGORITHMS[i];
                if (stopped[i]) continue;
                int k = a.randomized ? K : 0;
                Cell c = time_cell(a, v, k);
                write_cell(f, a, fam, bits, k, c);
                std::printf("  %-12s %-14s %2d bits %14.0f ns\n", a.name, fam, bits, c.median);
                stopped[i] = c.median > max_seconds * 1e9;
            }
        }
    }
    std::fclose(f);
}

// Time per call for k = 1..20 on 60-64 bit inputs.
void time_k() {
    FILE* f = open_out("time_k.csv", CELL_HEADER);
    const Subset sets[] = {{"prime", "prime", 64, 64}, {"rand_composite", "rand_composite", 64, 64},
                           {"carmichael", "carmichael", 60, 64}};
    for (const Subset& s : sets) {
        auto v = of_family(all, s.family, s.lo, s.hi);
        for (int k = 1; k <= 20; k++)
            for (const Algorithm* a : RANDOMIZED) write_cell(f, *a, s.name, s.lo, k, time_cell(*a, v, k));
    }
    std::fclose(f);
}

const Subset FAMILIES_64[] = {
    {"prime_64", "prime", 64, 64},
    {"rand_composite_64", "rand_composite", 64, 64},
    {"semiprime_64", "semiprime", 64, 64},
    {"carmichael_60plus", "carmichael", 60, 64},
    {"hard_60plus", "hard", 60, 64},
};

// Times each call separately at k = 9 to measure the slowest calls.
void tails() {
    double tick = 1e9; // smallest non-zero step of the clock
    for (int i = 0; i < 1000000; i++) {
        double a = now_ns(), b = now_ns();
        if (b > a) tick = std::min(tick, b - a);
    }
    FILE* f = open_out("tails.csv", "algorithm,subset,instances,calls,k,mean_rounds,wrong,"
                                    "mean_ns,p50_ns,p90_ns,p99_ns,p999_ns,max_ns,timer_tick_ns");
    const int CALLS = 200000;
    for (const Subset& s : FAMILIES_64)
        for (const Algorithm* a : RANDOMIZED) {
            auto v = of_family(all, s.family, s.lo, s.hi);
            std::mt19937_64 rng(SEED);
            TestParams p{K, rng};
            for (int i = 0; i < 5000; i++) sink = a->fn(v[i % v.size()].n, p);
            std::vector<double> t(CALLS);
            double rounds = 0;
            long wrong = 0;
            for (int i = 0; i < CALLS; i++) {
                const Instance& in = v[i % v.size()];
                double t0 = now_ns();
                bool res = a->fn(in.n, p);
                t[i] = now_ns() - t0;
                rounds += p.rounds_used;
                wrong += res != in.prime;
            }
            double mean = std::accumulate(t.begin(), t.end(), 0.0) / CALLS;
            std::fprintf(f, "%s,%s,%zu,%d,%d,%.4f,%ld,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", a->name, s.name,
                         v.size(), CALLS, K, rounds / CALLS, wrong, mean, pct(t, .5), pct(t, .9), pct(t, .99),
                         pct(t, .999), pct(t, 1), tick);
        }
    std::fclose(f);
}

// Error rate for every k from one run. Each call runs up to KMAX rounds, and the
// number of rounds that returned prime is recorded. The error rate at k is the
// fraction of calls in which the first k rounds all returned prime.
void error() {
    const int KMAX = 20;
    FILE* f = open_out("error_vs_k.csv", "algorithm,subset,instances,calls,k,errors,rate,bound_4_pow_minus_k");
    const Subset sets[] = {
        {"rand_composite_64", "rand_composite", 64, 64}, {"semiprime_64", "semiprime", 64, 64},
        {"carmichael", "carmichael", 1, 64}, {"spsp2", "spsp2", 1, 64},
        {"spsp_multi", "spsp_multi", 1, 64}, {"hard", "hard", 1, 64},
    };
    for (const Algorithm* a : RANDOMIZED)
        for (const Subset& s : sets) {
            auto v = of_family(all, s.family, s.lo, s.hi);
            std::mt19937_64 rng(SEED + 7);
            TestParams p{KMAX, rng};
            std::vector<long> passed(KMAX + 1, 0); // passed[j] = calls in which exactly j rounds returned prime
            for (long i = 0; i < calls; i++)
                passed[a->fn(v[i % v.size()].n, p) ? KMAX : p.rounds_used - 1]++;
            long errors = 0;
            std::vector<long> at_least(KMAX + 1);
            for (int k = KMAX; k >= 1; k--) at_least[k] = errors += passed[k];
            for (int k = 1; k <= KMAX; k++)
                std::fprintf(f, "%s,%s,%zu,%ld,%d,%ld,%.3e,%.3e\n", a->name, s.name, v.size(), calls, k,
                             at_least[k], (double)at_least[k] / calls, std::pow(4.0, -k));
            std::printf("  %-12s %-18s error k=1 %.4f  k=9 %.2e\n", a->name, s.name,
                        (double)at_least[1] / calls, (double)at_least[9] / calls);
        }
    std::fclose(f);
}

// Probability that one round returns prime for each Carmichael number, for Fermat and Miller-Rabin.
void carmichael() {
    const int TRIALS = 2000;
    FILE* f = open_out("carmichael_k1.csv", "n,bits,trials,fermat_k1_error,mr_k1_error");
    std::mt19937_64 rng(SEED + 11);
    TestParams p{1, rng};
    for (const Instance& in : of_family(all, "carmichael", 1, 64)) {
        int fe = 0, mr = 0;
        for (int t = 0; t < TRIALS; t++) {
            fe += fermat(in.n, p);
            mr += miller_rabin(in.n, p);
        }
        std::fprintf(f, "%llu,%d,%d,%.5f,%.5f\n", (unsigned long long)in.n, in.bits, TRIALS,
                     (double)fe / TRIALS, (double)mr / TRIALS);
    }
    std::fclose(f);
}

// For every odd composite n up to 20000, counts the bases a in [2, n-2] for which
// one round returns prime. Computed with powmod directly, not the implementations.
void liars() {
    const u64 LIMIT = 20000;
    FILE* f = open_out("liars.csv", "n,fermat_liar_fraction,mr_liar_fraction,carmichael");
    for (u64 n = 9; n <= LIMIT; n += 2) {
        bool prime = true;
        for (u64 q = 3; q * q <= n && prime; q += 2) prime = n % q != 0;
        if (prime) continue;
        u64 d = n - 1;
        int s = 0;
        while ((d & 1) == 0) d >>= 1, s++;
        u64 fe = 0, mr = 0, coprime = 0;
        for (u64 a = 2; a <= n - 2; a++) {
            fe += powmod(a, n - 1, n) == 1;
            coprime += std::gcd(a, n) == 1;
            u64 x = powmod(a, d, n);
            bool liar = x == 1 || x == n - 1;
            for (int r = 1; r < s && !liar; r++) liar = (x = mulmod(x, x, n)) == n - 1;
            mr += liar;
        }
        std::fprintf(f, "%llu,%.6f,%.6f,%d\n", (unsigned long long)n, fe / (n - 3.0), mr / (n - 3.0), fe == coprime);
    }
    std::fclose(f);
}

// AKS time per step: one call per bit length.
void aks_steps() {
    FILE* f = open_out("aks_steps.csv", "family,bits,n,verdict,r,log2n_sq,a_limit,a_checked,squarings,"
                                        "t_step1_s,t_step2_s,t_step3_s,t_step5_s,total_s");
    for (const char* fam : {"prime", "semiprime"})
        for (int bits = 8; bits <= 64; bits += 4) {
            auto v = of_family(all, fam, bits, bits);
            AksStats s;
            double t0 = now_ns();
            aks_with_stats(v[0].n, &s);
            double total = (now_ns() - t0) / 1e9, lg = std::log2((double)v[0].n);
            std::fprintf(f, "%s,%d,%llu,%s,%llu,%.1f,%llu,%llu,%llu,%.3e,%.3e,%.3e,%.3e,%.3e\n", fam, bits,
                         (unsigned long long)v[0].n, s.verdict, (unsigned long long)s.r, lg * lg,
                         (unsigned long long)s.a_limit, (unsigned long long)s.a_checked,
                         (unsigned long long)s.squarings, s.t_step1, s.t_step2, s.t_step3, s.t_step5, total);
            if (total > max_seconds) break;
        }
    std::fclose(f);
}

// AKS on balanced semiprimes of 29-35 bits. Below 2^32 the coefficient products
// are summed in 64-bit arithmetic; from 2^32 each one needs a 128-bit remainder.
// time / (squarings * r^2) is the cost of one coefficient multiply-add.
void aks_32bit() {
    FILE* f = open_out("aks_32bit.csv", "bits,n,r,a_checked,squarings,correct,median_s,ns_per_coefficient_op");
    for (const Instance& in : of_family(all, "semiprime_near_2^32", 1, 64)) {
        AksStats s;
        bool correct = aks_with_stats(in.n, &s) == in.prime;
        std::vector<double> t;
        for (int r = 0; r < 5; r++) {
            double t0 = now_ns();
            sink = aks_with_stats(in.n, nullptr);
            t.push_back((now_ns() - t0) / 1e9);
        }
        double med = pct(t, 0.5);
        std::fprintf(f, "%d,%llu,%llu,%llu,%llu,%d,%.4e,%.3f\n", in.bits, (unsigned long long)in.n,
                     (unsigned long long)s.r, (unsigned long long)s.a_checked, (unsigned long long)s.squarings,
                     correct, med, med * 1e9 / ((double)s.squarings * s.r * s.r));
        std::printf("  aks %2d bits  r %4llu  %8.4f s\n", in.bits, (unsigned long long)s.r, med);
    }
    std::fclose(f);
}

}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::string exp = "all";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--max-seconds" && i + 1 < argc) max_seconds = std::atof(argv[++i]);
        else if (arg == "--calls" && i + 1 < argc) calls = std::atol(argv[++i]);
        else exp = arg;
    }
    try {
        all = load_instances();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    struct { const char* name; void (*fn)(); } experiments[] = {
        {"time_bits", time_bits}, {"time_k", time_k}, {"tails", tails}, {"error", error},
        {"carmichael", carmichael}, {"liars", liars}, {"aks_steps", aks_steps}, {"aks_32bit", aks_32bit},
    };
    bool ran = false;
    for (auto& e : experiments)
        if (exp == "all" || exp == e.name) e.fn(), ran = true;
    if (!ran) {
        std::fprintf(stderr, "unknown experiment %s\n", exp.c_str());
        return 1;
    }
    return 0;
}
