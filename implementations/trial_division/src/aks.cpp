// AKS deterministic primality test. See include/aks.hpp.
// Primary author: <NAME - fill in>
#include "aks.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

namespace aks {
namespace {

using u64  = uint64_t;
using u128 = unsigned __int128;

struct Timer {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    double lap() {
        auto t1 = std::chrono::steady_clock::now();
        double s = std::chrono::duration<double>(t1 - t0).count();
        t0 = t1;
        return s;
    }
};

// ---------- Step 1: perfect power test ----------
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
        for (u64 c = (a > 1 ? a - 1 : 2); c <= a + 1; ++c) {
            if (c >= 2 && pow_equals(c, b, n)) return true;
        }
    }
    return false;
}

// ---------- Step 2: smallest r with ord_r(n) > log2(n)^2 ----------
u64 find_r(u64 n) {
    const double lg = std::log2((double)n);
    const u64 maxk = (u64)std::floor(lg * lg);
    for (u64 r = 2;; ++r) {
        if (std::gcd(n, r) != 1) continue;  // order undefined; step 3 deals with factors
        u64 nm = n % r, x = 1;
        bool ok = true;
        for (u64 k = 1; k <= maxk; ++k) {
            x = (u64)((u128)x * nm % r);
            if (x == 1) { ok = false; break; }  // ord_r(n) = k <= log^2 n
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

// ---------- Step 5: polynomial arithmetic in (Z/nZ)[X] / (X^r - 1) ----------
struct PolyCtx {
    u64 n;
    u64 r;
    bool small;             // n < 2^32 : products fit in 64 bits, allows lazy reduction
    std::vector<u64> rev;   // scratch: reversed copy of the operand
};

// out = in^2 mod (X^r - 1, n)
// Small path (n < 2^32): compute each output coefficient as one dot product
//   out[k] = sum_i in[i] * in[(k - i) mod r]
// using a reversed copy of `in` so both operands are read contiguously, with ONE modular
// reduction per output coefficient instead of one per product.
void poly_square(PolyCtx& c, const std::vector<u64>& in, std::vector<u64>& out) {
    const u64 r = c.r, n = c.n;
    if (c.small) {
        std::vector<u64>& rev = c.rev;
        for (u64 j = 0; j < r; ++j) rev[j] = in[r - 1 - j];
        for (u64 k = 0; k < r; ++k) {
            const u64 s = r - 1 - k;  // in[(k-i) mod r] == rev[(s + i) mod r]
            u128 sum = 0;
            const u64 first = r - s;  // i in [0, first) reads rev[s+i]; the rest wraps
            for (u64 i = 0; i < first; ++i) sum += (u64)(in[i] * rev[s + i]);
            for (u64 i = first; i < r; ++i) sum += (u64)(in[i] * rev[s + i - r]);
            out[k] = (u64)(sum % n);
        }
    } else {
        std::fill(out.begin(), out.end(), 0);
        for (u64 i = 0; i < r; ++i) {
            const u64 ai = in[i];
            if (ai == 0) continue;
            for (u64 j = 0; j < r; ++j) {
                u64 k = i + j; if (k >= r) k -= r;
                u64 prod = (u64)((u128)ai * in[j] % n);
                u128 s = (u128)out[k] + prod;
                out[k] = (u64)(s >= n ? s - n : s);
            }
        }
    }
}

// out = in * (X + a) mod (X^r - 1, n)   -- O(r)
void poly_mul_xa(const PolyCtx& c, const std::vector<u64>& in, u64 a, std::vector<u64>& out) {
    const u64 r = c.r, n = c.n;
    for (u64 i = 0; i < r; ++i) {
        u64 shifted = in[i == 0 ? r - 1 : i - 1];
        u64 scaled  = (u64)((u128)in[i] * a % n);
        u128 s = (u128)shifted + scaled;
        out[i] = (u64)(s >= n ? s - n : s);
    }
}

// Checks (X + a)^n == X^(n mod r) + a  in (Z/nZ)[X]/(X^r - 1)
bool poly_congruence_holds(PolyCtx& c, u64 a, Stats* st,
                           std::vector<u64>& cur, std::vector<u64>& tmp) {
    const u64 n = c.n, r = c.r;
    std::fill(cur.begin(), cur.end(), 0);
    cur[0] = 1;  // the polynomial "1"
    int top = 63;
    while (!((n >> top) & 1)) --top;
    for (int bit = top; bit >= 0; --bit) {
        poly_square(c, cur, tmp);
        cur.swap(tmp);
        if (st) ++st->poly_squarings;
        if ((n >> bit) & 1) {
            poly_mul_xa(c, cur, a % n, tmp);
            cur.swap(tmp);
            if (st) ++st->poly_linear_mults;
        }
    }
    // Expected: X^(n mod r) + a
    const u64 e = n % r;
    for (u64 i = 0; i < r; ++i) {
        u64 want = 0;
        if (i == e) want = 1;
        if (i == 0) want = (want + a) % n;
        if (cur[i] != want) return false;
    }
    return true;
}

}  // namespace

const char* verdict_name(Verdict v) {
    switch (v) {
        case Verdict::TooSmall:      return "too_small";
        case Verdict::PerfectPower:  return "perfect_power";
        case Verdict::FactorFound:   return "factor_found";
        case Verdict::SmallPrime:    return "n_le_r";
        case Verdict::PolyMismatch:  return "poly_mismatch";
        case Verdict::PolyAllPassed: return "poly_all_passed";
    }
    return "?";
}

bool is_prime(u64 n, Stats* st) {
    Stats local;
    Stats* s = st ? st : &local;
    *s = Stats{};
    Timer tm;

    if (n < 2) { s->verdict = Verdict::TooSmall; return false; }

    // Step 1
    bool pp = is_perfect_power(n);
    s->t_step1_s = tm.lap();
    if (pp) { s->verdict = Verdict::PerfectPower; return false; }

    // Step 2
    const u64 r = find_r(n);
    s->r = r;
    s->t_step2_s = tm.lap();

    // Step 3
    for (u64 a = 2; a <= r && a < n; ++a) {
        u64 g = std::gcd(a, n);
        if (g > 1 && g < n) {
            s->t_step3_s = tm.lap();
            s->verdict = Verdict::FactorFound;
            return false;
        }
    }
    s->t_step3_s = tm.lap();

    // Step 4
    if (n <= r) { s->verdict = Verdict::SmallPrime; return true; }

    // Step 5
    const double lg = std::log2((double)n);
    const u64 limit = (u64)std::ceil(std::sqrt((double)euler_phi(r)) * lg);
    s->a_limit = limit;

    PolyCtx ctx{n, r, n < (1ULL << 32), std::vector<u64>(r)};
    std::vector<u64> cur(r), tmp(r);
    for (u64 a = 1; a <= limit; ++a) {
        ++s->a_checked;
        if (!poly_congruence_holds(ctx, a, s, cur, tmp)) {
            s->t_step5_s = tm.lap();
            s->verdict = Verdict::PolyMismatch;
            return false;
        }
    }
    s->t_step5_s = tm.lap();
    s->verdict = Verdict::PolyAllPassed;
    return true;
}

}  // namespace aks
