/*
 * Copyright (c) 2021, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibTest/TestCase.h>

#include <AK/HeapSort.h>
#include <AK/Noncopyable.h>
#include <AK/Random.h>
#include <AK/StdLibExtras.h>

struct NoCopy {
    AK_MAKE_NONCOPYABLE(NoCopy);

public:
    explicit NoCopy(int value)
        : m_value(value)
    {
    }
    NoCopy(NoCopy&&) = default;
    NoCopy& operator=(NoCopy&&) = default;

    int m_value { 0 };
};

static void try_sort_random_ints(size_t offset, size_t length)
{
    Vector<NoCopy> vector;

    // Prepare
    vector.ensure_capacity(offset + length);
    for (size_t i = 0; i < offset; ++i) {
        vector.empend(-42);
    }
    for (int i = 0; i < (int)length; ++i) {
        vector.empend(i);
    }
    // No need for a redzone at the end, because any accesses there will cause a VERIFY in Vector.

    // Shuffle
    for (size_t i = 0; i < length; ++i) {
        size_t j = get_random_uniform(length - i);
        if (i != j)
            swap(vector[offset + length - i - 1], vector[offset + j]);
    }
    if constexpr (false) {
        out("Sorting [");
        for (size_t i = 0; i < length; ++i) {
            out("{},", vector[offset + i].m_value);
        }
        outln("]");
    }

    // Sort
    heap_sort(vector, offset, offset + length - 1,
        [](auto& a, auto& b) { return a.m_value < b.m_value; });

    // Check
    EXPECT(vector.size() == offset + length);
    for (size_t i = 0; i < offset; ++i) {
        EXPECT_EQ(vector[i].m_value, -42);
    }
    for (int i = 0; i < (int)length; ++i) {
        EXPECT_EQ(vector[offset + i].m_value, i);
    }
}

TEST_CASE(basic_check)
{
    try_sort_random_ints(0, 10);
    try_sort_random_ints(5, 10);
    try_sort_random_ints(50, 3);
}

TEST_CASE(check_evil_setup)
{
    Vector<NoCopy> vector;
    vector.empend(2);
    vector.empend(0);
    vector.empend(1);

    heap_sort(vector, 0, 3 - 1,
        [](auto& a, auto& b) { return a.m_value < b.m_value; });

    EXPECT(vector.size() == 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(vector[i].m_value, i);
    }
}

TEST_CASE(check_edgecases)
{
    try_sort_random_ints(0, 2);
    try_sort_random_ints(512, 2);
    try_sort_random_ints(0, 3);
    try_sort_random_ints(0, 4);
    for (size_t length = 15; length <= 32; ++length) {
        try_sort_random_ints(0, length);
    }
    try_sort_random_ints(7, 1024);
}
