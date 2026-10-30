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
