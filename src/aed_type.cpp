#include <tadah/core/aed_type.h>

namespace tadah {
namespace core {

aed_type& aed_type::operator=(const aed_type& other) {
    if (this != &other) {
        if (allocated_) {
            free(data_);
        }
        size_ = other.size_;
        stride_ = other.stride_;
        allocated_ = true;
        data_ = static_cast<double*>(malloc(size_ * stride_ * sizeof(double)));
        std::memcpy(data_, other.data_, size_ * stride_ * sizeof(double));
    }
    return *this;
}

aed_type& aed_type::operator=(aed_type&& other) noexcept {
    if (this != &other) {
        if (allocated_) {
            free(data_);
        }
        data_ = other.data_;
        size_ = other.size_;
        stride_ = other.stride_;
        allocated_ = other.allocated_;
        other.data_ = nullptr;
        other.allocated_ = false;
    }
    return *this;
}

bool aed_type::isApprox(const aed_type& v1, double EPS) const {
    if (size() != v1.size()) return false;
    for (size_t i = 0; i < size(); ++i) {
        if (std::abs((*this)[i] - v1[i]) > EPS) return false;
    }
    return true;
}

double* aed_type::data() { return data_; }
const double* aed_type::data() const { return data_; }

void aed_type::resize(const size_t n) {
  size_t size_orig = size_;
  if (n != size_) {
    if (allocated_) {
      data_ = static_cast<double*>(realloc(data_, stride_ * n * sizeof(double)));
    } else {
      data_ = static_cast<double*>(malloc(stride_ * n * sizeof(double)));
      allocated_ = true;
    }
    size_ = n;
  }
  // Zero-fill only when growing. On a shrink the old fill range
  // (data_+size_orig, data_+n) has first > last, and std::fill would run
  // off the end of the reallocated block (heap-buffer-overflow).
  if (n > size_orig) {
    std::fill(data_+stride_*size_orig, data_ + stride_ * n, 0.0);
  }
}

aed_type aed_type::copy(size_t nelements, size_t start) const {
    if (nelements + start > size()) {
        throw std::runtime_error("Out of range");
    }
    aed_type temp(nelements);
    for (size_t i = 0; i < nelements; ++i) {
        temp[i] = (*this)[start + i];
    }
    return temp;
}

double aed_type::mean() const {
  double total = 0;
  for (size_t i = 0; i < size(); ++i) {
    total += (*this)[i];
  }
  return total / size();
}

double aed_type::std_dev(double mean, double N) const {
  double variance = 0;
  for (size_t i = 0; i < size(); ++i) {
    double diff = (*this)[i] - mean;
    variance += diff * diff;
  }
  return std::sqrt(variance / N);
}

void aed_type::random(int seed, double lower, double upper) {
    std::uniform_real_distribution<double> unif(lower, upper);
    std::default_random_engine engine(seed);
    for (size_t i = 0; i < size(); ++i) {
        (*this)[i] = unif(engine);
    }
}

} // namespace core
} // namespace tadah

