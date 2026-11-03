#!/bin/sh
# Rebuild, test, benchmark and chart everything: sh run_all.sh
# Needs CMake, a C++17 compiler with __int128 (GCC or Clang), python3 and matplotlib.
# Takes several minutes. Leave the machine idle: timings are sensitive to other load.
set -e
cd "$(dirname "$0")"
mkdir -p results

[ -f data/instances.csv ] || python3 scripts/gen_data.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

{
  echo "compiler: $(${CXX:-c++} --version | head -1)"
  echo "flags: -std=c++17 -O3 (CMake Release)"
  echo "cpu: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
  echo "os: $(uname -sr)"
  echo "date: $(date '+%Y-%m-%d %H:%M:%S %Z')"
  echo "python: $(python3 --version)"
} > results/env.txt

build/tests | tee results/test_log.txt
build/bench all | tee results/bench_log.txt
python3 scripts/make_charts.py
