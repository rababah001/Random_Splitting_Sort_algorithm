#include "../include/dual_pivot_quicksort.hpp"
#include "../include/Random_Splitting_Sort.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

template <typename SortFn>
static bool run_suite(SortFn sort_fn) {
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
            return false;
        }
    }

    std::mt19937 rng(777u);
    std::uniform_int_distribution<int> size_dist(0, 5000);
    std::uniform_int_distribution<int> value_dist(-100000, 100000);

    for (int tc = 0; tc < 2500; ++tc) {
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

    const bool rs_ok = run_suite([&](std::vector<int>& d) { rs.sort(d); });
    if (!rs_ok) {
        std::cerr << "RandomSplitting tests failed\n";
        return 1;
    }

    const bool dual_ok = run_suite([&](std::vector<int>& d) { dual.sort(d); });
    if (!dual_ok) {
        std::cerr << "DualPivot tests failed\n";
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}