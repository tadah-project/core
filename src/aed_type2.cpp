#include <tadah/core/aed_type2.h>

aed_type2& aed_type2::operator=(const aed_type2& other) {
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

aed_type2& aed_type2::operator=(aed_type2&& other) noexcept {
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

bool aed_type2::isApprox(const aed_type2& v1, double EPS) const {
    if (size() != v1.size()) return false;
    for (size_t i = 0; i < size(); ++i) {
        if (std::abs((*this)[i] - v1[i]) > EPS) return false;
    }
    return true;
}

double* aed_type2::data() { return data_; }
const double* aed_type2::data() const { return data_; }

void aed_type2::resize(const size_t n) {
  if (n != size_) {
    if (allocated_) {
      data_ = static_cast<double*>(realloc(data_, stride_ * n * sizeof(double)));
    } else {
      data_ = static_cast<double*>(malloc(stride_ * n * sizeof(double)));
      allocated_ = true;
    }
    size_ = n;
  }
}

aed_type2 aed_type2::copy(size_t nelements, size_t start) const {
    if (nelements + start > size()) {
        throw std::runtime_error("Out of range");
    }
    aed_type2 temp(nelements);
    for (size_t i = 0; i < nelements; ++i) {
        temp[i] = (*this)[start + i];
    }
    return temp;
}

double aed_type2::mean() const {
  double total = 0;
  for (size_t i = 0; i < size(); ++i) {
    total += (*this)[i];
  }
  return total / size();
}

double aed_type2::std_dev(double mean, double N) const {
  double variance = 0;
  for (size_t i = 0; i < size(); ++i) {
    double diff = (*this)[i] - mean;
    variance += diff * diff;
  }
  return std::sqrt(variance / N);
}

void aed_type2::random(int seed, double lower, double upper) {
    std::uniform_real_distribution<double> unif(lower, upper);
    std::default_random_engine engine(seed);
    for (size_t i = 0; i < size(); ++i) {
        (*this)[i] = unif(engine);
    }
}
