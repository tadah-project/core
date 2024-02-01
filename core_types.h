#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <vector>
#include <iostream>


class Vec3d {
    public:
        double v[3];
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
        inline friend Vec3d operator-(const Vec3d &v1, const Vec3d &v2)
        {
            return Vec3d(v1[0]-v2[0], v1[1]-v2[1], v1[2]-v2[2]);
        }
        inline friend Vec3d operator+(const Vec3d &v1, const Vec3d &v2)
        {
            return Vec3d(v1[0]+v2[0], v1[1]+v2[1], v1[2]+v2[2]);
        }
        inline friend double operator*(const Vec3d &v1, const Vec3d &v2)
        {
            return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
        }
        template <typename T>
        inline friend Vec3d operator*(const double s, const T &v1)
        {
            return Vec3d(v1[0]*s , v1[1]*s , v1[2]*s);
        }
        template <typename T>
        inline friend Vec3d operator*(const T &v1, const double s)
        {
            return s*v1;
        }
        void setZero() { v[0]=0; v[1]=0; v[2]=0; }

        inline friend std::ostream &operator<<(std::ostream &os, const Vec3d &v)
        {
            os << '(' << v[0] << ',' << v[1] << ',' << v[2] << ')';
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
};

class Mat6R3C {
    // Row major order
    public:
        int rows=6;
        int cols=3;
        double m[18];
        inline double &operator()(const size_t i, const size_t j)
        {
            return m[i*cols + j];
        }
        inline Vec3d row(const size_t r) {
            return Vec3d(m[r*cols],m[r*cols+1],m[r*cols+2]);
        }
};



class aed_type2 {
    // The aim is to make it a column vector
    public:
        std::vector<double> data;
        aed_type2() {}
        aed_type2(size_t n):
            data(n)
    {}
        template <typename T>
            aed_type2(T &v)
            {
                data.resize(v.size());
                for (size_t i=0; i<v.size(); ++i) {
                    data[i]=v[i];
                }
            }
        inline double &operator[](size_t i)
        {
            return data[i];
        }
        inline double &operator()(size_t i)
        {
            return data[i];
        }
        inline const double &operator[](size_t i) const
        {
            return data[i];
        }
        inline const double &operator()(size_t i) const
        {
            return data[i];
        }
        inline aed_type2 transpose() const
        {
            return *this;
        }
        inline aed_type2 operator-() const
        {
            aed_type2 temp(*this);
            for (size_t i=0;i<temp.size();++i) {
                temp.data[i]*=-1;
            }
            return temp;;
        }
        template <typename T>
            inline aed_type2 &operator+=(const T &v1)
            {
                for (size_t i=0;i<data.size();++i) {
                    data[i]+=v1[i];
                }
                return *this;
            }
        template <typename T>
            inline aed_type2 &operator-=(const T &v1)
            {
                for (size_t i=0;i<data.size();++i) {
                    data[i]-=v1[i];
                }
                return *this;
            }
        inline aed_type2 &operator*=(const double s)
        {
            for (size_t i=0;i<data.size();++i) {
                data[i]*=s;
            }
            return *this;
        }
        inline friend aed_type2 operator-(const aed_type2 &v1, const aed_type2 &v2)
        {
            aed_type2 temp(v1);
            for (size_t i=0;i<temp.size();++i) {
                temp.data[i]-=v2.data[i];
            }
            return temp;
        }
        inline friend aed_type2 operator+(const aed_type2 &v1, const aed_type2 &v2)
        {
            aed_type2 temp(v1);
            for (size_t i=0;i<temp.size();++i) {
                temp.data[i]+=v2.data[i];
            }
            return temp;
        }
        template <typename T, typename D>
            inline friend double operator*(const T &v1, const D &v2)
            {
                double temp=0;
                for (size_t i=0;i<v1.size();++i) {
                    temp+=v1(i)*v2(i);
                }
                return temp;
            }
        template <typename T>
            inline friend aed_type2 operator*(const double s, const T &v1)
            {
                aed_type2 temp(v1);
                for (size_t i=0;i<temp.size();++i) {
                    temp[i]*=s;
                }
                return temp;
            }
        template <typename T>
            inline friend aed_type2 operator*(const T &v1, const double s)
            {
                return s*v1;
            }
        void setZero()
        {
            for (size_t i=0;i<data.size();++i) {
                data[i]=0;
            }
        }

        size_t size() const
        {
            return data.size();
        }
        size_t rows() const
        {
            return data.size();
        }

        inline friend std::ostream &operator<<(std::ostream &os, const aed_type2 &v)
        {
            os << '(';
            for (size_t i=0;i<v.size()-1;++i) {
                os << v[i] << ',';
            }
            os << v[v.size()-1] << ')';
            return os;
        }
        bool isApprox(const aed_type2 &v1, double EPS) const{
            for (size_t i=0;i<data.size();++i) {
                if (std::abs(data[i]-v1[i])>EPS) return false;
            }
            return true;
        }
        inline bool operator==(const aed_type2 &v1) const
        {
            double EPS = std::numeric_limits<double>::epsilon();
            return this->isApprox(v1, EPS);
        }
        inline bool operator!=(const aed_type2 &v1) const
        {
            return !(*this==v1);
        }
        inline void resize(const size_t n) {
            data.resize(n);
        }
};
class aeds_type2{
    // column-wise
    public:
        std::vector<aed_type2> data;
        aeds_type2() {}
        aeds_type2(size_t dim, size_t natoms):
            data(natoms, aed_type2(dim))
    {}
        inline double &operator()(size_t i, size_t j)
        {
            return data[i][j];
        }
        inline aed_type2 &operator[](size_t i)
        {
            return data[i];
        }
        size_t cols() const
        {
            return data.size();
        }
        size_t rows() const
        {
            return data[0].size();
        }
        size_t size() const
        {
            return data.size();
        }
        inline aed_type2 &col(size_t i)
        {
            return data[i];
        }
        inline const aed_type2 &col(size_t i) const
        {
            return data[i];
        }
        inline void resize(size_t dim, size_t natoms)
        {
            data.resize(natoms,dim);
        }
        inline void setZero()
        {
            for (auto &a:data) a.setZero();
        }
        inline friend std::ostream &operator<<(std::ostream &os, const aeds_type2 &v)
        {
            for (const aed_type2 &a:v.data) os << a << std::endl;
            return os;
        }
};
class fd_type {
    // column-wise????????
    // data alignment here is just wrong
    public:
        std::vector<aed_type2> data;
        fd_type() {}
        fd_type(size_t dim):
            data(3,aed_type2(dim))
    {}
        inline double &operator()(size_t i, size_t k)
        {
            // to match old implementation
            return data[k][i];
        }
        inline const double &operator()(size_t i, size_t k) const
        {
            // to match old implementation
            return data[k][i];
        }
        inline aed_type2 &operator[](size_t i)
        {
            return data[i];
        }
        size_t cols() const
        {
            return data.size();
        }
        size_t rows() const
        {
            return data[0].size();
        }
        //size_t size() const
        //{
        //    return data.size();
        //}
        inline aed_type2 &col(size_t k)
        {
            return data[k];
        }
        inline const aed_type2 &col(size_t k) const
        {
            return data[k];
        }
        inline void resize(size_t dim, size_t natoms)
        {
            data.resize(natoms,dim);
        }
        inline void setZero()
        {
            for (auto &a:data) a.setZero();
            //data=std::vector<aed_type2>(data.size(), aed_type2(data[0].size()));
        }
        inline friend std::ostream &operator<<(std::ostream &os, const fd_type &v)
        {
            for (const aed_type2 &a:v.data) os << a << std::endl;
            return os;
        }
        inline fd_type transpose() const {
            return *this;
        }
        template <typename T>
            inline friend Vec3d operator*(const fd_type &v1, const T &v2)
            {
                Vec3d temp;
                temp.setZero();
                for (size_t k=0; k<3; ++k) {
                    for (size_t i=0;i<v2.size();++i) {
                        temp[k]+=v1(k,i)*v2[i];
                    }
                }
                return temp;
            }
        inline fd_type operator-() const
        {
            fd_type temp(*this);
            for (size_t k=0; k<3; ++k) {
                for (size_t i=0;i<temp.col(k).size();++i) {
                    temp(i,k)*=-1;
                }
            }
            return temp;;
        }
};

typedef Vec3d force_type;
typedef aed_type2 t_type;
typedef aed_type2 rho_type;
typedef aeds_type2 rhos_type;
typedef std::vector<double> v_type;

////////////////////////////////////////////////////////////////////////////////
// Define dummy mat class to satisfy Sigma in core model classes. //////////////
////////////////////////////////////////////////////////////////////////////////
#ifdef M_MD_BASE_H
//#define TEMP_MAT_SIGMA
class mat {
    public:
        t_type diagonal() {
            return t_type();
        }
        t_type diagonal() const {
            return t_type();
        }

};
#endif
/////////////////////////////////////////

#endif
