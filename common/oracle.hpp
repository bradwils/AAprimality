// Independent correctness oracle shared by every test and by the benchmark harness.
// Deliberately trial division: it must NOT depend on AKS, Fermat or Miller-Rabin,
// so that a bug in one algorithm can never hide behind another.
#pragma once
#include <cstdint>

inline bool trial_division(uint64_t n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0) return false;
    for (uint64_t d = 3; d * d <= n; d += 2)
        if (n % d == 0) return false;
    return true;
}
