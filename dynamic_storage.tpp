#include "dynamic_storage.h"

#include <cstdlib>
#include <initializer_list>
#include <new>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <cstdint>

template<typename T>
dynamic_storage<T>::big_data::deleter::deleter() = default;

template<typename T>
dynamic_storage<T>::big_data::deleter::~deleter() = default;

template<typename T>
void dynamic_storage<T>::big_data::deleter::operator()(T* p) {
    operator delete(p);
}

template<typename T>
dynamic_storage<T>::big_data::big_data() = default;

template<typename T>
dynamic_storage<T>::big_data::big_data(size_t capacity, T* data) : capacity(capacity), data(data, deleter()) { }

template<typename T>
dynamic_storage<T>::big_data::big_data(const dynamic_storage<T>::big_data& other) = default;

template<typename T>
dynamic_storage<T>::big_data::~big_data() = default;

template<typename T>
dynamic_storage<T>::any_data::any_data() { }

template<typename T>
dynamic_storage<T>::any_data::~any_data() { }

namespace {
template<typename T>
typename std::enable_if<!std::is_trivially_copy_constructible<T>::value, void>::type copy_construct(T* dst, T* src,
        size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        new(dst + i) T(src[i]);
    }
}

template<typename T>
typename std::enable_if<std::is_trivially_copy_constructible<T>::value, void>::type copy_construct(T* dst, T* src,
        size_t size)
{
    memcpy(dst, src, size * sizeof(T));
}

template<typename T>
typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type destruct(T* dst, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        dst[i].~T();
    }
}

template<typename T>
typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type destruct(T* dst, size_t size) { }
}

template<typename T>
void dynamic_storage<T>::set_capacity(size_type capacity) {
    if (capacity > small_data::capacity) {
        auto tmp = (T*) operator new(capacity * sizeof(T));
        if (is_data_big()) {
            copy_construct(tmp, current, std::min(_size, capacity));
            if (_data.big.data.use_count() == 1) {
                destruct(current, _size);
            }
            current = tmp;
            _data.big.capacity = capacity;
            _data.big.data.reset(current, typename big_data::deleter());
        } else {
            copy_construct(tmp, current, _size);
            destruct(current, _size);
            current = tmp;
            new(&_data.big) big_data(capacity, current);
        }
    } else {
        if (is_data_big()) {
            T tmp[small_data::capacity];
            copy_construct(tmp, current, capacity);
            if (_data.big.data.use_count() == 1) {
                destruct(current, _size);
            }
            _data.big.~big_data();
            current = _data.small.data;
            copy_construct(current, tmp, capacity);
        } else {
            if (capacity < _size) {
                destruct(current + capacity, _size - capacity);
            }
        }
    }
    _size = std::min(_size, capacity);
}

template<typename T>
void dynamic_storage<T>::prepare_for_modification() {
    if (is_data_big() && _data.big.data.use_count() > 1) {
        set_capacity(_data.big.capacity);
    }
}

template<typename T>
dynamic_storage<T>::dynamic_storage() : _size(0), current(_data.small.data) { }

template<typename T>
dynamic_storage<T>::dynamic_storage(size_t n, T value) : _size(n) {
    if (_size <= small_data::capacity) {
        current = _data.small.data;
    } else {
        current = (T*) operator new(_size * sizeof(T));
        new (&_data.big) big_data(_size, current);
    }
    for (size_t i = 0; i < _size; ++i) {
        new (current + i) T(value);
    }
}

template<typename T>
dynamic_storage<T>::dynamic_storage(dynamic_storage const& other) : _size(other._size) {
    if (_size <= small_data::capacity) {
        memcpy(_data.small.data, other._data.small.data, small_data::capacity * sizeof(T));
        current = _data.small.data;
    } else {
        new (&_data.big) big_data(other._data.big);
        current = _data.big.data.get();
    }
}

template<typename T>
dynamic_storage<T>::dynamic_storage(std::initializer_list<T> init) : _size(init.size()) {
    auto it = init.begin();
    if (_size <= small_data::capacity) {
        current = _data.small.data;
    } else {
        current = (T*) operator new(_size * sizeof(T));
        new (&_data.big) big_data(_size, current);
    }
    for (size_t i = 0; i < _size; ++i, ++it) {
        new (current + i) T(*it);
    }
}

template<typename T>
dynamic_storage<T>::~dynamic_storage() {
    if (is_data_big()) {
        _data.big.~big_data();
    }
}

template<typename T>
bool dynamic_storage<T>::empty() const noexcept {
    return _size == 0;
}

template<typename T>
T& dynamic_storage<T>::back() noexcept {
    assert(_size != 0);
    return (*this)[_size - 1];
}

template<typename T>
T const& dynamic_storage<T>::back() const noexcept {
    assert(_size != 0);
    return (*this)[_size - 1];
}

template<typename T>
T& dynamic_storage<T>::operator[](size_type n) noexcept {
    assert(n < _size);
    prepare_for_modification();
    return current[n];
}

template<typename T>
T const& dynamic_storage<T>::operator[](size_type n) const noexcept {
    assert(n < _size);
    return current[n];
}

template<typename T>
typename dynamic_storage<T>::size_type dynamic_storage<T>::size() const noexcept {
    return _size;
}

template<typename T>
typename dynamic_storage<T>::size_type dynamic_storage<T>::capacity() const noexcept {
    return is_data_big() ? _data.big.capacity : small_data::capacity;
}

template<typename T>
void dynamic_storage<T>::reserve(size_type n) {
    if (n > capacity()) {
        set_capacity(n);
    }
}

template<typename T>
void dynamic_storage<T>::shrink_to_fit() {
    if (is_data_big() && _size != _data.big.capacity) {
        set_capacity(_size);
    }
}

template<typename T>
void dynamic_storage<T>::swap(dynamic_storage<T>& other) noexcept {
    std::swap(_size, other._size);
    if (is_data_big() && other.is_data_big()) {
        std::swap(current, other.current);
    } else if (is_data_big()) {
        other.current = current;
        current = _data.small.data;
    } else if (other.is_data_big()) {
        current = other.current;
        other.current = other._data.small.data;
    }
    uint8_t tmp[sizeof(any_data)];
    memcpy(tmp, &this->_data, sizeof(any_data));
    memcpy(&this->_data, &other._data, sizeof(any_data));
    memcpy(&other._data, tmp, sizeof(any_data));
}

template<typename T>
typename dynamic_storage<T>::iterator dynamic_storage<T>::begin() noexcept {
    prepare_for_modification();
    return current;
}

template<typename T>
typename dynamic_storage<T>::const_iterator dynamic_storage<T>::begin() const noexcept {
    return current;
}

template<typename T>
typename dynamic_storage<T>::iterator dynamic_storage<T>::end() noexcept {
    return begin() + _size;
}

template<typename T>
typename dynamic_storage<T>::const_iterator dynamic_storage<T>::end() const noexcept {
    return begin() + _size;
}

template<typename T>
typename dynamic_storage<T>::reverse_iterator dynamic_storage<T>::rbegin() noexcept {
    return dynamic_storage<T>::reverse_iterator(end());
}

template<typename T>
typename dynamic_storage<T>::const_reverse_iterator dynamic_storage<T>::rbegin() const noexcept {
    return dynamic_storage<T>::const_reverse_iterator(end());
}

template<typename T>
typename dynamic_storage<T>::reverse_iterator dynamic_storage<T>::rend() noexcept {
    return dynamic_storage<T>::reverse_iterator(begin());
}

template<typename T>
typename dynamic_storage<T>::const_reverse_iterator dynamic_storage<T>::rend() const noexcept {
    return dynamic_storage<T>::const_reverse_iterator(begin());
}

template<typename T>
template<class... Args>
void dynamic_storage<T>::emplace_back(Args&& ... args)
{
    assert(_size != std::numeric_limits<size_type>::max());
    if (_size == capacity()) {
        set_capacity(_size << 1);
    } else {
        prepare_for_modification();
    }
    new (current + _size) T(args...);
    ++_size;
}

template<typename T>
void dynamic_storage<T>::push_back(T value)
{
    emplace_back(value);
}

template<typename T>
void dynamic_storage<T>::pop_back()
{
    assert(_size != 0);
    if (_size == small_data::capacity + 1) {
        set_capacity(small_data::capacity);
    } else {
        prepare_for_modification();
        --_size;
        destruct(current + _size, 1);
    }
}

template<typename T>
bool dynamic_storage<T>::is_data_big() const noexcept
{
    return current != _data.small.data;
}

template<typename T>
dynamic_storage& dynamic_storage<T>::operator=(dynamic_storage const& other)
{
    dynamic_storage copy(other);
    swap(copy);
    return *this;
}
