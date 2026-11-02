# HELPED.md: log of notable AI-agent changes

Policy: every notable change by an AI agent (more than 2 bugs fixed, major rewriting, major structuring, or anything
more than a small chore) is logged here. Detail is never more than 3 sentences. See `guidance.md`.

| Date | Overview | File(s) | Detail |
|---|---|---|---|
| 2026-10-27 | Initial draft of AKS, shared interface, tests, Makefile | `include/aks.hpp`, `include/common.hpp`, `src/aks.cpp`, `tests/test_aks.cpp`, `Makefile` | Wrote the full textbook AKS (perfect-power check, order search for `r`, gcd loop, polynomial congruence check) with 128-bit modular arithmetic so any 64-bit `n` is accepted. Added a `Stats` struct so each run reports `r`, number of `a` values checked and per-step times. |
| 2026-10-27 | Test suite fixed and made fast by default | `tests/test_aks.cpp` | First run contained a wrong test constant (94906249 labelled as 9739^2, but it is actually prime) and the suite took over 2 minutes. Replaced the constant with a computed square, reduced the default exhaustive range to 3000, and moved large primes and the 20000 exhaustive range behind `--slow`; default suite now takes about 8 seconds and passes. |
| 2026-10-27 | Rewrote polynomial squaring for the `n < 2^32` path | `src/aks.cpp` | Replaced a scatter-style loop (add each product into `out[(i+j) mod r]`) with one dot product per output coefficient and a single modular reduction each. Measured 1.11 s to 0.96 s for n = 1000003 (about 14% faster), all tests still pass; `-O3` gave no further gain, so `-O2` was kept. |
| 2026-10-27 | Benchmark harness, README, guidance, gitignore | `bench/bench.cpp`, `README.md`, `guidance.md`, `.gitignore` | Built a registry-based harness (same seeded inputs per algorithm, untimed warm-up, median of repetitions, trial-division oracle, CSV with AKS step diagnostics). README documents build, run, methodology and how to add Fermat and Miller-Rabin; `.gitignore` excludes `CLAUDE.md` and large media. |
| 2026-10-27 | Ran benchmarks, found and documented 32-bit cliff, added plots | `results/*.csv`, `scripts/plot_aks.py`, `README.md` | Ran prime (8-26 bits), composite (8-40 bits) and 30-35 bit semiprime sweeps; all answers matched the oracle, and the slow test tier (including 2^32-5) passes in about 2 minutes. Discovered a 17x slowdown when `n` crosses 2^32 (64-bit lazy path vs 128-bit division), documented under "Known limitations" rather than hidden. |
| 2026-10-27 | Video script (AKS portions only) | `docs/video_script_aks.md` | Drafted the AKS-only parts of the required video structure with timing budget and results placeholders filled from the benchmark CSV. Parts needing Fermat and Miller-Rabin are marked as pending. |

## Where the AI was wrong, unhelpful or misleading (candidates for the AI-use disclosure)

These are real events from this session, kept here so the group can cite them (the assignment needs at least 3 across the group).

1. **Wrong test data.** The AI asserted that 94906249 equals 9739^2 when writing the perfect-power tests. It is prime (9739^2 is 94848121). It was only caught because AKS (correctly) returned "prime", which disagreed with the test's assumption; fixed by computing squares in code (`9739ULL * 9739ULL`) instead of typing constants.
2. **Unmeasured assumptions about test cost.** The AI wrote a test suite with large primes and a 20000 exhaustive range without estimating AKS run time first. The first run timed out at 2 minutes and the revised one still took 2m21s; the fix came only after timing individual inputs.
3. **Optimisation oversold.** The AI expected the squaring rewrite and `-O3` to give a large speed-up; the measured gain was about 14% and `-O3` gave none. AKS's cost is dominated by the sheer count of `O(r^2)` squarings, not by loop overhead.
4. **Tooling slip.** The AI used shell brace expansion (`mkdir -p a/{b,c}`) in a shell that does not support it, so the first batch of file writes silently failed and had to be redone. Output only revealed this because the error message was read.
5. **Output truncated by the AI's own command.** A `cut -c1-120` made a CSV value look like `0.0`; the AI briefly suspected a timing bug before noticing the line was truncated.
