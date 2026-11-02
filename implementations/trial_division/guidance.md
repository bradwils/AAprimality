# guidance.md

Rules for anyone (human or AI agent) making decisions about documentation, tests, benchmarks or
structure in this repository. This is a **comparative** assignment (Programming Assignment 2,
Theme 2: deterministic vs randomized), so every decision should be judged by one question:

> Does this make the comparison between the algorithms more fair, more reproducible, or better explained?

## What we are working on

We are comparing three primality tests on the same problem ("is `n` prime?"):

| Algorithm | Kind | Key property |
|---|---|---|
| **AKS** | Deterministic | Always correct, polynomial time, but with a very large constant factor |
| **Fermat** | Probabilistic | Fast, but only works for some numbers: it is fooled by Carmichael numbers |
| **Miller-Rabin** | Probabilistic | Fast, and works for all numbers (error probability <= 1/4 per round) |

Each algorithm has a named primary author (see the header of each source file and the README).

## Decision rules

1. **Same inputs for every algorithm.** Benchmarks generate each input once from a fixed seed and give the
   same `n` to every algorithm. Never special-case inputs for one algorithm.
2. **Independent oracle.** Correctness is checked against trial division, never against one of the three
   algorithms under study.
3. **Predictions come before results.** The report's "Theoretical comparison and predictions" section must be
   written down before the final experiments and must not be edited afterwards to look better. If a
   prediction was wrong, say so in "What surprised you".
4. **Test the adversarial cases, not just random ones.** At minimum: small exhaustive range, primes, random
   composites, semiprimes with close factors, perfect powers, and Carmichael numbers (561, 1105, 1729, ...).
5. **Measure the algorithm, not the harness.** Only the call to `is_prime` is timed. No I/O, no input
   generation inside the timed region. Report median of several repetitions after a warm-up.
6. **Same effort and same build flags** for all implementations (`-O2`, same compiler, same machine).
7. **Document limits honestly.** If an implementation only works up to some size (AKS here is practical up
   to roughly 2^32), the README and report say so.
8. **Keep docs and tests in step with code.** If behaviour changes, update the README, tests and (if the
   change was notable) `HELPED.md` in the same change.
9. **Tests must stay fast by default.** Slow checks go behind `--slow`.

## AI use rules (from the AI Use Logging policy)

- Every notable agent change (more than 2 bugs fixed, major rewriting, major restructuring, anything more than
  a small chore) is logged in `HELPED.md`. Table columns: **Date, Overview, File(s), Detail** (Detail is at
  most 3 sentences).
- **Never add a co-author credit** for an AI agent in commits, files or the report.
- If a commit is made **entirely** by an AI agent, record it in the commit log below and reference the commit hash.
- Agent instruction files (`CLAUDE.md`) and large media files are not committed (see `.gitignore`).
- The assignment also requires, per member, a paragraph on personal AI use, at least three specific examples
  across the group where AI was wrong/unhelpful/misleading, and a reflection on AI in group work. Candidate
  examples are collected in `HELPED.md` under "Where the AI was wrong" so they are not forgotten.

## Commit log: commits made entirely by an AI agent

| Date | Commit | What it contains |
|---|---|---|
| _none yet_ | | |

(The first AKS draft was delivered as files, not as a git commit. If you commit it unchanged, add the hash here.)

## Marking criteria checklist

| Criterion (weight) | Where it is addressed |
|---|---|
| Implementation correctness and quality (20%) | `src/aks.cpp`, `tests/test_aks.cpp`, `make test` |
| Experimental methodology (15%) | `bench/bench.cpp`, README "Methodology" |
| Analysis quality (20%) | Report section 4 (use `aks_r`, `aks_a_checked`, step timings from the CSV) |
| What surprised you (10%) | Report section 5; candidate AKS observations in `docs/video_script_aks.md` |
| Video (10%) | `docs/video_script_aks.md` (AKS parts only so far) |
| AI use disclosure (10%) | `HELPED.md` |
