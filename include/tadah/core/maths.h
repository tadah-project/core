#ifndef MATHS_H
#define MATHS_H

#include <tadah/core/aed_type.h>
#include <tadah/core/vec3d.h>

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>
#include <limits>
#include <random>
#include <iostream>
#include <random>
#include <chrono>
#include <cstring> // for std::memcpy

#include <array>

template <typename D>
class Matrix_Base {
  // column-major order

public:
  size_t rows_;
  size_t cols_;
  D m_;

  inline Matrix_Base<D>(): rows_(0), cols_(0) {};
  inline Matrix_Base<D>(D data, size_t rows, size_t cols)
  : rows_(rows), cols_(cols), m_(data) {}

  inline void swap_columns(size_t col1, size_t col2) {
    // double *matrix = this->ptr();
    for (size_t i = 0; i < rows(); ++i) {
      std::swap(m_[i + col1 * rows()], m_[i + col2 * rows()]);
    }
  }

  inline double& operator()(size_t i, size_t j)
  {
    return m_[i + rows_*j];
  }

  inline double operator()(size_t i, size_t j) const
  {
    return m_[i + rows_*j];
  }
  inline double& operator[](size_t i)
  {
    return m_[i];
  }

  inline double operator[](size_t i) const
  {
    return m_[i];
  }

  inline size_t rows() const {
    return rows_;
  }
  inline size_t cols() const {
    return cols_;
  }
  inline bool exists(size_t row, size_t col) const
  {
    return (row < rows_ && col < cols_);
  }
  /** Provide a view of the r row of the matrix */
  inline aed_type row(size_t r) {
    size_t stride = rows();
    aed_type temp(&m_[r], cols(), stride);
    return temp;
  }
  /** Provide a view of the c column of the matrix */
  inline aed_type col(size_t c) {
    size_t start = rows()*c;
    aed_type temp(&m_[start], rows());
    return temp;
  }
  /** Provide a copy of the r row of the matrix */
  inline aed_type col(size_t c) const {
    size_t start = rows()*c;
    aed_type temp(&m_[start], rows());
    return temp;
  }
  /** Provide a copy of the c column of the matrix */
  inline aed_type row(size_t r) const {
    size_t stride = rows();
    aed_type temp(&m_[r], cols(), stride);
    return temp;
  }
  inline void set_zero() {
    for (size_t i=0;i<m_.size();++i) {
      m_[i]=0;
    }
  }
  inline size_t size() const {
    // Number of coefficients in the matrix
    return m_.size();
  }
  //inline t_pred operator*(const aed_type) {
  //}
  inline aed_type diagonal() const {
    aed_type temp(rows());
    for (size_t i=0; i<rows(); ++i) {
      temp(i)=(*this)(i,i);
    }
    return temp;
  }
  inline friend std::ostream &operator<<(std::ostream &os,
                                         const Matrix_Base<D> &m)
  {
    for (size_t r=0;r<m.rows();++r) {
      for (size_t c=0;c<m.cols()-1;++c) {
        os << m(r,c) << "  ";
      }
      os << m(r,m.cols()-1) << std::endl;
    }
    return os;
  }
  template <typename T>
  inline aed_type operator*(const T &v) const
  {
    // naive matrix vector product
    //m is M x N matrix and v is N x 1 column vector
    aed_type temp((*this).rows());
    temp.set_zero();
    for (size_t i=0; i<(*this).rows(); i++){
      for (size_t j=0; j<(*this).cols(); j++) {
        temp(i) += (*this)(i,j)*v(j);
      }
    }
    return temp;
  }
  inline void set_rows(const size_t r)
  {
    rows_=r;
  }
  inline void set_cols(const size_t c)
  {
    cols_=c;
  }
  // Maths
  inline Matrix_Base<D> operator-() const
  {
    Matrix_Base<D> temp(*this);
    for (size_t i=0;i<temp.size();++i) {
      temp[i]*=-1;
    }
    return temp;;
  }
  template <typename T>
  inline Matrix_Base<D> &operator+=(const T &m)
  {
    for (size_t i=0;i<m_.size();++i) {
      m_[i]+=m[i];
    }
    return *this;
  }
  template <typename T>
  inline Matrix_Base<D> &operator-=(const T &m)
  {
    for (size_t i=0;i<m_.size();++i) {
      m_[i]-=m[i];
    }
    return *this;
  }
  inline double trace() const {
    double temp=0;
    for (size_t i=0; i<rows(); ++i)
      temp+= (*this)(i,i);
    return temp;
  }
  bool isApprox(const Matrix_Base<D> &m, double EPS) const {
    for (size_t i=0;i<size();++i) {
      if (std::abs((*this)[i]-m[i])>EPS) return false;
    }
    return true;
  }
  inline bool operator==(const aed_type &v1) const
  {
    double EPS = std::numeric_limits<double>::epsilon();
    return this->isApprox(v1, EPS);
  }
  //  template <typename... T>
  //      friend Matrix_Base<D> operator<<(Matrix_Base<D> &mat,T... list)
  //      {
  //          size_t i=0;
  //          for( const auto elem : {list...} )
  //          {
  //              mat.data()[i++]=elem ;
  //          }
  //          return mat;
  //      }
  template <typename... T>
  void load(T... list)
  {
    size_t i=0;
    for( const auto elem : {list...} )
    {
      m_[rows()*(i%cols()) + i/cols()]=elem ;
      i++;
    }
  }
  void random(
    int seed=std::chrono::system_clock::now().time_since_epoch().count(),
    double lower=-1e4, double upper=1e4
  ) {
    std::uniform_real_distribution<double> unif(lower,upper);
    std::default_random_engine engine(seed);
    for (size_t i=0; i<size(); ++i)
      (*this)[i] = unif(engine);
  }
  inline double *data() {
    return &(*this)[0];
  }
  const double *data() const {
    return &(*this)[0];
  }
  /** Return the pointer to the first element of the data array */
  inline double *ptr() {
    return &(*this)[0];
  }
  /** Return the pointer to the c column */
  inline double *ptr(size_t c) {
    return ptr() + c*rows();
  }
  /** Return the pointer to the (r,c) */
  inline double *ptr(size_t r, size_t c) {
    return ptr() + c*rows() + r;
  }
};
template<typename T=std::vector<double>>
class Matrix_T: public Matrix_Base<T> {
public:
  Matrix_T() {};
  Matrix_T(T &d, size_t r, size_t c):
    Matrix_Base<T>(d,r,c)
  {}
  Matrix_T(double *d, size_t r, size_t c):
    Matrix_Base<T>(T(d,d+r*c),r,c)
  {};
  Matrix_T(size_t r, size_t c):
    Matrix_Base<T>(T(r*c),r,c)
  {}
  Matrix_T(int r, int c, std::initializer_list<double> values):
    Matrix_Base<T>(values,r,c)
  {}
  inline void resize(size_t r, size_t c)
  {
    Matrix_Base<T>::m_.resize(r*c);
    Matrix_Base<T>::rows_= r;
    Matrix_Base<T>::cols_= c;
  }
};
template <typename D>
Matrix_T<D> transpose(const Matrix_T<D>& mat) {
  Matrix_T<D> result(mat.cols(), mat.rows());
  for (size_t i = 0; i < mat.rows(); ++i) {
    for (size_t j = 0; j < mat.cols(); ++j) {
      result(j, i) = mat(i, j);
    }
  }
  return result;
}
typedef Matrix_T<> Matrix;

class MatrixView: public Matrix_Base<double*> {
public:
  inline MatrixView(double* d, size_t rows, size_t cols):
    Matrix_Base<double*>(d, rows, cols) {}

  // Full view of the matrix
  template <typename D>
  inline MatrixView(Matrix_Base<D>& other):
    Matrix_Base<double*>(other.ptr(), other.rows, other.cols)
  {}

  // View to a matrix starting at column c
  template <typename D>
  inline MatrixView(Matrix_Base<D>& other, size_t c):
    Matrix_Base<double*>(other.ptr(c), other.rows(), other.cols()-c)  {}
};


// typedef Matrix_Base<double*> MatrixView;

template<typename T=std::array<double,9>>
class Matrix3d_T: public Matrix_Base<T> {
  // class for 3x3 array
public:
  Matrix3d_T()
  {
    Matrix_Base<T>::set_rows(3);
    Matrix_Base<T>::set_cols(3);
  }
  Matrix3d_T(T &d, size_t r, size_t c):
    Matrix_Base<T>(d,r,c)
  {}
  inline Vec3d row(size_t r) const
  {
    Vec3d temp((*this)[r], (*this)[r+3], (*this)[r+6]);
    return temp;
  }
  inline Vec3d col(size_t c) const
  {
    size_t s=3*c;
    Vec3d temp((*this)[s], (*this)[s+1], (*this)[s+2]);
    return temp;
  }
  inline Matrix3d_T<T> transpose() const {
    Matrix3d_T<T> temp;
    for (size_t i=0; i<3; i++){
      for (size_t j=0; j<3; j++) {
        temp(j,i) = (*this)(i,j);
      }
    }
    return temp;
  }
  inline Vec3d operator*(const Vec3d &v) const
  {
    // naive matrix vector product
    //m is M x N matrix and v is N x 1 column vector
    Vec3d temp;
    temp.set_zero();
    for (size_t i=0; i<3; i++){
      for (size_t j=0; j<3; j++) {
        temp(i) += (*this)(i,j)*v(j);
      }
    }
    return temp;
  }
  inline Matrix3d_T<T> inverse() const
  {
    // computes the inverse of a 3x3 matrix
    double det = (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) -
      (*this)(2, 1) * (*this)(1, 2)) -
      (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) -
      (*this)(1, 2) * (*this)(2, 0)) +
      (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) -
      (*this)(1, 1) * (*this)(2, 0));

    double invdet = 1 / det;

    Matrix3d_T<T> minv;
    minv(0, 0) = ((*this)(1, 1) * (*this)(2, 2) -
      (*this)(2, 1) * (*this)(1, 2)) * invdet;
    minv(0, 1) = ((*this)(0, 2) * (*this)(2, 1) -
      (*this)(0, 1) * (*this)(2, 2)) * invdet;
    minv(0, 2) = ((*this)(0, 1) * (*this)(1, 2) -
      (*this)(0, 2) * (*this)(1, 1)) * invdet;
    minv(1, 0) = ((*this)(1, 2) * (*this)(2, 0) -
      (*this)(1, 0) * (*this)(2, 2)) * invdet;
    minv(1, 1) = ((*this)(0, 0) * (*this)(2, 2) -
      (*this)(0, 2) * (*this)(2, 0)) * invdet;
    minv(1, 2) = ((*this)(1, 0) * (*this)(0, 2) -
      (*this)(0, 0) * (*this)(1, 2)) * invdet;
    minv(2, 0) = ((*this)(1, 0) * (*this)(2, 1) -
      (*this)(2, 0) * (*this)(1, 1)) * invdet;
    minv(2, 1) = ((*this)(2, 0) * (*this)(0, 1) -
      (*this)(0, 0) * (*this)(2, 1)) * invdet;
    minv(2, 2) = ((*this)(0, 0) * (*this)(1, 1) -
      (*this)(1, 0) * (*this)(0, 1)) * invdet;

    return minv;
  }
  inline Matrix3d_T<T> &operator+=(const double d)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]+=d;
    }
    return *this;
  }
  inline Matrix3d_T<T> &operator-=(const double d)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]-=d;
    }
    return *this;
  }
  inline Matrix3d_T<T> &operator+=(const int d)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]+=d;
    }
    return *this;
  }
  inline Matrix3d_T<T> &operator-=(const int d)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]-=d;
    }
    return *this;
  }

  template <typename D>
  inline Matrix3d_T<T> &operator+=(const D &m)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]+=m[i];
    }
    return *this;
  }
  template <typename D>
  inline Matrix3d_T<T> &operator-=(const D &m)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]-=m[i];
    }
    return *this;
  }
  template <typename D>
  inline Matrix3d_T<T> &operator*=(const D fac)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]*=fac;
    }
    return *this;
  }
  template <typename D>
  inline Matrix3d_T<T> &operator/=(const D fac)
  {
    for (size_t i=0;i<9;++i) {
      Matrix_Base<T>::m_[i]/=fac;
    }
    return *this;
  }
  // Compute the Frobenius norm: sqrt( sum_{i=1..9} (m_[i])^2 ).
  inline double norm() const {
    double sum = 0.0;
    for (size_t i = 0; i < 9; ++i) {
      sum += Matrix_Base<T>::m_[i] * Matrix_Base<T>::m_[i];
    }
    return std::sqrt(sum);
  }
  static Matrix3d_T<T> Identity() {
    Matrix3d_T I;
    I.set_zero();
    I(0,0) = 1.0;
    I(1,1) = 1.0;
    I(2,2) = 1.0;
    return I;
  }
  inline friend Matrix3d_T<T> operator+(const Matrix3d_T<T> &m1, const Matrix3d_T<T> &m2)
  {
    Matrix3d_T<T> temp;
    for (size_t i=0;i<9;++i) {
      temp[i] = m1[i] + m2[i];
    }
    return temp;
  }
  inline friend Matrix3d_T<T> operator-(const Matrix3d_T<T> &m1, const Matrix3d_T<T> &m2)
  {
    Matrix3d_T<T> temp;
    for (size_t i=0;i<9;++i) {
      temp[i] = m1[i] - m2[i];
    }
    return temp;
  }
  template <typename D>
  inline friend Matrix3d_T<T> operator*(const D &fac, const Matrix3d_T<T> &m1)
  {
    Matrix3d_T<T> temp;
    for (size_t i=0;i<9;++i) {
      temp[i] = m1[i] * fac;
    }
    return temp;
  }
  template <typename D>
  inline friend Matrix3d_T<T> operator/(const Matrix3d_T<T> &m1, const D &fac)
  {
    Matrix3d_T<T> temp;
    for (size_t i=0;i<9;++i) {
      temp[i] = m1[i] / fac;
    }
    return temp;
  }
};

class Mat6R3C {
public:
  static const int rows = 6;
  static const int cols = 3;
  double m[rows * cols] = {}; // Zero-initialize

  inline double &operator()(const size_t i, const size_t j) {
    return m[i * cols + j];
  }

  inline const double &operator()(const size_t i, const size_t j) const {
    return m[i * cols + j];
  }

  inline Vec3d row(const size_t r) const {
    return Vec3d(m[r * cols], m[r * cols + 1], m[r * cols + 2]);
  }
};

typedef Matrix3d_T<> Matrix3d;
#endif
