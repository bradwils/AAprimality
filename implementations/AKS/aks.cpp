// AKS primality test (deterministic, polynomial time).
// Primary author: Ben
//
// 1. n = a^b for some b >= 2 -> composite
// 2. find the smallest r with ord_r(n) > log2(n)^2
// 3. 1 < gcd(a, n) < n for some a <= r -> composite
// 4. n <= r -> prime
// 5. (X + a)^n != X^n + a mod (X^r - 1, n) for some a <= sqrt(phi(r)) log2 n -> composite
// 6. otherwise prime
//
// Plain textbook version: dense polynomials, schoolbook O(r^2) squaring, no NTT.

#include "aks.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

#include "modarith.hpp"

namespace {

using u128 = unsigned __int128;

bool pow_equals(u64 base, unsigned e, u64 target) {
    u128 acc = 1;
    for (unsigned i = 0; i < e; ++i) {
        acc *= base;
        if (acc > target) return false;
    }
    return acc == target;
}

bool is_perfect_power(u64 n) {
    for (unsigned b = 2; b <= 63; ++b) {
        u64 a = (u64)std::llround(std::pow((long double)n, 1.0L / b));
        if (a < 2) break;
        for (u64 c = a - 1; c <= a + 1; ++c) // pow() can be off by one
            if (c >= 2 && pow_equals(c, b, n)) return true;
    }
    return false;
}

u64 find_r(u64 n) {
    const double lg = std::log2((double)n);
    const u64 maxk = (u64)std::floor(lg * lg);
    for (u64 r = 2;; ++r) {
        if (std::gcd(n, r) != 1) continue; // order undefined; the factor is found in step 3
        u64 nm = n % r, x = 1;
        bool ok = true;
        for (u64 k = 1; k <= maxk; ++k) {
            x = mulmod(x, nm, r);
            if (x == 1) { ok = false; break; } // ord_r(n) = k <= log2(n)^2
        }
        if (ok) return r;
    }
}

u64 euler_phi(u64 r) {
    u64 res = r, m = r;
    for (u64 p = 2; p * p <= m; ++p) {
        if (m % p == 0) {
            while (m % p == 0) m /= p;
            res -= res / p;
        }
    }
    if (m > 1) res -= res / m;
    return res;
}

struct Poly {
    u64 n, r;
    bool small; // n < 2^32: coefficient products fit in 64 bits, so reduce once per sum
    std::vector<u64> rev; // scratch
};

// out = in^2 mod (X^r - 1, n)
void poly_square(Poly& c, const std::vector<u64>& in, std::vector<u64>& out) {
    const u64 r = c.r, n = c.n;
    if (c.small) {
        // out[k] = sum_i in[i] * in[(k - i) mod r], read contiguously via a reversed copy
        for (u64 j = 0; j < r; ++j) c.rev[j] = in[r - 1 - j];
        for (u64 k = 0; k < r; ++k) {
            const u64 s = r - 1 - k, first = r - s;
            u128 sum = 0;
            for (u64 i = 0; i < first; ++i) sum += in[i] * c.rev[s + i];
            for (u64 i = first; i < r; ++i) sum += in[i] * c.rev[s + i - r];
            out[k] = (u64)(sum % n);
        }
    } else {
        std::fill(out.begin(), out.end(), 0);
        for (u64 i = 0; i < r; ++i) {
            if (in[i] == 0) continue;
            for (u64 j = 0; j < r; ++j) {
                u64 k = i + j >= r ? i + j - r : i + j;
                u128 s = (u128)out[k] + mulmod(in[i], in[j], n);
                out[k] = (u64)(s >= n ? s - n : s);
            }
        }
    }
}

// out = in * (X + a) mod (X^r - 1, n)
void poly_mul_xa(const Poly& c, const std::vector<u64>& in, u64 a, std::vector<u64>& out) {
    for (u64 i = 0; i < c.r; ++i) {
        u128 s = (u128)in[i == 0 ? c.r - 1 : i - 1] + mulmod(in[i], a, c.n);
        out[i] = (u64)(s >= c.n ? s - c.n : s);
    }
}

// (X + a)^n == X^(n mod r) + a in (Z/nZ)[X] / (X^r - 1)
bool congruence_holds(Poly& c, u64 a, AksStats& st, std::vector<u64>& cur, std::vector<u64>& tmp) {
    const u64 n = c.n, r = c.r;
    std::fill(cur.begin(), cur.end(), 0);
    cur[0] = 1;
    for (int bit = 63 - __builtin_clzll(n); bit >= 0; --bit) {
        poly_square(c, cur, tmp);
        cur.swap(tmp);
        ++st.squarings;
        if ((n >> bit) & 1) {
            poly_mul_xa(c, cur, a % n, tmp);
            cur.swap(tmp);
        }
    }
    for (u64 i = 0; i < r; ++i) {
        u64 want = (i == n % r) + (i == 0 ? a : 0);
        if (cur[i] != want % n) return false;
    }
    return true;
}

}  // namespace

bool aks_with_stats(u64 n, AksStats* out) {
    AksStats st;
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    auto lap = [&](double& field) {
        if (!out) return;
        auto t1 = Clock::now();
        field = std::chrono::duration<double>(t1 - t0).count();
        t0 = t1;
    };
    auto finish = [&](const char* verdict, bool result) {
        st.verdict = verdict;
        if (out) *out = st;
        return result;
    };

    if (n < 2) return finish("too_small", false);

    bool pp = is_perfect_power(n);
    lap(st.t_step1);
    if (pp) return finish("perfect_power", false);

    const u64 r = find_r(n);
    st.r = r;
    lap(st.t_step2);

    for (u64 a = 2; a <= r && a < n; ++a) {
        u64 g = std::gcd(a, n);
        if (g > 1 && g < n) {
            lap(st.t_step3);
            return finish("factor_found", false);
        }
    }
    lap(st.t_step3);

    if (n <= r) return finish("n_le_r", true);

    st.a_limit = (u64)std::ceil(std::sqrt((double)euler_phi(r)) * std::log2((double)n));
    Poly c{n, r, n < (1ULL << 32), std::vector<u64>(r)};
    std::vector<u64> cur(r), tmp(r);
    for (u64 a = 1; a <= st.a_limit; ++a) {
        ++st.a_checked;
        if (!congruence_holds(c, a, st, cur, tmp)) {
            lap(st.t_step5);
            return finish("poly_mismatch", false);
        }
    }
    lap(st.t_step5);
    return finish("poly_all_passed", true);
}

bool aks(u64 n, TestParams&) { return aks_with_stats(n, nullptr); }
