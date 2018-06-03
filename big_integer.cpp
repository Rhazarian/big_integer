#include "big_integer.h"

#include <string>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <tuple>
#include <functional>
#include <cassert>
#include <utility>
#include <limits>

struct big_integer::helper {

    helper() = delete;

    static bool is_zero(big_integer const& x)
    {
        return !x.data.back() && (x.data.size() == 1);
    }

    static bool is_negative(big_integer const& x) // x - in twos-complement representation
    {
        return (x.data.back() >> 31) == 1;
    }

    static void logical_complement(big_integer& x) // x - in twos-complement representation
    {
        for (auto it = x.data.begin(); it != x.data.end(); ++it) {
            *it = ~(*it);
        }
    }

    static void negate(big_integer& x) // x - in twos-complement representation
    {
        bool is_neg = is_negative(x);
        logical_complement(x);
        bool carry = true;
        for (auto it = x.data.begin(); carry && it != x.data.end(); ++it) {
            carry = *it == std::numeric_limits<uint32_t>::max();
            ++(*it);
        }
        if (is_neg && is_negative(x)) {
            x.data.emplace_back(0);
        }
    }

    static void normalize(big_integer& x) // x - in twos-complement representation
    {
        if (is_negative(x)) {
            for (auto i = x.data.size() - 1;
                 i && x.data[i] == std::numeric_limits<uint32_t>::max() && (x.data[i - 1] >> 31) == 1;
                 --i) {
                x.data.pop_back();
            }
        }
        else {
            for (auto i = x.data.size() - 1; i && x.data[i] == 0 && (x.data[i - 1] >> 31) == 0; --i) {
                x.data.pop_back();
            }
        }
    }

    static void to_twos_complement(big_integer& x, bool sign)
    {
        if (is_negative(x)) {
            x.data.emplace_back(0);
        }
        if (sign) {
            negate(x);
        }
    }

    static void to_sign_magnitude(big_integer& x)
    {
        if (is_negative(x)) {
            negate(x);
        }
        if (!x.data.back()) {
            x.data.pop_back();
        }
    }

    static void add_uint(big_integer& x, uint32_t val) // x  in sign-magnitude representation
    {
        for (auto it = x.data.begin(); val != 0 && it != x.data.end(); ++it) {
            auto overflow = val > std::numeric_limits<uint32_t>::max() - *it;
            *it += val;
            val = overflow;
        }
        if (val) {
            x.data.emplace_back(1);
        }
        else if (is_negative(x)) {
            x.data.emplace_back(0);
        }
    }

    static big_integer mul_uint(big_integer const& x, uint32_t val) // x in sign-magnitude representation
    {
        big_integer dest((big_integer::container_t(x.data.size())));
        uint32_t carry = 0;
        for (big_integer::container_t::size_type i = 0; i < x.data.size(); ++i) {
            uint64_t tmp = static_cast<uint64_t>(x.data[i]) * static_cast<uint64_t>(val);
            dest.data[i] = static_cast<uint32_t>(tmp);
            auto overflow = carry > std::numeric_limits<uint32_t>::max() - dest.data[i];
            dest.data[i] += carry;
            carry = static_cast<uint32_t>(tmp >> 32) + overflow;
        }
        if (carry) {
            dest.data.emplace_back(carry);
        }
        return dest;
    }

    static uint32_t div_uint(big_integer& x, uint32_t val) // x - in sign-magnitude representation, val != 0
    {
        assert(val != 0);
        uint32_t carry = 0;
        for (auto it = x.data.rbegin(); it != x.data.rend(); ++it) {
            uint64_t tmp = (static_cast<uint64_t>(carry) << 32) | *it;
            *it = static_cast<uint32_t>(tmp / val);
            carry = static_cast<uint32_t>(tmp % val);
        }
        while (x.data.size() > 1 && !x.data.back()) {
            x.data.pop_back();
        }
        return carry;
    }

    static int8_t cmp_in_sm(big_integer const& lhs,
            big_integer const& rhs) // lhs, rhs - in sign-magnitude representation
    {
        int8_t val = 0;
        if (lhs.data.size() != rhs.data.size()) {
            val = (lhs.data.size() > rhs.data.size()) - (lhs.data.size() < rhs.data.size());
        }
        else {
            for (auto i = lhs.data.size(); val == 0 && i--;) {
                if (lhs.data[i] != rhs.data[i]) {
                    val = (lhs.data[i] > rhs.data[i]) - (lhs.data[i] < rhs.data[i]);
                }
            }
        }
        return val;
    }

    static int8_t cmp(big_integer const& lhs, big_integer const& rhs) // lhs, rhs - in twos-complement representation
    {
        bool rhs_is_neg = is_negative(rhs), lhs_is_neg = is_negative(lhs);
        if (lhs_is_neg != rhs_is_neg) {
            return rhs_is_neg - lhs_is_neg;
        }
        int8_t val = 0;
        if (lhs.data.size() != rhs.data.size()) {
            val = (lhs.data.size() > rhs.data.size()) - (lhs.data.size() < rhs.data.size());
        }
        else {
            for (auto i = lhs.data.size(); val == 0 && i--;) {
                if (lhs.data[i] != rhs.data[i]) {
                    val = (lhs.data[i] > rhs.data[i]) - (lhs.data[i] < rhs.data[i]);
                }
            }
        }
        return lhs_is_neg ? -val : val;
    }

    static big_integer bit_operation(big_integer const& lhs, big_integer const& rhs,
            std::function<uint32_t(uint32_t, uint32_t)> func)
    {
        dynamic_storage<uint32_t> hello;
        auto min_len = std::min(lhs.data.size(), rhs.data.size());
        auto max_len = std::max(lhs.data.size(), rhs.data.size());
        auto const&[max, min] = lhs.data.size() == max_len ? std::forward_as_tuple(lhs, rhs) :
                std::forward_as_tuple(rhs, lhs);
        big_integer res((big_integer::container_t(max_len)));
        for (big_integer::container_t::size_type i = 0; i < min_len; ++i) {
            res.data[i] = func(lhs.data[i], rhs.data[i]);
        }
        uint32_t ext = big_integer::helper::is_negative(min) ? std::numeric_limits<uint32_t>::max() : 0;
        for (auto i = min_len; i < max_len; ++i) {
            res.data[i] = func(max.data[i], ext);
        }
        return res;
    }

};

big_integer::big_integer() : data{0} { }

big_integer::big_integer(big_integer const& x) : data(x.data) { }

big_integer::big_integer(int32_t val) : data{static_cast<uint32_t>(val)} { }

big_integer::big_integer(std::string_view str) : data{0}
{
    if (str.empty()) {
        return;
    }
    bool is_negative = str[0] == '-';
    if (is_negative) {
        str = str.substr(1);
    }
    for (auto ch : str) {
        if (!isdigit(ch)) {
            throw std::invalid_argument("big_integer::_M_copy_from_string");
        }
        *this = big_integer::helper::mul_uint(*this, 10);
        big_integer::helper::add_uint(*this, ch - '0');
    }
    big_integer::helper::to_twos_complement(*this, is_negative);
}

big_integer::big_integer(container_t data) : data(data) { }

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(big_integer const& rhs)
{
    big_integer copy(rhs);
    swap(copy);
    return *this;
}

big_integer& big_integer::operator+=(big_integer const& rhs)
{
    return *this = *this + rhs;
}

big_integer& big_integer::operator-=(big_integer const& rhs)
{
    return *this = *this - rhs;
}

big_integer& big_integer::operator*=(big_integer const& rhs)
{
    return *this = *this * rhs;
}

big_integer& big_integer::operator/=(big_integer const& rhs)
{
    return *this = *this / rhs;
}

big_integer& big_integer::operator%=(big_integer const& rhs)
{
    return *this = *this % rhs;
}

big_integer& big_integer::operator&=(big_integer const& rhs)
{
    return *this = *this & rhs;
}

big_integer& big_integer::operator|=(big_integer const& rhs)
{
    return *this = *this | rhs;
}

big_integer& big_integer::operator^=(big_integer const& rhs)
{
    return *this = *this ^ rhs;
}

big_integer& big_integer::operator<<=(int rhs)
{
    return *this = *this << rhs;
}

big_integer& big_integer::operator>>=(int rhs)
{
    return *this = *this >> rhs;
}

big_integer big_integer::operator+() const
{
    return *this;
}

big_integer big_integer::operator-() const
{
    big_integer copy(*this);
    big_integer::helper::negate(copy);
    return copy;
}

big_integer big_integer::operator~() const
{
    big_integer copy(*this);
    big_integer::helper::logical_complement(copy);
    return copy;
}

big_integer& big_integer::operator++()
{
    bool carry = 1;
    for (auto it = data.begin(); carry && it != data.end(); ++it) {
        carry = *it == std::numeric_limits<uint32_t>::max();
        ++(*it);
    }
    if (carry) {
        data.emplace_back(carry);
    }
    return *this;
}

big_integer big_integer::operator++(int)
{
    big_integer copy(*this);
    ++*this;
    return copy;
}

big_integer& big_integer::operator--()
{
    uint32_t carry = std::numeric_limits<uint32_t>::max();
    for (auto it = data.begin(); carry && it != data.end(); ++it) {
        carry = *it != 0;
        *it += std::numeric_limits<uint32_t>::max();
    }
    if (carry) {
        data.emplace_back(carry);
    }
    return *this;
}

big_integer big_integer::operator--(int)
{
    big_integer r = *this;
    --*this;
    return r;
}

void big_integer::swap(big_integer& other) noexcept
{
    data.swap(other.data); // noexcept since C++17
}

big_integer operator+(big_integer const& lhs, big_integer const& rhs)
{
    bool lhs_sign = big_integer::helper::is_negative(lhs);
    bool same_sign = lhs_sign == big_integer::helper::is_negative(rhs);
    auto min_len = std::min(lhs.data.size(), rhs.data.size());
    auto max_len = std::max(lhs.data.size(), rhs.data.size());
    auto const&[max, min] = lhs.data.size() == max_len ? std::forward_as_tuple(lhs, rhs) :
            std::forward_as_tuple(rhs, lhs);
    big_integer res((big_integer::container_t(max_len)));
    bool carry = 0;
    for (big_integer::container_t::size_type i = 0; i < min_len; ++i) {
        bool overflow = lhs.data[i] > std::numeric_limits<uint32_t>::max() - rhs.data[i]
                || (carry && lhs.data[i] == std::numeric_limits<uint32_t>::max() - rhs.data[i]);
        res.data[i] = lhs.data[i] + rhs.data[i] + carry;
        carry = overflow;
    }
    uint32_t ext = big_integer::helper::is_negative(min) ? std::numeric_limits<uint32_t>::max() : 0;
    for (auto i = min_len; i < max_len; ++i) {
        bool overflow = max.data[i] > std::numeric_limits<uint32_t>::max() - ext
                || (carry && max.data[i] == std::numeric_limits<uint32_t>::max() - ext);
        res.data[i] = max.data[i] + ext + carry;
        carry = overflow;
    }
    if (same_sign) {
        if (lhs_sign) {
            res.data.emplace_back(std::numeric_limits<uint32_t>::max());
        }
        else {
            res.data.emplace_back(0);
        }
    }
    big_integer::helper::normalize(res);
    return res;
}

big_integer operator-(big_integer const& lhs, big_integer const& rhs)
{
    return lhs + (-rhs);
}

big_integer operator*(big_integer const& _lhs, big_integer const& _rhs)
{
    if (big_integer::helper::is_zero(_lhs) || big_integer::helper::is_zero(_rhs))
        return 0;
    const bool sign = big_integer::helper::is_negative(_lhs) != big_integer::helper::is_negative(_rhs);
    big_integer lhs(_lhs), rhs(_rhs);
    big_integer::helper::to_sign_magnitude(lhs);
    big_integer::helper::to_sign_magnitude(rhs);
    if (lhs.data.size() < rhs.data.size()) {
        lhs.swap(rhs);
    }
    big_integer res((big_integer::container_t(lhs.data.size() + rhs.data.size())));
    for (big_integer::container_t::size_type i = 0; i < rhs.data.size(); ++i) {
        big_integer tmp(lhs);
        tmp = big_integer::helper::mul_uint(tmp, rhs.data[i]);
        bool carry = 0;
        for (big_integer::container_t::size_type j = i; j < i + tmp.data.size(); ++j) {
            bool overflow = res.data[j] > std::numeric_limits<uint32_t>::max() - tmp.data[j - i]
                    || (carry && res.data[j] == std::numeric_limits<uint32_t>::max() - tmp.data[j - i]);
            res.data[j] += tmp.data[j - i] + carry;
            carry = overflow;
        }
        if (carry) {
            res.data[i + tmp.data.size()] = 1;
        }
    }
    big_integer::helper::to_twos_complement(res, sign);
    big_integer::helper::normalize(res);
    return res;
}

big_integer operator/(big_integer const& _lhs, big_integer const& _rhs)
{
    if (big_integer::helper::is_zero(_rhs)) {
        throw std::invalid_argument("big_integer::_M_division_by_zero");
    }
    const bool sign = big_integer::helper::is_negative(_lhs) != big_integer::helper::is_negative(_rhs);
    big_integer lhs(_lhs), rhs(_rhs);
    big_integer::helper::to_sign_magnitude(lhs);
    big_integer::helper::to_sign_magnitude(rhs);
    if (big_integer::helper::cmp_in_sm(lhs, rhs) < 0) {
        return 0;
    }
    if (rhs.data.size() == 1) {
        big_integer::helper::div_uint(lhs, rhs.data[0]);
        big_integer::helper::to_twos_complement(lhs, sign);
        return lhs;
    }
    auto n = lhs.data.size();
    auto l = lhs.data.size() - rhs.data.size() + 1;
    uint32_t scaling_factor = static_cast<uint32_t>((static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1)
            / (static_cast<uint64_t>(rhs.data.back()) + 1));
    lhs = big_integer::helper::mul_uint(lhs, scaling_factor);
    if (lhs.data.size() == n) {
        lhs.data.emplace_back(0);
    }
    rhs = big_integer::helper::mul_uint(rhs, scaling_factor);
    big_integer res((big_integer::container_t()));
    big_integer tmp;
    for (big_integer::container_t::size_type cnt = 0, k = 0; k < l; ++k) {
        uint32_t trial = static_cast<uint32_t>(std::min(((static_cast<uint64_t>(lhs.data.back()) << 32) |
                        (lhs.data.size() > 1 ? lhs.data[lhs.data.size() - 2] : 0)) / rhs.data.back(),
                static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())));
        tmp = big_integer::helper::mul_uint(rhs, trial);
        if (tmp.data.size() == rhs.data.size()) {
            tmp.data.emplace_back(0);
        }
        if (!cnt) {
            while ([&tmp, &lhs]() {
                for (auto i = tmp.data.size(), j = lhs.data.size() - 1; i--; --j) {
                    if (tmp.data[i] != lhs.data[j]) {
                        return tmp.data[i] > lhs.data[j];
                    }
                }
                return false;
            }()) {
                tmp = big_integer::helper::mul_uint(rhs, --trial);
                if (tmp.data.size() == rhs.data.size()) {
                    tmp.data.emplace_back(0);
                }
            }
        }
        bool borrow = false;
        for (big_integer::container_t::size_type i = 0, j = lhs.data.size() - tmp.data.size() - cnt;
             i < tmp.data.size();
             ++i, ++j) {
            bool overflow = (!borrow && lhs.data[j] < tmp.data[i]) || (borrow && lhs.data[j] <= tmp.data[i]);
            lhs.data[j] -= tmp.data[i] + borrow;
            borrow = overflow;
        }
        for (big_integer::container_t::size_type i = lhs.data.size() - cnt;
             borrow && i < lhs.data.size();
             ++i) {
            borrow = lhs.data[i] == 1;
            --lhs.data[i];
        }
        for (++cnt; !lhs.data.back() && cnt; --cnt) {
            lhs.data.pop_back();
        }
        res.data.emplace_back(trial);
    }
    std::reverse(res.data.begin(), res.data.end());
    big_integer::helper::to_twos_complement(res, sign);
    big_integer::helper::normalize(res);
    return res;
}

big_integer operator%(big_integer const& lhs, big_integer const& rhs)
{
    return lhs - (lhs / rhs) * rhs;
}

big_integer operator&(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::bit_operation(lhs, rhs, std::bit_and<uint32_t>());
}

big_integer operator|(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::bit_operation(lhs, rhs, std::bit_or<uint32_t>());
}

big_integer operator^(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::bit_operation(lhs, rhs, std::bit_xor<uint32_t>());
}

big_integer operator<<(big_integer const& lhs, int val) // lhs - is negative --> UB
{
    if (val < 0)
        return lhs >> -val;
    unsigned skip = val / 32;
    val %= 32;
    big_integer res((big_integer::container_t(lhs.data.size() + skip)));
    uint32_t carry = 0;
    for (big_integer::container_t::size_type i = 0; i < skip; ++i) {
        res.data[i] = 0;
    }
    for (big_integer::container_t::size_type i = skip; i < lhs.data.size(); ++i) {
        res.data[i] = (lhs.data[i - skip] << val) | carry;
        carry = lhs.data[i - skip] >> (32 - val);
    }
    if (carry) {
        res.data.emplace_back(carry);
    }
    big_integer::helper::normalize(res);
    return res;
}

big_integer operator>>(big_integer const& lhs, int val)
{
    if (val < 0) {
        return lhs << -val;
    }
    unsigned skip = val / 32;
    val %= 32;
    if (lhs.data.size() <= skip) {
        return 0;
    }
    big_integer res((big_integer::container_t(lhs.data.size() - skip)));
    res.data[lhs.data.size() - 1] = static_cast<int32_t>(lhs.data[lhs.data.size() - 1]) >> val;
    uint32_t carry = lhs.data[lhs.data.size() - 1] << (32 - val);
    for (big_integer::container_t::size_type i = lhs.data.size() - 1; i-- > skip;) {
        res.data[i] = (lhs.data[i] >> val) | carry;
        carry = (lhs.data[i] << (32 - val));
    }
    big_integer::helper::normalize(res);
    return res;
}

bool operator==(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::cmp(lhs, rhs) == 0;
}

bool operator!=(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::cmp(lhs, rhs) != 0;
}

bool operator<(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::cmp(lhs, rhs) < 0;
}

bool operator>(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::cmp(lhs, rhs) > 0;
}

bool operator<=(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::cmp(lhs, rhs) <= 0;
}

bool operator>=(big_integer const& lhs, big_integer const& rhs)
{
    return big_integer::helper::cmp(lhs, rhs) >= 0;
}

void swap(big_integer& lhs, big_integer& rhs) noexcept
{
    lhs.swap(rhs);
}

big_integer abs(big_integer const& x)
{
    return big_integer::helper::is_negative(x) ? -x : x;
}

std::string to_string(big_integer const& x)
{
    big_integer copy(x);
    bool is_negative = big_integer::helper::is_negative(copy);
    if (is_negative) {
        big_integer::helper::negate(copy);
    }
    std::string str;
    do {
        str += std::to_string(big_integer::helper::div_uint(copy, 10));
    }
    while (!big_integer::helper::is_zero(copy));
    if (is_negative) {
        str += '-';
    }
    std::reverse(str.begin(), str.end());
    return str;
}

std::ostream& operator<<(std::ostream& os, big_integer const& x)
{
    return os << to_string(x);
}