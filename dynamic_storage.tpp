#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <cstring>

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
    if (capacity <= small_data::capacity && _size >= small_data::capacity) {

    }
}

template<typename T>
void dynamic_storage<T>::prepare_for_modification() {
    if (_size >= small_data::capacity && _data.big.data.use_count() > 1) {
        T* ndata = (T*) operator new(_data.big.capacity * sizeof(T));
        copy_construct(ndata, _data.big.data.get(), _size);
        _data.big.data.reset(ndata, big_data::deleter());
    }
}

template<typename T>
dynamic_storage<T>::dynamic_storage() : _size(0) { }

template<typename T>
dynamic_storage<T>::dynamic_storage(size_t n, T value) : _size(n) {
    if (_size < small_data::capacity) {
        for (size_t i = 0; i < _size; ++i) {
            new (_data.small.data + i) T(value);
        }
    } else {
        new (&_data.big) big_data(_size, (T*) operator new(_size * sizeof(T)));
        for (size_t i = 0; i < _size; ++i) {
            new (_data.big.data.get() + i) T(value);
        }
    }
}

template<typename T>
dynamic_storage<T>::dynamic_storage(dynamic_storage const& other) : _size(other._size) {
    if (_size < small_data::capacity) {
        memcpy(_data.small.data, other._data.small.data, small_data::capacity * sizeof(T));
    } else {
        new (&_data.big) big_data(other._data.big);
    }
}

template<typename T>
dynamic_storage<T>::dynamic_storage(std::initializer_list<T> init) : _size(init.size()){
    auto it = init.begin();
    if (_size < small_data::capacity) {
        for (size_t i = 0; i < _size; ++i, ++it) {
            new (_data.small.data + i) T(*it);
        }
    } else {
        new (&_data.big) big_data(_size, (T*) operator new(_size * sizeof(T)));
        for (size_t i = 0; i < _size; ++i, ++it) {
            new (_data.big.data.get() + i) T(*it);
        }
    }
}

template<typename T>
dynamic_storage<T>::~dynamic_storage() {
    if (_size >= small_data::capacity) {
        _data.big.~big_data();
    }
}

template<typename T>
bool dynamic_storage<T>::empty() const noexcept {
    return _size == 0;
}

template<typename T>
typename dynamic_storage<T>::size_type dynamic_storage<T>::size() const noexcept {
    return _size;
}

template<typename T>
typename dynamic_storage<T>::size_type dynamic_storage<T>::capacity() const noexcept {
    return _size < small_data::capacity ? small_data::capacity : _data.big.capacity;
}

template<typename T>
void dynamic_storage<T>::shrink_to_fit() {
    if (_size >= small_data::capacity && _size != _data.big.capacity) {
        set_capacity(_size);
    }
}