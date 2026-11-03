# AAprimality

Assignment 3 (Advanced Algorithms): deterministic vs randomized primality testing.

| Algorithm | Type | Primary author | Source |
|---|---|---|---|
| Fermat | Randomized | CJ | [implementations/fermat_primality/fermat.cpp](implementations/fermat_primality/fermat.cpp) |
| Miller-Rabin | Randomized | Brad | [implementations/miller_rabin/miller_rabin.cpp](implementations/miller_rabin/miller_rabin.cpp) |
| AKS | Deterministic | Ben | [implementations/AKS/aks.cpp](implementations/AKS/aks.cpp) |

## Layout

- [include/primality.hpp](include/primality.hpp): the shared function signature and the algorithm registry
- [common/modarith.hpp](common/modarith.hpp): shared `mulmod` / `powmod`, so the comparison isn't skewed by different arithmetic
- [src/main.cpp](src/main.cpp): `primality`, runs any algorithm on numbers you give it
- [tests/tests.cpp](tests/tests.cpp): `tests`, correctness of every algorithm against known results
- [bench/bench.cpp](bench/bench.cpp): `bench`, the experiments; writes CSVs to `results/`
- [scripts/gen_data.py](scripts/gen_data.py): builds the shared input set `data/instances.csv`
- [scripts/make_charts.py](scripts/make_charts.py): turns `results/*.csv` into charts and `results/summary.md`
- [TESTING.md](TESTING.md): the testing methodology, and the question behind each result

Each algorithm builds as its own library, linked into all three executables. Numbers are `uint64_t`.

## Reproduce everything

Requires CMake ≥ 3.16, GCC or Clang with C++17, python3 and matplotlib. From the repo root:

```sh
sh run_all.sh
```

This generates the data (once), builds in Release (`-O3`), runs the tests and every experiment, and draws the charts. Leave the machine idle while it runs.

## Run pieces by hand

```sh
cmake -S . -B build && cmake --build build -j

./build/primality list                         # algorithms and authors
./build/primality run all 561 97               # every algorithm on some numbers
./build/primality run fermat 561 --rounds 1 --seed 3

./build/tests                                  # about 5 s; --slow widens the AKS range
./build/bench time_bits --max-seconds 2        # one experiment; see TESTING.md for the list
python3 scripts/make_charts.py
```

A fresh RNG seeded with `--seed` (default 42) is created for each algorithm, so runs can be reproduced and results don't depend on which other algorithms ran first. The default is 9 rounds.

## Adding an algorithm

1. Implement `bool name(u64 n, TestParams& p)` in its own folder. Randomized tests set `p.rounds_used`.
2. Declare it in `include/primality.hpp` and add it to `ALGORITHMS`.
3. Add an `add_library` line for it in `CMakeLists.txt` and add it to the link loop.

Every entry in `ALGORITHMS` is included in `tests` and `bench` automatically.
