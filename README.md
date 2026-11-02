# AAprimality: AKS vs Fermat vs Miller-Rabin

Group project for Assignment 3 (algorithm comparison), theme 2: **deterministic vs randomized**.
Problem: decide whether a 64-bit integer `n` is prime. See `guidance.md` for the assignment brief and the
decision rules for tests, docs and benchmarks.

| Algorithm | Kind | Primary author | Folder | Status |
|---|---|---|---|---|
| AKS | Deterministic | Ben | `implementations/aks/` | implemented, tested, benchmarked |
| Fermat | Probabilistic (fooled by Carmichael numbers) | CJ | `implementations/fermat_primality/` | not started |
| Miller-Rabin | Probabilistic (works for all numbers) | Brad | `implementations/miller_rabin/` | not started |

(Authors are taken from the placeholder files `BEN`, `CJ`, `BRAD` in each folder; edit this table if that is wrong.)

## Repository layout

```
Makefile                 builds every implementation, the shared benchmark and all tests
common/
  common.hpp             shared interface: bool f(uint64_t n) wrapped in an Algorithm record
  oracle.hpp             independent trial-division oracle used by all tests and the benchmark
bench/
  bench.cpp              unified benchmark harness (one registry, same inputs for every algorithm)
implementations/
  aks/                   one self-contained module per algorithm (same layout in each):
    include/aks.hpp        public header
    src/aks.cpp            implementation
    tests/test_aks.cpp     tests (oracle: common/oracle.hpp)
    scripts/, results/     AKS-specific plots and measured data
    docs/                  AKS video script
    README.md              AKS design notes, limits, measured results
  fermat_primality/      (placeholder; CJ)
  miller_rabin/          (placeholder; Brad)
results/                 default CSV output location for `make bench` (created on first run)
guidance.md              assignment brief, decision rules, AI commit log
HELPED.md                log of notable AI-agent changes (AI-use policy)
```

## Build and run

Needs a C++17 compiler with `unsigned __int128` (GCC or Clang) and GNU `make`. Run everything from the repo root.

```
make                       # build build/bench and every test (-O2)
make test                  # run all tests (fast tier, ~10 s for AKS)
TEST_ARGS=--slow make test # long tiers too (AKS: ~2 minutes)
make list                  # show which implementation folders the Makefile found
make clean
```

An implementation is picked up automatically if its folder has `src/*.cpp` (and optionally `tests/test_*.cpp`).

### Benchmark

```
make bench ARGS="--bits-min 8 --bits-max 26 --bits-step 2 --reps 3 --instances 3 \
                 --families prime --csv results/bench.csv --max-seconds 40"
```

Options: `--bits-min/--bits-max/--bits-step`, `--reps`, `--instances`, `--seed`, `--families`
(`prime, semiprime, random_composite, prime_square`), `--algos`, `--csv`, `--max-seconds`
(stop climbing bit sizes for a family once the median exceeds this).

CSV columns: algorithm, family, bits, instance, n, expected_prime, result, correct, median_s, min_s, max_s, reps,
then AKS-only diagnostics (blank for other algorithms): aks_verdict, aks_r, aks_a_limit, aks_a_checked,
aks_squarings, aks_t1_s .. aks_t5_s.

Reproduce the committed AKS data (from the repo root):

```
make bench ARGS="--bits-min 8 --bits-max 26 --bits-step 2 --reps 3 --instances 3 --families prime --csv implementations/aks/results/aks_primes.csv --max-seconds 40"
make bench ARGS="--bits-min 8 --bits-max 40 --bits-step 4 --reps 5 --instances 5 --families semiprime,random_composite,prime_square --csv implementations/aks/results/aks_composites.csv --max-seconds 40"
make bench ARGS="--bits-min 30 --bits-max 35 --bits-step 1 --reps 5 --instances 5 --families semiprime --csv implementations/aks/results/aks_semiprime_cliff.csv"
python3 implementations/aks/scripts/plot_aks.py     # needs matplotlib
```

## Methodology (applies to every algorithm)

- **Inputs:** generated once per (family, bits, instance) from a deterministic seed, identical for every algorithm and every run.
- **Families:** random primes of exact bit length; random odd composites; semiprimes with balanced factors; squares of primes.
  A **Carmichael-number family is still to be added**: it is the key adversarial input for Fermat and must be in the benchmark before Fermat is compared.
- **Timing:** `std::chrono::steady_clock` around the single `is_prime` call only; one untimed warm-up call; median of `--reps` timed calls; median over instances in the printed summary.
- **Correctness:** every answer is compared with trial division; mismatches are printed and flagged in the CSV `correct` column.
- **Same build for all:** one compiler, `-O2`, one machine.
- **Reference machine for the committed AKS data:** Intel Xeon @ 2.80 GHz, 1 vCPU, Linux 6.18, g++ 13.3. Re-record this if you rerun elsewhere.

## Adding an algorithm (Fermat, Miller-Rabin)

1. Put code in your folder using the same layout: `include/<name>.hpp`, `src/<name>.cpp`, `tests/test_<name>.cpp`. Name the primary author in each file header and in your folder README. Expose `bool is_prime(uint64_t n)` in your own namespace.
2. In `bench/bench.cpp`, add `#include "<name>.hpp"`, write a one-line adapter if needed, and uncomment or add your entry in `registry()`. The Makefile needs no change.
3. Test against `trial_division` from `common/oracle.hpp`, including Carmichael numbers (561, 1105, 1729, 2465, ...).
4. Log notable AI-assisted changes in `HELPED.md`.

## Files deliberately not committed

`CLAUDE.md` and large media files (`*.mp4`, `*.mov`, `*.wav`, ...) are in `.gitignore`. The assignment video is submitted separately.

## AI use

Notable AI-agent changes are logged in `HELPED.md`; the rules and the log of AI-authored commits are in `guidance.md`. No AI co-author credits are added to commits.
