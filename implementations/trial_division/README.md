# Primality testing: AKS vs Fermat vs Miller-Rabin

Group project for Programming Assignment 2 (Theme 2: deterministic vs randomized).
This repository currently contains the **AKS** implementation, the shared test and benchmark
framework, and the project documentation. Fermat and Miller-Rabin plug into the same framework
(see "Adding another algorithm").

| Algorithm | Kind | Primary author | Status |
|---|---|---|---|
| AKS | Deterministic | _fill in name_ | implemented (`src/aks.cpp`) |
| Fermat | Probabilistic (fooled by Carmichael numbers) | _fill in name_ | not started |
| Miller-Rabin | Probabilistic (works for all numbers) | _fill in name_ | not started |

## Repository layout

```
include/aks.hpp        AKS public interface (+ Stats struct for instrumentation)
include/common.hpp     Shared Algorithm interface: bool f(uint64_t n)
src/aks.cpp            AKS implementation
tests/test_aks.cpp     Correctness tests (oracle: trial division)
bench/bench.cpp        Unified benchmark harness (writes CSV)
results/               CSV output and logs from benchmark runs
docs/                  Video script and other notes
HELPED.md              Log of notable AI-agent changes (assignment AI-use policy)
guidance.md            Decision rules for docs/tests/benchmarks; AI-commit log
```

## Build and run

Requirements: a C++17 compiler (tested with g++ 13.3) supporting `unsigned __int128` (GCC/Clang), and `make`.

```
make              # builds build/test_aks and build/bench with -O2
make test         # fast correctness suite (about 10 seconds)
./build/test_aks --slow   # adds larger primes and a bigger exhaustive range (several minutes)
```

Benchmark (example that reproduces the AKS prime-timing curve):

```
mkdir -p results
./build/bench --bits-min 8 --bits-max 26 --bits-step 2 --reps 3 --instances 3 \
              --families prime --csv results/aks_primes.csv --max-seconds 40
```

Options: `--bits-min/--bits-max/--bits-step`, `--reps`, `--instances`, `--seed`, `--families`
(`prime, semiprime, random_composite, prime_square`), `--algos`, `--csv`, `--max-seconds`
(stop climbing bit sizes for a family once the median exceeds this).

CSV columns: algorithm, family, bits, instance, n, expected_prime, result, correct, median_s, min_s, max_s, reps,
then AKS-only diagnostics: aks_verdict (which step decided), aks_r, aks_a_limit, aks_a_checked, aks_squarings,
and per-step times aks_t1_s (perfect power), aks_t2_s (find r), aks_t3_s (gcd loop), aks_t5_s (polynomial checks).

## The AKS implementation in one page

1. If `n = a^b` for some `b >= 2`, return composite.
2. Find the smallest `r` with `ord_r(n) > log2(n)^2`.
3. If any `a <= r` has `1 < gcd(a, n) < n`, return composite.
4. If `n <= r`, return prime.
5. For `a = 1 .. ceil(sqrt(phi(r)) * log2 n)`: check `(X + a)^n == X^n + a` in `(Z/nZ)[X] / (X^r - 1)`. Any failure means composite.
6. Otherwise return prime.

Design choices: coefficients are `uint64_t` and products use `unsigned __int128`, so every 64-bit `n` is
supported. Polynomials are dense arrays of `r` coefficients and squaring is schoolbook `O(r^2)`;
multiplying by `(X + a)` is done directly in `O(r)`. For `n < 2^32` each output coefficient of a square is
accumulated as one dot product with a single modular reduction. There is no FFT/NTT and no multithreading, on
purpose: this is the plain textbook algorithm so the comparison measures the algorithm, not engineering tricks.
Measured practical limit on the reference machine: a 26-bit prime takes about 6 s, the 31-bit prime 2^31-1 about 12 s and the largest 32-bit prime 2^32-5 about 15 s. Above 2^32 the arithmetic switches to a slower 128-bit path (about 17x slower per exponentiation, see "Known limitations"), so primes of 33+ bits take minutes or more.

## Known limitations

- **32-bit cliff.** For `n >= 2^32` coefficient products need 128-bit division, which is far slower than the 64-bit lazy-accumulation path used below 2^32. Semiprime timing jumps from 15.7 ms (32 bits) to 264 ms (33 bits) while `r` barely changes (see `results/aks_semiprime_cliff.csv`). This is an implementation artefact, not something AKS's asymptotics predict; keep it in mind when comparing against Fermat and Miller-Rabin.
- Schoolbook polynomial multiplication, single-threaded, no NTT: deliberate, but it makes AKS slower than optimised implementations.
- `aks_t*` step times in the CSV come from the last timed repetition, while `median_s` is the median over repetitions, so their sum can differ from `median_s` by a few percent.

## Methodology (for reproducibility)

- Inputs: generated once per (family, bits, instance) from a deterministic seed; identical for every algorithm.
- Families: random primes of exact bit length; random odd composites; semiprimes with balanced factors; squares of primes.
  Carmichael numbers should be added as a family once Fermat exists (they are the key adversarial input for it).
- Timing: `std::chrono::steady_clock` around the single call to `is_prime`; one untimed warm-up call; the median of
  `--reps` timed calls; median over instances for the printed summary.
- Correctness: every answer is compared with trial division; mismatches are printed and flagged in the CSV (`correct` column).
- Hardware of the reference run: Intel Xeon @ 2.80 GHz, 1 vCPU, Linux 6.18, g++ 13.3, `-O2`. Re-record this if you rerun on another machine.

## Adding another algorithm (Fermat, Miller-Rabin)

1. Add `include/<name>.hpp` and `src/<name>.cpp` exposing `bool is_prime(uint64_t n)` and name the primary author in the file header.
2. Add it to `registry()` in `bench/bench.cpp` (a commented example is there) and to the `Makefile`.
3. Add a test file that checks against the trial-division oracle, including Carmichael numbers.

## Files deliberately not committed

`CLAUDE.md` (and `claude.md`) agent instruction files are ignored, as are large media files
(`*.mp4`, `*.mov`, `*.mkv`, `*.wav`, ...). See `.gitignore`. The assignment video is submitted separately.

## AI use

AI assistance is disclosed per the course policy. Notable AI changes are logged in `HELPED.md`; decision rules
and the log of AI-authored commits are in `guidance.md`. No AI co-author credits are added to commits.
