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

#include <AK/TestSuite.h>

#include <AK/DistinctNumeric.h>

template<typename T>
class ForType {
public:
    static void check_size()
    {
        TYPEDEF_DISTINCT_NUMERIC_GENERAL(T, false, false, false, false, false, false, TheNumeric);
        EXPECT_EQ(sizeof(T), sizeof(TheNumeric));
    }
};

TEST_CASE(check_size)
{
#define CHECK_SIZE_FOR_SIGNABLE(T)         \
    do {                                   \
        ForType<signed T>::check_size();   \
        ForType<unsigned T>::check_size(); \
    } while (false)
    CHECK_SIZE_FOR_SIGNABLE(char);
    CHECK_SIZE_FOR_SIGNABLE(short);
    CHECK_SIZE_FOR_SIGNABLE(int);
    CHECK_SIZE_FOR_SIGNABLE(long);
    CHECK_SIZE_FOR_SIGNABLE(long long);
    ForType<float>::check_size();
    ForType<double>::check_size();
}

TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, false, false, false, false, false, false, BareNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, true, false, false, false, false, false, SeqNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, false, true, false, false, false, false, CmpNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, false, false, true, false, false, false, TruthyNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, false, false, false, true, false, false, FlagsNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, false, false, false, false, true, false, ShiftNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, false, false, false, false, false, true, ArithNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, true, true, true, true, true, true, GeneralNumeric);

TEST_CASE(address_identity)
{
    BareNumeric a = 4;
    BareNumeric b = 5;
    EXPECT_EQ(&a == &a, true);
    EXPECT_EQ(&a == &b, false);
    EXPECT_EQ(&a != &a, false);
    EXPECT_EQ(&a != &b, true);
}

TEST_CASE(operator_identity)
{
    BareNumeric a = 4;
    BareNumeric b = 5;
    EXPECT_EQ(a == a, true);
    EXPECT_EQ(a == b, false);
    EXPECT_EQ(a != a, false);
    EXPECT_EQ(a != b, true);
}

TEST_CASE(operator_seq)
{
    SeqNumeric a = 4;
    SeqNumeric b = 5;
    SeqNumeric c = 6;
    EXPECT_EQ(++a, b);
    EXPECT_EQ(a++, b);
    EXPECT_EQ(a, c);
    EXPECT_EQ(--a, b);
    EXPECT_EQ(a--, b);
    EXPECT(a != b);
}

TEST_CASE(operator_cmp)
{
    CmpNumeric a = 4;
    CmpNumeric b = 5;
    CmpNumeric c = 5;
    EXPECT_EQ(a > b, false);
    EXPECT_EQ(a < b, true);
    EXPECT_EQ(a >= b, false);
    EXPECT_EQ(a <= b, true);
    EXPECT_EQ(b > a, true);
    EXPECT_EQ(b < a, false);
    EXPECT_EQ(b >= a, true);
    EXPECT_EQ(b <= a, false);
    EXPECT_EQ(b > c, false);
    EXPECT_EQ(b < c, false);
    EXPECT_EQ(b >= c, true);
    EXPECT_EQ(b <= c, true);
}

TEST_CASE(operator_truthy)
{
    TruthyNumeric a = 0;
    TruthyNumeric b = 42;
    TruthyNumeric c = 1337;
    EXPECT_EQ(!a, true);
    EXPECT_EQ((bool)a, false);
    EXPECT_EQ(!b, false);
    EXPECT_EQ((bool)b, true);
    EXPECT_EQ(!c, false);
    EXPECT_EQ((bool)c, true);
    EXPECT_EQ(a && b, false);
    EXPECT_EQ(a && c, false);
    EXPECT_EQ(b && c, true);
    EXPECT_EQ(a || a, false);
    EXPECT_EQ(a || b, true);
    EXPECT_EQ(a || c, true);
    EXPECT_EQ(b || c, true);
}

TEST_CASE(operator_flags)
{
    FlagsNumeric a = 0;
    FlagsNumeric b = 0xA60;
    FlagsNumeric c = 0x03B;
    EXPECT_EQ(~a, FlagsNumeric(~0x0));
    EXPECT_EQ(~b, FlagsNumeric(~0xA60));
    EXPECT_EQ(~c, FlagsNumeric(~0x03B));

    EXPECT_EQ(a & b, b & a);
    EXPECT_EQ(a & c, c & a);
    EXPECT_EQ(b & c, c & b);
    EXPECT_EQ(a | b, b | a);
    EXPECT_EQ(a | c, c | a);
    EXPECT_EQ(b | c, c | b);
    EXPECT_EQ(a ^ b, b ^ a);
    EXPECT_EQ(a ^ c, c ^ a);
    EXPECT_EQ(b ^ c, c ^ b);

    EXPECT_EQ(a & b, FlagsNumeric(0x000));
    EXPECT_EQ(a & c, FlagsNumeric(0x000));
    EXPECT_EQ(b & c, FlagsNumeric(0x020));
    EXPECT_EQ(a | b, FlagsNumeric(0xA60));
    EXPECT_EQ(a | c, FlagsNumeric(0x03B));
    EXPECT_EQ(b | c, FlagsNumeric(0xA7B));
    EXPECT_EQ(a ^ b, FlagsNumeric(0xA60));
    EXPECT_EQ(a ^ c, FlagsNumeric(0x03B));
    EXPECT_EQ(b ^ c, FlagsNumeric(0xA5B));

    EXPECT_EQ(a &= b, FlagsNumeric(0x000));
    EXPECT_EQ(a, FlagsNumeric(0x000));
    EXPECT_EQ(a |= b, FlagsNumeric(0xA60));
    EXPECT_EQ(a, FlagsNumeric(0xA60));
    EXPECT_EQ(a &= c, FlagsNumeric(0x020));
    EXPECT_EQ(a, FlagsNumeric(0x020));
    EXPECT_EQ(a ^= b, FlagsNumeric(0xA40));
    EXPECT_EQ(a, FlagsNumeric(0xA40));

    EXPECT_EQ(b, FlagsNumeric(0xA60));
    EXPECT_EQ(c, FlagsNumeric(0x03B));
}

TEST_CASE(operator_shift)
{
    ShiftNumeric a = 0x040;
    EXPECT_EQ(a << ShiftNumeric(0), ShiftNumeric(0x040));
    EXPECT_EQ(a << ShiftNumeric(1), ShiftNumeric(0x080));
    EXPECT_EQ(a << ShiftNumeric(2), ShiftNumeric(0x100));
    EXPECT_EQ(a >> ShiftNumeric(0), ShiftNumeric(0x040));
    EXPECT_EQ(a >> ShiftNumeric(1), ShiftNumeric(0x020));
    EXPECT_EQ(a >> ShiftNumeric(2), ShiftNumeric(0x010));

    EXPECT_EQ(a <<= ShiftNumeric(5), ShiftNumeric(0x800));
    EXPECT_EQ(a, ShiftNumeric(0x800));
    EXPECT_EQ(a >>= ShiftNumeric(8), ShiftNumeric(0x008));
    EXPECT_EQ(a, ShiftNumeric(0x008));
}

TEST_CASE(composability)
{
    GeneralNumeric a = 0;
    GeneralNumeric b = 1;
    // ident
    EXPECT_EQ(a == a, true);
    EXPECT_EQ(a == b, false);
    // seq
    EXPECT_EQ(++a, b);
    EXPECT_EQ(a--, b);
    EXPECT_EQ(a == b, false);
    // cmp
    EXPECT_EQ(a < b, true);
    EXPECT_EQ(a >= b, false);
    // truthy
    EXPECT_EQ(!a, true);
    EXPECT_EQ((bool)b, true);
    EXPECT_EQ(a && b, false);
    EXPECT_EQ(a || b, true);
    // flags
    EXPECT_EQ(a & b, GeneralNumeric(0));
    EXPECT_EQ(a | b, GeneralNumeric(1));
    // shift
    EXPECT_EQ(b << GeneralNumeric(4), GeneralNumeric(0x10));
    EXPECT_EQ(b >> b, GeneralNumeric(0));
    // arith
    // FIXME
}

TEST_MAIN(DistinctNumeric)
