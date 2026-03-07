#include "../include/Random_Splitting_Sort.hpp"

#include <algorithm>

namespace rssort {

RandomSplittingSorter::RandomSplittingSorter(std::uint32_t seed, SortConfig config)
    : rng_(seed), config_(config) {
    if (config_.small_threshold == 0) {
        config_.small_threshold = 64;
    }
    if (config_.sample_size < 3) {
        config_.sample_size = 3;
    }
}

void RandomSplittingSorter::sort(std::vector<int>& data) {
    if (data.size() <= 1) {
        return;
    }

    std::size_t depth_limit = 0;
    for (std::size_t m = data.size(); m > 1; m >>= 1) {
        ++depth_limit;
    }
    depth_limit *= 2;

    random_splitting_sort(data, depth_limit);
}

std::array<int, 3> RandomSplittingSorter::choose_pivots(const std::vector<int>& data) {
    const std::size_t n = data.size();
    const std::size_t sample_count = std::min(config_.sample_size, n);

    std::uniform_int_distribution<std::size_t> dist(0, n - 1);
    std::vector<int> sample;
    sample.reserve(sample_count);

    for (std::size_t i = 0; i < sample_count; ++i) {
        sample.push_back(data[dist(rng_)]);
    }

    std::sort(sample.begin(), sample.end());

    const std::size_t s = sample.size();
    int p1 = sample[s / 4];
    int p2 = sample[s / 2];
    int p3 = sample[(3 * s) / 4];

    if (p1 > p2) std::swap(p1, p2);
    if (p2 > p3) std::swap(p2, p3);
    if (p1 > p2) std::swap(p1, p2);

    return {p1, p2, p3};
}

void RandomSplittingSorter::random_splitting_sort(std::vector<int>& data, std::size_t depth_left) {
    const std::size_t n = data.size();
    if (n <= 1) {
        return;
    }

    if (n <= config_.small_threshold || depth_left == 0) {
        std::sort(data.begin(), data.end());
        return;
    }

    const auto pivots = choose_pivots(data);
    const int p1 = pivots[0];
    const int p2 = pivots[1];
    const int p3 = pivots[2];

    std::vector<int> q1, q2, q3, q4;

    q1.reserve(n / 4 + 8);
    q2.reserve(n / 4 + 8);
    q3.reserve(n / 4 + 8);
    q4.reserve(n / 4 + 8);

    for (int value : data) {
        if (value < p1)       q1.push_back(value);
        else if (value < p2)  q2.push_back(value);
        else if (value < p3)  q3.push_back(value);
        else                  q4.push_back(value);
    }

    if (q1.size() == n || q2.size() == n || q3.size() == n || q4.size() == n) {
        std::sort(data.begin(), data.end());
        return;
    }

    random_splitting_sort(q1, depth_left - 1);
    random_splitting_sort(q2, depth_left - 1);
    random_splitting_sort(q3, depth_left - 1);
    random_splitting_sort(q4, depth_left - 1);

    std::size_t pos = 0;
    for (int v : q1) data[pos++] = v;
    for (int v : q2) data[pos++] = v;
    for (int v : q3) data[pos++] = v;
    for (int v : q4) data[pos++] = v;
}

MemoryEstimate estimate_sort_ram_usage(std::size_t n) {
    const std::size_t input_vector_bytes = n * sizeof(int);
    const std::size_t temp_partition_bytes = n * sizeof(int);
    const std::size_t algorithm_overhead_bytes = 4 * sizeof(std::vector<int>);

    return MemoryEstimate{
        input_vector_bytes,
        temp_partition_bytes,
        algorithm_overhead_bytes,
        input_vector_bytes + temp_partition_bytes + algorithm_overhead_bytes
    };
}

double bytes_to_gib(std::size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
}

} // namespace rssort