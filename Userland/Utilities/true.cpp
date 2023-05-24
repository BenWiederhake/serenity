/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibMain/Main.h>

#include <AK/Format.h>
#include <AK/Math.h>

template<FloatingPoint T>
constexpr T manual_log2(T x)
{
    if (x == 0)
        return -AK::Infinity<T>;
    if (x <= 0 || __builtin_isnan(x))
        return AK::NaN<T>;

    // FIXME: Make this more accurate, currently this has an absolute maximum error of ~0.00054
    //        If this is done, use it for x86 floats/doubles as well
    // Approximation using a fitted 4th degree polynomial in its Horner-form
    // Before replacing this compare to
    // https://gist.github.com/Hendiadyoin1/f58346d66637deb9156ef360aa158bf9
    FloatExtractor<T> ext { .d = x };
    T exponent = ext.exponent - FloatExtractor<T>::exponent_bias;

    // When the mantissa shows 0b00 (implicitly 1.0) we are on a power of 2
    if (ext.mantissa == 0)
        return exponent;

    FloatExtractor<T> mantissa_ext {
        .mantissa = ext.mantissa,
        .exponent = FloatExtractor<T>::exponent_bias,
        .sign = ext.sign,
    };

    // (1 <= mantissa < 2)
    T mantissa = mantissa_ext.d;

    T log2_mantissa = mantissa * (mantissa * (((T)0.688101329132870 - (T)0.0873431279665709 * mantissa) * mantissa - (T)2.23432594978817) + (T)4.19641546493297) - (T)2.56284771631110;

    return exponent + log2_mantissa;
}

ErrorOr<int> serenity_main(Main::Arguments)
{
    double inputs[] = {
        0.0,
        1e-10,
        0.5,
        0.75,
        1.0,
        1.1,
        1.5,
        1.9,
        1.999,
        2.0,
        2.001,
        1e10,
    };
    for (double d : inputs) {
        double log2_val = manual_log2(d);
        dbgln("log2({}) = {}", d, log2_val);
    }
    // OUTPUT:
    // 9.636 true(45:45): log2(0) = -inf
    // 9.636 true(45:45): log2(0) = -33.219804
    // 9.636 true(45:45): log2(0.5) = -1
    // 9.640 true(45:45): log2(0.75) = -0.415290
    // 9.640 true(45:45): log2(1) = 0
    // 9.640 true(45:45): log2(1.1) = 0.137658
    // 9.640 true(45:45): log2(1.5) = 0.584709
    // 9.640 true(45:45): log2(1.9) = 0.925847
    // 9.640 true(45:45): log2(1.999) = 0.999278
    // 9.640 true(45:45): log2(2) = 1
    // 9.640 true(45:45): log2(2.000999) = 1.000721
    // 9.640 true(45:45): log2(10000000000) = 33.219555
    // Python seems to agree:
    // >>> [log2(x) for x in [1.1, 1.5, 1.9, 1.999, 2, 2.001, 1e10]]
    // [0.13750352374993502, 0.5849625007211562, 0.925999418556223, 0.9992784720825406, 1.0, 1.000721167243654, 33.219280948873624]
    return 0;
}
