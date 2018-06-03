#ifndef DYNAMIC_STORAGE_H
#define DYNAMIC_STORAGE_H

#include <cstdlib>
#include <initializer_list>
#include <memory>

template<typename T>
struct dynamic_storage {
public:
    typedef size_t size_type;

    dynamic_storage();
    explicit dynamic_storage(size_type n, T value = T());
    dynamic_storage(dynamic_storage const& other);
    dynamic_storage(std::initializer_list<T> init);
    ~dynamic_storage();

    bool empty() const noexcept;
    size_type size() const noexcept;
    size_type capacity() const noexcept;
    void shrink_to_fit();

private:
    size_t _size;
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
        static constexpr size_t capacity = sizeof(big_data) / sizeof(T) != 0 ? sizeof(big_data) / sizeof(T) : 1;
        T data[capacity];
    };
    union any_data {
        small_data small;
        big_data big;
        any_data();
        ~any_data();
    } _data;

    void set_capacity(size_type capacity);
    void prepare_for_modification();
};

#include "dynamic_storage.tpp"
#endif //DYNAMIC_STORAGE_H