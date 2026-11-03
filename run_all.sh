#!/bin/sh
# Rebuild, test, benchmark and chart everything: sh run_all.sh [--fixed-clock]
# Needs CMake, a C++17 compiler with __int128 (GCC or Clang), python3 and matplotlib.
# Takes several minutes. Leave the machine idle: timings are sensitive to other load.
# --fixed-clock (Linux, asks for sudo): sets the performance governor and turns
# boost off for the run, then restores the previous settings.
set -e
cd "$(dirname "$0")"
mkdir -p results

[ -f data/instances.csv ] || python3 scripts/gen_data.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

CPU=/sys/devices/system/cpu
if [ "$1" = "--fixed-clock" ]; then
  if [ -f $CPU/cpufreq/boost ] && [ -f $CPU/cpu0/cpufreq/scaling_governor ]; then
    old_governor=$(cat $CPU/cpu0/cpufreq/scaling_governor)
    old_boost=$(cat $CPU/cpufreq/boost)
    old_epp=$(cat $CPU/cpu0/cpufreq/energy_performance_preference 2> /dev/null || true)
    restore() {
      echo "$old_governor" | sudo tee $CPU/cpu*/cpufreq/scaling_governor > /dev/null
      echo "$old_boost" | sudo tee $CPU/cpufreq/boost > /dev/null
      # AMD/Intel pstate drivers: the governor change also overrides this preference
      [ -z "$old_epp" ] || echo "$old_epp" | sudo tee $CPU/cpu*/cpufreq/energy_performance_preference > /dev/null || true
      echo "restored governor $old_governor, boost $old_boost"
    }
    sudo -v
    # refresh the sudo timestamp so restoring at the end needs no second password
    while kill -0 $$ 2> /dev/null; do sudo -n true; sleep 60; done 2> /dev/null &
    trap restore EXIT
    trap 'exit 1' INT TERM
    echo performance | sudo tee $CPU/cpu*/cpufreq/scaling_governor > /dev/null
    echo 0 | sudo tee $CPU/cpufreq/boost > /dev/null
  else
    echo "--fixed-clock: no cpufreq boost or governor settings found; using the default clock" >&2
  fi
fi

# pin the benchmark to one core so it is not moved between cores mid-run
# (core 2: core 0 usually handles most hardware interrupts)
PIN=""
command -v taskset > /dev/null && [ "$(nproc)" -gt 2 ] && PIN="taskset -c 2"

{
  echo "compiler: $(${CXX:-c++} --version | head -1)"
  echo "flags: -std=c++17 -O3 (CMake Release)"
  echo "cpu: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
  echo "governor: $(cat $CPU/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unknown), boost: $(cat $CPU/cpufreq/boost 2>/dev/null || echo unknown)"
  echo "pinned: ${PIN:-no}"
  echo "os: $(uname -sr)"
  echo "date: $(date '+%Y-%m-%d %H:%M:%S %Z')"
  echo "python: $(python3 --version)"
} > results/env.txt

build/tests | tee results/test_log.txt
$PIN build/bench all | tee results/bench_log.txt
python3 scripts/make_charts.py
