# Outline

Theme: **deterministic vs randomized** primality. Source of truth: [guidance.md](guidance.md).
The point is the **comparison**, not any one implementation.

## Deliverables

| Deliverable | Who | Notes |
|---|---|---|
| Repo (modules, benchmark harness, repro scripts, README) | Group | Each module names its primary author |
| Report, 4000–6000 words | Group | Predictions written **before** experiments, never edited |
| Video, ≤15 min (hard cap) | Group | I record my own 2–3 min deep dive |
| Reflection, ~500 words | Me | On Miller-Rabin only |
| AI-use paragraph | Me | Group also needs ≥3 examples of AI being wrong |
| 3 peer responses, ~200 words each | Me | Due 1 week after submission |

## The three algorithms

| Algorithm | Guarantee | Weakness | Owner |
|---|---|---|---|
| AKS | Deterministic, always correct | Huge constants, slow in practice | ? (confirm) |
| Fermat | Probabilistic | Carmichael numbers fool it for every coprime base | CJ (probably) |
| **Miller-Rabin** | Error ≤ 4^-k, no known bad inputs | Needs random bases / k rounds | **Me** |

Open issue: repo has `trial_division/BEN`, not AKS. Confirm with Ben what he is doing.

## My part: Miller-Rabin

### Build
- Write it myself and be able to explain every line (Claude reviews, doesn't rewrite).
- Decompose `n-1 = 2^s · d`; for each base `a`, check `a^d ≡ 1` or `a^(2^r·d) ≡ -1` for some `r < s`.
- Take `k` (rounds) as a parameter so the harness can sweep it.
- Use modular exponentiation. Watch for overflow in `a*b mod n` at larger sizes.
- Handle edge cases: n < 2, even n, small primes, n = 3.

### Test (shared inputs, same as the others)
- Known primes, known composites, **Carmichael numbers**, strong pseudoprimes.
- Every algorithm runs the same list so results are comparable.

### Deep dive (video, 2–3 min)
- Why the squaring chain is needed: Fermat only checks `a^(n-1) ≡ 1`.
- What breaks without it: Carmichael numbers pass Fermat, but a non-trivial square root of 1 exposes them.
- Why ≥3/4 of bases are witnesses for any odd composite.

### Predictions (write before running anything)
- Cost ≈ `k · O(log³ n)` with schoolbook multiplication.
- Error ≤ 4^-k, and likely far lower in practice.
- Same behaviour across input families; no bad class.
- About k times slower than one Fermat round; orders of magnitude faster than AKS.

### Reflection prompts (~500 words)
- Worst case and the input that triggers it.
- Hardest bug or design decision.
- What I'd do differently.
- One thing I didn't understand before starting.

## How my work fits the others'

- **Fermat vs Miller-Rabin** is the main plot. Same Carmichael inputs: Fermat fails, Miller-Rabin catches them. That is where "no bad inputs" is shown, and it is the likeliest source of the report's surprise.
- **Miller-Rabin vs AKS** is the speed-vs-guarantee story: what does the certainty cost?
- The harness needs a common interface, e.g. `is_prime(n, k) -> bool`. Agree on it early; mine must plug in with no special cases.
- Shared generators produce primes, composites, semiprimes, Carmichael numbers and strong pseudoprimes. Agree on who writes them.
- Miller-Rabin is the middle point: it should be nearly as fast as Fermat and nearly as reliable as AKS. The report should show that.

## Timeline

| When | Milestone |
|---|---|
| Wk 1 | Email instructor: problem, algorithms, owners |
| Wk 2 | My Miller-Rabin working; group check-in |
| Wk 3 | All implementations + harness done; predictions written; experiments started |
| Wk 4 | Experiments, report, video submitted (aim for a few days early) |
| Wk 5 | Peer responses due |

## Housekeeping
- Log notable changes in [HELPED.md](HELPED.md); agent-made commits go in the guidance.md commit log.
- No `Co-Authored-By` trailers.
- Don't commit video files; link to them.
