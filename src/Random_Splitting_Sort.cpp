#include "../include/Random_Splitting_Sort.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
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
    if (data.size() <= 1) return;

    // Large arrays: distribution-aware scatter + bucket sort
    if (data.size() >= 55000) {
        distribution_sort(data);
        return;
    }

    // Small arrays: existing recursive 4-way partition
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

    // Stack-allocated buffer
    std::array<int, 32> buf;
    for (std::size_t i = 0; i < sample_count; ++i) {
        buf[i] = data[rng_.uniform(left, right)];
    }

    std::sort(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(sample_count));

    int pivot_low  = buf[sample_count / 4];
    int pivot_mid  = buf[sample_count / 2];
    int pivot_high = buf[(3 * sample_count) / 4];

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
            if (current != low) {
                int tmp        = data[current];
                data[current]  = data[middle];
                data[middle]   = data[low];
                data[low]      = tmp;
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

void RandomSplittingSorter::distribution_sort(std::vector<int>& data) {
    const std::size_t n = data.size();

    // ── Phase 1: Find range (one linear scan) ──
    int min_val = data[0];
    int max_val = data[0];
    for (std::size_t i = 1; i < n; ++i) {
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }
    if (min_val == max_val) return;  // all equal

    // Guard: detect near-uniform data (heavy duplicates)
    // Only run expensive duplicate detection at 2M+ where it pays off
    if (n >= 2'000'000) {
        std::array<int, 32> sample;
        for (int i = 0; i < 32; ++i)
            sample[i] = data[rng_.uniform(0, n - 1)];

        std::sort(sample.begin(), sample.end());

        int best_val = sample[16]; // start with median
        int best_count = 1, cur_count = 1;
        for (int i = 1; i < 32; ++i) {
            if (sample[i] == sample[i-1]) ++cur_count;
            else cur_count = 1;
            if (cur_count > best_count) {
                best_count = cur_count;
                best_val = sample[i];
            }
        }

        if (best_count >= 20) {  // 60%+ of sample is same value
            // One 3-way partition, equal zone is permanently done
            int pivot = best_val;
            std::size_t lo = 0, mid = 0, hi = n - 1;
            while (mid <= hi) {
                if (data[mid] < pivot)       std::swap(data[lo++], data[mid++]);
                else if (data[mid] > pivot)  std::swap(data[mid], data[hi--]);
                else                         ++mid;
            }
            // Create sub-vectors and re-sort through full pipeline
            if (lo > 1) {
                std::vector<int> left(data.begin(), data.begin() + lo);
                sort(left);
                std::copy(left.begin(), left.end(), data.begin());
            }
            if (hi < n - 1) {
                std::vector<int> right(data.begin() + hi + 1, data.end());
                sort(right);
                std::copy(right.begin(), right.end(), data.begin() + hi + 1);
            }
            return;
        }
    }

    // Guard: detect sorted or reverse-sorted input
    // Sample 12 evenly-spaced elements, check if already ordered
    {
        bool is_sorted = true, is_reverse = true;
        const std::size_t step = n / 12;
        for (std::size_t i = step; i < n; i += step) {
            if (data[i] <= data[i - step]) is_sorted = false;   // must be strictly increasing
            if (data[i] >= data[i - step]) is_reverse = false;  // must be strictly decreasing
            if (!is_sorted && !is_reverse) break;
        }
        if (is_sorted) return;  // already sorted, done
        if (is_reverse) {
            std::reverse(data.begin(), data.end());
            return;
        }
    }

    // ── Phase 2: Setup buckets ──
    // Target ~2048 elements per bucket so each fits in L1/L2.
    std::size_t num_buckets = std::max(std::size_t(4), n / 2048);
    num_buckets = std::min(num_buckets, std::size_t(4096));
    const std::uint64_t range = static_cast<std::uint64_t>(max_val) - min_val + 1;

    // ── Phase 3: Count elements per bucket ──
    std::vector<std::size_t> counts(num_buckets, 0);
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t bucket = static_cast<std::size_t>(
            (static_cast<std::uint64_t>(data[i] - min_val) * num_buckets) / range);
        if (bucket >= num_buckets) bucket = num_buckets - 1;
        ++counts[bucket];
    }

    // ── Phase 4: Prefix sum for bucket start positions ──
    std::vector<std::size_t> offsets(num_buckets, 0);
    for (std::size_t i = 1; i < num_buckets; ++i) {
        offsets[i] = offsets[i - 1] + counts[i - 1];
    }

    // ── Phase 5: Buffered scatter (rhythmic writes) ──
    // Each bucket gets a small buffer that fits in L1 cache.
    // Writes stay local until buffer is full, then flush as one burst.
    static constexpr std::size_t BUF_SIZE = 64;
    static constexpr std::size_t HYBRID_THRESHOLD = 250000;

    // Hybrid: memcpy for <250K (better at small-medium), for-loop for >=250K (better at large)
    const bool use_memcpy = (n < HYBRID_THRESHOLD);

    // Flat buffer: num_buckets × BUF_SIZE ints, contiguous in memory.
    // At 256 buckets × 64 ints × 4 bytes = 64KB → fits in L1.
    std::vector<int> buffers(num_buckets * BUF_SIZE);
    std::vector<std::size_t> buf_counts(num_buckets, 0);
    std::vector<std::size_t> write_pos = offsets;  // tracks where to flush next

    std::vector<int> temp(n);

    for (std::size_t i = 0; i < n; ++i) {
        std::size_t bucket = static_cast<std::size_t>(
            (static_cast<std::uint64_t>(data[i] - min_val) * num_buckets) / range);
        if (bucket >= num_buckets) bucket = num_buckets - 1;

        // Write to L1-resident buffer (fast — ~4 cycles)
        buffers[bucket * BUF_SIZE + buf_counts[bucket]] = data[i];
        ++buf_counts[bucket];

        // Buffer full → flush as sequential burst write
        if (buf_counts[bucket] == BUF_SIZE) {
            const std::size_t src = bucket * BUF_SIZE;
            const std::size_t dst = write_pos[bucket];
            if (use_memcpy) {
                std::memcpy(&temp[dst], &buffers[src], BUF_SIZE * sizeof(int));
            } else {
                for (std::size_t j = 0; j < BUF_SIZE; ++j) {
                    temp[dst + j] = buffers[src + j];
                }
            }
            write_pos[bucket] += BUF_SIZE;
            buf_counts[bucket] = 0;
        }
    }

    // Flush remaining partial buffers (always use loop — count is variable and small)
    for (std::size_t b = 0; b < num_buckets; ++b) {
        const std::size_t remaining = buf_counts[b];
        const std::size_t src = b * BUF_SIZE;
        const std::size_t dst = write_pos[b];
        for (std::size_t j = 0; j < remaining; ++j) {
            temp[dst + j] = buffers[src + j];
        }
    }

    // ── Phase 6: Move back ──
    data = std::move(temp);

    // ── Phase 7: Sort each bucket (recursive distribution or 4-way partition) ──
    for (std::size_t b = 0; b < num_buckets; ++b) {
        if (counts[b] <= 1) continue;

        std::size_t left = offsets[b];
        std::size_t right = left + counts[b] - 1;

        if (counts[b] > 4096) {
            // Large bucket: recurse with distribution sort
            std::vector<int> sub(data.begin() + static_cast<std::ptrdiff_t>(left),
                                 data.begin() + static_cast<std::ptrdiff_t>(right) + 1);
            distribution_sort(sub);
            std::copy(sub.begin(), sub.end(),
                      data.begin() + static_cast<std::ptrdiff_t>(left));
        } else {
            // Small bucket: 4-way partition sort (fits in cache)
            std::size_t depth_limit = 0;
            for (std::size_t m = counts[b]; m > 1; m >>= 1) {
                ++depth_limit;
            }
            depth_limit *= 2;
            random_splitting_sort(data, left, right, depth_limit);
        }
    }
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