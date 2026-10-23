# Assignment 3 — Algorithm Comparison Project

## Goal

In a group of 2 or 3, pick one algorithmic problem and implement multiple genuinely different algorithms for it. Compare them theoretically and empirically, write a report, produce a video, and respond to other groups' videos.

- The point is the **comparison**, not any single implementation.
- A good project finds the regimes where each algorithm wins, explains why in terms of the algorithms' structure, and ideally surfaces something unexpected.
- AI tools are permitted and expected, with mandatory disclosure (see [AI use](#6-ai-use-policy)).

## 1. Choose a theme

Pick one, or email a one-paragraph proposal for your own theme and get approval before committing.

| Theme | What to implement | Question explored | Candidate problems |
|---|---|---|---|
| **1. Exact vs approximate vs heuristic** (NP-hard problem) | Exact (branch-and-bound, subset DP, or ILP solver), approximation with proven ratio, and (groups of 3) a heuristic with no guarantee | Where does exact hit the wall? What does approximation give up? Where do heuristics shine? How does input structure matter? | Vertex cover, set cover, TSP (metric/general), max-cut, independent set, knapsack, graph colouring, scheduling, bin packing, max-SAT |
| **2. Deterministic vs randomized** | A deterministic and a randomized algorithm (groups of 3: two distinct randomized approaches, or a Monte Carlo / Las Vegas pair) | Where does determinism cost? Where does randomness cost (tail behaviour)? | Min cut (Karger vs Stoer-Wagner), primality (AKS vs Miller-Rabin), string matching (Karp-Rabin vs KMP), matrix product verification (Freivalds vs recompute), MST (Karger-Klein-Tarjan vs Kruskal/Prim), median finding (randomized vs deterministic linear-time) |
| **3. Theoretical-optimal vs practical** | The asymptotically best (impractical) algorithm and a simpler one that wins in practice, or several points along the spectrum | Find the crossover; what does theoretical complexity tell you and not tell you? | Integer multiplication (schoolbook/Karatsuba/FFT vs GMP), matrix multiplication (naive vs Strassen), max matching (augmenting paths vs Hopcroft-Karp), max-flow (Ford-Fulkerson/Edmonds-Karp/Dinic/push-relabel), sorting (only with serious literature engagement), convex hull (Graham/Jarvis/Chan) |

**Custom theme constraint:** a comparison of multiple algorithmic approaches to a single well-defined problem, and the comparison must be intellectually substantive.

## 2. Group structure and attribution

- Group of 2 → implement **2** algorithms. Group of 3 → implement **3**.
- Each algorithm has a **primary author**, named in the writeup and the repository.
- The primary author is responsible for that implementation and must be able to explain it in detail.
- Empirical study, analysis, writeup and video are collaborative.
- Review each other's code and understand each other's algorithms, but you are individually accountable for your own.

## 3. Deliverables

1. **Repository**
   - All implementations, each in its own clearly labelled module naming its primary author
   - A unified benchmarking framework
   - Scripts to reproduce the experiments
   - README with build instructions and how to run everything
2. **Written report**, about 4000–6000 words (structure below)
3. **Video** (length caps below)
4. **Individual reflection**, about 500 words per group member
5. **Peer viewing responses** to 3 other groups' videos

## 4. Report structure

1. **The problem and the algorithms**
   - State the problem precisely.
   - Describe each algorithm so a classmate who hasn't seen it can follow: key idea, complexity, what makes it different.
2. **Theoretical comparison and predictions**
   - Before any measurements: compare asymptotics, constant factors, memory, and the input features each algorithm should be sensitive to.
   - Commit to predictions: which algorithm wins in which regime, where crossovers fall.
   - **Do not revise predictions after running experiments.** The gap between predictions and results feeds sections 4 and 5.
3. **Experimental methodology**
   - Input families: how generated, sizes, structural parameters, why they probe the differences.
   - Benchmark setup: hardware, language/compiler and flags, timing method, repetitions, variance and warm-up handling.
   - Fairness: same instances for every algorithm, comparable implementation effort, measure the algorithms rather than I/O.
   - A skeptical reader should be able to rerun everything from this section plus the repo and reproduce your plots.
4. **Results and analysis**
   - Plots, tables and prose. The prose matters most.
   - Explain results by the algorithms' structure, not just "the asymptotics differ".
5. **What surprised you** (heavily weighted)
   - At least one unexpected result and your best explanation. Specific examples beat general statements.
6. **What we'd do next**
   - Natural extensions and open questions.
7. **Individual contributions**
   - One honest paragraph per member on what they did beyond their primary algorithm.
8. **AI use** (see below)

## 5. Individual reflection (each member, ~500 words)

Written about your own algorithm. Graded on specificity and evident understanding, not length or polish.

- What is the worst case for your algorithm, and what input triggers it?
- What was the hardest bug or design decision, and how did you resolve it?
- If you did it again from scratch, what would you do differently?
- What's one thing you didn't fully understand before starting, and now do?

## 6. AI use policy

Same as Assignment 1, with group additions:

- **Per member:** a short paragraph on how you personally used AI tools.
- **At least 3 specific examples across the group** where AI was wrong, unhelpful or misleading, and what you did about it.
- **Group reflection on AI:**
  - Where it helped the group function (explaining one member's algorithm to another, resolving disagreements, boilerplate)
  - Where it hurt or threatened to (skipping understanding each other's work, convincing but wrong analyses)

## 7. Video

Posted as an unlisted YouTube link, or submitted as a video file.

**Hard length caps** (even slightly over is penalized):

- Group of 2: **12 minutes**
- Group of 3: **15 minutes**

Shorter is better. Don't pad.

**Required structure:**

| Segment | Time | Content |
|---|---|---|
| Problem and algorithms overview | ~2 min | The problem, the algorithms, why compare them |
| Per-algorithm deep dives | ~2–3 min each | Each primary author presents the trickiest or most interesting aspect of their own algorithm: the invariant, why a case is needed, what would break without it |
| Results | ~3–4 min | Most important findings, at least one plot or visualization; tell the story, don't walk through every plot |
| Most surprising finding | ~1–2 min | Your headline surprise from the report |
| Closing | ~30 sec | One sentence on what you'd do next |

**Production:**

- Audio must be clear and visuals legible; polish is not graded. Voiceover with slides or webcam with screen-share is fine.
- Each primary author records their own deep-dive segment.
- Grading is on content: clarity, technical depth, how well segments fit together.

## 8. Peer viewing responses

After submission you are assigned 3 other groups' videos. Write about 200 words per video (~600 total), covering:

1. What problem did they work on, and which algorithms did they compare?
2. What did you find interesting or surprising?
3. What would you have done differently, or what question did their work raise that they didn't answer?

Expect roughly 2 hours total.

## 9. Grading

| Component | Weight |
|---|---|
| Implementation correctness and quality (per algorithm, attributed to primary author; group adjustment for repo/build quality) | 20% |
| Analysis quality | 20% |
| Experimental methodology | 15% |
| "What surprised you" | 10% |
| Video (especially per-author deep dives) | 10% |
| AI use disclosure (per member) | 10% |
| Individual reflection (per member) | 5% |
| Peer viewing responses | 5% |
| Report quality | 5% |

Group-level components default to the same grade for everyone, adjusted using the contribution paragraphs, primary-author work, video deep dives and individual reflections. The reflection and video deep dive are the main places individual understanding is assessed.

## 10. Timeline

| When | Milestone |
|---|---|
| End of Week 1 | Group and theme/problem chosen. Email the instructor: the problem, planned algorithms, and primary author for each. |
| End of Week 2 | At least one implementation per member substantially working. Group check-in: on track, or descope? |
| End of Week 3 | All implementations working, benchmarking infrastructure built, experiments started, report drafting begun. |
| End of Week 4 | Experiments done, report written, video produced, everything submitted. Aim to finish experiments and a full draft a few days early; reserve the last days for recording and editing the video. |
| Week 5 | Peer video assignments go out shortly after submission; the 3 responses are due one week later. |
