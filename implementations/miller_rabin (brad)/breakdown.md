# The algorithm

## The idea

We want to know whether a number `n` is prime. We pick a random number `a` and run a test on `n`. Every prime passes this test. Some composite numbers pass it too, so a pass alone is not enough.

To catch those composites, the test also watches the intermediate steps of the calculation. A prime never gives itself away along the way. A composite often does.

One random choice of `a` can miss a composite, so we repeat with new random choices until we are confident.

## Step by step

1. **Split `n-1` into parts.** Keep halving `n-1` until you get an odd number. Count the halvings: call that number `s`. The odd number you end with is `d`.
2. **Pick a random number `a`.** Multiply `a` by itself `d` times, keeping only the remainder after dividing by `n`. Call the result `x`.
3. **Check the first answer.** If `x` is `1` or `n-1`, this roundwwses. Stop here.
4. **Watch the steps.** Otherwise, square `x` up to `s-1` times, keeping the remainder each time:
   - If `x` becomes `n-1`, this round passes. Stop.
   - If `x` becomes `1` without having just been `n-1`, then `n` has given itself away. It is composite. Stop.
5. **No pass means composite.** If the steps run out without a pass, `n` is composite.
6. **Repeat.** Do this `k` times, each time with a different random `a`.

## What the answer means

- If any round shows `n` is composite, the answer is certain.
- If every round passes, the answer is "probably prime". One pass only means that particular `a` did not catch `n`.

## Technical analysis

### Why it works

Fermat's test only checks the last step, `a^(n-1) ≡ 1 (mod n)`. Miller-Rabin also checks how the chain of squarings reaches `1`.

- If `n` is prime, the only numbers that square to `1` (mod `n`) are `1` and `-1`.
- If `n` is composite, it can have other such numbers. These are called non-trivial square roots of 1. The squaring step looks for one.
- Carmichael numbers pass Fermat for every base coprime to them, but they have non-trivial square roots of 1. So for at least 3/4 of bases, the squaring check exposes them. See [guidance.md](../../guidance.md) for the comparison.

### Error bounds

- If `n` is composite, at least 3/4 of the bases are witnesses. A random base therefore catches a composite with probability at least 3/4.
- If `n` is prime, it always passes.
- After `k` rounds, the error for a composite is at most `4^-k`.

### Loop logic

```text
x = a^d mod n
if x == 1 or x == n-1: pass
repeat s-1 times:
    x = x² mod n
    if x == n-1: pass
    if x == 1: composite (previous x wasn't ±1)
fell off the end: composite
```

The last value of the chain, `a^(n-1)`, is the plain Fermat check. The loop never needs to reach it.

### Edge cases

- `n < 2`: not prime.
- `n` even: composite, unless `n = 2`.
- `n = 2` and `n = 3`: prime. Return early.
- Otherwise, pick `a` in `[2, n-2]`.
