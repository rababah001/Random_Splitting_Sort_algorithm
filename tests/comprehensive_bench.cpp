#include "../include/Random_Splitting_Sort.hpp"
#include "../include/dual_pivot_quicksort.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>

struct BenchResult {
    double avg_ms;
    double min_ms;
    std::string algorithm;
};

template<typename SortFunc>
BenchResult benchmark_algorithm(const std::string& name, std::size_t n, int runs, std::mt19937& rng, SortFunc sort_func) {
    std::uniform_int_distribution<int> dist(-1000000, 1000000);
    std::vector<double> times;

    for (int run = 0; run < runs; ++run) {
        std::vector<int> data(n);
        for (auto& v : data) v = dist(rng);

        auto start = std::chrono::high_resolution_clock::now();
        sort_func(data);
        auto end = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        times.push_back(ms);
    }

    double sum = 0;
    double min_time = times[0];
    for (double t : times) {
        sum += t;
        if (t < min_time) min_time = t;
    }

    return BenchResult{sum / runs, min_time, name};
}

void benchmark_size(std::size_t n, int runs) {
    std::mt19937 rng(42);

    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    if (n < 1000000) {
        std::cout << "  SIZE: " << (n / 1000.0) << "K elements\n";
    } else {
        std::cout << "  SIZE: " << (n / 1000000.0) << "M elements\n";
    }
    std::cout << "═══════════════════════════════════════════════════════════\n";

    std::vector<BenchResult> results;

    // Benchmark std::sort
    results.push_back(benchmark_algorithm("std::sort", n, runs, rng,
        [](std::vector<int>& data) {
            std::sort(data.begin(), data.end());
        }));

    // Benchmark Random Splitting Sort
    results.push_back(benchmark_algorithm("RandomSplittingSort", n, runs, rng,
        [](std::vector<int>& data) {
            rssort::RandomSplittingSorter sorter;
            sorter.sort(data);
        }));

    // Benchmark Dual Pivot Quicksort
    results.push_back(benchmark_algorithm("DualPivotQuicksort", n, runs, rng,
        [](std::vector<int>& data) {
            dqsort::DualPivotQuickSorter sorter;
            sorter.sort(data);
        }));

    // Print results
    std::cout << std::fixed << std::setprecision(2);

    // Find best time
    double best_time = results[0].avg_ms;
    for (const auto& r : results) {
        if (r.avg_ms < best_time) best_time = r.avg_ms;
    }

    for (const auto& r : results) {
        std::cout << std::setw(25) << std::left << r.algorithm << ": ";
        std::cout << std::setw(10) << std::right << r.avg_ms << " ms";
        std::cout << "  (min: " << std::setw(10) << r.min_ms << " ms)";

        double ratio = r.avg_ms / best_time;
        std::cout << "  ratio: " << std::setprecision(3) << ratio << "x";

        if (r.avg_ms == best_time) {
            std::cout << "  ← FASTEST";
        }
        std::cout << "\n";
    }

    std::cout << std::setprecision(2);
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     COMPREHENSIVE SORTING BENCHMARK (75K - 100M)         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << "\nAlgorithms tested:\n";
    std::cout << "  1. std::sort (introsort - hybrid quicksort/heapsort)\n";
    std::cout << "  2. RandomSplittingSort (4-way partition with pivot sampling)\n";
    std::cout << "  3. DualPivotQuicksort (Java-style dual pivot)\n";
    std::cout << "\n";

    // Test sizes and number of runs (fewer runs for larger sizes)
    struct TestConfig {
        std::size_t size;
        int runs;
    };

    std::vector<TestConfig> configs = {
        {75'000, 5},      // 75K
        {100'000, 5},     // 100K
        {250'000, 5},     // 250K
        {500'000, 5},     // 500K
        {1'000'000, 5},   // 1M
        {2'000'000, 5},   // 2M
        {5'000'000, 3},   // 5M
        {10'000'000, 3},  // 10M
        {25'000'000, 3},  // 25M
        {50'000'000, 2},  // 50M
        {100'000'000, 2}  // 100M
    };

    for (const auto& config : configs) {
        benchmark_size(config.size, config.runs);
    }

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    BENCHMARK COMPLETE                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    return 0;
}
