#ifndef DYNAMIC_STORAGE_H
#define DYNAMIC_STORAGE_H

#include <cstdlib>

template<typename T>
struct dynamic_storage {
    dynamic_storage() noexcept;
    explicit dynamic_storage(size_t n, T value = T());
    dynamic_storage(dynamic_storage const& other);
    ~dynamic_storage() noexcept;
};

#include <dynamic_storage.tpp>
#endif //DYNAMIC_STORAGE_H
