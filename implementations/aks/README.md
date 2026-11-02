# AKS (deterministic primality test)

Primary author: Ben. Part of the AAprimality comparison; see the repository-root `README.md` for build, test and
benchmark commands (`make`, `make test`, `make bench`). Files in this folder:

```
include/aks.hpp        public interface: aks::is_prime(n, &stats) and the Stats struct
src/aks.cpp            implementation
tests/test_aks.cpp     tests against the shared trial-division oracle (common/oracle.hpp)
scripts/plot_aks.py    plots from the CSVs in results/
results/               measured AKS data (CSV) and the four plots
docs/video_script_aks.md  AKS portions of the video script
```

## The AKS implementation in one page

1. If `n = a^b` for some `b >= 2`, return composite.
2. Find the smallest `r` with `ord_r(n) > log2(n)^2`.
3. If any `a <= r` has `1 < gcd(a, n) < n`, return composite.
4. If `n <= r`, return prime.
5. For `a = 1 .. ceil(sqrt(phi(r)) * log2 n)`: check `(X + a)^n == X^n + a` in `(Z/nZ)[X] / (X^r - 1)`. Any failure means composite.
6. Otherwise return prime.

Design choices: coefficients are `uint64_t` and products use `unsigned __int128`, so every 64-bit `n` is
supported. Polynomials are dense arrays of `r` coefficients and squaring is schoolbook `O(r^2)`;
multiplying by `(X + a)` is done directly in `O(r)`. For `n < 2^32` each output coefficient of a square is
accumulated as one dot product with a single modular reduction. There is no FFT/NTT and no multithreading, on
purpose: this is the plain textbook algorithm so the comparison measures the algorithm, not engineering tricks.
Measured practical limit on the reference machine: a 26-bit prime takes about 6 s, the 31-bit prime 2^31-1 about 12 s and the largest 32-bit prime 2^32-5 about 15 s. Above 2^32 the arithmetic switches to a slower 128-bit path (about 17x slower per exponentiation, see "Known limitations"), so primes of 33+ bits take minutes or more.

## Known limitations

- **32-bit cliff.** For `n >= 2^32` coefficient products need 128-bit division, which is far slower than the 64-bit lazy-accumulation path used below 2^32. Semiprime timing jumps from 15.7 ms (32 bits) to 264 ms (33 bits) while `r` barely changes (see `results/aks_semiprime_cliff.csv`). This is an implementation artefact, not something AKS's asymptotics predict; keep it in mind when comparing against Fermat and Miller-Rabin.
- Schoolbook polynomial multiplication, single-threaded, no NTT: deliberate, but it makes AKS slower than optimised implementations.
- `aks_t*` step times in the CSV come from the last timed repetition, while `median_s` is the median over repetitions, so their sum can differ from `median_s` by a few percent.

## Measured results (reference machine, `-O2`)

| Input | Time |
|---|---|
| 8-bit prime | 1.8 ms |
| 20-bit prime | 1.0 s |
| 26-bit prime | 6.2 s |
| 2^31 - 1 (prime) | 12.4 s |
| 2^32 - 5 (prime) | 15.2 s |
| 24-bit balanced semiprime | 3.8 ms |
| random composites up to 40 bits | microseconds to about 1 ms |

Time on primes grows like bits^6.8 (fit over 12 to 26 bits), close to the roughly log^7 n expected with schoolbook polynomial multiplication.
