# Performance Analysis - Sorting Algorithms Benchmark

## Time Complexity Analysis

### 1. RandomSplitting Sort
- **Best Case:** O(n log n) - balanced 4-way partitions
- **Average Case:** O(n log n) - expected with random pivot sampling
- **Worst Case:** O(n log n) - depth limit forces fallback to std::sort
- **Space Complexity:** O(log n) - recursion stack only (in-place partitioning)

**Algorithm Structure:**
```
RandomSplitting(data, left, right, depth):
    if size <= threshold OR depth == 0:
        fallback to std::sort                    // O(n log n)

    sample = choose_random_sample(size=9)        // O(9) ≈ O(1)
    pivots = [p1, p2, p3] from sample quartiles  // O(1)

    partition_4_way(data, pivots)                // O(n) single pass
    → creates 4 regions: <p1, [p1,p2), [p2,p3), ≥p3

    recursively sort 4 regions                   // T(n/4) × 4
```

**Recurrence:** T(n) = 4T(n/4) + O(n) → **O(n log n)** by Master Theorem

---

### 2. DualPivot Quicksort (Yaroslavskiy)
- **Best Case:** O(n log n) - balanced partitions
- **Average Case:** O(n log n) - ~1.9n ln(n) comparisons (better constant than classic quicksort)
- **Worst Case:** O(n²) - degenerate partitions (rare with good pivot selection)
- **Space Complexity:** O(log n) - recursion stack

**Algorithm Structure:**
```
DualPivot(data, left, right, div):
    if size < 27:
        insertion_sort(data)                     // O(n²) but fast for small n

    m1 = left + size/div
    m2 = right - size/div
    pivots = [data[m1], data[m2]]                // O(1) deterministic

    partition_3_way(data, pivots)                // O(n) single pass
    → creates 3 regions: <p1, [p1,p2], >p2

    recursively sort 3 regions                   // T(n/3) × 3 (approx)
```

**Recurrence:** T(n) ≈ 3T(n/3) + O(n) → **O(n log n)**

---

### 3. std::sort (Introsort)
- **Best Case:** O(n log n)
- **Average Case:** O(n log n)
- **Worst Case:** O(n log n) - guaranteed (switches to heapsort if recursion depth exceeds 2 log n)
- **Space Complexity:** O(log n)

**Algorithm Structure:**
- **Quicksort** initially (median-of-3 pivot)
- **Heapsort** fallback if recursion depth limit exceeded
- **Insertion sort** for small subarrays (< 16 elements typically)

---

## Benchmark Results Summary

| Size | RandomSplitting | std::sort | DualPivot | Winner | RS vs std | DP vs std |
|------|-----------------|-----------|-----------|--------|-----------|-----------|
| 1K | 0.027 ms | 0.028 ms | 0.024 ms | DualPivot | 0.96× | **0.84×** |
| 10K | 0.599 ms | 0.611 ms | 0.582 ms | DualPivot | 0.98× | **0.95×** |
| 100K | 8.195 ms | 12.371 ms | 8.810 ms | **RandomSplitting** | **0.66×** | 0.71× |
| 1M | 111.9 ms | 111.8 ms | 87.8 ms | DualPivot | 1.00× | **0.79×** |
| 10M | 1086 ms | 1101 ms | 793 ms | DualPivot | 0.99× | **0.72×** |
| 100M | 9645 ms | 8501 ms | 8336 ms | DualPivot | 1.13× | **0.98×** |

---

## Why DualPivot Outperforms RandomSplitting

### 1. **Lower Constant Factors**
**DualPivot:**
- Deterministic pivot selection: `O(1)` - just array indexing
- 3-way partition: fewer comparisons per element
- No random number generation overhead

**RandomSplitting:**
- Random sampling: 9 RNG calls + sort
- 4-way partition: more complex with more swaps
- Overhead: `std::mt19937` is ~20-30 cycles per call

**Impact:** ~5-10% overhead from RNG alone

---

### 2. **Partition Efficiency**
**DualPivot 3-way partition:**
```
Single pass through array:
- Compare with pivot1, pivot2
- Average 1.5-2 comparisons per element
- Fewer swaps (elements stay in place if in middle region)
```

**RandomSplitting 4-way partition:**
```
Single pass through array:
- Compare with pivot_low, pivot_mid, pivot_high
- Average 2-3 comparisons per element
- More swaps (cascading swaps for low region: lines 69-70)
- Complex pointer management for 4 regions
```

**Code example from RandomSplitting:**
```cpp
if (data[current] < pivot_low) {
    if (current != middle) std::swap(data[current], data[middle]);  // Swap 1
    if (middle != low)     std::swap(data[low], data[middle]);      // Swap 2
    ++low; ++middle; ++current;
}
```
→ **Two swaps** for single element in lowest partition

**DualPivot equivalent:**
```cpp
if (a[k] < pivot1) {
    std::swap(a[k], a[less]);  // Single swap
    ++less;
}
```
→ **One swap** per element

---

### 3. **Cache Behavior**
**DualPivot:**
- Sequential access pattern in partition
- Better prefetcher utilization
- Deterministic access (no RNG state)

**RandomSplitting:**
- Random sampling disrupts cache
- RNG state access (64 bytes for mt19937)
- 4 boundary pointers (more cache pressure)

---

### 4. **Recursion Overhead**
**DualPivot:** 3 recursive calls per level (when pivot1 ≠ pivot2)
**RandomSplitting:** 4 recursive calls per level

**Recursion depth:**
- DualPivot: ~log₃(n) ≈ 1.89 log₂(n)
- RandomSplitting: ~log₄(n) ≈ 1.60 log₂(n)

RandomSplitting has *fewer* levels, but each level has more overhead from:
- 4 boundary checks
- 4 recursive calls to set up
- More complex partition logic

**Net effect:** DualPivot's simpler per-level work compensates for extra depth

---

### 5. **Small Array Handling**
Both use insertion sort for small arrays:
- DualPivot: threshold = 27 elements
- RandomSplitting: threshold = 8192 elements (default)

**DualPivot switches earlier** → less recursion overhead for small subarrays

---

### 6. **Branch Prediction**
**DualPivot:**
- Simpler branching in partition (3 main cases)
- Deterministic pivot selection (no random branches)
- Better CPU branch prediction

**RandomSplitting:**
- 4 cases in partition (4-way)
- Random pivot values → less predictable branches
- `if (current != middle)` checks add branch overhead

---

## Performance by Size Category

### Small (< 10K): DualPivot wins
- Lower overhead dominates
- Fewer instructions per element
- Better cache utilization

### Medium (100K): RandomSplitting wins! 🎯
- **34% faster than std::sort**
- Sweet spot where 4-way partitioning shines
- Good balance between partition overhead and recursion reduction

### Large (1M+): DualPivot wins
- Cache misses dominate for both
- DualPivot's lower constant factors matter more
- Memory bandwidth becomes bottleneck
- Simpler partition logic = fewer instructions

---

## Theoretical Comparison Count

For n = 1,000,000:

**Classic Quicksort:** ~2n ln(n) ≈ 27.6M comparisons
**DualPivot:** ~1.9n ln(n) ≈ 26.2M comparisons (5% fewer)
**RandomSplitting:** ~2.2n ln(n) ≈ 30.4M comparisons (est., due to 4-way)

**Actual performance depends on:**
- Comparison cost (integers: cheap)
- Swap cost (integers: cheap)
- Branch mispredictions
- Cache misses
- Memory bandwidth

---

## Recommendations

### Use RandomSplitting when:
- Working with medium-sized arrays (10K - 1M)
- Worst-case guarantees needed (depth limit + std::sort fallback)
- Research/experimentation with k-way partitioning

### Use DualPivot when:
- Maximum performance needed across all sizes
- Working with very large datasets (> 1M)
- Lower constant factors matter

### Use std::sort when:
- Production code (battle-tested, optimized)
- Guaranteed O(n log n) worst case needed
- Compiler may have architecture-specific optimizations

---

## Potential Optimizations for RandomSplitting

1. **Reduce partition swaps:** Optimize 4-way partition to minimize cascading swaps
2. **Tune threshold:** 8192 might be too high; try 128-512
3. **Cache-aware sampling:** Sample from cache-friendly positions
4. **Hybrid approach:** Use 2-way for small n, 4-way for larger n
5. **SIMD partitioning:** Vectorize comparisons for modern CPUs

---

Generated from benchmark run: 2026-03-26
Platform: Windows 11, MinGW g++ 15.2.0, C++20
