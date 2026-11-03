---
title: Assignment 3 — Change Log
date: 2026-10-23
subject: Advanced Algorithms
tags: [assignment, change-log]
---

# Assignment 3 — Change Log

Log of notable changes to this repo, per [CLAUDE.md](CLAUDE.md) §3. A change is notable if it fixes more than two bugs, is a major rewrite or restructure, or is more than a small chore.

| Date | Overview | File(s) | Detail |
|---|---|---|---|
| 2026-10-30 | Single-executable scaffold | `CMakeLists.txt`, `include/primality.hpp`, `common/modarith.hpp`, `src/main.cpp`, `implementations/*/*.cpp`, `README.md`, `.gitignore`, `data/sample.txt` | Shared `PrimalityFn` interface + registry; one CMake library per author linked into `primality`; `list`/`run`/`bench` CLI with seeded RNG and CSV output; stub implementations for Fermat (CJ), Miller-Rabin (Brad), AKS (Ben); trial division replaced by AKS. |
| 2026-11-03 | Unified testing framework (Claude Code) | `tests/tests.cpp`, `bench/bench.cpp`, `include/instances.hpp`, `scripts/gen_data.py`, `scripts/make_charts.py`, `run_all.sh`, `TESTING.md`, `implementations/*/*`, `include/primality.hpp`, `src/main.cpp`, `CMakeLists.txt` | Merged the best of the `final-doc` (Brad's Miller-Rabin data generator, oracle, error-vs-k and liar experiments) and `Ben` (AKS, slow-algorithm cutoff, per-step AKS stats) branches onto the `main` scaffold. Ported CJ's Fermat, Brad's Miller-Rabin and Ben's AKS to the shared `TestParams` interface (added `rounds_used`). One `tests` and one `bench` executable are built for all three algorithms; `primality bench` removed in favour of `bench`; default rounds 9. |
