#include <tadah/core/fd_type.h>
#include <tadah/core/aed_type.h>


fd_type::fd_type(const fd_type& other) : rows_(other.rows_) {
  data_ = static_cast<double*>(malloc(3 * rows_ * sizeof(double)));
  std::memcpy(data_, other.data_, 3 * rows_ * sizeof(double));
  for (int i = 0; i < 3; ++i) {
    aed_type2_ptr[i] = new aed_type(&data_[i * rows_], rows_, 1, false);
  }
}

fd_type& fd_type::operator=(const fd_type& other) {
  if (this != &other) {
    for (int i = 0; i < 3; ++i) {
      delete aed_type2_ptr[i];
    }
    free(data_);
    rows_ = other.rows_;
    data_ = static_cast<double*>(malloc(3 * rows_ * sizeof(double)));
    std::memcpy(data_, other.data_, 3 * rows_ * sizeof(double));
    for (int i = 0; i < 3; ++i) {
      aed_type2_ptr[i] = new aed_type(&data_[i * rows_], rows_, 1, false);
    }
  }
  return *this;
}

fd_type::fd_type(fd_type&& other) noexcept : rows_(other.rows_), data_(other.data_) {
  for (int i = 0; i < 3; ++i) {
    aed_type2_ptr[i] = other.aed_type2_ptr[i];
    other.aed_type2_ptr[i] = nullptr;
  }
  other.data_ = nullptr;
  other.rows_ = 0;
}

fd_type& fd_type::operator=(fd_type&& other) noexcept {
  if (this != &other) {
    for (int i = 0; i < 3; ++i) {
      delete aed_type2_ptr[i];
    }
    free(data_);
    rows_ = other.rows_;
    data_ = other.data_;
    for (int i = 0; i < 3; ++i) {
      aed_type2_ptr[i] = other.aed_type2_ptr[i];
      other.aed_type2_ptr[i] = nullptr;
    }
    other.data_ = nullptr;
    other.rows_ = 0;
  }
  return *this;
}

aed_type& fd_type::operator[](size_t k) {
  return *aed_type2_ptr[k];
}

const aed_type& fd_type::operator()(size_t k) const {
  return *aed_type2_ptr[k];
}

aed_type& fd_type::operator()(size_t k) {
  return *aed_type2_ptr[k];
}


double* fd_type::ptr() {
  return data_;
}

const double* fd_type::ptr() const {
  return data_;
}

std::ostream& operator<<(std::ostream& os, const fd_type& v) {
  for (int i = 0; i < 3; ++i) {
    if (v.aed_type2_ptr[i]) {
      os << *(v.aed_type2_ptr[i]) << std::endl;
    }
  }
  return os;
}

fd_type fd_type::operator-(const fd_type& v2) const {
  fd_type temp(rows_);
  for (size_t i = 0; i < 3 * rows_; ++i) {
    temp.data_[i] = data_[i] - v2.data_[i];
  }
  return temp;
}

fd_type fd_type::operator+(const fd_type& v2) const {
  fd_type temp(rows_);
  for (size_t i = 0; i < 3 * rows_; ++i) {
    temp.data_[i] = data_[i] + v2.data_[i];
  }
  return temp;
}

void fd_type::operator*=(double s) {
  for (size_t i = 0; i < 3 * rows_; ++i) {
    data_[i] *= s;
  }
}

void fd_type::random(int seed, double lower, double upper) {
  std::uniform_real_distribution<double> unif(lower, upper);
  std::default_random_engine engine(seed);
  for (size_t i = 0; i < rows(); ++i) {
    (*this)(i, 0) = unif(engine);
    (*this)(i, 1) = unif(engine);
    (*this)(i, 2) = unif(engine);
  }
}

bool fd_type::isApprox(const fd_type& v1, double EPS) const {
  for (size_t i = 0; i < 3 * rows_; ++i) {
    if (std::abs(data_[i] - v1.data_[i]) > EPS) return false;
  }
  return true;
}

bool fd_type::operator==(const fd_type& v1) const {
  double EPS = std::numeric_limits<double>::epsilon();
  return isApprox(v1, EPS);
}

