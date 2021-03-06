#pragma once

#include <cstdint>
#include "bencode/detail/parser/parsing_error.hpp"
#include "bencode/detail/parser/helpers.hpp"

namespace bencode::detail
{

namespace swar {

inline constexpr auto power_of_10_lookup = std::array {
        1UL,                     // 0
        10UL,                    // 1
        100UL,                   // 2
        1000UL,                  // 3
        10000UL,                 // 4
        100000UL,                // 5
        1000000UL,               // 6
        10000000UL,              // 7
        100000000UL,             // 8
        1000000000UL,            // 9
        10000000000UL,           // 10
        100000000000UL,          // 11
        1000000000000UL,         // 12
        10000000000000UL,        // 13
        100000000000000UL,       // 14
        1000000000000000UL,      // 15
        10000000000000000UL,     // 16
        100000000000000000UL,    // 17
        1000000000000000000UL,   // 18
        10000000000000000000UL,  // 19
};


constexpr std::uint64_t byteswap(std::uint64_t data, std::size_t n_digits = 8)
{
    std::uint64_t swapped = __builtin_bswap64(data);
    std::uint64_t n_shifts = (8 * (8-n_digits));
    std::uint64_t out = swapped >> n_shifts;
    return out;
}

constexpr std::uint32_t byteswap(std::uint32_t data, std::size_t n_digits = 4)
{
    std::uint32_t swapped = __builtin_bswap32(data);
    std::uint32_t n_shifts = (8 * (4-n_digits));
    std::uint32_t out = swapped >> n_shifts;
    return out;
}

constexpr std::uint16_t byteswap(std::uint16_t data, std::size_t n_digits = 2)
{
    std::uint16_t swapped = __builtin_bswap16(data);
    std::uint16_t n_shifts = (8 * (2-n_digits));
    std::uint16_t out = swapped >> n_shifts;
    return out;
}

inline std::uint32_t load_bytes32(const char * data)
{
    std::uint32_t value;
    std::memcpy(&value, data, sizeof(std::uint32_t));
    return value;
}

inline std::uint64_t load_bytes64(const char * data)
{
    std::uint64_t value;
    std::memcpy(&value, data, sizeof(std::uint64_t));
    return value;
}

constexpr std::size_t digit_count(std::uint32_t v)
{
    // pseudocode
    std::uint32_t m, len;
    m = v + 0x46464646;               // roll '9' to 0x7F
    v -= 0x30303030;                  // unpacked binary coded decimal
    m = (m | v) & 0x80808080;         // detect first non-digit
    m = std::countr_zero(m >> 7);     // count run of
    return m / 8;
}

constexpr std::size_t digit_count(std::uint64_t v)
{
    std::uint64_t m, len;
    m = v + 0x4646464646464646;       // roll '9' to 0x7F
    v -= 0x3030303030303030;          // unpacked binary coded decimal
    m = (m | v) & 0x8080808080808080; // detect first non-digit
    m = std::countr_zero(m >> 7);     // count run of
    return m / 8;
}

static constexpr std::array<std::uint32_t, 5> swar32_mask_lookup {
        0x00000000, 0x00000001, 0x00000101, 0x00010101, 0x01010101};

static constexpr std::array<std::uint64_t, 9> swar64_mask_lookup {
        0x0, 0x01, 0x0101, 0x010101, 0x01010101,
        0x0101010101, 0x010101010101, 0x01010101010101, 0x0101010101010101};

constexpr std::uint64_t parse_digits(std::uint64_t input, std::size_t n_digits)
{
    std::uint64_t swapped_input = byteswap(input, n_digits);
    std::uint64_t t1 = swapped_input - (std::uint8_t('0') * swar64_mask_lookup[n_digits]);
    std::uint64_t t2 = (t1 * 10) >> 8;
    std::uint64_t t3 = t1 + t2;
    std::uint64_t t4 = t3 & 0x00ff00ff00ff00ff;
    std::uint64_t t5 = (t4 * (100 + 65536)) >> 16;
    std::uint64_t t6 = (t5 & 0x0000FFFF0000FFFF);
    std::uint64_t out = (t6 * (10000 + 4294967296)) >> 32;
    return out;
}

constexpr std::uint32_t parse_digits(std::uint32_t input, std::size_t n_digits)
{
    std::uint32_t swapped_input = byteswap(input, n_digits);
    std::uint32_t t1 = swapped_input - (std::uint8_t('0') * swar32_mask_lookup[n_digits]);
    std::uint32_t t2 = (t1 * 10) >> 8;
    std::uint32_t t3 = t1 + t2;
    std::uint32_t t4 = t3 & 0x00ff00ff;
    std::uint32_t out = (t4 * (100 + 65536)) >> 16;
    return out;
}
}

struct from_chars_result
{
    const char* ptr;
    parsing_errc ec;
};

template <std::size_t Idx>
struct implementation_tag
{
    static constexpr std::size_t index = Idx;
};

namespace implementation {
inline constexpr auto serial = implementation_tag<0>{};
inline constexpr auto swar = implementation_tag<1>{};
};



constexpr from_chars_result from_chars(
        const char* first, const char* last, bool& value, implementation_tag<0>)
{
    std::uint8_t v = (*first - '0');

    if (v <= 1) [[likely]] {
        value = v;
        return {++first, parsing_errc{}};
    }
    else if (v <= 9) {
        return {++first, parsing_errc::result_out_of_range};
    }
    else {
        return {first, parsing_errc::expected_digit};
    }
}


template <std::integral T>
constexpr from_chars_result from_chars(
        const char* first, const char* last, T& value, implementation_tag<0>)
{
    int sign = 1;
    const char* start = first;
    from_chars_result res { .ptr = first, .ec = parsing_errc{}};

    if (first == last) [[unlikely]] {
        res.ec = parsing_errc::unexpected_eof;
        return res;
    }

    if constexpr (std::signed_integral<T>) {
        if (*first == '-') {
            sign = -1;
            ++first;
        }
        if (first == last)  [[unlikely]] {
            res.ec = parsing_errc::unexpected_eof;
            return res;
        }
    }

    using UT = typename std::make_unsigned<std::remove_cvref_t<T>>::type;
    constexpr auto max_digits = std::numeric_limits<UT>::digits10;
    UT val = 0;

    // do not advance here since we must read the zero again
    const bool leading_zero = (*first == '0');
    int digits = 0;

    unsigned short v;
    bool valid = true;

    for (; digits < max_digits && (first != last) &&
            ((v = static_cast<unsigned>(*first) - '0'), v <= 9); ++digits, ++first)
    {
        raise_and_add(val, 10, v);
    }

    // use integer safe math to check for overflow if we have only one digit precision left
    if (digits == max_digits && (first != last)) [[unlikely]] {
        while (v = static_cast<unsigned>(*first) - '0', v <= 9) {
            if (!raise_and_add_safe(val, 10, v)) {
                valid = false;
                break;
            }
        }
    }
    res.ptr = first;

    if (digits == 0) [[unlikely]] {
        res.ec = parsing_errc::expected_digit;
        return res;
    }

    if (!valid) [[unlikely]] {
        res.ec = parsing_errc::result_out_of_range;
        return res;
    }
    if (leading_zero && val != 0) [[unlikely]] {
        res.ec = parsing_errc::leading_zero;
        return res;
    }

    if constexpr (std::signed_integral<T>) {
        if (val == 0 && sign == -1) [[unlikely]] {
            res.ec = parsing_errc::negative_zero;
            return res;
        }

        T tmp;
        if (__builtin_mul_overflow(val, sign, &tmp))
            res.ec = parsing_errc::result_out_of_range;
        else
            value = tmp;
    }
    else if constexpr (std::numeric_limits<UT>::max() > std::numeric_limits<T>::max())
    {
        if (val > std::numeric_limits<T>::max())
            res.ec = parsing_errc::result_out_of_range;
        else
            value = val;
    }
    else
        value = val;

    return res;
}


constexpr from_chars_result from_chars(
        const char* first, const char* last, std::uint32_t& value, implementation_tag<1>)
{
    const std::ptrdiff_t buffer_size = std::distance(first, last);
    const bool leading_zero = (*first == '0');

    if (buffer_size < 12) [[unlikely]] {
        return from_chars(first, last, value, implementation::serial);
    }

    std::uint32_t val = 0;
    std::ptrdiff_t position = 0;
    std::uint32_t digits = 0;
    std::uint32_t n_digits = 0;
    std::uint32_t tmp_value;
    std::uint64_t data64;
    std::uint32_t data32;

    data64 = swar::load_bytes64(&first[position]);
    n_digits = swar::digit_count(data64);
    tmp_value = swar::parse_digits(data64, n_digits);
    position += n_digits;

    val = tmp_value;
    digits += n_digits;

    if (n_digits == 8) {
        data32 = swar::load_bytes32(&first[position]);
        n_digits = swar::digit_count(data32);
        tmp_value = swar::parse_digits(data32, n_digits);
        position += n_digits;
        digits += n_digits;
        auto base = swar::power_of_10_lookup[n_digits];

        if (!raise_and_add_safe(val, base, tmp_value)) {
            // increment position to first non digit character
            for (; position < buffer_size && first[position]-'0' < 9; ++position);

            return {.ptr = first+position,
                    .ec = parsing_errc::result_out_of_range};
        }
    }

    std::advance(first, position);
    from_chars_result res {.ptr =  first, .ec = parsing_errc{}};

    if (digits == 0) [[unlikely]] {
        res.ec = parsing_errc::expected_digit;
        return res;
    }
    if (leading_zero && val != 0) [[unlikely]] {
        res.ec = parsing_errc::leading_zero;
        return res;
    }

    value = val;
    return res;
}

constexpr from_chars_result from_chars(
        const char* first, const char* last, std::uint64_t& value, implementation_tag<1>)
{
    const std::ptrdiff_t buffer_size = std::distance(first, last);
    const bool leading_zero = (*first == '0');

    if (buffer_size < 20) [[unlikely]] {
        return from_chars(first, last, value, implementation::serial);
    }

    std::uint64_t val = 0;
    std::ptrdiff_t position = 0;
    std::size_t digits = 0;
    std::size_t n_digits = 0;
    std::uint64_t tmp_value;
    std::uint64_t data64;
    std::uint32_t data32;
    std::size_t base;

    data64 = swar::load_bytes64(&first[position]);
    n_digits = swar::digit_count(data64);
    tmp_value = swar::parse_digits(data64, n_digits);
    position += n_digits;
    digits += n_digits;

    val = tmp_value;

    if (n_digits == 8) {
        data64 = swar::load_bytes64(&first[position]);
        n_digits = swar::digit_count(data64);
        tmp_value = swar::parse_digits(data64, n_digits);
        position += n_digits;
        digits += n_digits;

        base = swar::power_of_10_lookup[n_digits];
        raise_and_add(val, base, tmp_value);

        if (n_digits == 8) {
            data32 = swar::load_bytes32(&first[position]);
            n_digits = swar::digit_count(data32);
            tmp_value = swar::parse_digits(data32, n_digits);
            position += n_digits;
            digits += n_digits;
            base = swar::power_of_10_lookup[n_digits];

            if (!raise_and_add_safe(val, base, tmp_value)) {
                // increment position to first non digit character
                for (; position < buffer_size && first[position] - '0' < 9; ++position);

                return {.ptr = first+position,
                        .ec = parsing_errc::result_out_of_range};
            }
        }
    }

    std::advance(first, position);
    from_chars_result res {.ptr =  first, .ec = parsing_errc{}};

    if (digits == 0) [[unlikely]] {
        res.ec = parsing_errc::expected_digit;
        return res;
    }
    if (leading_zero && val != 0) [[unlikely]] {
        res.ec = parsing_errc::leading_zero;
        return res;
    }

    value = val;
    return res;
}

template <std::signed_integral T>
constexpr from_chars_result from_chars(
        const char* first, const char* last, T& value, implementation_tag<1>)
{
    int sign = 1;
    const char* start = first;

    if (*first == '-') {
        sign = -1;
        ++first;

        if (first == last)  [[unlikely]] {
            return {first, parsing_errc::unexpected_eof};
        }
    }

    using UT = typename std::make_unsigned<std::remove_cvref_t<T>>::type;
    UT val = 0;

    from_chars_result res = from_chars(first, last, val, implementation::swar);

    if (res.ec != parsing_errc{}) [[unlikely]] {
        return res;
    }
    if (val == 0 && sign == -1) [[unlikely]] {
        res.ec = parsing_errc::negative_zero;
        return res;
    }

    T tmp;
    if (__builtin_mul_overflow(val, sign, &tmp)) {
        res.ec = parsing_errc::result_out_of_range;
    }
    else {
        value = tmp;
    }

    return res;
}


template <std::integral T, std::size_t ImplementationIdx>
constexpr from_chars_result binteger_from_chars(
        const char* first, const char* last, T& value, implementation_tag<ImplementationIdx>)
{
    const char* start = first;
    from_chars_result res {.ptr = first, .ec = parsing_errc{}};

    if (first == last) [[unlikely]] {
        res.ec = parsing_errc::unexpected_eof;
        return res;
    }

    if (*first != 'i') [[unlikely]] {
        res.ec = parsing_errc::expected_integer_start_token;
        return res;
    }

    // skip 'i'
    ++first;
    T val;
    from_chars_result from_chars_res = from_chars(first, last, val,
                                                  implementation_tag<ImplementationIdx>{});

    first = from_chars_res.ptr;

    // pass possible errors from parse_integer
    if (from_chars_res.ec != parsing_errc{}) [[unlikely]] {
        res = from_chars_res;
        return res;
    }

    // verify the integer is correctly terminated with the "e"
    if (first == last) [[unlikely]] {
        res.ec = parsing_errc::unexpected_eof;
        res.ptr = first;
        return res;
    }
    if (*first != symbol::end) [[unlikely]] {
        res.ec = parsing_errc::expected_end;
        res.ptr = first;
        return res;
    }

    ++first;
    res.ptr = first;
    value = val;
    return res;
}



template <std::size_t ImplementationIdx>
constexpr from_chars_result bstring_from_chars(const char* first, const char* last,
                                               std::size_t& offset, std::size_t& size,
                                               implementation_tag<ImplementationIdx>)
{
    auto start = first;
    from_chars_result res {.ptr = first, .ec = parsing_errc{}};

    if (first == last) [[unlikely]] {
        res.ec = parsing_errc::unexpected_eof;
        res.ptr = first;
        return res;
    }
    if (*first == '-') [[unlikely]] {
        res.ec = parsing_errc::negative_string_length;
        res.ptr = first;
        return res;
    }

    std::size_t size_val;
    const from_chars_result from_chars_res = from_chars(first, last, size_val,
                                                        implementation_tag<ImplementationIdx>{});

    if (from_chars_res.ec != parsing_errc{}) [[unlikely]] {
        res = from_chars_res;
        return res;
    }
    first = from_chars_res.ptr;

    if (first == last) [[unlikely]] {
        res.ec = parsing_errc::unexpected_eof;
        res.ptr = first;
        return res;
    }
    if (*first != ':') [[unlikely]] {
        res.ec = parsing_errc::expected_colon;
        res.ptr = first;
        return res;
    }

    ++first;
    offset = std::distance(start, first);
    std::advance(first, size_val);

    if (first > last) [[unlikely]] {
        res.ec = parsing_errc::unexpected_eof;
        res.ptr = first;
        return res;
    }

    size = size_val;
    res.ptr = first;
    return res;
}


template <std::size_t ImplementationIdx>
constexpr from_chars_result bstring_from_chars(const char* first, const char* last,
        std::string_view& sv,
        implementation_tag<ImplementationIdx>)
{
    std::size_t offset;
    std::size_t size;

    auto res = bstring_from_chars(first, last, offset, size,
                                  implementation_tag<ImplementationIdx>{});
    if (res.ec != parsing_errc{}) [[unlikely]] {
        return res;
    }
    sv = std::string_view(std::next(first, offset), std::next(first, offset+size));
    return res;
}

}

