# Miller-Rabin: report section (draft)

Sections follow the brief (guidance.md4). Bullets in 4 to 6 flesh out, complete Brad: ...]

**Before we start, this algorithm depends on a value of k, which is the number of rounds we run our test. The more we run, the lower our error rate. I've given the arbitrary value of five 9's of certainty (99.99962% rate for non-errors, where k=9). This will be analysed in our results laer, but we'll be operating on k=9.

## 1. The problem / this algorithm

**Problem.** Given an integer n, decide whether it is prime. Inputs fit in a `uint64_t` (n < 2^64), so every modular multiply is a native 128-bit multiply and remainder.

**Key idea.** Write n - 1 = 2^s · d where d is odd and look at the chain a^d, a^(2d), a^(4d), ..., a^(n-1) (mod n).
 
If n is prime, 1 and -1 are the only square roots of 1, so the chain starts at 1 or hits n - 1 before the end. A chain that reaches 1 from anything else shows a non-trivial square root of 1, so n is composite.

We name the bases that do this is a witness. One round picks a random a in [2, n-2], computes a^d, then squares up to s - 1 times.

**Example.** n = 561 = 3 · 11 · 17 and base 2. Here 560 = 2^4 · 35 and the chain is 263, 166, 67, 1. It reaches 1 from 67 and not from 560, so 561 is composite. Fermat only sees 2^560 = 1 and passes it.

**Guarantee and cost:** "Composite" is always right. "Probably prime" is wrong with probability at most 4^-k on an odd composite, because at least 3/4 of bases are witnesses. Primes will always pass. 

As mentioned at the start, we're using aiming for five 9's of confidence. k = 8 gives 1.5e-5, which just misses. A round is one modular exponentiation, about b squarings and b/2 multiplies for a b-bit n, so k rounds cost O(k · b) fixed-width multiplies and O(1) memory. [claude]

**Against the others.** Fermat checks only the last link, a^(n-1) = 1, and Carmichael numbers pass that for every coprime base. Miller-Rabin checks how the chain gets there. AKS never errs but costs far more per answer.

## 2. Theoretical comparison and predictions

- **Asymptotics and constants.** Miller-Rabin and Fermat both do done modular exponentiation per round, so O(b) multiplies. AKS is polynomial in b with a much higher exponent. The 128-bit remainder dominates the constant. Memory is O(1) for all three.

- The Miller-rabin algorithm's implementation is most easily affected by bit length, k, and primality (primes run all k rounds, composites typically exit early by finding a witness). Carmichael numbers and strong (sucessful faking) pseudoprimes are where the tests should differ.

| ID | Prediction |
|---|---|
| 1 | Time on primes is linear in k. |
| 3 | Random composites exit in about 1 round, so they are 6 to 9.5 times cheaper than primes at k = 9. |
| 6 | One round costs 1.0 to 1.1 times one a^(n-1) mod n (a Fermat round). At k = 9 on primes I would expect about 9 times a single Fermat round, and about equal on composites. Fermat's pass rate on Carmichael numbers stays near 1 however many rounds. |
| 8 | p99 / median: primes under 1.2, random composites under 1.5, worst-case family at least 2.5. |

## 3. Experimental methodology

- **Inputs** (`gen_data.py`, seed 20261102, 49,386 instances):
  - primes, random odd composites and semiprimes: 1000 each at every size from 8 to 64 bits in steps of 4;
  - Carmichael numbers: all 105 below 10^7 plus every Chernick number (6m+1)(12m+1)(18m+1) up to 64 bits, 1,777 in all;
  - strong pseudoprimes: 162 to base 2 below 10^7 and 7 known ones to several fixed bases;
  - worst-case composites p(2p-1) with both factors prime and p = 3 mod 4, 2,440 of them, chosen because they topped an exhaustive search up to 20000.
- **Ground truth** comes from a separate Python Miller-Rabin with 12 fixed bases (exact below 3.3e24), checked against a sieve, and from how composites are built. It never comes from the C++ code. [claude, what is this?]
- **Timing.** Instances load before the clock starts. A point is 1000 calls timed as one batch, repeated 21 times after a warm-up, median reported. Tail runs time each call, and the clock ticks every 41 ns. [claude]
- **Error rates.** One run per family at k = 30 records the round that first rejects n, so the rate at every k comes from the same draws. The worst-case family gets 2 × 10^7 calls, enough to see rates near 4^-9.
- **Fairness.** One instance file for every experiment. Only Miller-Rabin is timed here. Fermat is compared through exact liar counts and the a^(n-1) mod n reference.
- **Reproduce.** `sh run_all.sh` in `implementations/miller_rabin (brad)/`, about 3 minutes.

## 4. Results and analysis

Charts are SVGs in `results/`, with the numbers behind them in the CSVs.

| ID | Observed | Verdict |
|---|---|---|
| P1 | slope 1.33 (1.01 below 32 bits, 1.07 above 36); 10.4 µs | Near miss, plus an unpredicted step |
| P3 | 9.2 times | Hit |
| P6 | 1.004 on average (0.94 to 1.04 by size) | Hit on average |
| P8 | 1.41, 1.39, 3.8 | Prime missed |

- **Round cost matches the theory.** (P1, P2, P6)
  - A round on a 64-bit prime takes 1.17 µs and time is linear in k. One round costs 1.004 times one a^(n-1) mod n, so the squaring chain adds nothing.
  - At k = 9 on a prime, Miller-Rabin is about 9 times one Fermat round (10.4 against 1.17 µs). That fits my hypothesis that repetition makes it slower. At equal k they cost the same.
- **Early exit does almost all of the saving.** (P3)
  - All 200,000 random composites and semiprimes were rejected in round 1, so a call is 9.2 times cheaper than on a prime.
  - So "slower because of repetition" only applies to primes.
- **Linear in bits, with a step.** (P1)
  - Time jumps 1.7 times between 32 and 36 bits, and it lands at 33 bits (356 ns to 490 ns at k = 1), where a · b can exceed 2^64.
  - Likely cause: the 128-bit remainder has a faster path when the dividend fits in 64 bits. A micro-benchmark of back-to-back `mul_mod` shows only +12% there, so the full jump is unexplained. [Brad: say so, or read the assembly.]
- **The bound is loose for most composites and tight for a few.**
  - Random composites: 0 errors in 2,000,000 calls at k = 1. Carmichael: 0.097 at k = 1, 1.2e-3 at k = 3, none at k = 9.
  - Worst-case family: 0.2500 at k = 1 and 3.70e-6 at k = 9 (74 in 2 × 10^7) against a bound of 3.81e-6.
  - No n up to 20000 reaches 1/4. The largest is 0.2452 at 12403 = 79 · 157.
- **Carmichael numbers split Fermat from Miller-Rabin.** (P6)
  - Fermat's liar share is 0.84 on average below 24 bits and 1.0000 above 40 bits, so more rounds cannot help. Miller-Rabin's is at most 0.244, about 0.10 above 40 bits, and at k = 9 it accepted a Carmichael number 0 times in 2,000,000 calls.
  - Fermat figures come from the factorisation (`data/carmichael_fermat.csv`), not a Fermat implementation.
- **Tails are the price of randomness.** (P5, P8)
  - The worst-case family averages 1.333 rounds, which is 1/(1 - 1/4), and 1.5% of its calls take 4 rounds or more. p99 / median is 3.8 there, 2.6 for Carmichael, 1.4 for random composites and primes.
- **Not tested.** P7. Nothing here measures AKS.

### Varying the error target (2 to 10 nines, k = 4 to 17)

k = 9 was a choice, so I swept the target to see what it costs. Each row uses the smallest k with 4^-k under the target. This was done after the predictions were locked and is not one of P1 to P9. Numbers are in `nines_table.csv` and the chart is `time_vs_nines.svg`.

| Nines | k | Bound 4^-k | Prime, 64 bit | Worst-case family, measured | Carmichael, measured |
|---|---|---|---|---|---|
| 2 | 4 | 3.9e-3 | 4.63 µs | 3.89e-3 | 1.4e-4 |
| 3 | 5 | 9.8e-4 | 5.80 µs | 9.77e-4 | 1.6e-5 |
| 4 | 7 | 6.1e-5 | 8.25 µs | 6.09e-5 | 0 |
| 5 | 9 | 3.8e-6 | 10.64 µs | 3.70e-6 | 0 |
| 6 | 10 | 9.5e-7 | 11.82 µs | 1.05e-6 | 0 |
| 7 | 12 | 6.0e-8 | 13.99 µs | 1.0e-7 | 0 |
| 8 | 14 | 3.7e-9 | 16.22 µs | n/a | n/a |
| 9 | 15 | 9.3e-10 | 17.39 µs | n/a | n/a |
| 10 | 17 | 5.8e-11 | 19.78 µs | n/a | n/a |

- **Cost is linear in the target.** A line through k = 4 to 17 gives 1.15 µs per round (R² 0.9994, every point within 3%), so each nine costs about 1.9 µs on a prime. Four to five nines (k = 7 to 9) is 8.3 to 10.6 µs and ten nines is 19.8 µs, 1.9 times the k = 9 cost. The headline 10.4 µs is a separate run of the k = 9 point, 2% lower.
- **Only primes pay.** Random composites stay at 1.15 µs for every k because they leave in round 1, so the prime to composite cost ratio is about k. The "slower because of repetition" effect grows with the target and only applies to primes.
- **Measured error follows the bound to k = 12.**
  - The worst-case family is within 3% of 4^-k for k = 4 to 9. At k = 10 to 12 it has 21, 7 and 2 events against 19.1, 4.8 and 1.2 expected, so the rates above the bound there are counting noise.
  - Random composites and semiprimes never err. Carmichael numbers reach 1.4e-4 at k = 4 and nothing from k = 7. Only a chosen input like p(2p-1) gets near the bound.
  - The 7 multi-base pseudoprimes predict 1.08, 0.27 and 0.07 surviving calls at k = 10, 11, 12 (`check_spsp_multi.py`) and I saw 1, 1 and 1. One call surviving 12 rounds is unlikely (about 6% once it survived 10) but not a bound failure. [Brad: another seed would settle it.]
- **Past k = 12 only the bound speaks.** 2 × 10^7 calls resolve rates down to about 5e-8, so the rows for 8.4 to 10.2 nines are the proven bound. One event at k = 17 would take about 2 × 10^10 calls.
- **Fixed bases beat random ones at this size.** The 12 fixed prime bases in my Python checker are exact below 3.3e24, so at 64 bits 12 fixed rounds give a certain answer in fewer rounds than the 17 random ones. The nines only matter with random bases, or when n is too big for a fixed list.

## 5. Unexpected results

- **Carmichael numbers are not the worst case for Miller-Rabin.** [de-technicalify it]
  - They fool Fermat on every coprime base (1.0000 above 40 bits) and Miller-Rabin on about 0.10 at k = 1. The numbers closest to the 1/4 bound are p(2p-1), such as 12403 = 79 · 157, which fool Miller-Rabin on 0.2452 of bases and Fermat on only 0.49.
  - For squarefree n with ω prime factors, p_i - 1 = 2^(s_i) · d_i and each d_i dividing d, the share of fooling bases is [1 + (2^(ω·s_min) - 1) / (2^ω - 1)] / 2^(s_1 + ... + s_ω) for a Carmichael number. This matches the exhaustive counts for all 5,842 squarefree n up to 20000 (`check_liar_formula.py`).
  - More prime factors shrink the share. Two factors with s = 1 and 2, which is p(2p-1), give exactly 1/4. Chernick numbers with m odd give 1/8, which fits the 0.10 average over the whole set.
- **Two 64-bit primes can differ in cost by 2 times.**
  - Per-prime medians at k = 9 run from 6.6 µs (1st percentile) to 13.9 µs (99th). Timing the same 1000 primes forwards and then backwards gives a correlation of 0.96, so it is the number and not the machine. The multiply count explains little (which had a correlation 0.30)
  - The 22 primes whose low 32 bits are below 2^26 have a median of 6.58 µs against 10.40 µs for the rest. This also explains the prime miss in P8.
  - Best explanation, untested: the 128-by-64-bit remainder is a software long division whose correction steps depend on the divisor's bit pattern. A Montgomery multiply should remove it.

## 6. What we'd do next

- Go past 64 bits with GMP and look for the AKS crossover.
- Swap `% n` for Montgomery multiplication and see whether the 33-bit step and the 2 times spread vanish.
- Run Fermat and AKS on the same instance files to fill in P6 and P7.
