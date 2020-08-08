/*
 * Copyright (c) 2020, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

// FIXME: Probably not all are needed.
#include <AK/Assertions.h>
#include <AK/StdLibExtras.h>
#include <AK/Types.h>
#include <AK/kmalloc.h>

namespace AK {

namespace internal {
template<int N>
constexpr int literal_length(const char (&)[N])
{
    return N - 1;
}
}

/**
 * This implements a "distinct" numeric type that is intentionally incompatible
 * to other incantations. The intention is that each "distinct" type that you
 * want simply gets different values for `fn_length` and `line`. The macros
 * `TYPEDEF_DISTINCT_NUMERIC_*()` at the bottom of `DistinctNumeric.h`.
 *
 * `seq`, `cmp`, `truthy`, `flags`, `shift`, and `arith` simply split up the
 * space of operators into 6 simple categories:
 * - No matter the values of these, `DistinctNumeric` always implements `==` and `!=`.
 * - If `seq` is true, then `++a`, `a++`, `--a`, and `a--` are implemented.
 * - If `cmp` is true, then `a>b`, `a<b`, `a>=b`, and `a<=b` are implemented.
 * - If `truthy` is true, then `!a`, `a&&b`, and `a||b` are implemented (through `operator bool()`.
 * - If `flags` is true, then `~a`, `a&b`, `a|b`, `a^b`, `a&=b`, `a|=b`, and `a^=b` are implemented.
 * - If `shift` is true, then `a<<b`, `a>>b`, `a<<=b`, `a>>=b` are implemented.
 * - If `arith` is true, then `a+b`, `a-b`, `+a`, `-a`, `a*b`, `a/b`, `a%b`, and the respective `a_=b` versions are implemented.
 * The semantics are always those of the underlying basic type `T`.
 *
 * These can be combined arbitrarily. Want a numeric type that supports `++a`
 * and `a >> b` but not `a > b`? Sure thing, just set
 * `seq=true, cmp=false, shift=true` and you're done!
 * Furthermore, some of these overloads make more sense with specific types, like `a&&b` which should be able to operate
 *
 * I intentionally decided against overloading `&a` because these shall remain
 * numeric types.
 *
 * The C++20 `operator<=>` would require, among other things `std::weak_equality`.
 * Since we do not have that, it cannot be implemented.
 *
 * The are many operators that do not work on `int`, so I left them out:
 * `a[b]`, `*a`, `a->b`, `a.b`, `a->*b`, `a.*b`.
 *
 * There are many more operators that do not make sense for numerical types,
 * or cannot be overloaded in the first place. Naturally, they are not implemented.
 */
template<typename T, bool seq, bool cmp, bool truthy, bool flags, bool shift, bool arith, size_t fn_len, size_t line>
class DistinctNumeric {
    typedef DistinctNumeric<T, seq, cmp, truthy, flags, shift, arith, fn_len, line> Self;

public:
    DistinctNumeric(T value)
        : m_value { value }
    {
    }

    const T& value() const { return m_value; }

    // Always implemented: identity.
    bool operator==(const Self& other) const
    {
        return this->m_value == other.m_value;
    }
    bool operator!=(const Self& other) const
    {
        return this->m_value != other.m_value;
    }

    // Only implemented when `seq` is true:
    Self& operator++()
    {
        static_assert(seq, "'++a' is only available for DistinctNumeric types with 'seq'.");
        this->m_value += 1;
        return *this;
    }
    Self operator++(int)
    {
        static_assert(seq, "'a++' is only available for DistinctNumeric types with 'seq'.");
        Self ret = this->m_value;
        this->m_value += 1;
        return ret;
    }
    Self& operator--()
    {
        static_assert(seq, "'--a' is only available for DistinctNumeric types with 'seq'.");
        this->m_value -= 1;
        return *this;
    }
    Self operator--(int)
    {
        static_assert(seq, "'a--' is only available for DistinctNumeric types with 'seq'.");
        Self ret = this->m_value;
        this->m_value -= 1;
        return ret;
    }

    // Only implemented when `cmp` is true:
    bool operator>(const Self& other) const
    {
        static_assert(cmp, "'a>b' is only available for DistinctNumeric types with 'cmp'.");
        return this->m_value > other.m_value;
    }
    bool operator<(const Self& other) const
    {
        static_assert(cmp, "'a<b' is only available for DistinctNumeric types with 'cmp'.");
        return this->m_value < other.m_value;
    }
    bool operator>=(const Self& other) const
    {
        static_assert(cmp, "'a>=b' is only available for DistinctNumeric types with 'cmp'.");
        return this->m_value >= other.m_value;
    }
    bool operator<=(const Self& other) const
    {
        static_assert(cmp, "'a<=b' is only available for DistinctNumeric types with 'cmp'.");
        return this->m_value <= other.m_value;
    }
    // 'operator<=>' cannot be implemented. See class comment.

    // Only implemented when `truthy` is true:
    operator bool() const
    {
        static_assert(truthy, "'!a', 'a&&b', 'a||b' and similar operators are only available for DistinctNumeric types with 'truthy'.");
        return this->m_value;
    }
    // The default implementations for `operator!()`, `operator&&(bool)`, `operator||(bool)` const are fine.

    // Only implemented when `flags` is true:
    Self operator~() const;
    Self operator&(const Self&) const;
    Self operator|(const Self&) const;
    Self operator^(const Self&) const;
    Self& operator&=(const Self&);
    Self& operator|=(const Self&);
    Self& operator^=(const Self&);
    // FIXME: impl, test

    // Only implemented when `shift` is true:
    // TODO: Should this take `int` instead?
    Self operator<<(const Self&) const;
    Self operator>>(const Self&) const;
    Self& operator<<=(const Self&);
    Self& operator>>=(const Self&);
    // FIXME: impl, test

    // Only implemented when `arith` is true:
    Self operator+(const Self&) const;
    Self operator-(const Self&) const;
    Self operator+() const;
    Self operator-() const;
    Self operator*(const Self&) const;
    Self operator/(const Self&) const;
    Self operator%(const Self&) const;
    // The default implementations for compound assignment are fine.
    // FIXME: impl, test

private:
    T m_value;
};

// NOTE: Please name all macros that end up calling this macro also something
// like `TYPEDEF_DISTINCT_`. This way, writing a linter that checks for
// collisions is easy.
#define TYPEDEF_DISTINCT_NUMERIC_GENERAL(T, seq, cmp, truthy, flags, shift, arith, NAME) typedef DistinctNumeric<T, seq, cmp, truthy, flags, shift, arith, ::AK::internal::literal_length(__FILE__), __LINE__> NAME

// TODO: Easier typedef's

}

using AK::DistinctNumeric;
