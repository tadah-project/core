#ifndef TADAH_CORE_AED_TYPE2_H
#define TADAH_CORE_AED_TYPE2_H
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <limits>
#include <random>
#include <vector>
#include <iostream>
#include <chrono>

namespace tadah {
namespace core {

/**
 * @brief A vector of doubles that owns its values, or a view of values it
 *        does not own (a column of an fd_type or an aeds_type2).
 *
 * A view is made by the non-owning pointer constructor (allocated false)
 * or rebind(). An assignment to a view writes the other's values into the
 * viewed storage, whatever the other is (copy or move): the sizes must
 * match (std::invalid_argument otherwise), and a view is not resized to
 * another size. An assignment to an owning (or default-constructed)
 * aed_type replaces its values. Moving from a view copies its values (a
 * view owns nothing to give) and leaves it as it is.
 */
class aed_type {
  double *data_ = nullptr;
  size_t size_ = 0;
  size_t stride_ = 1;
  bool allocated_ = false;
  bool view_ = false;   // values of storage another object owns

  // The values of `other` into this view; sizes must match.
  void assign_to_view(const aed_type& other);

public:

  inline aed_type(std::initializer_list<double> init) {
    size_ = init.size();
    data_ = static_cast<double*>(malloc(size_ * sizeof(double)));
    allocated_=true;
    size_t i = 0;
    for (double value : init) {
      data_[i++] = value;
    }
  }

  inline aed_type() : data_(nullptr), size_(0), stride_(1), allocated_(false) {}

  inline aed_type(size_t n, size_t s = 1)
  : size_(n), stride_(s), allocated_(true) {
    data_ = static_cast<double*>(malloc(s * n * sizeof(double)));
    std::fill(data_, data_ + s * n, 0.0);
  }

  inline aed_type(const double* arr, size_t size, size_t stride = 1)
  : size_(size), stride_(stride), allocated_(true) {
    data_ = static_cast<double*>(malloc(size_ * stride_ * sizeof(double)));
    std::memcpy(data_, arr, size_ * stride_ * sizeof(double));
  }

  /// A view of @p size values of @p arr, @p stride apart (allocated
  /// false), or, with allocated true, an owner of the malloc'ed @p arr.
  inline aed_type(double* arr, size_t size, size_t stride = 1, bool allocated = false)
  : data_(arr), size_(size), stride_(stride), allocated_(allocated),
    view_(!allocated) {}

  /// An owning copy, also of a view.
  inline aed_type(const aed_type& other)
  : size_(other.size_), stride_(other.stride_), allocated_(true) {
    data_ = static_cast<double*>(malloc(size_ * stride_ * sizeof(double)));
    if (size_ * stride_ > 0)
      std::memcpy(data_, other.data_, size_ * stride_ * sizeof(double));
  }

  /// Takes the other's buffer, leaving it empty; from a view, an owning
  /// copy (the view keeps viewing its storage).
  inline aed_type(aed_type&& other) noexcept
  : data_(other.data_), size_(other.size_), stride_(other.stride_), allocated_(other.allocated_) {
    if (other.view_) {
      allocated_ = true;
      data_ = static_cast<double*>(malloc(size_ * stride_ * sizeof(double)));
      if (size_ * stride_ > 0)
        std::memcpy(data_, other.data_, size_ * stride_ * sizeof(double));
      return;
    }
    other.data_ = nullptr;
    other.size_ = 0;
    other.allocated_ = false;
  }

  inline ~aed_type() {
    if (allocated_) {
      free(data_);
    }
  }

  /// Into a view: the other's values written through (sizes must match,
  /// std::invalid_argument otherwise); into an owner: an owning copy.
  aed_type& operator=(const aed_type& other);
  /// Into a view: as the copy assignment. Into an owner: takes the
  /// other's buffer (an owning copy of a view), leaving it empty.
  aed_type& operator=(aed_type&& other);

  /// Makes this aed_type a view of @p size values of @p data, @p stride
  /// apart, freeing what it owned (fd_type and aeds_type2 point their
  /// columns at their storage with it).
  void rebind(double* data, size_t size, size_t stride = 1) noexcept;

  /// True for a view (rebind(), or the non-owning pointer constructor).
  bool is_view() const noexcept { return view_; }

  inline double& operator[](size_t i) {
    return data_[i * stride_];
  }

  inline const double& operator[](size_t i) const {
    return data_[i * stride_];
  }

  inline double& operator()(size_t i) {
    return data_[i * stride_];
  }

  inline const double& operator()(size_t i) const {
    return data_[i * stride_];
  }

  template <typename T>
  inline aed_type& operator+=(const T& v1) {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i] += v1[i];
    }
    return *this;
  }

  inline aed_type& operator+=(const double d) {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i] += d;
    }
    return *this;
  }

  inline aed_type operator+(const double d) const {
    aed_type temp(*this);
    for (size_t i = 0; i < size(); ++i) {
      temp[i] += d;
    }
    return temp;
  }

  template <typename T>
  inline aed_type& operator-=(const T& v1) {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i] -= v1[i];
    }
    return *this;
  }

  inline aed_type& operator-=(const double d) {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i] -= d;
    }
    return *this;
  }

  inline aed_type operator-(const double d) const {
    aed_type temp(*this);
    for (size_t i = 0; i < size(); ++i) {
      temp[i] -= d;
    }
    return temp;
  }

  inline aed_type& operator*=(const double s) {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i] *= s;
    }
    return *this;
  }

  template <typename T>
  inline aed_type operator-(const T& v2) const {
    aed_type temp(*this);
    for (size_t i = 0; i < size(); ++i) {
      temp[i] -= v2[i];
    }
    return temp;
  }

  template <typename T>
  inline aed_type operator+(const T& v2) const {
    aed_type temp(*this);
    for (size_t i = 0; i < size(); ++i) {
      temp[i] += v2[i];
    }
    return temp;
  }

  template <typename T>
  inline double operator*(const T& v2) const {
    double result = 0;
    for (size_t i = 0; i < size(); ++i) {
      result += (*this)[i] * v2[i];
    }
    return result;
  }

  inline aed_type operator*(const double s) const {
    aed_type temp(*this);
    for (size_t i = 0; i < size(); ++i) {
      temp[i] *= s;
    }
    return temp;
  }

  friend inline aed_type operator*(const double s, const aed_type& v) {
    aed_type temp(v);
    for (size_t i = 0; i < v.size(); ++i) {
      temp[i] *= s;
    }
    return temp;
  }

  inline void set_zero() {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i] = 0;
    }
  }

  inline size_t size() const {
    return size_;
  }

  inline size_t rows() const {
    return size_;
  }

  bool isApprox(const aed_type& v1, double EPS) const;

  inline bool operator==(const aed_type& v1) const {
    if (size() != v1.size()) return false;
    double EPS = std::numeric_limits<double>::epsilon();
    return isApprox(v1, EPS);
  }

  inline bool operator!=(const aed_type& v1) const {
    return !(*this == v1);
  }

  /// Resizes an owner (new values 0). A view keeps its size: resizing it
  /// to another size throws std::invalid_argument.
  void resize(const size_t n);

  double* data();
  const double* data() const;

  aed_type copy(size_t nelements, size_t start = 0) const;

  inline double* ptr() {
    return data_;
  }

  double mean() const;
  double std_dev(double mean, double N) const;

  void random(int seed = std::chrono::system_clock::now().time_since_epoch().count(),
              double lower = -1e4, double upper = 1e4);

  inline double* begin() { return data_; }
  inline double* end() { return data_ + size_; }
  inline const double* begin() const { return data_; }
  inline const double* end() const { return data_ + size_; }
};

inline std::ostream& operator<<(std::ostream& os, const aed_type& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        os << v[i];
        if (i < v.size() - 1) os << "  ";
    }
    return os;
}


// class aeds_type2 {
// public:
//     std::vector<aed_type> data_;
//
//     inline aeds_type2() {}
//
//     inline aeds_type2(size_t dim, size_t natoms)
//         : data_(natoms, aed_type(dim)) {}
//
//     inline double& operator()(size_t r, size_t c) {
//         return data_[c][r];
//     }
//
//     inline aed_type& operator[](size_t i) {
//         return data_[i];
//     }
//
//     inline size_t cols() const {
//         return data_.size();
//     }
//
//     inline size_t rows() const {
//         return data_[0].size();
//     }
//
//     inline size_t size() const {
//         return data_.size();
//     }
//
//     inline aed_type& col(size_t i) {
//         return data_[i];
//     }
//
//     inline const aed_type& col(size_t i) const {
//         return data_[i];
//     }
//
//     void resize(size_t dim, size_t natoms);
//     void set_zero();
//
//     friend std::ostream& operator<<(std::ostream& os, const aeds_type2& v) {
//         for (const aed_type& a : v.data_) os << a << std::endl;
//         return os;

//     }
// };

} // namespace core
} // namespace tadah
#endif // AED_TYPE2_H

