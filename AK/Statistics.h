/*
 * Copyright (c) 2021, Tobias Christiansen <tobyase@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Concepts.h>
#include <AK/Math.h>
#include <AK/QuickSelect.h>
#include <AK/Vector.h>

namespace AK {

template<Arithmetic T = float>
class Statistics {
public:
    Statistics() = default;
    ~Statistics() = default;

    void add(T const& value)
    {
        // FIXME: Check for an overflow
        m_sum += value;
        m_values.append(value);
    }

    T const sum() const { return m_sum; }
    float average() const { return (float)sum() / size(); }

    T const min() const
    {
        T minimum = m_values[0];
        for (T number : values()) {
            if (number < minimum) {
                minimum = number;
            }
        }
        return minimum;
    }

    T const max() const
    {
        T maximum = m_values[0];
        for (T number : values()) {
            if (number > maximum) {
                maximum = number;
            }
        }
        return maximum;
    }

    T const median()
    {
        quick_sort(m_values);
        // If the number of values is even, the median is the arithmetic mean of the two middle values
        if (size() % 2 == 0) {
            auto index = size() / 2;
            auto median1 = m_values.at(AK::quick_select(m_values, index));
            auto median2 = m_values.at(AK::quick_select(m_values, index - 1));
            return (median1 + median2) / 2;
        }
        return m_values.at(AK::quick_select(m_values, size() / 2));
    }

    float standard_deviation() const { return sqrt(variance()); }
    float variance() const
    {
        float summation = 0;
        float avg = average();
        for (T number : values()) {
            float difference = (float)number - avg;
            summation += (difference * difference);
        }
        summation = summation / size();
        return summation;
    }

    Vector<T> const& values() const { return m_values; }
    size_t size() const { return m_values.size(); }

private:
    Vector<T> m_values;
    T m_sum {};
};

}
