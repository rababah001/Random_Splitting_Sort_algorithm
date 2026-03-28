#include "../include/Random_Splitting_Sort.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>

// Instrumented version of RandomSplittingSorter that tracks metrics
class InstrumentedSorter {
public:
    struct Metrics {
        std::size_t comparisons = 0;
        std::size_t recursive_calls = 0;
        std::size_t partition_calls = 0;
        std::size_t max_depth = 0;
        std::size_t current_depth = 0;
    };

    Metrics metrics;

    void sort(std::vector<int>& data) {
        metrics = Metrics{};  // Reset
        if (data.size() <= 1) return;

        std::size_t depth_limit = 0;
        for (std::size_t m = data.size(); m > 1; m >>= 1) {
            ++depth_limit;
        }
        depth_limit *= 2;

        random_splitting_sort(data, 0, data.size() - 1, depth_limit);
    }

private:
    std::mt19937 rng_{42};
    std::size_t small_threshold_ = 8192;

    void random_splitting_sort(std::vector<int>& data, std::size_t left, std::size_t right, std::size_t depth_left) {
        ++metrics.recursive_calls;
        ++metrics.current_depth;
        if (metrics.current_depth > metrics.max_depth) {
            metrics.max_depth = metrics.current_depth;
        }

        const std::size_t n = right - left + 1;

        // Base case: small arrays use std::sort
        if (n < small_threshold_ || depth_left == 0) {
            // Count comparisons for std::sort (approximation: n log n)
            metrics.comparisons += static_cast<std::size_t>(n * std::log2(n));
            std::sort(data.begin() + left, data.begin() + right + 1);
            --metrics.current_depth;
            return;
        }

        // Choose pivots
        auto pivots = choose_pivots(data, left, right);
        int pivot_low = pivots[0];
        int pivot_mid = pivots[1];
        int pivot_high = pivots[2];

        // Partition
        auto bounds = partition_four_way(data, left, right, pivot_low, pivot_mid, pivot_high);
        std::size_t mid1 = bounds[0];
        std::size_t mid2 = bounds[1];

        // Recursively sort the 4 partitions
        if (left < mid1) {
            random_splitting_sort(data, left, mid1, depth_left - 1);
        }
        if (mid1 + 1 < mid2) {
            random_splitting_sort(data, mid1 + 1, mid2, depth_left - 1);
        }
        if (mid2 + 1 < right) {
            random_splitting_sort(data, mid2 + 1, right, depth_left - 1);
        }

        --metrics.current_depth;
    }

    std::array<int, 3> choose_pivots(std::vector<int>& data, std::size_t left, std::size_t right) {
        const std::size_t range_size = right - left + 1;
        const std::size_t sample_count = std::min(std::size_t(9), range_size);

        std::array<int, 32> buf;
        for (std::size_t i = 0; i < sample_count; ++i) {
            std::uniform_int_distribution<std::size_t> dist(left, right);
            buf[i] = data[dist(rng_)];
        }

        // Count comparisons for pivot selection sort
        metrics.comparisons += sample_count * std::log2(sample_count);
        std::sort(buf.begin(), buf.begin() + sample_count);

        int pivot_low  = buf[sample_count / 4];
        int pivot_mid  = buf[sample_count / 2];
        int pivot_high = buf[(3 * sample_count) / 4];

        // 3 comparisons for pivot ordering
        metrics.comparisons += 3;
        if (pivot_low > pivot_mid)  std::swap(pivot_low, pivot_mid);
        if (pivot_mid > pivot_high) std::swap(pivot_mid, pivot_high);
        if (pivot_low > pivot_mid)  std::swap(pivot_low, pivot_mid);

        return {pivot_low, pivot_mid, pivot_high};
    }

    std::array<std::size_t, 3> partition_four_way(
        std::vector<int>& data, std::size_t left, std::size_t right,
        int pivot_low, int pivot_mid, int pivot_high)
    {
        ++metrics.partition_calls;

        std::size_t low = left;
        std::size_t middle = left;
        std::size_t current = left;
        std::size_t high = right;

        // Each element is compared at most 3 times (against 3 pivots)
        std::size_t n = right - left + 1;
        metrics.comparisons += 3 * n;  // Upper bound: 3 comparisons per element

        while (current <= high) {
            if (data[current] < pivot_low) {
                if (current != low) {
                    int tmp = data[current];
                    data[current] = data[middle];
                    data[middle] = data[low];
                    data[low] = tmp;
                }
                ++low;
                ++middle;
                ++current;
            }
            else if (data[current] < pivot_mid) {
                if (current != middle) std::swap(data[current], data[middle]);
                ++middle;
                ++current;
            }
            else if (data[current] < pivot_high) {
                ++current;
            }
            else {
                std::swap(data[current], data[high]);
                if (high == 0) break;
                --high;
            }
        }

        return {low - 1, middle - 1};
    }
};

template<typename Generator>
void analyze_pattern(const std::string& pattern_name, std::size_t n, Generator gen_data) {
    auto data = gen_data();

    InstrumentedSorter sorter;
    sorter.sort(data);

    double n_log_n = n * std::log2(static_cast<double>(n));
    double comparison_ratio = static_cast<double>(sorter.metrics.comparisons) / n_log_n;
    double calls_per_n = static_cast<double>(sorter.metrics.recursive_calls) / n;

    std::cout << std::setw(25) << std::left << pattern_name;
    std::cout << std::setw(12) << std::right << sorter.metrics.comparisons;
    std::cout << std::setw(10) << sorter.metrics.recursive_calls;
    std::cout << std::setw(8) << sorter.metrics.partition_calls;
    std::cout << std::setw(8) << sorter.metrics.max_depth;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(10) << comparison_ratio;
    std::cout << std::setw(12) << (calls_per_n * 1000);
    std::cout << "\n";
}

int main() {
    std::mt19937 rng(42);

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          EMPIRICAL COMPLEXITY ANALYSIS - INSTRUMENTED METRICS              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    std::vector<std::size_t> sizes = {10'000, 100'000, 1'000'000, 10'000'000};

    for (std::size_t n : sizes) {
        std::cout << "\n";
        std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
        if (n < 1'000'000) {
            std::cout << "SIZE: " << (n / 1000) << "K elements\n";
        } else {
            std::cout << "SIZE: " << (n / 1'000'000) << "M elements\n";
        }
        std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
        std::cout << std::setw(25) << std::left << "Pattern";
        std::cout << std::setw(12) << std::right << "Comparisons";
        std::cout << std::setw(10) << "RecCalls";
        std::cout << std::setw(8) << "Parts";
        std::cout << std::setw(8) << "MaxDepth";
        std::cout << std::setw(10) << "C/(nlogn)";
        std::cout << std::setw(12) << "Calls/n×1K";
        std::cout << "\n";
        std::cout << "───────────────────────────────────────────────────────────────────────────\n";

        // Random uniform
        analyze_pattern("Random uniform", n, [&]() {
            std::uniform_int_distribution<int> dist(-1000000, 1000000);
            std::vector<int> data(n);
            for (auto& v : data) v = dist(rng);
            return data;
        });

        // Heavy duplicates
        analyze_pattern("Heavy duplicates", n, [&]() {
            std::uniform_int_distribution<int> dist(0, 1000);
            std::vector<int> data(n);
            for (auto& v : data) v = dist(rng);
            return data;
        });

        // All same
        analyze_pattern("All same", n, [&]() {
            std::vector<int> data(n, 42);
            return data;
        });

        // Nearly sorted (90%)
        analyze_pattern("Nearly sorted (90%)", n, [&]() {
            std::vector<int> data(n);
            for (std::size_t i = 0; i < n; ++i) data[i] = static_cast<int>(i);
            std::uniform_int_distribution<std::size_t> dist(0, n - 1);
            for (std::size_t i = 0; i < n / 10; ++i) {
                std::swap(data[dist(rng)], data[dist(rng)]);
            }
            return data;
        });

        // Reverse sorted
        analyze_pattern("Reverse sorted", n, [&]() {
            std::vector<int> data(n);
            for (std::size_t i = 0; i < n; ++i) data[i] = static_cast<int>(n - i);
            return data;
        });

        std::cout << "\n";
        std::cout << "Theoretical O(n log n): " << std::fixed << std::setprecision(0)
                  << (n * std::log2(static_cast<double>(n))) << " comparisons\n";
    }

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                            ANALYSIS COMPLETE                               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Key metrics:\n";
    std::cout << "  - Comparisons: Total number of element comparisons\n";
    std::cout << "  - RecCalls: Number of recursive function calls\n";
    std::cout << "  - Parts: Number of partitioning operations\n";
    std::cout << "  - MaxDepth: Maximum recursion depth reached\n";
    std::cout << "  - C/(n log n): Ratio of actual comparisons to theoretical O(n log n)\n";
    std::cout << "  - Calls/n×1K: Recursive calls per 1000 elements (measures overhead)\n";
    std::cout << "\n";
    std::cout << "Expected results:\n";
    std::cout << "  - All-same: Very low recursive calls (early termination)\n";
    std::cout << "  - Heavy duplicates: Reduced recursion depth\n";
    std::cout << "  - Random: Full O(n log n) behavior\n";
    std::cout << "\n";

    return 0;
}
