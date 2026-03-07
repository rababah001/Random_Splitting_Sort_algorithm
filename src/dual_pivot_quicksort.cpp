#include "../include/dual_pivot_quicksort.hpp"

#include <algorithm>

namespace dqsort {

DualPivotQuickSorter::DualPivotQuickSorter(std::uint32_t seed)
    : seed_(seed) {}

void DualPivotQuickSorter::insertion_sort(std::vector<int>& a, int left, int right) {
    for (int i = left + 1; i <= right; ++i) {
        int ai = a[i];
        int j = i - 1;
        while (j >= left && a[j] > ai) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = ai;
    }
}

void DualPivotQuickSorter::dual_pivot_quicksort(std::vector<int>& a, int left, int right, int div) {
    const int len = right - left;
    if (len < 27) {
        insertion_sort(a, left, right);
        return;
    }

    int third = len / div;
    int m1 = left + third;
    int m2 = right - third;

    if (m1 <= left) {
        m1 = left + 1;
    }
    if (m2 >= right) {
        m2 = right - 1;
    }

    if (a[m1] < a[m2]) {
        std::swap(a[m1], a[left]);
        std::swap(a[m2], a[right]);
    } else {
        std::swap(a[m1], a[right]);
        std::swap(a[m2], a[left]);
    }

    int pivot1 = a[left];
    int pivot2 = a[right];

    int less = left + 1;
    int great = right - 1;

    for (int k = less; k <= great; ++k) {
        if (a[k] < pivot1) {
            std::swap(a[k], a[less]);
            ++less;
        } else if (a[k] > pivot2) {
            while (k < great && a[great] > pivot2) {
                --great;
            }
            std::swap(a[k], a[great]);
            --great;

            if (a[k] < pivot1) {
                std::swap(a[k], a[less]);
                ++less;
            }
        }
    }

    int dist = great - less;
    if (dist < 13) {
        ++div;
    }

    --less;
    ++great;

    std::swap(a[left], a[less]);
    std::swap(a[right], a[great]);

    dual_pivot_quicksort(a, left, less - 1, div);
    dual_pivot_quicksort(a, great + 1, right, div);

    if (dist > len - 13 && pivot1 != pivot2) {
        for (int k = less + 1; k < great; ++k) {
            if (a[k] == pivot1) {
                std::swap(a[k], a[less + 1]);
                ++less;
            } else if (a[k] == pivot2) {
                while (a[great - 1] == pivot2 && k < great - 1) {
                    --great;
                }
                std::swap(a[k], a[great - 1]);
                --great;

                if (a[k] == pivot1) {
                    std::swap(a[k], a[less + 1]);
                    ++less;
                }
            }
        }
    }

    if (pivot1 < pivot2) {
        dual_pivot_quicksort(a, less + 1, great - 1, div);
    }
}

void DualPivotQuickSorter::sort(std::vector<int>& data) {
    if (data.size() <= 1) {
        return;
    }

    // Keeps behavior deterministic when the class seed is changed in future variants.
    (void)seed_;
    dual_pivot_quicksort(data, 0, static_cast<int>(data.size()) - 1, 3);
}

} // namespace dqsort