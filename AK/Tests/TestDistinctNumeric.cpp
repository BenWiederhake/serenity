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
        TYPEDEF_DISTINCT_NUMERIC_GENERAL(T, true, true, true, true, true, true, NumericOfSignedType);
        EXPECT_EQ(sizeof(T), sizeof(NumericOfSignedType));
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
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, true, false, false, false, false, false, TruthyNumeric);
TYPEDEF_DISTINCT_NUMERIC_GENERAL(int, true, true, true, true, true, true, GeneralNumeric);

TEST_CASE(address_identity)
{
    GeneralNumeric a = 4;
    GeneralNumeric b = 5;
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

TEST_MAIN(DistinctNumeric)
