#include "../include/dual_pivot_quicksort.hpp"
#include "../include/Random_Splitting_Sort.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace std::chrono;

int main() {
    rssort::SortConfig cfg;
    cfg.small_threshold = 64;
    cfg.sample_size = 9;

    rssort::RandomSplittingSorter rs(20260302u, cfg);
    dqsort::DualPivotQuickSorter dual(20260303u);

    std::mt19937 rng(12345u);

    std::vector<size_t> sizes = {1'000'000, 10'000'000, 100'000'000};

    std::cout << "=== HEAVY DUPLICATES BENCHMARK ===\n";
    std::cout << "(95% same value, 5% random)\n\n";

    for (size_t n : sizes) {
        std::cout << "Size: " << n << " elements\n";

        // Generate heavy duplicates (95% value 42, 5% random)
        std::vector<int> data(n, 42);
        std::uniform_int_distribution<int> val_dist(-10'000, 10'000);
        for (size_t i = 0; i < n / 20; ++i) {
            data[rng() % n] = val_dist(rng);
        }

        // Test RandomSplit
        std::vector<int> data_copy = data;
        auto start = high_resolution_clock::now();
        rs.sort(data_copy);
        auto end = high_resolution_clock::now();
        double rs_time = duration_cast<microseconds>(end - start).count() / 1000.0;

        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        bool rs_correct = (data_copy == expected);

        std::cout << "  RandomSplit: " << rs_time << " ms "
                  << (rs_correct ? "PASS" : "FAIL") << "\n";

        // Test DualPivot
        data_copy = data;
        start = high_resolution_clock::now();
        dual.sort(data_copy);
        end = high_resolution_clock::now();
        double dual_time = duration_cast<microseconds>(end - start).count() / 1000.0;

        bool dual_correct = (data_copy == expected);

        std::cout << "  DualPivot  : " << dual_time << " ms "
                  << (dual_correct ? "PASS" : "FAIL") << "\n";

        double speedup = dual_time / rs_time;
        std::cout << "  Speedup: " << speedup << "x";
        if (speedup > 1.0) {
            std::cout << " (RandomSplit FASTER)";
        } else {
            std::cout << " (DualPivot faster)";
        }
        std::cout << "\n\n";
    }

    return 0;
}
