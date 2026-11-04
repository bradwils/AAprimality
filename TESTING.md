# Testing methodology

All three algorithms share one data set, one test program and one benchmark program. This file is the source for report section 3 (methodology). `sh run_all.sh` reruns everything.

## Inputs: `data/instances.csv`

`scripts/gen_data.py` generates the inputs from a fixed seed, so the same numbers are used in every run and for every algorithm.

Each number is labelled prime or composite by a separate Python prime check. This check is a Miller-Rabin test with the first 12 primes as fixed bases, which is proven correct for every n below 3.3·10^24. Before use, it is checked against a sieve up to 2·10^6. The labels never come from the C++ code being tested.

| Family | Sizes | Count | Why it is included |
|---|---|---|---|
| `prime` | 8–64 bits, step 4 | 1000 per size | The slowest case for every algorithm: there is no early exit, so every round (or every AKS step) runs |
| `rand_composite` | 8–64 bits | 1000 per size | A typical input. Most have a small factor |
| `semiprime` | 8–64 bits | 1000 per size | Two prime factors of similar size, so no factor is found in AKS's gcd step and the slow step 5 runs |
| `prime_square` | 8–64 bits | 100 per size | Returned composite at AKS step 1, the perfect-power check |
| `semiprime_near_2^32` | 29–35 bits | 5 per size | Balanced semiprimes either side of 2^32, where the AKS arithmetic changes (`aks_32bit`) |
| `carmichael` | up to 64 bits | 1777 | Every Carmichael number below 10^7, plus larger ones of the form (6m+1)(12m+1)(18m+1). One Fermat round returns prime for every base that shares no factor with n |
| `spsp2` | up to 24 bits | 162 | Composites that pass a Miller-Rabin round with base 2, below 10^7 |
| `spsp_multi` | up to 62 bits | 7 | Composites that pass Miller-Rabin rounds with each of the first 2–9 prime bases (from Jaeschke) |
| `hard` | 16–64 bits | 2440 | n = p(2p − 1) with p ≡ 3 (mod 4). In the full search up to 20 000, numbers of this form had the most bases for which a Miller-Rabin round returns prime |

`data/carmichael_fermat.csv` gives the exact probability that one Fermat round returns prime for each Carmichael number, (φ(n) − 2)/(n − 3), worked out from its factors. It is used to check the Fermat implementation.

## Correctness: `build/tests`

- `mulmod` and `powmod` against hand-worked values and brute force.
- Every n up to 200 000 against a sieve (up to 3000 for AKS, or 20 000 with `--slow`).
- Hand-picked lists: large primes, Carmichael numbers, perfect powers, semiprimes with close factors, Miller-Rabin pseudoprimes and 64-bit composites.
- Every labelled instance in the data set. For AKS, only the first 10 of each family and size up to 16 bits are checked, because AKS is slow.
- Results from Fermat and Miller-Rabin can only be wrong in one direction, so the result for a prime must always be prime. At 20 rounds, the Miller-Rabin result for every composite must also be composite. Prime results from Fermat on composites are expected; they are counted and printed rather than treated as failures.
- `rounds_used` holds the correct round.

## Experiments: `build/bench <name>`

The timers wrap only the calls being measured, never loading or generating data. An untimed warm-up pass runs first. Fermat and Miller-Rabin run with k = 9 rounds unless k is what is being varied; at k = 9 Miller-Rabin's error is at most 4^-9 ≈ 3.8·10^-6. Seeds are fixed.

| Experiment | Output | What it measures | Question addressed |
|---|---|---|---|
| `time_bits` | `time_bits.csv`, `time_vs_bits.png` | Time per call at each bit length, for every algorithm and family | How much does AKS's guaranteed correct result cost, and how does each algorithm's time grow? |
| `time_k` | `time_k.csv`, `time_vs_k.png` | Time per call for k = 1 to 20 on 60–64 bit inputs, Fermat and Miller-Rabin | What does each extra round cost? Every round runs on primes; most composites are found composite in the first round |
| `tails` | `tails.csv`, `tails.png` | 200 000 individually timed calls per family: average rounds and the median, 90th, 99th and 99.9th percentile times | Do the randomized tests have occasional slow calls? |
| `error` | `error_vs_k.csv`, `error_vs_k.png` | How often the result for a composite is prime, for k = 1 to 20, over 10^6 calls per family | Does Miller-Rabin stay under its 4^-k bound? On which families is Fermat's error high? |
| `carmichael` | `carmichael_k1.csv`, `carmichael_k1.png` | The probability that one round returns prime for each Carmichael number, from 2000 trials each | How do Fermat and Miller-Rabin compare on the inputs that are worst for Fermat? |
| `liars` | `liars.csv`, `liars.png` | For each odd composite up to 20 000, the fraction of all bases for which one round returns prime | How close does any composite get to Miller-Rabin's proven limit of 1/4? |
| `aks_steps` | `aks_steps.csv`, `aks_steps.png` | One AKS call per bit length, timed step by step, with r and the step at which the result was determined | Which AKS step accounts for the time, and how large does r get? |
| `aks_32bit` | `aks_32bit.csv`, `aks_32bit.png` | AKS on 5 semiprimes at each size from 29 to 35 bits, median of 5 calls each, and the time per coefficient multiply-add, time ÷ (squarings × r²) | How much does AKS slow down once n passes 2^32, and is the cause r or the cost per operation? |

### How timing works

- **Batches** (`time_bits`, `time_k`). One Fermat or Miller-Rabin call can take under a microsecond, close to the smallest interval the clock can measure. So each repeat times a whole batch of instances and divides by the batch size, and the median repeat is reported. The warm-up pass checks every result against its label (the `wrong` column) and sets the batch:
  - Fast algorithms: every instance (1000), 21 repeats.
  - Slow algorithms (AKS): as many instances as fit in 0.2 s, 3 repeats.
- **Cutoff**. Once one call takes longer than `--max-seconds` (default 2 s), larger sizes in that family are skipped for that algorithm. This is why the AKS lines end early.
- **Single calls** (`tails`). Each call is timed on its own to get the spread of times. These times include reading the clock, and the clock's resolution is recorded in the CSV.
- **Rounds returning prime** (`error`). Each call runs up to 20 rounds, and the number of rounds that returned prime is recorded. The error rate at any k from 1 to 20 is then the fraction of calls in which the first k rounds all returned prime, so one run gives every k. With the default 10^6 calls (`--calls`), the smallest error rate it can show is 10^-6.

### Fairness

- The same instances, seeds, `mulmod` and `powmod` (`common/modarith.hpp`) are used for every algorithm.
- In `time_bits`, `time_k` and `tails` the algorithms are timed back to back for each input set (interleaved), so drift in clock speed or temperature over the run affects all of them equally.
- `run_all.sh` pins the benchmark to core 2 with `taskset` (Linux), so the process is not moved between cores mid-run. Core 0 is avoided because it usually handles most hardware interrupts.
- One build (`-O3`) and one benchmark program. The benchmark has no special-case code for any algorithm.
- AKS is the plain textbook algorithm: schoolbook polynomial squaring, no NTT and a single thread. For n below 2^32 a faster 64-bit arithmetic path is used. This makes its time jump between 32 and 33 bits (`aks_32bit.csv`); the jump comes from the implementation, not from the algorithm's complexity.
- The machine, CPU governor and boost state are recorded in `results/env.txt`. With `run_all.sh --fixed-clock` (Linux, sudo), the run uses the `performance` governor with boost off, so the clock speed does not change between cells; the previous settings are restored afterwards.

## Using the results

`scripts/make_charts.py` saves every chart to `results/` and writes the key numbers to `results/summary.md` as tables ready to paste into the report.

| Report section or video segment | What to use |
|---|---|
| Problem and algorithms (video, about 2 min) | `primality run all 561 1105 97` live: Fermat can return prime for a Carmichael number |
| Per-algorithm deep dives | Fermat: `carmichael_k1.png`. Miller-Rabin: `liars.png` (no composite above 1/4). AKS: `aks_steps.png` (r against log²n; almost all the time is spent in step 5) |
| Results (video, 3–4 min) | Lead with `time_vs_bits.png`, then `error_vs_k.png` and `tails.png` |
| Correctness | `results/test_log.txt` and the `wrong` column of `time_bits.csv` |
| Possible surprises | AKS's time jumping at 32 bits. On Carmichael numbers, Fermat runs all k rounds and still returns the wrong result (`time_k.png`, `tails.csv`). Measured error rates far below 4^-k on every family except `hard` |
