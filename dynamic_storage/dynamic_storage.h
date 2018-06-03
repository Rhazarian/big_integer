#ifndef DYNAMIC_STORAGE_H
#define DYNAMIC_STORAGE_H

#include <cstdlib>

template<typename T>
struct dynammic_storage {
    dynammic_storage() noexcept;
    explicit dynammic_storage(size_t n, T value = T());
    dynammic_storage(dynammic_storage const& other);
    ~dynammic_storage() noexcept;
};

#include <dynamic_storage.tpp>
#endif //DYNAMIC_STORAGE_H
