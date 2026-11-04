#!/usr/bin/env python3
"""Generate the shared input set for every algorithm. Pure Python, no packages needed.
Run from anywhere; output is deterministic (fixed seed).

Writes data/instances.csv with columns: family,bits,n,label,note
and data/carmichael_fermat.csv (for each Carmichael number, the exact fraction of
bases for which one Fermat round returns prime).

The labels never come from the C++ code. Primes are checked with a
deterministic Miller-Rabin in Python (first 12 prime bases, valid for
n < 3.3e24). Composites are known by construction (a product of primes) or
are checked the same way.
"""
import csv
import math
import os
import random
import sys

SEED = 20261102
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")
BITS = list(range(8, 65, 4))       # 8, 12, ..., 64
PER_BUCKET = 1000                  # instances per (family, bit length)
HARD_PER_BUCKET = 300              # worst-case composites per bit length
SQUARE_PER_BUCKET = 100            # prime squares per bit length (AKS step 1)
NEAR_2_32_BITS = range(29, 36)     # semiprimes either side of 2^32 (AKS arithmetic changes there)
NEAR_2_32_PER_BUCKET = 5
CARMICHAEL_LIMIT = 10_000_000      # brute-force search bound
CHERNICK_M_MAX = 242_000           # 1296 m^3 < 2^64 up to about this m

SMALL = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37]
rng = random.Random(SEED)


def strong_pass(n, a):
    """True if one Miller-Rabin round with base a returns prime (n odd, n > 2)."""
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    x = pow(a, d, n)
    if x == 1 or x == n - 1:
        return True
    for _ in range(s - 1):
        x = x * x % n
        if x == n - 1:
            return True
    return False


def is_prime(n):
    if n < 2:
        return False
    for p in SMALL:
        if n % p == 0:
            return n == p
    return all(strong_pass(n, a) for a in SMALL)


def check_prime_test():
    N = 2_000_000
    sieve = bytearray([1]) * (N + 1)
    sieve[0:2] = b"\x00\x00"
    for i in range(2, int(N ** 0.5) + 1):
        if sieve[i]:
            sieve[i * i::i] = bytearray(len(sieve[i * i::i]))
    for n in range(N + 1):
        assert is_prime(n) == bool(sieve[n]), n


def rand_odd(bits):
    return rng.getrandbits(bits - 2) << 1 | (1 << (bits - 1)) | 1


def rand_prime(bits):
    while True:
        n = rand_odd(bits)
        if is_prime(n):
            return n


def rand_composite(bits):
    while True:
        n = rand_odd(bits)
        if not is_prime(n):
            return n


def semiprime(bits):
    pb = bits // 2
    while True:
        p, q = rand_prime(pb), rand_prime(bits - pb)
        n = p * q
        if p != q and n.bit_length() == bits:
            return n, f"{p}*{q}"


def prime_square_root(bits):
    while True:
        p = rand_prime(bits // 2)
        if (p * p).bit_length() == bits:
            return p


def factor_small(n):
    out, p = [], 2
    while p * p <= n:
        while n % p == 0:
            out.append(p)
            n //= p
        p += 1 if p == 2 else 2
    if n > 1:
        out.append(n)
    return out


def korselt(n, factors):
    return len(set(factors)) == len(factors) and all((n - 1) % (p - 1) == 0 for p in factors)


def search_fermat_psp():
    """All odd composites below the limit that pass Fermat base 2, with factors."""
    out = []
    for n in range(3, CARMICHAEL_LIMIT, 2):
        if pow(2, n - 1, n) == 1 and not is_prime(n):
            out.append((n, factor_small(n)))
    return out


def phi_from_factors(factors):
    phi = 1
    for p in factors:
        phi *= p - 1
    return phi


def write(rows, path, header):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def main():
    os.makedirs(DATA, exist_ok=True)
    check_prime_test()
    print("Python prime check agrees with a sieve up to 2,000,000")

    rows = []

    for bits in BITS:
        for _ in range(PER_BUCKET):
            rows.append(("prime", bits, rand_prime(bits), "prime", ""))
        for _ in range(PER_BUCKET):
            rows.append(("rand_composite", bits, rand_composite(bits), "composite", ""))
        for _ in range(PER_BUCKET):
            n, note = semiprime(bits)
            rows.append(("semiprime", bits, n, "composite", note))

    # Carmichael numbers: every one below the limit, plus Chernick triples
    psp = search_fermat_psp()
    carm = [(n, f) for n, f in psp if korselt(n, f)]
    print(f"Carmichael numbers below {CARMICHAEL_LIMIT}: {len(carm)} (expected 105)")
    assert len(carm) == 105
    seen = {n for n, _ in carm}
    for m in range(1, CHERNICK_M_MAX):
        a, b, c = 6 * m + 1, 12 * m + 1, 18 * m + 1
        n = a * b * c
        if n >= 1 << 64:
            break
        if n not in seen and is_prime(a) and is_prime(b) and is_prime(c):
            carm.append((n, [a, b, c]))
            seen.add(n)
    carm.sort()
    fermat_rows = []
    for n, f in carm:
        assert not is_prime(n) and korselt(n, f)
        rows.append(("carmichael", n.bit_length(), n, "composite", "*".join(map(str, f))))
        phi = phi_from_factors(f)
        fermat_rows.append((n, n.bit_length(), "*".join(map(str, f)), phi, (phi - 2) / (n - 3)))
    write(fermat_rows, os.path.join(DATA, "carmichael_fermat.csv"),
          ["n", "bits", "factors", "phi", "fermat_liar_fraction"])
    print(f"Carmichael set: {len(carm)} numbers, up to {max(n for n, _ in carm).bit_length()} bits")

    # strong pseudoprimes to base 2 below the limit
    spsp2 = [n for n, _ in psp if strong_pass(n, 2)]
    for n in spsp2:
        rows.append(("spsp2", n.bit_length(), n, "composite", "strong pseudoprime to base 2"))
    print(f"strong pseudoprimes to base 2 below {CARMICHAEL_LIMIT}: {len(spsp2)}")

    # strong pseudoprimes to the first several prime bases (Jaeschke)
    multi = [
        (1373653, 2), (25326001, 3), (3215031751, 4), (2152302898747, 5),
        (3474749660383, 6), (341550071728321, 7), (3825123056546413051, 9),
    ]
    for n, nb in multi:
        bases = SMALL[:nb]
        assert not is_prime(n) and all(strong_pass(n, a) for a in bases), n
        rows.append(("spsp_multi", n.bit_length(), n, "composite",
                     "strong pseudoprime to bases " + ",".join(map(str, bases))))

    rows.extend(hard_family())

    # generated last so the families above match the original Miller-Rabin data set
    for bits in BITS:
        for _ in range(SQUARE_PER_BUCKET):
            p = prime_square_root(bits)
            rows.append(("prime_square", bits, p * p, "composite", f"{p}^2"))

    for bits in NEAR_2_32_BITS:
        for _ in range(NEAR_2_32_PER_BUCKET):
            n, note = semiprime(bits)
            rows.append(("semiprime_near_2^32", bits, n, "composite", note))

    write(rows, os.path.join(DATA, "instances.csv"), ["family", "bits", "n", "label", "note"])
    counts = {}
    for fam, *_ in rows:
        counts[fam] = counts.get(fam, 0) + 1
    print("instances.csv:", counts)


def hard_family():
    """Composites with the largest fraction of bases for which one round returns prime.

    In the exhaustive search in results/liars.csv (bench liars), the worst
    odd composites up to 20000 are n = p * (2p - 1) with both factors prime and
    p = 3 mod 4 (703 = 19*37, 1891 = 31*61, 12403 = 79*157). The share of
    such bases approaches 1/4 as p grows, so the same shape is used here
    at larger sizes.
    """
    rows = []
    for bits in range(16, 65, 4):
        got = set()
        tries = 0
        while len(got) < HARD_PER_BUCKET and tries < 300_000:
            tries += 1
            p = rand_prime(rng.choice([bits // 2, (bits + 1) // 2]))
            q = 2 * p - 1
            n = p * q
            if p % 4 == 3 and n.bit_length() == bits and is_prime(q):
                got.add(n)
        for n in sorted(got):
            p = (1 + math.isqrt(1 + 8 * n)) // 4
            assert p * (2 * p - 1) == n and not is_prime(n)
            rows.append(("hard", bits, n, "composite", f"{p}*{2 * p - 1}"))
    return rows


if __name__ == "__main__":
    sys.exit(main())
