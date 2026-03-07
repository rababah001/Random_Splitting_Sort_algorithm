#ifndef DUAL_PIVOT_QUICKSORT_HPP
#define DUAL_PIVOT_QUICKSORT_HPP

#include <cstdint>
#include <vector>

namespace dqsort {

class DualPivotQuickSorter {
public:
    explicit DualPivotQuickSorter(std::uint32_t seed = 5489u);
    void sort(std::vector<int>& data);

private:
    std::uint32_t seed_;
    void dual_pivot_quicksort(std::vector<int>& a, int left, int right, int div);
    void insertion_sort(std::vector<int>& a, int left, int right);  
};

} // namespace dqsort

#endif // DUAL_PIVOT_QUICKSORT_HPP