#ifndef RANDOM_SPLITTING_SORT_HPP
#define RANDOM_SPLITTING_SORT_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace rssort {

struct SortConfig {
    std::size_t small_threshold = 64;
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
    std::mt19937 rng_;
    SortConfig config_;

    std::array<int, 3> choose_pivots(const std::vector<int>& data);
    void random_splitting_sort(std::vector<int>& data, std::size_t depth_left);
};

MemoryEstimate estimate_sort_ram_usage(std::size_t n);
double bytes_to_gib(std::size_t bytes);

} // namespace rssort

#endif // RANDOM_SPLITTING_SORT_HPP