// AKS internals, exposed for the per-step timing in bench/bench.cpp.
#pragma once

#include "primality.hpp"

struct AksStats {
    const char* verdict = "too_small"; // the step at which the result was determined
    u64 r = 0; // smallest r with ord_r(n) > log2(n)^2
    u64 a_limit = 0; // how many values of a step 5 must check
    u64 a_checked = 0;
    u64 squarings = 0; // O(r^2) polynomial squarings
    double t_step1 = 0, t_step2 = 0, t_step3 = 0, t_step5 = 0; // seconds
};

bool aks_with_stats(u64 n, AksStats* s);
