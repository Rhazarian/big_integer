#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <iosfwd>
#include <vector>
#include <cstdint>
#include <string_view>
#include "dynamic_storage.h"

struct big_integer {
    big_integer();
    big_integer(big_integer const& x);
    big_integer(int32_t val);
    explicit big_integer(std::string_view str);
    ~big_integer();

    big_integer& operator=(big_integer const& rhs);

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer const& rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(big_integer const& rhs);
    big_integer& operator|=(big_integer const& rhs);
    big_integer& operator^=(big_integer const& rhs);

    big_integer& operator<<=(int val);
    big_integer& operator>>=(int val);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    void swap(big_integer& x) noexcept;

    friend big_integer operator+(big_integer const& lhs, big_integer const& rhs);
    friend big_integer operator-(big_integer const& lhs, big_integer const& rhs);
    friend big_integer operator*(big_integer const& lhs, big_integer const& rhs);
    friend big_integer operator/(big_integer const& lhs, big_integer const& rhs);
    friend big_integer operator%(big_integer const& lhs, big_integer const& rhs);

    friend big_integer operator&(big_integer const& lhs, big_integer const& rhs);
    friend big_integer operator|(big_integer const& lhs, big_integer const& rhs);
    friend big_integer operator^(big_integer const& lhs, big_integer const& rhs);

    friend big_integer operator<<(big_integer const& lhs, int val);
    friend big_integer operator>>(big_integer const& lhs, int val);

    friend bool operator==(big_integer const& lhs, big_integer const& rhs);
    friend bool operator!=(big_integer const& lhs, big_integer const& rhs);
    friend bool operator<(big_integer const& lhs, big_integer const& rhs);
    friend bool operator>(big_integer const& lhs, big_integer const& rhs);
    friend bool operator<=(big_integer const& lhs, big_integer const& rhs);
    friend bool operator>=(big_integer const& lhs, big_integer const& rhs);

    friend void swap(big_integer& lhs, big_integer& rhs) noexcept;

    friend big_integer abs(big_integer const& x);
    friend std::string to_string(big_integer const& x);

private:
    struct helper;

    typedef std::vector<uint32_t> container_t;
    container_t data;

    explicit big_integer(container_t data);
};

big_integer operator+(big_integer const& lhs, big_integer const& rhs);
big_integer operator-(big_integer const& lhs, big_integer const& rhs);
big_integer operator*(big_integer const& lhs, big_integer const& rhs);
big_integer operator/(big_integer const& lhs, big_integer const& rhs);
big_integer operator%(big_integer const& lhs, big_integer const& rhs);

big_integer operator&(big_integer const& lhs, big_integer const& rhs);
big_integer operator|(big_integer const& lhs, big_integer const& rhs);
big_integer operator^(big_integer const& lhs, big_integer const& rhs);

big_integer operator<<(big_integer const& lhs, int val);
big_integer operator>>(big_integer const& lhs, int val);

bool operator==(big_integer const& lhs, big_integer const& rhs);
bool operator!=(big_integer const& lhs, big_integer const& rhs);
bool operator<(big_integer const& lhs, big_integer const& rhs);
bool operator>(big_integer const& lhs, big_integer const& rhs);
bool operator<=(big_integer const& lhs, big_integer const& rhs);
bool operator>=(big_integer const& lhs, big_integer const& rhs);

void swap(big_integer& lhs, big_integer& rhs) noexcept;

big_integer abs(big_integer const& x);
std::string to_string(big_integer const& x);
std::ostream& operator<<(std::ostream& os, big_integer const& x);

#endif //BIG_INTEGER_H