# PERF Guide (Linux / WSL2)

This guide measures cache and branch behavior for each algorithm using `perf`.

## 1) Build
```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp src/Random_Splitting_Sort.cpp src/dual_pivot_quicksort.cpp -o benchmark
```

## 2) Verify perf availability
```bash
perf --version
```

## 3) Run isolated algorithm benchmarks
Use `--algo=` to isolate one algorithm path.

### RandomSplitting
```bash
./benchmark --algo=rs --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
```

### std::sort
```bash
./benchmark --algo=std --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
```

### DualPivot
```bash
./benchmark --algo=dual --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
```

## 4) Collect hardware counters with perf stat
Run each command separately and save outputs.
```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,branches,branch-misses ./benchmark --algo=rs --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
```
```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,branches,branch-misses ./benchmark --algo=std --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
```
```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,branches,branch-misses ./benchmark --algo=dual --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
```

## 5) Optional: repeat multiple times for stability
```bash
for i in {1..5}; do
  perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,branches,branch-misses ./benchmark --algo=dual --sizes=1000000 --correctness-cases=200 --trials-huge=20 --trials-large=20 --trials-medium=20 --trials-small=20
done
```

## Notes
- `perf` is Linux-only; use native Linux or WSL2.
- Keep CPU governor/machine load stable when comparing runs.
- Focus on both runtime and miss rates; lower misses can explain speedups.