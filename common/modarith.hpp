// Modular arithmetic shared by all three tests
// Apparently make timing reproducible
#pragma once

#include <cstdint>

using u64 = std::uint64_t;

// (a * b) mod m without overflow, via a 128-bit intermediate.
inline u64 mulmod(u64 a, u64 b, u64 m) {
    return static_cast<u64>(static_cast<unsigned __int128>(a) * b % m);
}

// (base ^ exp) mod m by square-and-multiply.
inline u64 powmod(u64 base, u64 exp, u64 m) {
    u64 result = 1 % m;
    base %= m;
    while (exp > 0) {
        if (exp & 1) result = mulmod(result, base, m);
        base = mulmod(base, base, m);
        exp >>= 1;
    }
    return result;
}
