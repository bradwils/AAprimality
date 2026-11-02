#!/usr/bin/env python3
"""Turn the CSVs in results/ into SVG charts. Pure Python, no packages needed.

The CSVs are laid out so they can be dropped straight into Excel as well.
"""
import csv
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "results")

# Okabe-Ito colours, readable with common colour blindness
BLUE, ORANGE, GREEN, PINK, VERM, SKY, GREY, BLACK = (
    "#0072B2", "#E69F00", "#009E73", "#CC79A7", "#D55E00", "#56B4E9", "#7a7a7a", "#222222")


def read(name):
    with open(os.path.join(RES, name)) as f:
        return list(csv.DictReader(f))


def nice_ticks(lo, hi, n=6):
    span = hi - lo
    raw = span / n
    mag = 10 ** math.floor(math.log10(raw))
    step = mag
    for m in (1, 2, 2.5, 5, 10):
        step = m * mag
        if span / step <= n:
            break
    t = math.ceil(lo / step - 1e-9) * step
    out = []
    while t <= hi + 1e-9 * step:
        out.append(round(t, 10))
        t += step
    return out


def fmt(v, log):
    if log:
        e = int(round(math.log10(v)))
        return f"1e{e}" if e not in (0, 1, 2) else str(int(10 ** e))
    if abs(v) >= 1000 and float(v).is_integer():
        return f"{int(v):,}"
    return f"{v:g}"


def chart(path, title, xlabel, ylabel, series, xlog=False, ylog=False, xlim=None, ylim=None,
          hlines=(), subtitle=None):
    W, H = 900, 480
    L, R, T, B = 78, 230, 62, 60
    pw, ph = W - L - R, H - T - B
    pts = [p for s in series for p in s["pts"]]
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts if p[1] is not None]
    x0, x1 = xlim or (min(xs), max(xs))
    y0, y1 = ylim or (min(ys), max(ys))
    if not ylim and not ylog:
        y0 = 0
        y1 = y1 * 1.05

    def tx(x):
        a = (math.log10(x) - math.log10(x0)) / (math.log10(x1) - math.log10(x0)) if xlog else (x - x0) / (x1 - x0)
        return L + a * pw

    def ty(y):
        a = (math.log10(y) - math.log10(y0)) / (math.log10(y1) - math.log10(y0)) if ylog else (y - y0) / (y1 - y0)
        return T + ph - a * ph

    o = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
         f'font-family="Helvetica, Arial, sans-serif" font-size="12">',
         f'<rect width="{W}" height="{H}" fill="#ffffff"/>',
         f'<text x="{L}" y="26" font-size="16" font-weight="bold" fill="{BLACK}">{title}</text>']
    if subtitle:
        o.append(f'<text x="{L}" y="44" fill="{GREY}">{subtitle}</text>')

    # grid and ticks
    if ylog:
        yt = [10 ** e for e in range(math.ceil(math.log10(y0) - 1e-9), math.floor(math.log10(y1) + 1e-9) + 1)]
    else:
        yt = nice_ticks(y0, y1)
    for v in yt:
        y = ty(v)
        o.append(f'<line x1="{L}" x2="{L + pw}" y1="{y:.1f}" y2="{y:.1f}" stroke="#e3e3e3"/>')
        o.append(f'<text x="{L - 8}" y="{y + 4:.1f}" text-anchor="end" fill="{GREY}">{fmt(v, ylog)}</text>')
    if xlog:
        xt = [10 ** e for e in range(math.ceil(math.log10(x0) - 1e-9), math.floor(math.log10(x1) + 1e-9) + 1)]
    else:
        xt = nice_ticks(x0, x1)
    for v in xt:
        x = tx(v)
        o.append(f'<line x1="{x:.1f}" x2="{x:.1f}" y1="{T}" y2="{T + ph}" stroke="#f0f0f0"/>')
        o.append(f'<text x="{x:.1f}" y="{T + ph + 18}" text-anchor="middle" fill="{GREY}">{fmt(v, xlog)}</text>')
    o.append(f'<rect x="{L}" y="{T}" width="{pw}" height="{ph}" fill="none" stroke="#999"/>')
    o.append(f'<text x="{L + pw / 2}" y="{H - 14}" text-anchor="middle" fill="{BLACK}">{xlabel}</text>')
    o.append(f'<text transform="translate(18 {T + ph / 2}) rotate(-90)" text-anchor="middle" fill="{BLACK}">{ylabel}</text>')

    for yv, label in hlines:
        y = ty(yv)
        o.append(f'<line x1="{L}" x2="{L + pw}" y1="{y:.1f}" y2="{y:.1f}" stroke="{GREY}" stroke-dasharray="6 4"/>')
        o.append(f'<text x="{L + 6}" y="{y - 5:.1f}" fill="{GREY}">{label}</text>')

    # series
    for s in series:
        col, dash = s["color"], s.get("dash")
        good = [(x, y) for x, y in s["pts"] if y is not None and y > 0 or (not ylog and y is not None)]
        if s.get("line", True) and len(good) > 1:
            d = " ".join(f"{'M' if i == 0 else 'L'}{tx(x):.1f} {ty(y):.1f}" for i, (x, y) in enumerate(good))
            da = f' stroke-dasharray="{dash}"' if dash else ""
            o.append(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="2"{da}/>')
        m = s.get("marker", "circle")
        r = s.get("size", 3.2)
        for x, y in s["pts"]:
            if y is None:
                continue
            if ylog and y <= 0:   # nothing seen: hollow marker on the floor
                o.append(f'<circle cx="{tx(x):.1f}" cy="{ty(y0):.1f}" r="{r + 0.5}" fill="#fff" stroke="{col}" stroke-width="1.5"/>')
            elif m == "circle":
                o.append(f'<circle cx="{tx(x):.1f}" cy="{ty(y):.1f}" r="{r}" fill="{col}"/>')
            elif m == "square":
                o.append(f'<rect x="{tx(x) - r:.1f}" y="{ty(y) - r:.1f}" width="{2 * r}" height="{2 * r}" fill="{col}"/>')
            elif m == "dot":
                o.append(f'<circle cx="{tx(x):.1f}" cy="{ty(y):.1f}" r="{r}" fill="{col}" fill-opacity="0.55"/>')

    # legend
    lx, ly = L + pw + 18, T + 8
    for i, s in enumerate(series):
        y = ly + i * 22
        da = f' stroke-dasharray="{s["dash"]}"' if s.get("dash") else ""
        if s.get("line", True):
            o.append(f'<line x1="{lx}" x2="{lx + 26}" y1="{y}" y2="{y}" stroke="{s["color"]}" stroke-width="2"{da}/>')
        else:
            o.append(f'<circle cx="{lx + 13}" cy="{y}" r="4" fill="{s["color"]}"/>')
        o.append(f'<text x="{lx + 34}" y="{y + 4}" fill="{BLACK}">{s["name"]}</text>')
    o.append("</svg>")
    with open(os.path.join(RES, path), "w") as f:
        f.write("\n".join(o))
    print("wrote", path)


def series_from(rows, filt, xkey, ykey):
    return [(float(r[xkey]), float(r[ykey])) for r in rows if filt(r)]


def nines_table(tk, er):
    """Error target in nines against rounds k, cost and measured error. Writes results/nines_table.csv.

    Each target N uses the smallest k with 4^-k <= 10^-N, so k = ceil(N * log4(10)). Error is only
    measured up to k = 12 (error_vs_k.csv), so later rows leave the measured columns blank.
    """
    t = {(r["family"], int(r["k"])): float(r["median_ns_per_call"]) for r in tk}
    e = {(r["subset"], int(r["k"])): r for r in er}
    out = []
    for nines in range(2, 11):
        k = math.ceil(nines * math.log(10) / math.log(4) - 1e-9)
        row = dict(nines=nines, k=k, bound=f"{4.0 ** -k:.3e}", bound_nines=f"{-math.log10(4.0 ** -k):.1f}",
                   prime_64_ns=t[("prime", k)], rand_composite_64_ns=t[("rand_composite", k)],
                   hard_family_rate="", hard_family_errors="", carmichael_rate="", carmichael_errors="")
        if ("hard_all", k) in e:
            row["hard_family_rate"] = e[("hard_all", k)]["rate"]
            row["hard_family_errors"] = e[("hard_all", k)]["errors"]
            row["carmichael_rate"] = e[("carmichael_all", k)]["rate"]
            row["carmichael_errors"] = e[("carmichael_all", k)]["errors"]
        out.append(row)
    with open(os.path.join(RES, "nines_table.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0]))
        w.writeheader()
        w.writerows(out)
    print("wrote nines_table.csv")
    return out


def main():
    tb = read("time_bits.csv")
    chart("time_vs_bits.svg", "Time per call against bit length",
          "bit length of n", "median ns per call (log scale)",
          [
              dict(name="prime, k = 9", color=BLUE, pts=series_from(tb, lambda r: r["family"] == "prime" and r["k"] == "9", "bits", "median_ns_per_call")),
              dict(name="prime, k = 1", color=SKY, pts=series_from(tb, lambda r: r["family"] == "prime" and r["k"] == "1", "bits", "median_ns_per_call")),
              dict(name="one a^(n-1) mod n", color=BLACK, dash="2 4", marker="square", size=2.6,
                   pts=series_from(tb, lambda r: r["family"] == "fermat_core_prime", "bits", "median_ns_per_call")),
              dict(name="semiprime, k = 9", color=VERM, pts=series_from(tb, lambda r: r["family"] == "semiprime" and r["k"] == "9", "bits", "median_ns_per_call")),
              dict(name="random composite, k = 9", color=GREEN, pts=series_from(tb, lambda r: r["family"] == "rand_composite" and r["k"] == "9", "bits", "median_ns_per_call")),
          ], ylog=True, ylim=(10, 100000),
          subtitle="1000 instances per point, median of 21 repetitions")

    tk = read("time_k.csv")
    chart("time_vs_k.svg", "Time per call against rounds k (64-bit inputs)",
          "rounds k", "median ns per call",
          [
              dict(name="prime", color=BLUE, pts=series_from(tk, lambda r: r["family"] == "prime", "k", "median_ns_per_call")),
              dict(name="semiprime", color=VERM, pts=series_from(tk, lambda r: r["family"] == "semiprime", "k", "median_ns_per_call")),
              dict(name="random composite", color=GREEN, pts=series_from(tk, lambda r: r["family"] == "rand_composite", "k", "median_ns_per_call")),
          ], xlim=(1, 20), subtitle="1000 instances per point, median of 21 repetitions")

    er = read("error_vs_k.csv")

    def ek(name):
        return [(float(r["k"]), float(r["rate"])) for r in er if r["subset"] == name]

    bound = [(k, 4.0 ** -k) for k in range(1, 13)]
    chart("error_vs_k.svg", "Wrongly accepted composites against rounds k",
          "rounds k", "share of calls that said prime (log scale)",
          [
              dict(name="bound 4^-k", color=GREY, dash="6 4", marker="none", pts=bound),
              dict(name="worst case p(2p-1)", color=VERM, pts=ek("hard_all")),
              dict(name="Carmichael", color=BLUE, pts=ek("carmichael_all")),
              dict(name="strong pseudoprimes", color=PINK, pts=ek("spsp_multi")),
              dict(name="semiprime, 64 bit", color=GREEN, pts=ek("semiprime_64")),
              dict(name="random composite, 64 bit", color=ORANGE, pts=ek("rand_composite_64")),
          ], ylog=True, ylim=(1e-8, 1), xlim=(1, 12),
          subtitle="Hollow markers on the floor mean no errors were seen. Detection floor is 1 in the call count.")

    nt = nines_table(tk, er)
    chart("time_vs_nines.svg", "Time per call against error target (64-bit inputs)",
          "error target in nines (k = 4, 5, 7, 9, 10, 12, 14, 15, 17)", "median ns per call",
          [
              dict(name="prime", color=BLUE, pts=[(r["nines"], r["prime_64_ns"]) for r in nt]),
              dict(name="random composite", color=GREEN, pts=[(r["nines"], r["rand_composite_64_ns"]) for r in nt]),
          ], xlim=(2, 10), subtitle="Smallest k that reaches each target; composites leave in round 1 at every k")

    fam = read("families.csv")
    names = [r["subset"] for r in fam]
    # bar chart drawn as a line-free scatter on a category axis
    bars = []
    for i, r in enumerate(fam):
        bars.append((i + 1, float(r["p99_ns"]) / float(r["p50_ns"])))
    chart("tail_p99_over_median.svg", "Tail of the call time at k = 9 (p99 divided by median)",
          "1 prime_64   2 rand_composite_64   3 semiprime_64   4 carmichael_60plus   5 hard_60plus",
          "p99 / median",
          [dict(name="p99 / median", color=BLUE, pts=bars, line=False, size=6, marker="square")],
          xlim=(0.5, len(fam) + 0.5), subtitle="200,000 calls per input family; the clock ticks every 42 ns")

    li = read("liars.csv")
    chart("liar_share_scatter.svg", "Share of bases that fool one Miller-Rabin round",
          "n (odd composites up to 20000)", "fraction of bases 2..n-2 that fail to expose n",
          [
              dict(name="odd composite", color=SKY, line=False, marker="dot", size=1.6,
                   pts=[(float(r["n"]), float(r["mr_liar_fraction"])) for r in li if r["carmichael"] == "0"]),
              dict(name="Carmichael (MR)", color=BLUE, line=False, size=4.5,
                   pts=[(float(r["n"]), float(r["mr_liar_fraction"])) for r in li if r["carmichael"] == "1"]),
              dict(name="Carmichael (Fermat)", color=VERM, line=False, size=4.5, marker="square",
                   pts=[(float(r["n"]), float(r["fermat_liar_fraction"])) for r in li if r["carmichael"] == "1"]),
          ], ylim=(0, 1), hlines=[(0.25, "1/4 bound")], subtitle="Every base checked for every n")


if __name__ == "__main__":
    main()
