# Errors in breakdown.md

Review of [breakdown.md](breakdown.md). Not yet fixed in the original.

## Errors

1. **Squaring section is muddled and partly backwards (line 32).**
   - The chain is `a^d, a^2d, a^4d, …, a^(n-1)`.
   - **Pass (probably prime):** `n-1` appears at any point in the chain, or the first value `x = a^d` is `1`.
   - **Composite, with certainty:** a `1` appears where the previous value was *not* `±1`. This is a nontrivial square root of 1.
   - "A 1 means nothing unless preceded by -1, which indicates a prime" overclaims. A pass only means this base didn't catch `n`, never that `n` is prime.
2. **"Our pattern" is undefined (line 36).** If `n-1` never appears in the chain, `n` is definitely composite.
3. **"75% accuracy" is the wrong claim (line 6).** The guarantee is one-sided.
   - If `n` is prime, it always passes.
   - If `n` is composite, at least 3/4 of bases are witnesses, so a random base catches it with probability ≥ 3/4.
   - After `k` rounds the error is ≤ 4^-k. "Composite" is certain; "prime" is probable.
4. **`S` is off by one (line 10).** `s` is how many times 2 divides `n-1`. The loop does `s-1` squarings. The last value, `a^(n-1)`, is the plain Fermat check and is never reached by the loop.

## Missing

- **Why it works.** Fermat only checks `a^(n-1) ≡ 1`. Miller-Rabin also checks how the value reaches 1.
  - If `n` is prime, `x² ≡ 1 (mod n)` has only the roots `±1`.
  - If `n` is composite, it can have other roots. The chain hunts for one.
  - Line 5 says composites need "filtering out" but never says how.
- **Link to the comparison.** Carmichael numbers pass Fermat for every coprime base, but the square-root check still catches them.
- **Edge cases.** `n < 2`, even `n`, and `n = 2, 3` are not covered. Pick `a` from `[2, n-2]`.

## Smaller things

- Line 4: "for any `a` coprime to `n`" is fine, but "some small bounds on a" is vague. Say `a` in `[2, n-2]`.
- Typos: "probaility", "infact", "preceeded".
- Use H3 headings for the stages rather than bold labels.

## Loop logic in plain words

```text
x = a^d mod n
if x == 1 or x == n-1: pass
repeat s-1 times:
    x = x² mod n
    if x == n-1: pass
    (if x == 1 here, previous x wasn't ±1 → composite)
fell off the end → composite
```
