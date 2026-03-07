# Random Splitting Sort

Random Splitting Sort is a randomized 4-way partition sorting algorithm for integer vectors.
This project also includes a paper-inspired Dual-Pivot Quicksort implementation (Yaroslavskiy-style) and a benchmark harness against `std::sort`.

## What It Offers
- A distinct alternative to classic 2-way partitioning.
- Robust randomized pivot sampling for quarter partitioning.
- Side-by-side benchmarking with:
  - `RandomSplitting`
  - `std::sort`
  - `DualPivot` (Yaroslavskiy-style)
- Strong correctness checks against `std::sort`.
- Safety controls for max `n`, max estimated RAM, and max single-run time.

## Project Structure
- `include/Random_Splitting_Sort.hpp`: RandomSplitting public API.
- `src/Random_Splitting_Sort.cpp`: RandomSplitting implementation.
- `include/dual_pivot_quicksort.hpp`: DualPivot public API.
- `src/dual_pivot_quicksort.cpp`: DualPivot implementation.
- `main.cpp`: benchmark CLI app.
- `tests/tests.cpp`: correctness tests for both custom algorithms.
- `.github/workflows/ci.yml`: CI build+test workflow.
- `PERF_GUIDE.md`: Linux/WSL hardware-counter profiling guide.

## Build
### Benchmark app
```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp src/Random_Splitting_Sort.cpp src/dual_pivot_quicksort.cpp -o benchmark
```

### Tests
```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/tests.cpp src/Random_Splitting_Sort.cpp src/dual_pivot_quicksort.cpp -o tests_runner
./tests_runner
```

## CLI Usage
```bash
./benchmark --sizes=10000,100000,1000000 --correctness-cases=3000 --algo=all
```

### Useful options
- `--algo=all|rs|std|dual`
- `--seed=20260302`
- `--small-threshold=64`
- `--sample-size=9`
- `--max-elements=2000000`
- `--max-ram-gib=0.5`
- `--max-single-run-ms=5000`
- `--trials-small=200 --trials-medium=100 --trials-large=30 --trials-huge=10`

## Complexity Notes
### RandomSplitting
- Expected near `O(n log n)` with balanced partitions.
- Uses temporary partition vectors: `O(n)` extra memory.
- Has safeguards (depth limit + degenerate split fallback).

### DualPivot
- In-place partitioning, expected near `O(n log n)`.
- Extra memory typically `O(log n)` recursion stack.

## Edge Cases
- `n = 0` and `n = 1`: immediate return.
- Duplicates and all-equal values: handled.
- Negative/positive mixes: handled.

## Production Note
`std::sort` remains the default baseline for production.
This repository is optimized for research, comparison, and algorithm exploration with strong validation tooling.