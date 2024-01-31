#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <vector>
#include "../LIBS/Eigen/Core"

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
            //v[0] = -v[0];
            //v[1] = -v[1];
            //v[2] = -v[2];
            //return *this;
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
            if (abs(v[0]-v1[0])>EPS) return false;
            if (abs(v[1]-v1[1])>EPS) return false;
            if (abs(v[2]-v1[2])>EPS) return false;
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

//typedef Vec3d vec3d;
typedef Eigen::VectorXd vec;
typedef Eigen::Ref<const Eigen::VectorXd> rvec;
typedef Eigen::MatrixXd mat;

typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> phi_type;
typedef Eigen::Matrix<double,Eigen::Dynamic,1> t_type;

typedef Eigen::Matrix3d stress_type;
typedef Vec3d force_type;

typedef std::vector<double> v_type;


/** Atomic Energy Descriptor (AED) type
 *
 * Dimensions:
 * [descriptor size]
 *
 */
typedef Eigen::VectorXd aed_type;
typedef Eigen::Ref<Eigen::VectorXd> aed_rtype;
typedef Eigen::Ref<const Eigen::VectorXd> aed_rctype;
typedef Eigen::MatrixXd aeds_type;  // [s,natoms]

/** Force Descriptor (FD) type
 *
 * Dimensions:
 * [descriptor size, 3]
 *
 */
typedef Eigen::Matrix<double, Eigen::Dynamic, 3> fd_type;

/** Stress Descriptor (SD) type
 *
 * Dimensions:
 * [descriptor size,6]
 *
 */
typedef Eigen::Matrix<double, Eigen::Dynamic, 6> sd_type;   // sigma_ij order: 11,12,13,22,23,33

/** Density array to be used by many-body descriptors... TODO
 *
 * Dimensions:
 * [atom i][n]
 *
 * where n is specific to given descriptor
 *
 */
typedef Eigen::Ref<Eigen::VectorXd> rho_rtype;
typedef Eigen::MatrixXd rhos_type;

#endif
