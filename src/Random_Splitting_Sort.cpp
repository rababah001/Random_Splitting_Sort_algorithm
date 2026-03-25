#include "../include/Random_Splitting_Sort.hpp"
#include <algorithm>
#include <bit>
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

    random_splitting_sort(data, 0, data.size() - 1, depth_limit);
}

std::array<int, 3> RandomSplittingSorter::choose_pivots(
    std::vector<int>& data, std::size_t left, std::size_t right)
{
    const std::size_t range_size = right - left + 1;
    const std::size_t sample_count = std::min(config_.sample_size, range_size);

    std::uniform_int_distribution<std::size_t> dist(left, right);
    std::vector<int> sample;
    sample.reserve(sample_count);

    for (std::size_t i = 0; i < sample_count; ++i) {
        sample.push_back(data[dist(rng_)]);
    }

    std::sort(sample.begin(), sample.end());

    const std::size_t s = sample.size();
    int pivot_low  = sample[s / 4];
    int pivot_mid  = sample[s / 2];
    int pivot_high = sample[(3 * s) / 4];

    if (pivot_low > pivot_mid)  std::swap(pivot_low, pivot_mid);
    if (pivot_mid > pivot_high) std::swap(pivot_mid, pivot_high);
    if (pivot_low > pivot_mid)  std::swap(pivot_low, pivot_mid);

    return {pivot_low, pivot_mid, pivot_high};
}

std::array<std::size_t, 3> RandomSplittingSorter::partition_four_way(
    std::vector<int>& data, std::size_t left, std::size_t right,
    int pivot_low, int pivot_mid, int pivot_high)
{
    std::size_t low     = left;
    std::size_t middle  = left;
    std::size_t current = left;
    std::size_t high    = right;

    while (current <= high) {
        if (data[current] < pivot_low) {
            if (current != middle) std::swap(data[current], data[middle]);
            if (middle != low)     std::swap(data[low], data[middle]);
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
            // Guard against underflow: if high is 0, the swapped element is now at position 0,
            // which is correct for the high partition, so we can safely exit the loop.
            if (high == 0) break;
            --high;
            // Note: current is NOT incremented - the swapped element needs re-evaluation
        }
    }

    return {low, middle, high + 1};
}

void RandomSplittingSorter::random_splitting_sort(
    std::vector<int>& data, std::size_t left, std::size_t right,
    std::size_t depth_left)
{
    if (left >= right) return;

    const std::size_t range_size = right - left + 1;

    if (range_size <= config_.small_threshold || depth_left == 0) {
        std::sort(data.begin() + static_cast<std::ptrdiff_t>(left),
                  data.begin() + static_cast<std::ptrdiff_t>(right) + 1);
        return;
    }

    const auto pivots = choose_pivots(data, left, right);
    const auto bounds = partition_four_way(
        data, left, right, pivots[0], pivots[1], pivots[2]);

    const std::size_t boundary_one   = bounds[0];
    const std::size_t boundary_two   = bounds[1];
    const std::size_t boundary_three = bounds[2];

    // boundary_one <= boundary_two <= boundary_three is always maintained by the partition.
    // Checking the middle values is redundant and right+1 can overflow if right == SIZE_MAX.
    bool degenerate_split =
        (boundary_one == left && boundary_three <= left + 1) ||
        (boundary_one >= right);

    if (degenerate_split) {
        std::sort(data.begin() + static_cast<std::ptrdiff_t>(left),
                  data.begin() + static_cast<std::ptrdiff_t>(right) + 1);
        return;
    }

    if (boundary_one > left)
        random_splitting_sort(data, left, boundary_one - 1, depth_left - 1);
    if (boundary_two > boundary_one)
        random_splitting_sort(data, boundary_one, boundary_two - 1, depth_left - 1);
    if (boundary_three > boundary_two)
        random_splitting_sort(data, boundary_two, boundary_three - 1, depth_left - 1);
    if (right >= boundary_three)
        random_splitting_sort(data, boundary_three, right, depth_left - 1);
}

MemoryEstimate estimate_sort_ram_usage(std::size_t n) {
    const std::size_t input_vector_bytes = n * sizeof(int);
    const std::size_t temp_partition_bytes = 0;

    const std::size_t depth = n > 1 ? 2 * (std::bit_width(n) - 1) : 0;
    const std::size_t stack_bytes = depth * 4 * sizeof(std::size_t);
    const std::size_t sample_bytes = 9 * sizeof(int);

    return MemoryEstimate{
        input_vector_bytes,
        temp_partition_bytes,
        stack_bytes + sample_bytes,
        input_vector_bytes + stack_bytes + sample_bytes
    };
}

double bytes_to_gib(std::size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
}

} // namespace rssort