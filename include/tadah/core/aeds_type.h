#ifndef AEDS_TYPE_H
#define AEDS_TYPE_H

#include <tadah/core/aed_type.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

class aeds_type2 {
    double* data_ = nullptr;
    size_t dim_ = 0;
    size_t natoms_ = 0;
    aed_type** aptr = nullptr;

public:
    aeds_type2();
    aeds_type2(size_t dim, size_t natoms);
    aeds_type2(const aeds_type2& other);
    aeds_type2(aeds_type2&& other) noexcept;
    ~aeds_type2();

    aeds_type2& operator=(const aeds_type2& other);
    aeds_type2& operator=(aeds_type2&& other) noexcept;

    inline double& operator()(size_t r, size_t c) {
        return data_[r * natoms_ + c];
    }

    inline const double& operator()(size_t r, size_t c) const {
        return data_[r * natoms_ + c];
    }

    inline aed_type& operator[](size_t i) {
        return *aptr[i];
    }

    inline const aed_type& operator[](size_t i) const {
        return *aptr[i];
    }

    inline aed_type& col(size_t i) {
        return *aptr[i];
    }

    inline const aed_type& col(size_t i) const {
        return *aptr[i];
    }

    inline size_t cols() const {
        return natoms_;
    }

    inline size_t rows() const {
        return dim_;
    }

    inline size_t size() const {
        return dim_ * natoms_;
    }

    void resize(size_t dim, size_t natoms);
    void set_zero();

    friend std::ostream& operator<<(std::ostream& os, const aeds_type2& v);
};

#endif // AEDS_TYPE2_H
