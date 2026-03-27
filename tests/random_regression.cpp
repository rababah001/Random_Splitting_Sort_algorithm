#include "../include/dual_pivot_quicksort.hpp"
#include "../include/Random_Splitting_Sort.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace std::chrono;

struct TrialResults {
    std::vector<double> times;
    double avg;
    double min_time;
};

template <typename SortFn>
TrialResults run_trials(SortFn sort_fn, const std::vector<std::vector<int>>& datasets) {
    TrialResults results;
    results.times.reserve(datasets.size());

    for (const auto& data : datasets) {
        std::vector<int> copy = data;

        auto start = high_resolution_clock::now();
        sort_fn(copy);
        auto end = high_resolution_clock::now();

        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        results.times.push_back(time_ms);
    }

    results.avg = std::accumulate(results.times.begin(), results.times.end(), 0.0) / results.times.size();
    results.min_time = *std::min_element(results.times.begin(), results.times.end());

    return results;
}

void print_results(const char* name, const TrialResults& r) {
    std::cout << name << ":\n";
    std::cout << "  Trials: ";
    for (size_t i = 0; i < r.times.size(); ++i) {
        std::cout << std::fixed << std::setprecision(2) << r.times[i];
        if (i < r.times.size() - 1) std::cout << ", ";
    }
    std::cout << "\n";
    std::cout << "  Average: " << std::fixed << std::setprecision(2) << r.avg << " ms\n";
    std::cout << "  Min:     " << std::fixed << std::setprecision(2) << r.min_time << " ms\n\n";
}

int main() {
    rssort::SortConfig cfg;
    cfg.small_threshold = 64;
    cfg.sample_size = 9;

    rssort::RandomSplittingSorter rs(20260302u, cfg);
    dqsort::DualPivotQuickSorter dual(20260303u);

    std::vector<size_t> sizes = {500'000, 1'000'000, 2'000'000, 5'000'000, 10'000'000};
    const int num_trials = 10;

    std::cout << "=== RANDOM UNIFORM DATA BENCHMARK ===\n";
    std::cout << "10 trials per size, value range [-1M, 1M]\n\n";

    for (size_t n : sizes) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Size: " << n << " elements\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

        // Generate datasets for all trials
        std::vector<std::vector<int>> datasets;
        datasets.reserve(num_trials);

        std::mt19937 rng(42 + n);  // Seed varies by size for different data
        std::uniform_int_distribution<int> val_dist(-1'000'000, 1'000'000);

        for (int trial = 0; trial < num_trials; ++trial) {
            std::vector<int> data(n);
            for (size_t i = 0; i < n; ++i) {
                data[i] = val_dist(rng);
            }
            datasets.push_back(std::move(data));
        }

        // Test RandomSplittingSorter
        auto rs_results = run_trials([&](std::vector<int>& d) { rs.sort(d); }, datasets);
        print_results("RandomSplittingSorter", rs_results);

        // Test DualPivotQuickSorter
        auto dual_results = run_trials([&](std::vector<int>& d) { dual.sort(d); }, datasets);
        print_results("DualPivotQuickSorter", dual_results);

        // Test std::sort
        auto std_results = run_trials([](std::vector<int>& d) { std::sort(d.begin(), d.end()); }, datasets);
        print_results("std::sort", std_results);

        // Summary comparison
        std::cout << "Summary (average times):\n";
        std::cout << "  RandomSplit: " << std::fixed << std::setprecision(2) << rs_results.avg << " ms\n";
        std::cout << "  DualPivot:   " << std::fixed << std::setprecision(2) << dual_results.avg << " ms";
        if (dual_results.avg < rs_results.avg) {
            std::cout << "  (1." << std::fixed << std::setprecision(2)
                      << ((rs_results.avg / dual_results.avg - 1.0) * 100) << "% faster)\n";
        } else {
            std::cout << "  (1." << std::fixed << std::setprecision(2)
                      << ((dual_results.avg / rs_results.avg - 1.0) * 100) << "% slower)\n";
        }
        std::cout << "  std::sort:   " << std::fixed << std::setprecision(2) << std_results.avg << " ms\n";
        std::cout << "\n";
    }

    return 0;
}
