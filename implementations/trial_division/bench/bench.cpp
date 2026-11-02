// Unified benchmark harness for the primality-testing comparison.
//
//   ./build/bench --bits-min 8 --bits-max 26 --bits-step 2 --reps 5 --instances 3
//                 --families prime,semiprime,random_composite,prime_square
//                 --csv results/aks.csv --max-seconds 20
//
// Fairness rules built into the harness (keep these when adding Fermat / Miller-Rabin):
//   * Inputs are generated ONCE per (family, bits, instance) from a fixed seed, before any
//     timing, and the SAME n is handed to every registered algorithm.
//   * Only the call to the algorithm is inside the timed region (no I/O, no generation).
//   * One untimed warm-up call, then `reps` timed calls; the median is reported.
//   * Every result is checked against a trial-division oracle (independent of all algorithms).
//     A wrong answer is recorded in the CSV and printed loudly.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "aks.hpp"
#include "common.hpp"

using u64 = uint64_t;

// ---------------- AKS adapter (captures Stats of the most recent call) ----------------
static aks::Stats g_aks_stats;
static bool aks_adapter(u64 n) { return aks::is_prime(n, &g_aks_stats); }

// ---------------- Algorithm registry: add Fermat and Miller-Rabin here ----------------
static std::vector<Algorithm> registry() {
    return {
        {"aks", "<AKS author>", aks_adapter},
        // {"fermat", "<Fermat author>", fermat::is_prime_adapter},
        // {"miller_rabin", "<MR author>", miller_rabin::is_prime_adapter},
    };
}

// ---------------- Oracle + input generation ----------------
static bool trial_division(u64 n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0) return false;
    for (u64 d = 3; d * d <= n; d += 2)
        if (n % d == 0) return false;
    return true;
}

static u64 random_with_bits(std::mt19937_64& rng, int bits) {
    u64 lo = 1ULL << (bits - 1);
    return lo | (rng() & (lo - 1)) | 1ULL;  // exactly `bits` bits, odd
}
static u64 random_prime(std::mt19937_64& rng, int bits) {
    for (;;) {
        u64 c = random_with_bits(rng, bits);
        if (trial_division(c)) return c;
    }
}

struct Instance { std::string family; int bits; int index; u64 n; };

static bool make_instance(const std::string& family, int bits, std::mt19937_64& rng, u64& out) {
    if (family == "prime") { out = random_prime(rng, bits); return true; }
    if (family == "random_composite") {
        for (;;) { u64 c = random_with_bits(rng, bits); if (!trial_division(c)) { out = c; return true; } }
    }
    if (family == "semiprime") {  // p*q with p,q of ~bits/2 bits each
        if (bits < 6) return false;
        int b1 = bits / 2, b2 = bits - b1 + 1;
        for (int tries = 0; tries < 100; ++tries) {
            u64 p = random_prime(rng, b1), q = random_prime(rng, b2);
            u64 n = p * q;
            if (p != q && 63 - __builtin_clzll(n) + 1 == bits) { out = n; return true; }
        }
        return false;
    }
    if (family == "prime_square") {  // exercises the perfect-power step
        if (bits < 6) return false;
        int b = (bits + 1) / 2;
        u64 p = random_prime(rng, b);
        out = p * p;
        return true;
    }
    std::fprintf(stderr, "unknown family '%s'\n", family.c_str());
    std::exit(2);
}

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> v; std::stringstream ss(s); std::string t;
    while (std::getline(ss, t, ',')) if (!t.empty()) v.push_back(t);
    return v;
}

int main(int argc, char** argv) {
    int bits_min = 8, bits_max = 24, bits_step = 2, reps = 5, instances = 3;
    u64 seed = 12345;
    double max_seconds = 20.0;
    std::string csv_path = "results/bench.csv";
    std::string families_arg = "prime,semiprime,random_composite,prime_square";
    std::string algos_arg;  // empty = all

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", a.c_str()); std::exit(2);} return argv[++i]; };
        if      (a == "--bits-min")    bits_min = std::stoi(next());
        else if (a == "--bits-max")    bits_max = std::stoi(next());
        else if (a == "--bits-step")   bits_step = std::stoi(next());
        else if (a == "--reps")        reps = std::stoi(next());
        else if (a == "--instances")   instances = std::stoi(next());
        else if (a == "--seed")        seed = std::stoull(next());
        else if (a == "--max-seconds") max_seconds = std::stod(next());
        else if (a == "--csv")         csv_path = next();
        else if (a == "--families")    families_arg = next();
        else if (a == "--algos")       algos_arg = next();
        else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
    }

    std::vector<std::string> families = split(families_arg);
    std::vector<std::string> wanted_algos = split(algos_arg);
    std::vector<Algorithm> algos;
    for (auto& al : registry())
        if (wanted_algos.empty() || std::find(wanted_algos.begin(), wanted_algos.end(), al.name) != wanted_algos.end())
            algos.push_back(al);

    std::ofstream csv(csv_path);
    if (!csv) { std::fprintf(stderr, "cannot open %s (does the directory exist?)\n", csv_path.c_str()); return 1; }
    csv << "algorithm,family,bits,instance,n,expected_prime,result,correct,median_s,min_s,max_s,reps,"
           "aks_verdict,aks_r,aks_a_limit,aks_a_checked,aks_squarings,aks_t1_s,aks_t2_s,aks_t3_s,aks_t5_s\n";

    std::printf("seed=%llu reps=%d instances=%d max_seconds=%.1f\n", (unsigned long long)seed, reps, instances, max_seconds);
    int wrong = 0;

    for (auto& fam : families) {
        std::map<std::string, bool> stopped;  // algorithm -> stop climbing bits for this family
        for (int bits = bits_min; bits <= bits_max; bits += bits_step) {
            // Seed depends only on (seed, family, bits) so inputs are identical across algorithms and runs.
            std::mt19937_64 rng(seed ^ (std::hash<std::string>{}(fam) * 1000003ULL) ^ (u64)bits * 0x9E3779B97F4A7C15ULL);
            std::vector<Instance> insts;
            for (int k = 0; k < instances; ++k) {
                u64 n;
                if (!make_instance(fam, bits, rng, n)) break;
                insts.push_back({fam, bits, k, n});
            }
            for (auto& al : algos) {
                if (stopped[al.name]) continue;
                std::vector<double> medians;
                for (auto& in : insts) {
                    const bool expected = trial_division(in.n);
                    volatile bool sink = al.fn(in.n);  // warm-up (untimed)
                    (void)sink;
                    std::vector<double> times;
                    bool result = false;
                    for (int rep = 0; rep < reps; ++rep) {
                        auto t0 = std::chrono::steady_clock::now();
                        result = al.fn(in.n);
                        auto t1 = std::chrono::steady_clock::now();
                        times.push_back(std::chrono::duration<double>(t1 - t0).count());
                    }
                    std::sort(times.begin(), times.end());
                    double med = times[times.size() / 2];
                    medians.push_back(med);
                    bool ok = (result == expected);
                    if (!ok) { ++wrong; std::printf("!!! WRONG ANSWER: %s n=%llu got=%d expected=%d\n", al.name, (unsigned long long)in.n, result, expected); }
                    csv << al.name << ',' << fam << ',' << bits << ',' << in.index << ',' << in.n << ',' << expected << ',' << result << ',' << ok << ','
                        << med << ',' << times.front() << ',' << times.back() << ',' << reps;
                    if (std::strcmp(al.name, "aks") == 0)
                        csv << ',' << aks::verdict_name(g_aks_stats.verdict) << ',' << g_aks_stats.r << ',' << g_aks_stats.a_limit << ','
                            << g_aks_stats.a_checked << ',' << g_aks_stats.poly_squarings << ',' << g_aks_stats.t_step1_s << ','
                            << g_aks_stats.t_step2_s << ',' << g_aks_stats.t_step3_s << ',' << g_aks_stats.t_step5_s;
                    else csv << ",,,,,,,,";
                    csv << '\n';
                    csv.flush();
                }
                if (medians.empty()) continue;
                std::sort(medians.begin(), medians.end());
                double m = medians[medians.size() / 2];
                std::printf("%-8s %-17s bits=%2d  median=%.3es\n", al.name, fam.c_str(), bits, m);
                if (m > max_seconds) { stopped[al.name] = true; std::printf("  (stopping %s on %s: exceeded --max-seconds)\n", al.name, fam.c_str()); }
            }
        }
    }
    std::printf(wrong ? "FINISHED WITH %d WRONG ANSWERS\n" : "finished: all answers matched the oracle\n", wrong);
    return wrong ? 1 : 0;
}
