/*
 * Copyright (c) 2021, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StdLibExtras.h>
#include <AK/Types.h>

namespace AK {

/* Like QuickSort, HeapSort is an unstable sort, meaning that the order of two equal
 * elements might be reversed. However, Serenity only needs a stable sort in a single
 * place: the JS engine, which also has additional requirements like early-stopping.
 *
 * In contrast to QuickSort, HeapSort uses only constant additional memory and doesn't
 * do any recursion, and is therefore an attractive alternative. Also, it runs in
 * O(n log n) time, avoiding the n^2 worst-case running time of QuickSort.
 */

ALWAYS_INLINE size_t node_parent(size_t index)
{
    return (index - 1) / 2;
}

ALWAYS_INLINE size_t node_child_left(size_t index)
{
    return index * 2 + 1;
}

ALWAYS_INLINE size_t node_child_right(size_t index)
{
    return index * 2 + 2;
}

template<typename Collection, typename LessThan>
void sift_down(size_t i, Collection& col, size_t start, size_t end, LessThan& less_than)
{
    const size_t last_parent = node_parent(end - start);
    while (i <= last_parent) {
        size_t candidate = i;
        const size_t left_child = node_child_left(i);
        if (less_than(col[start + i], col[start + left_child])) {
            candidate = left_child;
        }
        const size_t right_child = node_child_right(i);
        if (start + right_child <= end && less_than(col[start + candidate], col[start + right_child])) {
            candidate = right_child;
        }
        if (i == candidate) {
            // Node 'i' is consistent with its children. There is nothing else to do!
            break;
        }

        swap(col[start + i], col[start + candidate]);
        i = candidate;
    }
}

template<typename Collection, typename LessThan>
void heapify(Collection& col, size_t start, size_t end, LessThan& less_than)
{
    // Starting at the last parent, work "backwards" to index 0.
    const size_t last_parent = node_parent(end - start);
    for (size_t i = last_parent; i > 0; --i) {
        sift_down(i, col, start, end, less_than);
    }
    // We would like to use 'i >= 0' as a conditional in the loop, and stop when
    // i == -1. However, that doesn't work with unsigned indices.
    sift_down(0, col, start, end, less_than);
}

template<typename Collection, typename LessThan>
void heap_sort(Collection& col, size_t start, size_t end, LessThan less_than)
{
    // heapify assumes that the last element has a parent, so handle this special case first:
    if (end <= start)
        return;

    heapify(col, start, end, less_than);

    while (end > start + 1) {
        swap(col[start], col[end]);
        end -= 1;
        sift_down(0, col, start, end, less_than);
    }
    VERIFY(end == start + 1);
    // Two elements remain to be sorted, and they are a heap.
    // Therefore, they are in reverse order.
    swap(col[start], col[start + 1]);
}

template<typename Collection, typename LessThan>
void heap_sort(Collection& collection, LessThan less_than)
{
    if (collection.size() <= 1)
        return;

    heap_sort(collection, 0, collection.size() - 1, move(less_than));
}

template<typename Collection>
void heap_sort(Collection& collection)
{
    heap_sort(collection, [](auto& a, auto& b) { return a < b; });
}

}

using AK::heap_sort;
