#ifndef TADAH_CORE_VEC3D_H
#define TADAH_CORE_VEC3D_H

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>

namespace tadah {
namespace core {

class Vec3d {
public:
  double v[3] = { };
  Vec3d() {}

  template <typename T>
  Vec3d(T &v1)
  {
    v[0]=v1[0]; v[1]=v1[1]; v[2]=v1[2];
  }
  Vec3d(const double x, const double y, const double z)
  {
    v[0]=x; v[1]=y; v[2]=z;
  }

  inline Vec3d operator-() const
  {
    return Vec3d(-v[0],-v[1],-v[2]);
  }
  inline double &operator[](size_t i)
  {
    return v[i];
  }
  inline double &operator()(size_t i)
  {
    return v[i];
  }
  inline const double &operator[](size_t i) const
  {
    return v[i];
  }
  inline const double &operator()(size_t i) const
  {
    return v[i];
  }
  template <typename T>
  inline Vec3d &operator+=(const T &v1)
  {
    v[0]+=v1[0];
    v[1]+=v1[1];
    v[2]+=v1[2];
    return *this;
  }
  template <typename T>
  inline Vec3d &operator-=(const T &v1)
  {
    v[0]-=v1[0];
    v[1]-=v1[1];
    v[2]-=v1[2];
    return *this;
  }
  inline Vec3d &operator*=(const double s)
  {
    v[0]*=s;
    v[1]*=s;
    v[2]*=s;
    return *this;
  }
  inline Vec3d &operator/=(const double s)
  {
    v[0]/=s;
    v[1]/=s;
    v[2]/=s;
    return *this;
  }
  inline Vec3d operator-(const Vec3d &v2) const
  {
    return Vec3d((*this)[0]-v2[0], (*this)[1]-v2[1], (*this)[2]-v2[2]);
  }
  inline Vec3d operator+(const Vec3d &v2) const
  {
    return Vec3d((*this)[0]+v2[0], (*this)[1]+v2[1], (*this)[2]+v2[2]);
  }
  inline double operator*(const Vec3d &v2) const
  {
    return (*this)[0]*v2[0] + (*this)[1]*v2[1] + (*this)[2]*v2[2];
  }
  inline double dot(const Vec3d &other) const {
    return v[0]*other[0] + v[1]*other[1] + v[2]*other[2];
  }
  template <typename T>
  inline Vec3d operator*(const T s) const
  {
    return Vec3d((*this)[0]*s, (*this)[1]*s, (*this)[2]*s);
  }
  template <typename T>
  inline Vec3d operator/(const T s) const
  {
    return Vec3d((*this)[0]/s, (*this)[1]/s, (*this)[2]/s);
  }
  template <typename T>
  inline friend Vec3d operator*(const double s, const T &v)
  {
    return Vec3d(v[0]*s, v[1]*s, v[2]*s);
  }
  template <typename T>
  inline friend Vec3d operator/(const double s, const T &v)
  {
    return Vec3d(s/v[0], s/v[1], s/v[2]);
  }
  inline void set_zero() { v[0]=0; v[1]=0; v[2]=0; }

  inline friend std::ostream &operator<<(std::ostream &os, const Vec3d &v)
  {
    os << v[0] << "  " << v[1] << "  " << v[2];
    return os;
  }
  bool isApprox(const Vec3d &v1, double EPS) const{
    if (std::abs(v[0]-v1[0])>EPS) return false;
    if (std::abs(v[1]-v1[1])>EPS) return false;
    if (std::abs(v[2]-v1[2])>EPS) return false;
    return true;
  }
  inline bool operator==(const Vec3d &v1) const
  {
    double EPS = std::numeric_limits<double>::epsilon();
    return this->isApprox(v1, EPS);
  }
  inline bool operator!=(const Vec3d &v1) const
  {
    return !(*this==v1);
  }
  template <typename T>
  inline Vec3d &operator=(const T &v1)
  {
    v[0] = v1[0];
    v[1] = v1[1];
    v[2] = v1[2];
    return *this;
  }
  inline void operator()(const double x, const double y, const double z)
  {
    v[0]=x;
    v[1]=y;
    v[2]=z;
  }
  inline Vec3d cross(const Vec3d &v) const
  {
    Vec3d temp;
    temp[0] = (*this)[1] * v[2] - (*this)[2] * v[1];
    temp[1] = -((*this)[0] * v[2] - (*this)[2] * v[0]);
    temp[2] = (*this)[0] * v[1] - (*this)[1] * v[0];
    return temp;
  }

  inline double norm() const {
    return std::sqrt((*this)*(*this));
  }
  inline double squaredNorm() const {
    return (*this)*(*this);
  }
};
} // namespace core
} // namespace tadah
#endif
