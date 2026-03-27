#include "../include/dual_pivot_quicksort.hpp"
#include "../include/Random_Splitting_Sort.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace std::chrono;

struct TestResult {
    const char* name;
    const char* pattern;
    size_t size;
    bool passed;
    double time_ms;
};

template <typename SortFn>
TestResult test_pattern(SortFn sort_fn, const char* name, const char* pattern,
                        std::vector<int>& data) {
    TestResult result;
    result.name = name;
    result.pattern = pattern;
    result.size = data.size();

    std::vector<int> expected = data;
    std::sort(expected.begin(), expected.end());

    auto start = high_resolution_clock::now();
    sort_fn(data);
    auto end = high_resolution_clock::now();

    result.time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.passed = (data == expected);

    return result;
}

void print_result(const TestResult& r) {
    std::cout << r.name << " | " << r.pattern << " | n=" << r.size
              << " | " << (r.passed ? "PASS" : "FAIL")
              << " | " << r.time_ms << " ms\n";
}

int main() {
    rssort::SortConfig cfg;
    cfg.small_threshold = 64;
    cfg.sample_size = 9;

    rssort::RandomSplittingSorter rs(20260302u, cfg);
    dqsort::DualPivotQuickSorter dual(20260303u);

    std::mt19937 rng(12345u);

    std::vector<size_t> sizes = {
        75'000,
        100'000,
        500'000,
        1'000'000,
        5'000'000,
        10'000'000,
        50'000'000,
        100'000'000
    };

    std::cout << "=== LARGE SCALE SKEWED DATA TESTS ===\n\n";

    for (size_t n : sizes) {
        std::cout << "\n--- Testing size: " << n << " ---\n";

        // Pattern 1: Sorted
        {
            std::cout << "Generating sorted data...\n";
            std::vector<int> data(n);
            for (size_t i = 0; i < n; ++i) {
                data[i] = static_cast<int>(i % 1'000'000);
            }

            std::vector<int> data_copy = data;
            auto r1 = test_pattern([&](std::vector<int>& d) { rs.sort(d); },
                                   "RandomSplit", "Sorted", data);
            print_result(r1);

            auto r2 = test_pattern([&](std::vector<int>& d) { dual.sort(d); },
                                   "DualPivot  ", "Sorted", data_copy);
            print_result(r2);
        }

        // Pattern 2: Reverse sorted
        {
            std::cout << "Generating reverse sorted data...\n";
            std::vector<int> data(n);
            for (size_t i = 0; i < n; ++i) {
                data[i] = static_cast<int>((n - i) % 1'000'000);
            }

            std::vector<int> data_copy = data;
            auto r1 = test_pattern([&](std::vector<int>& d) { rs.sort(d); },
                                   "RandomSplit", "Reverse", data);
            print_result(r1);

            auto r2 = test_pattern([&](std::vector<int>& d) { dual.sort(d); },
                                   "DualPivot  ", "Reverse", data_copy);
            print_result(r2);
        }

        // Pattern 3: Heavy duplicates (95% same value)
        {
            std::cout << "Generating heavy duplicates data...\n";
            std::vector<int> data(n, 42);
            std::uniform_int_distribution<int> val_dist(-10'000, 10'000);
            for (size_t i = 0; i < n / 20; ++i) {
                data[rng() % n] = val_dist(rng);
            }

            std::vector<int> data_copy = data;
            auto r1 = test_pattern([&](std::vector<int>& d) { rs.sort(d); },
                                   "RandomSplit", "HeavyDup", data);
            print_result(r1);

            auto r2 = test_pattern([&](std::vector<int>& d) { dual.sort(d); },
                                   "DualPivot  ", "HeavyDup", data_copy);
            print_result(r2);
        }

        // Pattern 4: All same
        {
            std::cout << "Generating all-same data...\n";
            std::vector<int> data(n, -999);

            std::vector<int> data_copy = data;
            auto r1 = test_pattern([&](std::vector<int>& d) { rs.sort(d); },
                                   "RandomSplit", "AllSame", data);
            print_result(r1);

            auto r2 = test_pattern([&](std::vector<int>& d) { dual.sort(d); },
                                   "DualPivot  ", "AllSame", data_copy);
            print_result(r2);
        }

        // Pattern 5: Random uniform (for sizes <= 10M to keep reasonable runtime)
        if (n <= 10'000'000) {
            std::cout << "Generating random uniform data...\n";
            std::vector<int> data(n);
            std::uniform_int_distribution<int> val_dist(-1'000'000, 1'000'000);
            for (size_t i = 0; i < n; ++i) {
                data[i] = val_dist(rng);
            }

            std::vector<int> data_copy = data;
            auto r1 = test_pattern([&](std::vector<int>& d) { rs.sort(d); },
                                   "RandomSplit", "Random", data);
            print_result(r1);

            auto r2 = test_pattern([&](std::vector<int>& d) { dual.sort(d); },
                                   "DualPivot  ", "Random", data_copy);
            print_result(r2);
        }

        std::cout << std::flush;
    }

    std::cout << "\n=== ALL LARGE SCALE TESTS COMPLETE ===\n";
    return 0;
}
