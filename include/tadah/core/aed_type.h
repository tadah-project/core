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
 *
 * A copy (the copy constructor, an assignment into an owner, a move from
 * a view, and the const double* constructor) is an owner of the size()
 * values one after another (stride 1), read through the source's stride:
 * a strided source (Matrix::row, size() values rows() apart) holds only
 * the values it views, so a copy of size() * stride doubles from its first
 * value would read past them.
 */
class aed_type {
  double *data_ = nullptr;
  size_t size_ = 0;
  size_t stride_ = 1;
  bool allocated_ = false;
  bool view_ = false;   // values of storage another object owns

  // The values of `other` into this view; sizes must match.
  void assign_to_view(const aed_type& other);

  // A new (malloc'ed) buffer of the `n` values `stride` apart in `src`,
  // one after another.
  static double* compact_copy(const double* src, size_t n, size_t stride) {
    double* out = static_cast<double*>(malloc(n * sizeof(double)));
    if (stride == 1) {
      if (n > 0) std::memcpy(out, src, n * sizeof(double));
    } else {
      for (size_t i = 0; i < n; ++i) out[i] = src[i * stride];
    }
    return out;
  }

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

  /// An owning copy of @p size values of @p arr, @p stride apart (stride
  /// 1 in the copy).
  inline aed_type(const double* arr, size_t size, size_t stride = 1)
  : data_(compact_copy(arr, size, stride)), size_(size), stride_(1),
    allocated_(true) {}

  /// A view of @p size values of @p arr, @p stride apart (allocated
  /// false), or, with allocated true, an owner of the malloc'ed @p arr.
  inline aed_type(double* arr, size_t size, size_t stride = 1, bool allocated = false)
  : data_(arr), size_(size), stride_(stride), allocated_(allocated),
    view_(!allocated) {}

  /// An owning copy (stride 1), also of a view.
  inline aed_type(const aed_type& other)
  : data_(compact_copy(other.data_, other.size_, other.stride_)),
    size_(other.size_), stride_(1), allocated_(true) {}

  /// Takes the other's buffer, leaving it empty; from a view, an owning
  /// copy (stride 1; the view keeps viewing its storage).
  inline aed_type(aed_type&& other) noexcept
  : data_(other.data_), size_(other.size_), stride_(other.stride_), allocated_(other.allocated_) {
    if (other.view_) {
      allocated_ = true;
      stride_ = 1;
      data_ = compact_copy(other.data_, size_, other.stride_);
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
  /// std::invalid_argument otherwise); into an owner: an owning copy
  /// (stride 1), also of a view of the owner's own values.
  aed_type& operator=(const aed_type& other);
  /// Into a view: as the copy assignment. Into an owner: takes the
  /// other's buffer (an owning copy of a view, stride 1), leaving it empty.
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

