# Miller-Rabin predictions (locked)

Written 2026-11-02, before any test, benchmark or measurement was run on the C++ implementation. Do not edit after this point. The SHA-256 of this file is in `results/predictions.sha256`, recorded before the first experiment.

## Setup these predictions assume

- Apple M3, clang++ with `-O2`, `uint64_t` inputs, `mul_mod` through `unsigned __int128`.
- Headline rounds k = 9, so the error bound is 4^-9 = 3.8e-6 (five nines).
- Bases are uniform random in [2, n-2].

## Cost model

One round is one modular exponentiation. For a b-bit n that is about b squarings and about b/2 multiplies. Each one is a 128-bit multiply followed by a 128-by-64-bit remainder. Because the width is fixed, one multiply costs the same at 20 bits as at 60 bits.

## Predictions

| ID | Prediction | How I will judge it |
|---|---|---|
| P1 | Time per call grows **linearly** in bit length, not cubically. Log-log slope of time against bits is between 0.8 and 1.3. At 64 bits one round takes 1 to 4 µs, so a prime at k = 9 takes 10 to 40 µs. | Fit on the time-vs-bits data for primes. |
| P2 | Time on primes is linear in k (every round runs). | Time at k = 20 divided by time at k = 1 is between 18 and 21. |
| P3 | Early exit. A random odd composite is rejected in round 1 almost every time, so a 64-bit composite call is 6 to 9.5 times cheaper than a 64-bit prime call at k = 9. | Ratio of median call times, same bit length. |
| P4 | The false-positive rate is at or below 4^-k for every family and every k. Random odd composites and semiprimes with large factors show 0 errors in 10^6 calls at k = 1. Carmichael numbers average a k = 1 error between 0.01 and 0.15 and none exceed 0.25. A hand-picked worst-case family gets a k = 1 error between 0.10 and 0.25. | Rounds-until-rejection counts over millions of calls. |
| P5 | No input family causes a time spike. Mean call time on any composite family at k = 9 is within 1.5 times the random composite time at the same size. | Mean time per family. |
| P6 | One Miller-Rabin round costs 1.0 to 1.1 times one `a^(n-1) mod n`, which is a whole Fermat round, because the squaring chain reuses the same squarings. On Carmichael numbers the Fermat pass rate per base is the share of bases coprime to n, close to 1 for large factors, so adding Fermat rounds does not help. Miller-Rabin's per-round error is at most 0.25. At k = 9 on a prime I expect Miller-Rabin to be about 9 times slower than a single Fermat round, which fits my hypothesis that repetition makes it slower. On composites it exits after about 1 round, so it costs about the same as a single Fermat round. | `pow_mod(a, n-1, n)` timing against one round; Fermat liar fraction against Miller-Rabin liar fraction on Carmichael numbers. |
| P7 | Against AKS, Miller-Rabin at k = 9 is at least 10^4 times faster at 64 bits and the gap grows with bit length. There is no crossover between 8 and 64 bits. This one needs the AKS code, so I can only state it here. | Check once AKS is available. |
| P8 | Tail behaviour. Prime calls always run k rounds, so p99 / median is below 1.2. Random composites have p99 / median below 1.5. On the worst-case family, where about a quarter of bases fool one round, p99 / median is at least 2.5. | Per-call timing percentiles. |
| P9 | Checking every base for every odd composite n up to 20000, the fraction of bases that fool one round is below 1/4 for every n. The largest observed fraction is between 0.15 and 0.25. | Exhaustive liar count. |

## Hypothesis in one line

Miller-Rabin is much faster than AKS, matches Fermat per round, and at k = 9 is a few times slower than a single Fermat round on primes because of the repetition.
