#ifndef DYNAMIC_STORAGE_H
#define DYNAMIC_STORAGE_H

#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <algorithm>

template<typename T>
struct dynamic_storage {
public:
    typedef size_t size_type;
    typedef T* iterator;
    typedef T const* const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    dynamic_storage();
    explicit dynamic_storage(size_type n, T value = T());
    dynamic_storage(dynamic_storage const& other);
    dynamic_storage(std::initializer_list<T> init);
    ~dynamic_storage();

    dynamic_storage& operator=(dynamic_storage const &other);

    template<class... Args>
    void emplace_back(Args&&... args);
    void push_back(T value);
    void pop_back();

    bool empty() const noexcept;
    T& back() noexcept;
    T const& back() const noexcept;
    T& operator[](size_type n) noexcept;
    T const& operator[](size_type n) const noexcept;
    size_type size() const noexcept;
    size_type capacity() const noexcept;
    void reserve(size_type n);
    void shrink_to_fit();

    void swap(dynamic_storage& other) noexcept;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;
    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crend() const noexcept;

private:
    size_type _size;
    struct big_data {
        struct deleter {
            deleter();
            ~deleter();
            void operator()(T* p);
        };
        size_type capacity;
        std::shared_ptr<T> data;
        big_data();
        big_data(size_type capacity, T* data);
        big_data(big_data const& other);
        ~big_data();
    };
    struct small_data {
        static constexpr size_t capacity = std::min(sizeof(big_data) / sizeof(T), static_cast<size_t>(1));
        T data[capacity];
    };
    union any_data {
        small_data small;
        big_data big;
        any_data();
        ~any_data();
    } _data;
    T* current;

    void set_capacity(size_type capacity);
    void prepare_for_modification();
    bool is_data_big() const noexcept;
};

#include "dynamic_storage.tpp"
#endif //DYNAMIC_STORAGE_H