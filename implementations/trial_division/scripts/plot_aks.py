#!/usr/bin/env python3
"""Plots for the AKS results. Usage: python3 scripts/plot_aks.py   (needs matplotlib)
Reads results/aks_primes.csv, aks_composites.csv, aks_semiprime_cliff.csv; writes PNGs to results/."""
import csv, collections, statistics as st
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

def load(path):
    return list(csv.DictReader(open(path)))

def med_by(rows, key):
    d = collections.defaultdict(list)
    for r in rows:
        d[key(r)].append(float(r["median_s"]))
    return {k: st.median(v) for k, v in d.items()}

primes = load("results/aks_primes.csv")
comps = load("results/aks_composites.csv")
cliff = load("results/aks_semiprime_cliff.csv")

# 1. Time vs bits by input family (log y): primes are the expensive case
fig, ax = plt.subplots(figsize=(7, 4.5))
p = med_by(primes, lambda r: int(r["bits"]))
ax.plot(sorted(p), [p[k] for k in sorted(p)], "o-", label="prime (full test)")
for fam in ["semiprime", "random_composite", "prime_square"]:
    m = med_by([r for r in comps if r["family"] == fam], lambda r: int(r["bits"]))
    ax.plot(sorted(m), [m[k] for k in sorted(m)], "s--", label=fam)
ax.set_yscale("log"); ax.set_xlabel("bits of n"); ax.set_ylabel("median time (s)")
ax.set_title("AKS time by input family"); ax.legend(); ax.grid(alpha=.3)
fig.tight_layout(); fig.savefig("results/aks_time_by_family.png", dpi=150)

# 2. Prime time vs bits on log-log axes with fitted exponent
import math
xs = sorted(p); ys = [p[k] for k in xs]
use = [(x, y) for x, y in zip(xs, ys) if x >= 12]
lx = [math.log(x) for x, _ in use]; ly = [math.log(y) for _, y in use]
n = len(use); mx = sum(lx)/n; my = sum(ly)/n
slope = sum((a-mx)*(b-my) for a, b in zip(lx, ly)) / sum((a-mx)**2 for a in lx)
fig, ax = plt.subplots(figsize=(7, 4.5))
ax.loglog(xs, ys, "o-")
ax.set_xlabel("bits of n (log scale)"); ax.set_ylabel("median time (s)")
ax.set_title(f"AKS on primes: time ~ bits^{slope:.1f} (fit, bits >= 12)"); ax.grid(alpha=.3, which="both")
fig.tight_layout(); fig.savefig("results/aks_prime_loglog.png", dpi=150)
print(f"fitted exponent of time vs bits (primes, bits>=12): {slope:.2f}")

# 3. The 32-bit cliff
c = med_by(cliff, lambda r: int(r["bits"]))
fig, ax = plt.subplots(figsize=(7, 4.5))
ax.bar([str(k) for k in sorted(c)], [c[k]*1000 for k in sorted(c)])
ax.set_xlabel("bits of n (semiprime)"); ax.set_ylabel("median time (ms)")
ax.set_title("One polynomial exponentiation: cliff at 2^32"); ax.grid(alpha=.3, axis="y")
fig.tight_layout(); fig.savefig("results/aks_32bit_cliff.png", dpi=150)

# 4. r versus log2(n)^2
rr = collections.defaultdict(list)
for r in primes: rr[int(r["bits"])].append(int(r["aks_r"]))
bs = sorted(rr)
fig, ax = plt.subplots(figsize=(7, 4.5))
ax.plot(bs, [st.median(rr[b]) for b in bs], "o-", label="r found")
ax.plot(bs, [b*b for b in bs], "k--", label="bits^2 = log2(n)^2")
ax.set_xlabel("bits of n"); ax.set_ylabel("r"); ax.legend(); ax.grid(alpha=.3)
ax.set_title("Step 2: r tracks log2(n)^2 closely")
fig.tight_layout(); fig.savefig("results/aks_r_vs_log2sq.png", dpi=150)
