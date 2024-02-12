#include "maths.h"
#include "lapack.h"


// Some usefull operations on vectors and matrices
aed_type2 T_dgemv(Matrix &mat, aed_type2 &vec, char trans) {
    int m = mat.rows();
    int n = mat.cols();
    double alpha = 1.0;
    int lda = m;
    double *a = &mat.data()[0];
    double *x = &vec.data()[0];
    int incx = 1;
    double beta = 0.0;

    aed_type2 res(trans=='N' ? mat.rows() : mat.cols());
    double *y = &res.data()[0];
    int incy = 1;

    dgemv_(&trans, &m, &n, &alpha, a, &lda, x, &incx, &beta, y, &incy);
    return res;
}
aed_type2 T_MDMT_diag(const Matrix &M, const Matrix &D) {
    /** Computes diagonal entries of (M D M^T) where M is m by n
     * and D is n by n diagonal matrix
     *
     * [m,n][n,n][n,m]=[m,m]
     *
     * */

    aed_type2 res(M.rows());
    res.setZero();
    for (size_t i=0; i<M.rows(); ++i) {
        for (size_t j=0; j<M.cols(); ++j) {
            res[i] += D(j,j)*M(i,j)*M(i,j);
        }
    }

    return res;
}
double norm_X(Matrix &A, char norm) {
    double *a = A.ptr();
    int m=(int)A.rows();
    int n=(int)A.cols();
    double *work;
    if (norm=='I') work = new double[n];

    double res = dlange_(&norm, &m, &n, a, &m, work);

    if (norm=='I') delete [] work;

    return 1.0/res;
}
aed_type2 solve_posv(Matrix &A, aed_type2 /*&*/T, char uplo) {
    double *a = A.ptr();
    double *b = T.ptr();
    int n = A.rows();   // A is square
    int nrhs = 1;
    int info;

    dposv_(&uplo, &n, &nrhs, a, &n, b, &n, &info);

    return T;
}
double condition_number(Matrix &A, char uplo) {
    double *a = A.ptr();
    int n = A.rows();   // A is square
    double anorm =norm_X(A,'1');
    double rcond;
    int info;
    double *work = new double[3*n];
    int  *iwork = new int[n];

    dpocon_(&uplo, &n, a, &n, &anorm, &rcond, work, iwork, &info);

    delete [] work;
    delete [] iwork;

    return 1/rcond;
}


// aed_type2 class

aed_type2::aed_type2(double* arr, size_t size, size_t stride)
 {
     stride_ = stride;
     size_   = size;
     data_   = arr;
 }

aed_type2::aed_type2(double* arr, size_t size, bool copy, size_t stride)
{
    stride_= stride;
    size_ = size;
    if (copy) {
        data_ = (double*)malloc(stride_*size_*sizeof(data_));
        std::memcpy(data_, arr, stride_*size_*sizeof(data_));
        allocated_ = true;
    }
    else {
        data_ = arr;
    }
}
