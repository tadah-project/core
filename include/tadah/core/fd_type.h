#ifndef FD_TYPE_H
#define FD_TYPE_H

#include <tadah/core/aed_type.h>
#include <tadah/core/vec3d.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <random>
#include <cstring>
#include <limits>

class fd_type {
  size_t rows_ = 0;
  double* data_ = nullptr;
  aed_type* aed_type2_ptr[3] = {nullptr, nullptr, nullptr};

public:

  inline fd_type() = default;

  inline fd_type(size_t dim) : rows_(dim) {
    data_ = static_cast<double*>(malloc(3 * dim * sizeof(double)));
    std::fill(data_, data_ + 3 * rows_, 0.0);
    for (int i = 0; i < 3; ++i) {
      aed_type2_ptr[i] = new aed_type(&data_[i * dim], dim, 1, false);
    }
  }

  inline ~fd_type() {
    for (int i = 0; i < 3; ++i) {
      delete aed_type2_ptr[i];
    }
    free(data_);
  }
  fd_type(const fd_type& other);
  fd_type(fd_type&& other) noexcept;

  fd_type& operator=(const fd_type& other);
  fd_type& operator=(fd_type&& other) noexcept;

  inline double& operator()(size_t i, size_t k) {
    // Added assert checks for out-of-range indexing
    assert(i < rows_);
    assert(k < 3);
    return data_[i + k * rows_];
  }

  inline const double& operator()(size_t i, size_t k) const {
    // Added assert checks for out-of-range indexing
    assert(i < rows_);
    assert(k < 3);
    return data_[i + k * rows_];
  }

  aed_type& operator[](size_t k);
  const aed_type& operator()(size_t k) const;
  aed_type& operator()(size_t k);

  inline size_t rows() const { return rows_; }
  inline void set_zero() {
    std::fill(data_, data_ + 3 * rows_, 0.0);
  }
  double* ptr();
  const double* ptr() const;

  friend std::ostream& operator<<(std::ostream& os, const fd_type& v);

  template <typename T>
  Vec3d operator*(const T& v2) const {
    Vec3d temp;
    for (size_t k = 0; k < 3; ++k) {
      for (size_t i = 0; i < v2.size(); ++i) {
        // We do not assert here but rely on the calling context
        temp[k] += (*this)(i, k) * v2[i];
      }
    }
    return temp;
  }

  fd_type operator-(const fd_type& v2) const;
  fd_type operator+(const fd_type& v2) const;

  void operator*=(double s);
  void random(int seed = std::chrono::system_clock::now().time_since_epoch().count(),
              double lower = -1e4, double upper = 1e4);

  bool isApprox(const fd_type& v1, double EPS) const;
  bool operator==(const fd_type& v1) const;
};

#endif // FD_TYPE_H

