/*
 * Copyright (c) 2018-2020, Andreas Kling <awesomekling@gmail.com>
 * Copyright (c) 2020, Fei Wu <f.eiwu@yahoo.com>
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

#include <AK/Noncopyable.h>
#include <AK/Optional.h>
#include <AK/String.h>
#include <AK/StringUtils.h>
#include <AK/StringView.h>

namespace AK {

namespace StringUtils {

bool matches(const StringView& str, const StringView& mask, CaseSensitivity case_sensitivity)
{
    if (str.is_null() || mask.is_null())
        return str.is_null() && mask.is_null();

    if (case_sensitivity == CaseSensitivity::CaseInsensitive) {
        const String str_lower = String(str).to_lowercase();
        const String mask_lower = String(mask).to_lowercase();
        return matches(str_lower, mask_lower, CaseSensitivity::CaseSensitive);
    }

    const char* string_ptr = str.characters_without_null_termination();
    const char* string_end = string_ptr + str.length();
    const char* mask_ptr = mask.characters_without_null_termination();
    const char* mask_end = mask_ptr + mask.length();

    // Match string against mask directly unless we hit a *
    while ((string_ptr < string_end) && (mask_ptr < mask_end) && (*mask_ptr != '*')) {
        if ((*mask_ptr != *string_ptr) && (*mask_ptr != '?'))
            return false;
        mask_ptr++;
        string_ptr++;
    }

    const char* cp = nullptr;
    const char* mp = nullptr;

    while (string_ptr < string_end) {
        if ((mask_ptr < mask_end) && (*mask_ptr == '*')) {
            // If we have only a * left, there is no way to not match.
            if (++mask_ptr == mask_end)
                return true;
            mp = mask_ptr;
            cp = string_ptr + 1;
        } else if ((mask_ptr < mask_end) && ((*mask_ptr == *string_ptr) || (*mask_ptr == '?'))) {
            mask_ptr++;
            string_ptr++;
        } else if ((cp != nullptr) && (mp != nullptr)) {
            mask_ptr = mp;
            string_ptr = cp++;
        } else {
            break;
        }
    }

    // Handle any trailing mask
    while ((mask_ptr < mask_end) && (*mask_ptr == '*'))
        mask_ptr++;

    // If we 'ate' all of the mask and the string then we match.
    return (mask_ptr == mask_end) && string_ptr == string_end;
}

int convert_to_int(const StringView& str, bool& ok)
{
    return str.to_int(ok);
}

unsigned convert_to_uint(const StringView& str, bool& ok)
{
    return str.to_uint(ok);
}

bool isspace(char ch) {
    // Can't access ctype.h from here
    switch (ch) {
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
        return true;
    default:
        return false;
    }
}

static size_t measure_space(const StringView& str)
{
    size_t i = 0;
    for (; i < str.length() && isspace(str[i]); ++i) {
        // no-op
    }
    return i;
}

enum Sign {
    Negative,
    Positive,
};

static Sign strtosign(const char* str, size_t& sign_len)
{
    if (*str == '+') {
        sign_len = 1;
        return Sign::Positive;
    } else if (*str == '-') {
        sign_len = 1;
        return Sign::Negative;
    } else {
        sign_len = 0;
        return Sign::Positive;
    }
}

enum DigitConsumeDecision {
    Consumed,
    PosOverflow,
    NegOverflow,
    Invalid,
};

template<typename T, T min_value, T max_value>
class NumParser {
    AK_MAKE_NONMOVABLE(NumParser)

public:
    NumParser(Sign sign, int base)
        : m_base(base)
        , m_num(0)
        , m_sign(sign)
    {
        m_cutoff = positive() ? (max_value / base) : (min_value / base);
        m_max_digit_after_cutoff = positive() ? (max_value % base) : (min_value % base);
    }

    int parse_digit(char ch)
    {
        int digit;
        if ('0' <= ch && ch <= '9')
            digit = ch - '0';
        else if ('a' <= ch && ch <= 'z')
            digit = ch - ('a' - 10);
        else if ('A' <= ch && ch <= 'Z')
            digit = ch - ('A' - 10);
        else
            return -1;

        if (static_cast<T>(digit) >= m_base)
            return -1;

        return digit;
    }

    DigitConsumeDecision consume(char ch)
    {
        int digit = parse_digit(ch);
        if (digit == -1)
            return DigitConsumeDecision::Invalid;

        if (!can_append_digit(digit)) {
            if (m_sign != Sign::Negative) {
                return DigitConsumeDecision::PosOverflow;
            } else {
                return DigitConsumeDecision::NegOverflow;
            }
        }

        m_num *= m_base;
        m_num += positive() ? digit : -digit;

        return DigitConsumeDecision::Consumed;
    }

    T number() const { return m_num; };

private:
    bool can_append_digit(int digit)
    {
        const bool is_below_cutoff = positive() ? (m_num < m_cutoff) : (m_num > m_cutoff);

        if (is_below_cutoff) {
            return true;
        } else {
            return m_num == m_cutoff && digit < m_max_digit_after_cutoff;
        }
    }

    bool positive() const
    {
        return m_sign != Sign::Negative;
    }

    const T m_base;
    T m_num;
    T m_cutoff;
    int m_max_digit_after_cutoff;
    Sign m_sign;
};

typedef NumParser<int, INT_MIN, INT_MAX> IntParser;
typedef NumParser<long long, LONG_LONG_MIN, LONG_LONG_MAX> LongLongParser;
typedef NumParser<unsigned long long, 0ULL, ULONG_LONG_MAX> ULongLongParser;

static Optional<double> convert_inf_nan(Sign sign, const StringView& view) {
    if (view.starts_with("inf") || view.starts_with("infinity")) {
        if (sign != Sign::Negative) {
            return __builtin_huge_val();
        } else {
            return -__builtin_huge_val();
        }
    }
    if (view.starts_with("nan")) {
        if (sign != Sign::Negative) {
            return __builtin_huge_val();
        } else {
            return -__builtin_huge_val();
        }
    }

    return Optional<double>();
}

double convert_to_double(const StringView& view, bool& ok) {
    if (view.length() == 0) {
        ok = false;
        return 0.0;
    }

    size_t position = 0;
    auto ch = [&view, &position] { return (position < view.length()) ? view[position] : 0; };
    const char* parse_ptr = view.characters_without_null_termination();
    const char* strend_ptr = parse_ptr + view.length();

    size_t sign_len;
    const Sign sign = strtosign(parse_ptr, sign_len);

    // Parse inf/nan, if applicable.
    Optional<double> inf_nan = convert_inf_nan(view.substring_view(sign_len, view.length() - sign_len));
    if (inf_nan.has_value()) {
        ok = true;
        return inf_nan.value();
    }

    // We cannot easily use StringView::matches here,
    // because there may or may not be a sign.
    if (is_either(parse_ptr + 0, strend_ptr, 'i', 'I')) {
        if (is_either(parse_ptr, 1, 'n', 'N')) {
            if (is_either(parse_ptr, 2, 'f', 'F')) {
                parse_ptr += 3;
                if (is_either(parse_ptr, 0, 'i', 'I')) {
                    if (is_either(parse_ptr, 1, 'n', 'N')) {
                        if (is_either(parse_ptr, 2, 'i', 'I')) {
                            if (is_either(parse_ptr, 3, 't', 'T')) {
                                if (is_either(parse_ptr, 4, 'y', 'Y')) {
                                    parse_ptr += 5;
                                }
                            }
                        }
                    }
                }
                if (endptr)
                    *endptr = parse_ptr;
                // Don't set errno to ERANGE here:
                // The caller may want to distinguish between "input is
                // literal infinity" and "input is not literal infinity
                // but did not fit into double".
                if (sign != Sign::Negative) {
                    return __builtin_huge_val();
                } else {
                    return -__builtin_huge_val();
                }
            }
        }
    }
    if (is_either(parse_ptr, 0, 'n', 'N')) {
        if (is_either(parse_ptr, 1, 'a', 'A')) {
            if (is_either(parse_ptr, 2, 'n', 'N')) {
                if (endptr)
                    *endptr = parse_ptr + 3;
                errno = ERANGE;
                if (sign != Sign::Negative) {
                    return __builtin_nan("");
                } else {
                    return -__builtin_nan("");
                }
            }
        }
    }

    // Parse base
    char exponent_lower;
    char exponent_upper;
    int base = 10;
    if (*parse_ptr == '0') {
        const char base_ch = *(parse_ptr + 1);
        if (base_ch == 'x' || base_ch == 'X') {
            base = 16;
            parse_ptr += 2;
        }
    }

    if (base == 10) {
        exponent_lower = 'e';
        exponent_upper = 'E';
    } else {
        exponent_lower = 'p';
        exponent_upper = 'P';
    }

    // Parse "digits", possibly keeping track of the exponent offset.
    // We parse the most significant digits and the position in the
    // base-`base` representation separately. This allows us to handle
    // numbers like `0.0000000000000000000000000000000000001234` or
    // `1234567890123456789012345678901234567890` with ease.
    LongLongParser digits { sign, base };
    bool digits_usable = false;
    bool should_continue = true;
    bool digits_overflow = false;
    bool after_decimal = false;
    int exponent = 0;
    do {
        if (!after_decimal && *parse_ptr == '.') {
            after_decimal = true;
            parse_ptr += 1;
            continue;
        }

        bool is_a_digit;
        if (digits_overflow) {
            is_a_digit = digits.parse_digit(*parse_ptr) != -1;
        } else {
            DigitConsumeDecision decision = digits.consume(*parse_ptr);
            switch (decision) {
            case DigitConsumeDecision::Consumed:
                is_a_digit = true;
                // The very first actual digit must pass here:
                digits_usable = true;
                break;
            case DigitConsumeDecision::PosOverflow:
                // fallthrough
            case DigitConsumeDecision::NegOverflow:
                is_a_digit = true;
                digits_overflow = true;
                break;
            case DigitConsumeDecision::Invalid:
                is_a_digit = false;
                break;
            default:
                ASSERT_NOT_REACHED();
            }
        }

        if (is_a_digit) {
            exponent -= after_decimal ? 1 : 0;
            exponent += digits_overflow ? 1 : 0;
        }

        should_continue = is_a_digit;
        parse_ptr += should_continue;
    } while (should_continue);

    if (!digits_usable) {
        // No actual number value available.
        if (endptr)
            *endptr = const_cast<char*>(str);
        return 0.0;
    }

    // Parse exponent.
    // We already know the next character is not a digit in the current base,
    // nor a valid decimal point. Check whether it's an exponent sign.
    if (*parse_ptr == exponent_lower || *parse_ptr == exponent_upper) {
        // Need to keep the old parse_ptr around, in case of rollback.
        char* old_parse_ptr = parse_ptr;
        parse_ptr += 1;

        // Can't use atol or strtol here: Must accept excessive exponents,
        // even exponents >64 bits.
        Sign exponent_sign = strtosign(parse_ptr, &parse_ptr);
        IntParser exponent_parser { exponent_sign, base };
        bool exponent_usable = false;
        bool exponent_overflow = false;
        should_continue = true;
        do {
            bool is_a_digit;
            if (exponent_overflow) {
                is_a_digit = exponent_parser.parse_digit(*parse_ptr) != -1;
            } else {
                DigitConsumeDecision decision = exponent_parser.consume(*parse_ptr);
                switch (decision) {
                case DigitConsumeDecision::Consumed:
                    is_a_digit = true;
                    // The very first actual digit must pass here:
                    exponent_usable = true;
                    break;
                case DigitConsumeDecision::PosOverflow:
                    // fallthrough
                case DigitConsumeDecision::NegOverflow:
                    is_a_digit = true;
                    exponent_overflow = true;
                    break;
                case DigitConsumeDecision::Invalid:
                    is_a_digit = false;
                    break;
                default:
                    ASSERT_NOT_REACHED();
                }
            }

            should_continue = is_a_digit;
            parse_ptr += should_continue;
        } while (should_continue);

        if (!exponent_usable) {
            parse_ptr = old_parse_ptr;
        } else if (exponent_overflow) {
            // Technically this is wrong. If someone gives us 5GB of digits,
            // and then an exponent of -5_000_000_000, the resulting exponent
            // should be around 0.
            // However, I think it's safe to assume that we never have to deal
            // with that many digits anyway.
            if (sign != Sign::Negative) {
                exponent = INT_MIN;
            } else {
                exponent = INT_MAX;
            }
        } else {
            // Literal exponent is usable and fits in an int.
            // However, `exponent + exponent_parser.number()` might overflow an int.
            // This would result in the wrong sign of the exponent!
            long long new_exponent = static_cast<long long>(exponent) + static_cast<long long>(exponent_parser.number());
            if (new_exponent < INT_MIN) {
                exponent = INT_MIN;
            } else if (new_exponent > INT_MAX) {
                exponent = INT_MAX;
            } else {
                exponent = static_cast<int>(new_exponent);
            }
        }
    }

    // Parsing finished. now we only have to compute the result.
    if (endptr)
        *endptr = const_cast<char*>(parse_ptr);

    // If `digits` is zero, we don't even have to look at `exponent`.
    if (digits.number() == 0) {
        if (sign != Sign::Negative) {
            return 0.0;
        } else {
            return -0.0;
        }
    }

    // Deal with extreme exponents.
    // The smallest normal is 2^-1022.
    // The smallest denormal is 2^-1074.
    // The largest number in `digits` is 2^63 - 1.
    // Therefore, if "base^exponent" is smaller than 2^-(1074+63), the result is 0.0 anyway.
    // This threshold is roughly 5.3566 * 10^-343.
    // So if the resulting exponent is -344 or lower (closer to -inf),
    // the result is 0.0 anyway.
    // We only need to avoid false positives, so we can ignore base 16.
    if (exponent <= -344) {
        errno = ERANGE;
        // Definitely can't be represented more precisely.
        // I lied, sometimes the result is +0.0, and sometimes -0.0.
        if (sign != Sign::Negative) {
            return 0.0;
        } else {
            return -0.0;
        }
    }
    // The largest normal is 2^+1024-eps.
    // The smallest number in `digits` is 1.
    // Therefore, if "base^exponent" is 2^+1024, the result is INF anyway.
    // This threshold is roughly 1.7977 * 10^-308.
    // So if the resulting exponent is +309 or higher,
    // the result is INF anyway.
    // We only need to avoid false positives, so we can ignore base 16.
    if (exponent >= 309) {
        errno = ERANGE;
        // Definitely can't be represented more precisely.
        // I lied, sometimes the result is +INF, and sometimes -INF.
        if (sign != Sign::Negative) {
            return __builtin_huge_val();
        } else {
            return -__builtin_huge_val();
        }
    }

    // TODO: If `exponent` is large, this could be made faster.
    double value = digits.number();
    if (exponent < 0) {
        exponent = -exponent;
        for (int i = 0; i < exponent; ++i) {
            value /= base;
        }
        if (value == -0.0 || value == +0.0) {
            errno = ERANGE;
        }
    } else if (exponent > 0) {
        for (int i = 0; i < exponent; ++i) {
            value *= base;
        }
        if (value == -__builtin_huge_val() || value == +__builtin_huge_val()) {
            errno = ERANGE;
        }
    }

    return value;
}

static inline char to_lowercase(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c | 0x20;
    return c;
}

bool equals_ignoring_case(const StringView& a, const StringView& b)
{
    if (a.impl() && a.impl() == b.impl())
        return true;
    if (a.length() != b.length())
        return false;
    for (size_t i = 0; i < a.length(); ++i) {
        if (to_lowercase(a.characters_without_null_termination()[i]) != to_lowercase(b.characters_without_null_termination()[i]))
            return false;
    }
    return true;
}

}

}
