#include "../include/dual_pivot_quicksort.hpp"
#include "../include/Random_Splitting_Sort.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

template <typename SortFn>
static bool run_suite(SortFn sort_fn, const char* name) {
    std::vector<std::vector<int>> cases = {
        {},
        {1},
        {2, 1},
        {5, 4, 3, 2, 1},
        {1, 1, 1, 1},
        {-3, 10, 0, -3, 7, 7, 2}
    };

    for (auto data : cases) {
        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on basic case\n";
            return false;
        }
    }

    // SKEWED DATA TESTS - Critical for real-world performance
    std::mt19937 rng(777u);

    // Test 1: Already sorted arrays (various sizes)
    for (int n : {0, 1, 10, 100, 1000, 5000}) {
        std::vector<int> data(n);
        for (int i = 0; i < n; ++i) data[i] = i;
        std::vector<int> expected = data;
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on sorted array size " << n << "\n";
            return false;
        }
    }

    // Test 2: Reverse sorted arrays
    for (int n : {0, 1, 10, 100, 1000, 5000}) {
        std::vector<int> data(n);
        for (int i = 0; i < n; ++i) data[i] = n - i;
        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on reverse sorted array size " << n << "\n";
            return false;
        }
    }

    // Test 3: Heavy duplicates (90% same value)
    for (int n : {10, 100, 1000, 5000}) {
        std::vector<int> data(n, 42);
        for (int i = 0; i < n / 10; ++i) {
            data[rng() % n] = static_cast<int>(rng() % 1000);
        }
        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on heavy duplicates size " << n << "\n";
            return false;
        }
    }

    // Test 4: All same value
    for (int n : {0, 1, 10, 100, 1000, 5000}) {
        std::vector<int> data(n, -999);
        std::vector<int> expected = data;
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on all-same array size " << n << "\n";
            return false;
        }
    }

    // Test 5: Nearly sorted (5% out of place)
    for (int n : {10, 100, 1000, 5000}) {
        std::vector<int> data(n);
        for (int i = 0; i < n; ++i) data[i] = i;
        for (int i = 0; i < n / 20; ++i) {
            int a = rng() % n;
            int b = rng() % n;
            std::swap(data[a], data[b]);
        }
        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on nearly sorted size " << n << "\n";
            return false;
        }
    }

    // Test 6: Zipf distribution (power law - common in real data)
    for (int n : {100, 1000, 5000}) {
        std::vector<int> data;
        data.reserve(n);
        for (int i = 0; i < n; ++i) {
            // Simple zipf: value proportional to 1/(rank^0.8)
            double u = static_cast<double>(rng()) / rng.max();
            int val = static_cast<int>(std::pow(u, -0.8) * 10.0);
            data.push_back(val % 100); // Keep values bounded
        }
        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on zipf distribution size " << n << "\n";
            return false;
        }
    }

    // Test 7: Pipe organ pattern (sorted then reverse sorted)
    for (int n : {10, 100, 1000, 5000}) {
        std::vector<int> data;
        data.reserve(n);
        for (int i = 0; i < n / 2; ++i) data.push_back(i);
        for (int i = n / 2; i > 0; --i) data.push_back(i);
        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());
        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on pipe organ size " << n << "\n";
            return false;
        }
    }

    // Test 8: Random uniform (original test - reduced count)
    std::uniform_int_distribution<int> size_dist(0, 5000);
    std::uniform_int_distribution<int> value_dist(-100000, 100000);

    for (int tc = 0; tc < 500; ++tc) {
        const int n = size_dist(rng);
        std::vector<int> data;
        data.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            data.push_back(value_dist(rng));
        }

        std::vector<int> expected = data;
        std::sort(expected.begin(), expected.end());

        sort_fn(data);
        if (data != expected) {
            std::cerr << name << " failed on random uniform case " << tc << "\n";
            return false;
        }
    }

    return true;
}

int main() {
    rssort::SortConfig cfg;
    cfg.small_threshold = 64;
    cfg.sample_size = 9;

    rssort::RandomSplittingSorter rs(20260302u, cfg);
    dqsort::DualPivotQuickSorter dual(20260303u);

    std::cout << "Testing RandomSplittingSort...\n";
    const bool rs_ok = run_suite([&](std::vector<int>& d) { rs.sort(d); }, "RandomSplittingSort");
    if (!rs_ok) {
        return 1;
    }
    std::cout << "RandomSplittingSort: PASSED\n\n";

    std::cout << "Testing DualPivotQuickSort...\n";
    const bool dual_ok = run_suite([&](std::vector<int>& d) { dual.sort(d); }, "DualPivotQuickSort");
    if (!dual_ok) {
        return 1;
    }
    std::cout << "DualPivotQuickSort: PASSED\n\n";

    std::cout << "All tests passed\n";
    return 0;
}