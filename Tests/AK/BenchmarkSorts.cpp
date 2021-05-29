/*
 * Copyright (c) 2021, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibTest/TestCase.h>

#include <AK/HeapSort.h>
#include <AK/QuickSort.h>
#include <AK/StdLibExtras.h>

static const size_t BENCHMARK_SIZE = 1024 * 1024 * 12;

static Vector<size_t> vec_for_quicksort_single;
static Vector<size_t> vec_for_quicksort_dual;
static Vector<size_t> vec_for_heapsort;

// Setup and final equality check may be slow, so separate these out from the actual
// benchmark. Thankfully, the TestCaseRunner will always run the tests in the order
// they are declared within each compilation unit.

TEST_CASE(aaa_setup)
{
    Vector<size_t> vector;
    vector.ensure_capacity(BENCHMARK_SIZE);
    for (size_t i = 0; i < BENCHMARK_SIZE; ++i) {
        // Inspired by FNV1
        vector.empend((i + 0x811c9dc5) * 0x01000193);
    }

    vec_for_quicksort_single = vector;
    vec_for_quicksort_dual = vector;
    vec_for_heapsort = vector;
    EXPECT_EQ(vec_for_quicksort_single.size(), BENCHMARK_SIZE);
    EXPECT_EQ(vec_for_quicksort_dual.size(), BENCHMARK_SIZE);
    EXPECT_EQ(vec_for_heapsort.size(), BENCHMARK_SIZE);
}

static bool size_t_less_than(const size_t& lhs, const size_t& rhs)
{
    return lhs < rhs;
}

BENCHMARK_CASE(run_quicksort_single)
{
    AK::single_pivot_quick_sort(vec_for_quicksort_single.begin(), vec_for_quicksort_single.end(), size_t_less_than);
}

BENCHMARK_CASE(run_quicksort_dual)
{
    AK::dual_pivot_quick_sort(vec_for_quicksort_dual, 0, vec_for_quicksort_dual.size() - 1, size_t_less_than);
}

BENCHMARK_CASE(run_heapsort)
{
    heap_sort(vec_for_heapsort, 0, vec_for_heapsort.size() - 1, size_t_less_than);
}

TEST_CASE(zzz_check_equality)
{
    EXPECT_EQ(vec_for_quicksort_single.size(), BENCHMARK_SIZE);
    EXPECT_EQ(vec_for_quicksort_single, vec_for_quicksort_dual);
    EXPECT_EQ(vec_for_quicksort_single, vec_for_heapsort);
    EXPECT_EQ(vec_for_quicksort_dual, vec_for_heapsort);
    for (size_t i = 0; i < BENCHMARK_SIZE - 1; ++i) {
        EXPECT(vec_for_quicksort_single[i] < vec_for_quicksort_single[i + 1]);
    }
}
