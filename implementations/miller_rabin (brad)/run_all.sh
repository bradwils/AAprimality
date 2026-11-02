#!/bin/sh
# Reproduce everything for the Miller-Rabin section.
# Run from this folder:  sh run_all.sh
# Needs: clang++ (or g++) with __int128 support, python3 (standard library only).
# Takes about 3 minutes, most of it the timing and error runs.
# Leave the machine idle while it runs, the timings are sensitive to other load.
set -e
cd "$(dirname "$0")"
mkdir -p build data results

CXX=${CXX:-clang++}
FLAGS="-std=c++17 -O2 -Wall -Wextra"

{
  echo "compiler: $($CXX --version | head -1)"
  echo "flags: $FLAGS"
  echo "cpu: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || grep -m1 'model name' /proc/cpuinfo)"
  echo "os: $(uname -sr)"
  echo "date: $(date '+%Y-%m-%d %H:%M:%S %Z')"
  echo "python: $(python3 --version)"
} > results/env.txt

python3 gen_data.py                                   # data/instances.csv, data/carmichael_fermat.csv
$CXX $FLAGS -o build/mr algorithm.cpp                 # command line tool
$CXX $FLAGS -o build/tests tests.cpp
$CXX $FLAGS -o build/bench bench.cpp
build/tests | tee results/test_log.txt                # exits non-zero if any check fails
build/bench all | tee results/bench_log.txt           # liars, time_bits, time_k, families, error, diag
python3 check_liar_formula.py                         # confirms the liar-count formula used in the report
python3 check_spsp_multi.py                           # expected vs seen survivors on the multi-base pseudoprimes
python3 make_charts.py                                # results/*.svg and results/nines_table.csv
