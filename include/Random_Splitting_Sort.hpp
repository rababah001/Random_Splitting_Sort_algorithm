#ifndef RANDOM_SPLITTING_SORT_HPP
#define RANDOM_SPLITTING_SORT_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rssort {

class SplitMix64 {
public:
    explicit SplitMix64(std::uint64_t seed) : state_(seed) {}
    std::uint64_t operator()() {
        state_ += 0x9e3779b97f4a7c15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
    std::size_t uniform(std::size_t lo, std::size_t hi) {
        return lo + (*this)() % (hi - lo + 1);
    }
private:
    std::uint64_t state_;
};

struct SortConfig {
    std::size_t small_threshold = 8192;
    std::size_t sample_size = 9;
};

struct MemoryEstimate {
    std::size_t input_vector_bytes;
    std::size_t temp_partition_bytes;
    std::size_t algorithm_overhead_bytes;
    std::size_t estimated_peak_bytes;
};

class RandomSplittingSorter {
public:
    explicit RandomSplittingSorter(
        std::uint32_t seed = 5489u,
        SortConfig config = SortConfig{});

    void sort(std::vector<int>& data);

private:
    SplitMix64 rng_;
    SortConfig config_;

    std::array<int, 3> choose_pivots(
        std::vector<int>& data, std::size_t left, std::size_t right);

    std::array<std::size_t, 3> partition_four_way(
        std::vector<int>& data, std::size_t left, std::size_t right,
        int pivot_low, int pivot_mid, int pivot_high);

    void random_splitting_sort(
        std::vector<int>& data, std::size_t left, std::size_t right,
        std::size_t depth_left);

    void distribution_sort(std::vector<int>& data);
};

MemoryEstimate estimate_sort_ram_usage(std::size_t n);
double bytes_to_gib(std::size_t bytes);

} // namespace rssort

#endif // RANDOM_SPLITTING_SORT_HPP