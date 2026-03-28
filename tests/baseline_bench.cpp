#include "../include/Random_Splitting_Sort.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>

void benchmark_size(std::size_t n) {
    const int runs = 5;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1000000, 1000000);

    std::cout << "=== " << (n / 1'000'000.0) << "M random integers ===\n";

    // Benchmark std::sort
    std::vector<double> std_times;
    for (int run = 0; run < runs; ++run) {
        std::vector<int> data(n);
        for (auto& v : data) v = dist(rng);

        auto start = std::chrono::high_resolution_clock::now();
        std::sort(data.begin(), data.end());
        auto end = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std_times.push_back(ms);
    }

    double std_avg = 0;
    double std_min = std_times[0];
    for (double t : std_times) {
        std_avg += t;
        if (t < std_min) std_min = t;
    }
    std_avg /= runs;

    // Benchmark Random Splitting Sort
    rssort::RandomSplittingSorter sorter;
    std::vector<double> rss_times;
    for (int run = 0; run < runs; ++run) {
        std::vector<int> data(n);
        for (auto& v : data) v = dist(rng);

        auto start = std::chrono::high_resolution_clock::now();
        sorter.sort(data);
        auto end = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        rss_times.push_back(ms);
    }

    double rss_avg = 0;
    double rss_min = rss_times[0];
    for (double t : rss_times) {
        rss_avg += t;
        if (t < rss_min) rss_min = t;
    }
    rss_avg /= runs;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "std::sort:\n";
    for (int i = 0; i < runs; ++i) {
        std::cout << "  Run " << (i+1) << ": " << std_times[i] << " ms\n";
    }
    std::cout << "  Average: " << std_avg << " ms, Min: " << std_min << " ms\n\n";

    std::cout << "RandomSplittingSorter:\n";
    for (int i = 0; i < runs; ++i) {
        std::cout << "  Run " << (i+1) << ": " << rss_times[i] << " ms\n";
    }
    std::cout << "  Average: " << rss_avg << " ms, Min: " << rss_min << " ms\n";

    double ratio = rss_avg / std_avg;
    std::cout << "  Ratio (RSS/std): " << std::setprecision(3) << ratio << "x\n\n";
}

int main() {
    std::cout << "Baseline Random Splitting Sort benchmark\n";
    std::cout << "=========================================\n\n";

    benchmark_size(1'000'000);   // 1M
    benchmark_size(2'000'000);   // 2M
    benchmark_size(5'000'000);   // 5M
    benchmark_size(10'000'000);  // 10M

    return 0;
}
