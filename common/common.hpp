// Shared interface so Fermat, Miller-Rabin and AKS can all be plugged into the same
// benchmark harness and tests. Every algorithm exposes: bool f(uint64_t n).
#pragma once
#include <cstdint>

using IsPrimeFn = bool (*)(uint64_t);

struct Algorithm {
    const char* name;
    const char* primary_author;
    IsPrimeFn   fn;
};
