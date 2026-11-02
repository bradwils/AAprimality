# Report scaffold — Brad's section (Miller-Rabin)

Skeleton only: key ideas to fill in, what to analyse and why. No analysis yet. Headings follow the brief's report structure ([guidance.md](../../guidance.md) §4) so the pieces slot straight into the group report.

## Constraints from the brief

- **Total report: ~4000–6000 words, whole group.** Budget my share at **~1300–1700 words** so three authors fit. Agree the split with CJ and Ben early.
- **Comparative, not descriptive.** Every subsection ends by linking Miller-Rabin back to Fermat / the deterministic algorithm. Pure internals belong in the video deep dive, not here.
- **§2 predictions are written before any measurement and never edited.** Date-stamp them and commit them before experiments run. The gap between prediction and result feeds §4 and §5.
- **Explain by structure, not "the asymptotics differ"** (§4). Each claim needs a mechanism.
- **State the guarantee type** for every algorithm: Miller-Rabin is probabilistic, error ≤ 4^-k per composite.
- **Reproducible** (§3): a skeptical reader can rerun from the text plus the repo.
- **Not in this file:** the ~500-word individual reflection (separate deliverable, about my algorithm only) and the video script.
- Open item: the repo has `trial_division/BEN`, not AKS. Confirm Ben's algorithm before writing comparisons against "the deterministic baseline".

## Flow of the argument

1. What Miller-Rabin is and what makes it different from Fermat →
2. What that predicts about cost and error (committed up front) →
3. How the inputs and setup test those predictions →
4. What happened, explained by structure →
5. What was unexpected →
6. What to do next →
7–8. Contributions and AI use.

Each section hands the next a question: structure → prediction → test → result → surprise.

---

## 1. The problem and the algorithms — Miller-Rabin part (~300–400 words)

Group writes the shared problem statement. I write Miller-Rabin so a classmate who hasn't seen it can follow.

- **Problem statement (shared):** decision problem, input `n`, output prime/composite. Note the input range (`uint64_t`, so n < 2^64) because it limits everything later. Explain this choice with a comparative example
- **Key idea:** a prime has only two square roots of 1 (`1` and `-1`). Test `a` against that, not just against `a^(n-1) ≡ 1`.
- **Mechanism, one pass:** `n-1 = 2^s · d`, compute `a^d`, square up to `s-1` times, look for `-1` (pass) or a non-trivial root of 1 (composite).
- **Guarantee type:** We get directions to investigate: "Composite" is always correct, so we continue, but "probably prime" is wrong with probability ≤ 4^-k.
- **Complexity:** k rounds × O(log n) squarings × cost of one modular multiply. State the multiply cost assumption (fixed-width here, so ~constant per op).
- **What makes it different:** the one-line contrast with Fermat (Fermat checks only the last step) and with the deterministic algorithm (certainty vs speed).
- **Figure idea:** the squaring chain for one prime and one composite, showing where the composite gives itself awa
Repeating moves us towards the asympotote of certainty.

**Why this section matters:** the brief wants the key idea, complexity and distinguishing feature. Section 2's predictions all lean on these facts.

## 2. Theoretical comparison and predictions (~400–500 words) — LOCKED once written

Compare, then commit. Everything here is phrased as a falsifiable prediction.

### 2.1 Theory to compare (before measuring)

- **Asymptotics:** Miller-Rabin vs Fermat per round (same modular exponentiation, plus the squaring check); vs the deterministic algorithm.
- **Constant factors:** extra squarings and branch checks over Fermat; `mul_mod` via 128-bit intermediate; cost of random base generation.
- **Memory:** O(1) for all, so memory should not separate them. Say so and move on.
- **Input features each algorithm is sensitive to:**
  - Bit length of `n` (all).
  - Carmichael numbers (Fermat fails; Miller-Rabin should not).
  - Strong pseudoprimes to specific bases (probe Miller-Rabin's per-base weakness).
  - Primes vs composites: primes always run all k rounds, composites usually exit early.
  - Number of rounds `k`.

### 2.2 Predictions to commit (fill in numbers before running anything)


- **P2 Early exit:** composites cost far less than primes because most are rejected in round 1. Predicted ratio: ___.
- **P3 Error rate:** observed false-positive rate on composites ≤ 4^-k; predicted to be far lower in practice. Predicted rate at k = 1 on random composites: ___.
- **P5 Same behaviour across input families:** no family causes a spike.
- **P6 Fermat comparison:** Miller-Rabin costs about ___× one Fermat round at equal k.
- **P7 Deterministic comparison:** Miller-Rabin is ___ orders of magnitude faster. Predicted crossover (if any) at ___ bits.

**Why these matter:** P3–P4 test the central claim (no bad class); P1, P2, P6, P7 test the cost story. Each lets §4 say "predicted X, observed Y".

## 3. Experimental methodology — Miller-Rabin-relevant parts (~250–300 words)

Group owns the common setup. I make sure the Miller-Rabin-specific choices are justified.

- **Input families (shared generators):** random primes, random composites, semiprimes, Carmichael numbers, strong pseudoprimes. For each, state how it's generated, the size range, and *why it probes a difference* (e.g. Carmichael numbers isolate Fermat's flaw).
- **Range constraint:** `uint64_t` caps `n` below 2^64. Say what this hides (nothing about 1000-bit behaviour) and what it allows (native-speed timing).
- **Rounds `k`:** sweep values; use the same `k` for Miller-Rabin and Fermat when comparing, so the cost per unit of confidence is fair.
- **Randomness:** seeding, base selection, number of repetitions per instance. Because the algorithm is randomized, report distributions, not single runs.
- **Fairness:** same instances for every algorithm, comparable implementation effort, timing excludes I/O and instance generation, warm-up handling.
- **Reproducibility:** hardware, compiler and flags, run command from the README.
- **Error measurement:** how a false positive is detected (ground truth from known list), and how many trials give enough power to see rates near 4^-k.

**Why this matters:** methodology is 15% of the grade. Randomized algorithms need repeated runs and a way to observe rare errors.

## 4. Results and analysis — Miller-Rabin-relevant parts (~300–400 words)

Prose matters most. Plots and tables support claims; they don't replace them. No results yet.

### 4.1 What to analyse, and why

- **Error vs k (log scale) per input family** vs the 4^-k bound — shows whether the bound is tight or loose and why.
- **Carmichael inputs: Fermat vs Miller-Rabin false-positive rate** — the headline comparison; shows what the squaring chain buys.
- **Time vs bit length** (log-log) per algorithm — slope check against the predicted exponent; locates any crossover.
- **Time on primes vs composites** — shows early exit and its effect on average cost.
- **Cost per unit of confidence:** time to reach error ≤ 10^-x for Miller-Rabin vs Fermat vs the deterministic algorithm — the actual trade-off the theme asks about.
- **Run-to-run variance** — the "tail behaviour" cost of randomness named in the theme.

### 4.2 For each result, structure the prose as

1. Prediction (cite P-number).
2. Observation.
3. Match or gap.
4. Mechanism, in terms of the algorithm's structure.
5. Link to the other algorithms.

### 4.3 Likely discussion points

- Where does randomness cost something (variance, rare error) and where does it cost nothing?
- Where does the deterministic guarantee cost time, and is it worth it at these sizes?

## 5. What surprised you — my contribution (~150 words)

Heavily weighted (10%). Group picks the headline; I supply material for it. Fill in only after §4.

- Candidates to watch for (not predictions): observed error far below 4^-k; composites with unusually many witnesses/few witnesses; a speed difference that isn't explained by round count; strong pseudoprimes behaving differently from predicted.
- Format: specific example (the number, the base, the numbers observed), expected vs observed, best explanation, how I'd test the explanation.
- Rule: a specific example beats a general statement.

## 6. What we'd do next — my ideas (~50–80 words)

Natural extensions and open questions, each tied to a gap found in §4:

- Larger `n` with GMP; deterministic base sets for fixed widths.
- Adversarially chosen composites against fixed bases.
- Compare against Baillie-PSW as a stronger practical test.

## 7. Individual contributions (~100 words)

One honest paragraph on what I did **beyond** Miller-Rabin: e.g. shared generators, harness pieces, report sections, reviewing others' code. State only what's true.

## 8. AI use (~100–150 words)

- My own paragraph: how I personally used AI tools (explanations, review, boilerplate), and that Miller-Rabin is my own implementation.
- Contribute my share of the group's **≥ 3 specific examples where AI was wrong, unhelpful or misleading**, with what I did about each.
- Contribute to the group reflection: where AI helped (explaining another member's algorithm) and where it risked hurting (skipping understanding, convincing but wrong analyses).

---

## Word budget (my share)

| Section | Words |
|---|---|
| 1 Algorithm description | 300–400 |
| 2 Theory + predictions | 400–500 |
| 3 Methodology | 250–300 |
| 4 Results/analysis | 300–400 |
| 5 Surprise | ~150 |
| 6 Next | 50–80 |
| 7 Contributions | ~100 |
| 8 AI use | 100–150 |
| **Total** | **~1650–2080** |

Trim §1 or §4 if the group total nears 6000 words.

## Checklist before submission

- [ ] Predictions P1–P7 filled, dated and committed **before** experiments
- [ ] Every claim in §4 cites its prediction and a structural mechanism
- [ ] Guarantee type stated for each algorithm
- [ ] Every Miller-Rabin claim links back to a comparison
- [ ] Reproducibility details (hardware, flags, seeds, command) present
- [ ] Word count within the group total
