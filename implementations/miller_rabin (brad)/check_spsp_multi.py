#!/usr/bin/env python3
"""Expected number of calls that get through k rounds on the 7 multi-base strong pseudoprimes.

Uses the same liar-count formula as check_liar_formula.py, with the factorisations written out
(the product is checked against n). Compares with the events counted in results/error_vs_k.csv.
"""
import csv
import os
from math import gcd

HERE = os.path.dirname(os.path.abspath(__file__))

FACTORS = {
    1373653: [829, 1657],
    25326001: [2251, 11251],
    3215031751: [151, 751, 28351],
    2152302898747: [6763, 10627, 29947],
    3474749660383: [1303, 16927, 157543],
    341550071728321: [10670053, 32010157],
    3825123056546413051: [149491, 747451, 34233211],
}


def liar_share(n, primes):
    """Share of bases in [2, n-2] that fool one round (n squarefree)."""
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
    liars = (1 + (2 ** (w * smin) - 1) // (2 ** w - 1)) * prod   # in [1, n-1]
    return (liars - 2) / (n - 3)


shares = []
for n, f in FACTORS.items():
    prod = 1
    for p in f:
        prod *= p
    assert prod == n, n
    shares.append(liar_share(n, f))
print("per-round liar shares:", ", ".join(f"{s:.4f}" for s in shares))

seen = {}
for r in csv.DictReader(open(os.path.join(HERE, "results", "error_vs_k.csv"))):
    if r["subset"] == "spsp_multi":
        seen[int(r["k"])] = (int(r["calls"]), int(r["errors"]))

print("k  calls     expected  observed")
for k in sorted(seen):
    calls, errors = seen[k]
    expected = sum(calls / len(shares) * s ** k for s in shares)   # calls are spread evenly over the 7
    print(f"{k:<2} {calls:<9} {expected:8.2f}  {errors}")
