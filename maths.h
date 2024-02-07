#ifndef MATHS_H
#define MATHS_H

#include <vector>
#include <iostream>
#include <limits>


extern "C" {
    /** the Cholesky factorization of a real symmetric
     *  positive definite matrix a.*/
    void dpotrf_(char *uplo, int *n, double *a, int *lda, int *info);

    /** Cholesky decomposition for packed matrices*/
    void dpptrf_(char *uplo, int *n, double *ap, int *info);

    /** LU decomoposition of a general matrix */
    void dgetrf_(int *m, int *n, double *a, int *lda, int *ipiv, int *info);

    /** Compute inverse of the matrix from its  LU factorisation */
    void dgetri_(int *n, double *a, int *lda, int *ipiv,
            double *work, int *lwork, int *info);

    void dgesdd_(char *jobz, int *m, int *n, double *a, int *lda,
            double *s, double *u, int *ldu, double *vt, int *ldvt,
            double *work, int *lwork, int *iwork, int *info);

    void dgemv_(const char *trans, const int *m, const int *n,
            const double *alpha, const double *a, const int *lda, const double *x,
            const int *incx, const double *beta, double *y, const int *incy);

    void dgelsd_ (int *m, int* n, int* nrhs, double *a, int* lda,
            double *b, int* ldb, double *s, double* rcond, int* rank,
            double *work, int* lwork, int* iwork, int* info);

    void dgemm_(char *transa, char *transb, int *m, int *n,
            int *k, double *alpha, double *a, int *lda, double *b, int *ldb,
            double *beta, double *c, int *ldc);

}


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
        inline void setZero() { v[0]=0; v[1]=0; v[2]=0; }

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
        std::vector<double> data_;
        aed_type2() {}
        aed_type2(size_t n):
            data_(n)
    {}
        template <typename T>
            aed_type2(T &v)
            {
                data_.resize(v.size());
                for (size_t i=0; i<v.size(); ++i) {
                    data_[i]=v[i];
                }
            }
        template <typename T>
            aed_type2(double* arr, const T size)
            {
                data_.resize(size);
                for (size_t i=0; i<size; ++i) {
                    data_[i]=arr[i];
                }
            }
        inline double &operator[](size_t i)
        {
            return data_[i];
        }
        inline double &operator()(size_t i)
        {
            return data_[i];
        }
        inline const double &operator[](size_t i) const
        {
            return data_[i];
        }
        inline const double &operator()(size_t i) const
        {
            return data_[i];
        }
        inline aed_type2 transpose() const
        {
            return *this;
        }
        inline aed_type2 operator-() const
        {
            aed_type2 temp(*this);
            for (size_t i=0;i<temp.size();++i) {
                temp.data_[i]*=-1;
            }
            return temp;;
        }
        template <typename T>
            inline aed_type2 &operator+=(const T &v1)
            {
                for (size_t i=0;i<data_.size();++i) {
                    data_[i]+=v1[i];
                }
                return *this;
            }
        template <typename T>
            inline aed_type2 &operator-=(const T &v1)
            {
                for (size_t i=0;i<data_.size();++i) {
                    data_[i]-=v1[i];
                }
                return *this;
            }
        inline aed_type2 &operator*=(const double s)
        {
            for (size_t i=0;i<data_.size();++i) {
                data_[i]*=s;
            }
            return *this;
        }
        inline friend aed_type2 operator-(const aed_type2 &v1, const aed_type2 &v2)
        {
            aed_type2 temp(v1);
            for (size_t i=0;i<temp.size();++i) {
                temp.data_[i]-=v2.data_[i];
            }
            return temp;
        }
        inline friend aed_type2 operator+(const aed_type2 &v1, const aed_type2 &v2)
        {
            aed_type2 temp(v1);
            for (size_t i=0;i<temp.size();++i) {
                temp.data_[i]+=v2.data_[i];
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
        inline void setZero()
        {
            for (size_t i=0;i<data_.size();++i) {
                data_[i]=0;
            }
        }

        size_t size() const
        {
            return data_.size();
        }
        size_t rows() const
        {
            return data_.size();
        }

        inline friend std::ostream &operator<<(std::ostream &os, const aed_type2 &v)
        {
            for (size_t i=0;i<v.size()-1;++i) {
                os << v[i] << "  ";
            }
            os << v[v.size()-1];
            return os;
        }
        bool isApprox(const aed_type2 &v1, double EPS) const{
            for (size_t i=0;i<data_.size();++i) {
                if (std::abs(data_[i]-v1[i])>EPS) return false;
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
            data_.resize(n);
        }
        double *data() {
            return data_.data();
        }
        const double *data() const {
            return data_.data();
        }
        aed_type2 copy(size_t nelements, size_t start=0) {
            if (nelements+start>data_.size())
                throw std::runtime_error("Out of range\n");
            //std::vector<double> new_data(data_.begin()+start, data_.begin()+(start+nelements));
            std::vector<double> new_data(nelements);
            for (size_t i=0; i<nelements; ++i) {
                new_data[i]=data_[start+i];
            }
            aed_type2 temp(new_data);
            return temp;
        }
        //inline void operator=(const aed_type2 &v1)
        //{
        //    this->data_ = v1.data_;
        //}
};
class aeds_type2 {
    // column-wise
    public:
        std::vector<aed_type2> data_;
        aeds_type2() {}
        aeds_type2(size_t dim, size_t natoms):
            data_(natoms, aed_type2(dim))
    {}
        inline double &operator()(size_t i, size_t j)
        {
            return data_[i][j];
        }
        inline aed_type2 &operator[](size_t i)
        {
            return data_[i];
        }
        size_t cols() const
        {
            return data_.size();
        }
        size_t rows() const
        {
            return data_[0].size();
        }
        size_t size() const
        {
            return data_.size();
        }
        inline aed_type2 &col(size_t i)
        {
            return data_[i];
        }
        inline const aed_type2 &col(size_t i) const
        {
            return data_[i];
        }
        inline void resize(size_t dim, size_t natoms)
        {
            data_.resize(natoms);
            for (size_t i = 0; i < natoms; ++i)
                data_[i].resize(dim);
        }
        inline void setZero()
        {
            for (auto &a:data_) a.setZero();
        }
        inline friend std::ostream &operator<<(std::ostream &os, const aeds_type2 &v)
        {
            for (const aed_type2 &a:v.data_) os << a << std::endl;
            return os;
        }
};
class fd_type {
    // column-wise
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

class Matrix {
    // column-major order
    using vec = std::vector<double>;
    size_t rows_;
    size_t cols_;
    vec m_;

    public:
        Matrix() {};
        Matrix(double *d, size_t r, size_t c):
            rows_(r),
            cols_(c),
            m_(d,d+r*c)
    {}
        Matrix(vec &d, size_t r, size_t c):
            rows_(r),
            cols_(c),
            m_(d)
    {}
        Matrix(size_t r, size_t c):
            rows_(r),
            cols_(c),
            m_(r*c)
    {}

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
        inline double *data() {
            return m_.data();
        }
        inline bool exists(size_t row, size_t col) const
        {
            return (row < rows_ && col < cols_);
        }
        //inline t_type row(size_t row_) {
        //    /* rows = 5
        //     * cols = 4
        //     *
        //     * ex.
        //     * r=0 return m_(0,0+cols)
        //     * r=1 return m_(row_*cols, row_*cols+cols)
        //     */
        //    size_t start=row_*cols;
        //    size_t stop=cols*(1+row_);
        //    vec temp(m_[start,stop]);
        //    return aed_type2(temp);
        //}
        inline void resize(size_t r, size_t c)
        {
            m_.resize(r*c);
            rows_= r;
            cols_= c;
        }
        inline void setZero() {
            for (size_t i=0;i<m_.size();++i) {
                m_[i]=0;
            }
        }
        inline size_t size() const {
            // Number of coefficients in the matrix
            return m_.size();
        }
        //inline t_pred operator*(const aed_type2) {
        //}
        inline aed_type2 diagonal() const {
            aed_type2 temp(rows());
            for (size_t i=0; i<rows(); ++i) {
                temp(i)=(*this)(i,i);
            }
            return temp;
        }
        inline friend std::ostream &operator<<(std::ostream &os, const Matrix &m)
        {
            for (size_t r=0;r<m.rows();++r) {
                for (size_t c=0;c<m.cols()-1;++c) {
                    os << m(r,c) << "  ";
                }
                os << m(r,m.cols()-1) << std::endl;
            }
            return os;
        }
        inline aed_type2 col(size_t c) {
            size_t start = rows()*c;
            size_t stop = rows()*c+rows();  // not included
            std::vector<double> new_data(rows());
            for (int i=0; i<rows(); ++i) {
                new_data[i]=m_[i+start];
            }
            aed_type2 temp(new_data.data(), rows());
            return temp;
        }
        template <typename T>
            inline friend aed_type2 operator*(const Matrix &m, const T &v)
            {
                // naive matrix vector product
                //m is M x N matrix and v is N x 1 column vector
                aed_type2 temp(m.rows());
                temp.setZero();
                for (size_t i=0; i<m.rows(); i++){
                    for (size_t j=0; j<m.cols(); j++) {
                        temp(i) += m(i,j)*v(j);
                    }
                }
                return temp;
            }
};
#endif
