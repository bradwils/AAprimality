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
- [src/main.cpp](src/main.cpp): the single executable, which runs and benchmarks every algorithm
- [data/](data/): input files for benchmarks

Each algorithm builds as its own library, and all three are linked into one `primality` executable. Numbers are `uint64_t`.

## Build

Requires CMake ≥ 3.16 and GCC or Clang with C++17.

```sh
cmake -S . -B build
cmake --build build
```

The default build type is Release (`-O3`).

## Run

```sh
./build/primality list                                  # list algorithms and authors
./build/primality run all 561 97                        # test numbers with every algorithm
./build/primality run miller-rabin 561 --rounds 10 --seed 7

./build/primality bench all --input data/sample.txt --reps 5 --warmup 1 > results.csv
```

`bench` writes CSV to stdout with the columns `algorithm,n,bits,rep,result,ns`. Input files have one number per line, and lines starting with `#` are ignored.

Randomized tests get a fresh RNG seeded with `--seed` (default 42) for each algorithm, so runs can be reproduced and don't depend on which other algorithms ran first.

## Adding an algorithm

1. Implement `bool name(u64 n, TestParams& p)` in its own folder.
2. Declare it in `include/primality.hpp` and add it to `ALGORITHMS`.
3. Add an `add_library` line for it in `CMakeLists.txt` and link it into `primality`.
