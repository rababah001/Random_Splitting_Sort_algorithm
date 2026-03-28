#include "../include/Random_Splitting_Sort.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>

struct BenchResult {
    double avg_ms;
    double min_ms;
};

template<typename Generator>
BenchResult benchmark(std::size_t n, int runs, Generator gen_data, bool use_rss) {
    std::vector<double> times;

    for (int run = 0; run < runs; ++run) {
        auto data = gen_data();

        auto start = std::chrono::high_resolution_clock::now();
        if (use_rss) {
            rssort::RandomSplittingSorter sorter;
            sorter.sort(data);
        } else {
            std::sort(data.begin(), data.end());
        }
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

    return BenchResult{sum / runs, min_time};
}

void benchmark_heavy_duplicates(std::size_t n, int runs) {
    std::mt19937 rng(42);

    auto gen_data = [&]() {
        std::uniform_int_distribution<int> dist(0, 1000);  // Only 1001 unique values
        std::vector<int> data(n);
        for (auto& v : data) v = dist(rng);
        return data;
    };

    auto std_result = benchmark(n, runs, gen_data, false);
    auto rss_result = benchmark(n, runs, gen_data, true);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  std::sort           : " << std::setw(10) << std_result.avg_ms << " ms";
    std::cout << "  (min: " << std::setw(10) << std_result.min_ms << " ms)\n";

    std::cout << "  RandomSplittingSort : " << std::setw(10) << rss_result.avg_ms << " ms";
    std::cout << "  (min: " << std::setw(10) << rss_result.min_ms << " ms)";

    double ratio = rss_result.avg_ms / std_result.avg_ms;
    std::cout << "  ratio: " << std::setprecision(3) << ratio << "x";

    if (rss_result.avg_ms < std_result.avg_ms) {
        std::cout << "  ← RSS WINS";
    } else {
        std::cout << "  ← std::sort wins";
    }
    std::cout << "\n";
}

void benchmark_all_same(std::size_t n, int runs) {
    auto gen_data = [&]() {
        std::vector<int> data(n, 42);  // All values = 42
        return data;
    };

    auto std_result = benchmark(n, runs, gen_data, false);
    auto rss_result = benchmark(n, runs, gen_data, true);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  std::sort           : " << std::setw(10) << std_result.avg_ms << " ms";
    std::cout << "  (min: " << std::setw(10) << std_result.min_ms << " ms)\n";

    std::cout << "  RandomSplittingSort : " << std::setw(10) << rss_result.avg_ms << " ms";
    std::cout << "  (min: " << std::setw(10) << rss_result.min_ms << " ms)";

    double ratio = rss_result.avg_ms / std_result.avg_ms;
    std::cout << "  ratio: " << std::setprecision(3) << ratio << "x";

    if (rss_result.avg_ms < std_result.avg_ms) {
        std::cout << "  ← RSS WINS";
    } else {
        std::cout << "  ← std::sort wins";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║    PATTERN-SPECIFIC BENCHMARK: DUPLICATES & ALL-SAME     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    struct TestConfig {
        std::size_t size;
        int runs;
    };

    std::vector<TestConfig> configs = {
        {100'000, 5},     // 100K
        {500'000, 5},     // 500K
        {1'000'000, 5},   // 1M
        {5'000'000, 3},   // 5M
        {10'000'000, 3},  // 10M
        {25'000'000, 2},  // 25M
        {50'000'000, 2},  // 50M
        {100'000'000, 2}  // 100M
    };

    // Test 1: Heavy Duplicates
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "PATTERN 1: HEAVY DUPLICATES (range 0-1000, many repeats)\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    for (const auto& config : configs) {
        if (config.size < 1000000) {
            std::cout << "SIZE: " << (config.size / 1000.0) << "K elements\n";
        } else {
            std::cout << "SIZE: " << (config.size / 1000000.0) << "M elements\n";
        }
        benchmark_heavy_duplicates(config.size, config.runs);
        std::cout << "\n";
    }

    // Test 2: All Same
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "PATTERN 2: ALL-SAME (every element = 42)\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    for (const auto& config : configs) {
        if (config.size < 1000000) {
            std::cout << "SIZE: " << (config.size / 1000.0) << "K elements\n";
        } else {
            std::cout << "SIZE: " << (config.size / 1000000.0) << "M elements\n";
        }
        benchmark_all_same(config.size, config.runs);
        std::cout << "\n";
    }

    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    BENCHMARK COMPLETE                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    return 0;
}
