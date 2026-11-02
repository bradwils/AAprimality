#!/usr/bin/env python3
"""Check the strong-liar counting formula against the exhaustive counts in results/liars.csv.

For squarefree odd composite n = p_1 ... p_w, write n - 1 = 2^s d and p_i - 1 = 2^(s_i) d_i.
The number of bases a in [1, n-1] that fool one Miller-Rabin round is

    (1 + (2^(w * min s_i) - 1) / (2^w - 1)) * product of gcd(d, p_i - 1)

results/liars.csv counts a in [2, n-2], so 2 is added for a = 1 and a = n - 1.
"""
import csv
import os
from math import gcd

HERE = os.path.dirname(os.path.abspath(__file__))


def factor(n):
    f, p = {}, 2
    while p * p <= n:
        while n % p == 0:
            f[p] = f.get(p, 0) + 1
            n //= p
        p += 1
    if n > 1:
        f[n] = f.get(n, 0) + 1
    return f


def predicted_liars(n, primes):
    w = len(primes)
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    smin, prod = 10 ** 9, 1
    for p in primes:
        q, si = p - 1, 0
        while q % 2 == 0:
            q //= 2
            si += 1
        smin = min(smin, si)
        prod *= gcd(d, p - 1)
    return (1 + (2 ** (w * smin) - 1) // (2 ** w - 1)) * prod


checked = bad = 0
for r in csv.DictReader(open(os.path.join(HERE, "results", "liars.csv"))):
    n = int(r["n"])
    f = factor(n)
    if any(e > 1 for e in f.values()):
        continue  # formula above is for squarefree n
    checked += 1
    if predicted_liars(n, list(f)) != int(r["mr_liars"]) + 2:
        bad += 1
print(f"squarefree odd composites checked: {checked}, formula mismatches: {bad}")
