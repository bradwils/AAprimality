# Video script: AKS portions only

Assignment limit for a group of 3 is **15 minutes, hard cap** (12 for a group of 2). Over the cap is penalised, so
the budget below sums to 14:30 and leaves slack. Everything about Fermat, Miller-Rabin and the head-to-head
comparison is marked **[PENDING]** because that work has not been done yet.

Numbers quoted below come from `implementations/aks/results/*.csv` on the reference machine (Xeon 2.8 GHz, 1 vCPU, g++ -O2). Re-run
`make bench` (commands in the root README) and `python3 implementations/aks/scripts/plot_aks.py` before recording and update any figure that changed.

| Segment | Budget | Who | Status |
|---|---|---|---|
| 1. Problem and algorithms overview | 2:00 | whole group | AKS part drafted below; Fermat/MR **[PENDING]** |
| 2a. AKS deep dive | 2:30 | AKS primary author | drafted below |
| 2b. Fermat deep dive | 2:30 | Fermat author | **[PENDING]** |
| 2c. Miller-Rabin deep dive | 2:30 | MR author | **[PENDING]** |
| 3. Results | 3:30 | whole group | AKS findings drafted; comparison **[PENDING]** |
| 4. Most surprising thing | 1:30 | whole group | candidates below; final pick **[PENDING]** |
| 5. Closing | 0:30 | whole group | **[PENDING]** |
| Total | 15:00 target 14:30 | | |

---

## Segment 1: problem and algorithms overview (~2:00)

**Say (problem):** "Given an integer n, decide whether it is prime. We compare three tests that sit at different
points on the determinism / certainty / speed spectrum: AKS, which is deterministic and provably correct in
polynomial time; Fermat, which is randomized and can be fooled by Carmichael numbers; and Miller-Rabin, which is
randomized, works on every number, and has a bounded error probability."

**Say (AKS in 30 seconds):** "AKS, from 2002, was the first test that is general, deterministic and provably
polynomial time. It never errs and uses no randomness. The price is a very large constant: it does a lot of
polynomial arithmetic to be certain."

**Show:** a one-slide table: algorithm | deterministic? | can be wrong? | known weakness.

**[PENDING]** one sentence each on Fermat and Miller-Rabin, and the group's framing question: "what does
determinism cost us, and what does randomness cost us?"

---

## Segment 2a: AKS deep dive (~2:30), recorded by the AKS primary author

Required by the assignment: not "what it does" but **the invariant, why each case is needed, and what breaks without it.**

**Slide: the algorithm in five lines** (perfect power, find r, gcd loop, n <= r, polynomial checks).

**The core identity (say):** "If n is prime, then (X + a)^n is congruent to X^n + a modulo n, because every middle
binomial coefficient C(n, k) is divisible by n. AKS's theorem says the converse holds too: if the congruence
holds for enough values of a, in a ring cut down to size r, then n is prime or a prime power."

**The invariant (say, with code on screen, `poly_square` and `poly_mul_xa` in `implementations/aks/src/aks.cpp`):** "All arithmetic
happens in the ring (Z/nZ)[X] modulo X^r - 1. So every polynomial is exactly r coefficients, each in [0, n).
Reducing mod X^r - 1 just means exponents wrap around: the coefficient of X^(i+j) is added into slot (i+j) mod r."

**Why each step exists / what breaks without it (the heart of this segment):**
- *Without the reduction mod X^r - 1:* (X + a)^n has n + 1 coefficients. For n near 2^32 that is four billion
  numbers per polynomial. The wrap-around is what makes the test feasible at all.
- *Without a large enough r (step 2):* the proof needs the order of n modulo r to exceed log2(n)^2; that is what
  limits the number of a values we must check to about sqrt(phi(r)) * log2(n). Our code finds the smallest such
  r by walking powers of n mod r. Measured: r lands very close to log2(n)^2 (for 26-bit n, r is about 660-700
  versus log2(n)^2 of at most 676).
- *Without the perfect-power check (step 1):* the correctness proof assumes n is not a prime power, so the
  algorithm's guarantee does not cover n = p^k. The check is cheap: under a microsecond for every prime square
  we tried.
- *Without the gcd loop and the n <= r case (steps 3 and 4):* the polynomial argument needs gcd(n, r) = 1 and
  n > r. Steps 3-4 clear those cases, and as a bonus step 3 catches most random composites cheaply.

**One concrete bug-prone spot to show:** the wrap in `poly_mul_xa` at index 0 (the coefficient of X^0 receives
`in[r-1]`, the top coefficient wrapping around), and the final comparison against X^(n mod r) + a. Mention that
the first draft of the *tests* (not the algorithm) had a wrong constant, caught because AKS disagreed with it
(see `HELPED.md`).

**Say (cost structure):** "Step 5 dominates: about 95 to 100 percent of run time on primes. It does about
sqrt(phi(r)) * log n values of a, each needing log n polynomial squarings that cost r^2 each. With r about
log^2 n, that is roughly log^7 n bit operations with schoolbook multiplication."

---

## Segment 3: results (~3:30), AKS findings drafted

**Plot A: `implementations/aks/results/aks_time_by_family.png`** (log y-axis by input family)

- "AKS is only slow on primes, because only a prime has to survive every one of the roughly r checks. A composite
  usually fails the very first check."
- Primes: 1.8 ms at 8 bits, 1.0 s at 20 bits, 6.2 s at 26 bits, 12.4 s for 2^31 - 1, 15.2 s for 2^32 - 5.
- Random composites: microseconds to about a millisecond even at 40 bits, because they almost always have a small
  factor below r and are caught by the gcd step.
- Balanced semiprimes (p x q, both about half the bits) are the hard composites: no small factor, so they reach
  step 5, but fail on the first value of a. That costs roughly 1/a_limit of a prime's time (at 24 bits: 3.8 ms vs 2.9 s).
- Prime squares: constant, about 0.4 microseconds, from the perfect-power check.

**Plot B: `implementations/aks/results/aks_prime_loglog.png`.** Fitted slope of time against bits for primes is about 6.8. That is
close to the roughly log^7 n predicted for AKS with schoolbook polynomial multiplication. Say clearly that this
is a fit over 12 to 26 bits only (10 data points, 3 primes per size).

**Plot C: `implementations/aks/results/aks_r_vs_log2sq.png`.** r hugs log2(n)^2. Combined with a_limit being about r, the work per
test is about r^3 log n.

**[PENDING] the head-to-head slide:** AKS time vs Fermat vs Miller-Rabin on the same primes, same bit sizes (use
the shared harness; same seeded inputs). Expect AKS to be orders of magnitude slower; the interesting content
is by how much, where, and what the other two give up. Also **[PENDING]**: Carmichael-number family (the input
that breaks Fermat and not AKS).

---

## Segment 4: the most surprising thing (~1:30), candidates from AKS

Pick one with the group once Fermat/MR data exist. Candidates already observed:

1. **A 17x cliff at 2^32 that the algorithm's theory does not predict** (`implementations/aks/results/aks_32bit_cliff.png`). One
   polynomial exponentiation on a 32-bit semiprime takes 15.7 ms; on a 33-bit semiprime 264 ms, although r only
   moved from about 1050 to about 1070. Explanation: below 2^32 a product of two coefficients fits in 64 bits, so
   we accumulate a whole dot product and reduce once per output coefficient; above it each product needs a 128-bit
   remainder. Lesson for the comparison: constants and word size can matter as much as the asymptotics, and this
   must be mentioned when comparing against algorithms whose inner loop is a single modular multiplication.
   *(Be honest on video that this is partly our implementation, not AKS itself. A natural fix is Montgomery
   reduction, which would be a follow-up experiment.)*
2. **The asymmetry between primes and composites.** The same algorithm takes 3 seconds on a 24-bit prime and under
   4 ms on a 24-bit semiprime. "Proving primality" is hundreds of times more expensive than "refuting" it.
3. **Hard composites switch regime when factors outgrow r.** Semiprimes are caught by the cheap gcd step up to
   16 bits (factors smaller than r) and need step 5 from 20 bits on (jump from 73 microseconds to 1.7 ms).

**Honesty note for the report (important):** the assignment requires predictions to be written *before* the
experiments. The AKS numbers above were seen while building the harness, so the AKS predictions in report section 2
cannot honestly claim to be made blind. Say so in the report. Write the Fermat and Miller-Rabin predictions, and
the cross-algorithm predictions (where crossovers fall), **before** running those benchmarks.

---

## Segment 5: closing (~0:30) **[PENDING]**

One sentence on what is next. AKS-specific options: faster polynomial multiplication (Karatsuba or NTT), Montgomery
arithmetic to remove the 2^32 cliff, multithreading the independent values of a, or Lenstra-Pomerance style
parameters.

---

## Recording checklist

- Total length under 15:00 for a group of 3. Time each segment separately, then the stitched cut.
- The AKS author records segment 2a personally; it is the main video evidence of individual understanding.
- At least one plot on screen in the results section; keep text legible at 1080p.
- Audio clear; no need for animation.
- Do not commit the video file to the repo (`.gitignore` excludes common media types).
- Questions the AKS author should be ready to answer from the individual reflection (worst case input, hardest bug,
  what they would do differently, one thing they now understand): worst case is a prime near the top of a size class
  (all about r values of a checked); hardest bug candidates are the cyclic wrap and the `n < 2^32` path switch.
