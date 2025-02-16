#ifndef LAPACK_H
#define LAPACK_H

#include <tadah/core/maths.h>

extern "C" {
    /**
     * @brief LAPACK function for solving least squares problems using QR or LQ factorization.
     *
     * dgels solves overdetermined or underdetermined real linear systems involving an M-by-N
     * matrix A, or its transpose, using a QR or LQ factorization of A. It is assumed that A
     * has full rank.
     *
     * @param trans Specifies the transposition operation:
     *              - 'N': No transpose, solve A * X = B
     *              - 'T': Transpose, solve A^T * X = B
     * @param m The number of rows of the matrix A. m >= 0.
     * @param n The number of columns of the matrix A. n >= 0.
     * @param nrhs The number of right-hand sides, i.e., the number of columns of the matrix B. nrhs >= 0.
     * @param a On entry, the M-by-N matrix A. On exit, if m >= n, A is overwritten by details of its QR factorization.
     * @param lda The leading dimension of the array A. lda >= max(1, m).
     * @param b On entry, the matrix B of right-hand side vectors, stored columnwise; dimensions (ldb, nrhs). On exit, if no errors, B contains the solution vectors X.
     * @param ldb The leading dimension of the array B. ldb >= max(1, max(m, n)).
     * @param work Workspace array of dimension at least lwork.
     * @param lwork The dimension of the array work. lwork >= max(1, min(m, n) + max(min(m, n), nrhs)).
     * @param info
     *              - = 0: successful exit.
     *              - < 0: if info = -i, the i-th argument had an illegal value.
     */
    void dgels_(const char* trans, const int* m, const int* n, const int* nrhs,
                double* a, const int* lda, double* b, const int* ldb,
                double* work, const int* lwork, int* info);

    /**
     * @brief LAPACK function for solving least squares problems using the singular value decomposition (SVD).
     *
     * dgelsd computes the minimum-norm solution to a real linear least squares problem:
     *     minimize || B - A * X ||_2
     * using the singular value decomposition (SVD) of A. A is an M-by-N matrix which may be rank-deficient.
     *
     * @param m The number of rows of the matrix A. m >= 0.
     * @param n The number of columns of the matrix A. n >= 0.
     * @param nrhs The number of right-hand sides, i.e., the number of columns of B. nrhs >= 0.
     * @param a On entry, the M-by-N matrix A. On exit, the contents are destroyed.
     * @param lda The leading dimension of A. lda >= max(1, m).
     * @param b On entry, the M-by-NRHS right-hand side matrix B. On exit, the N-by-NRHS solution matrix X.
     * @param ldb The leading dimension of B. ldb >= max(1, max(m, n)).
     * @param s The singular values of A in decreasing order. Dimension at least min(m, n).
     * @param rcond The reciprocal of the condition number threshold. Singular values smaller than rcond * max(s) are treated as zero.
     * @param rank The effective rank of A, i.e., the number of singular values which are greater than rcond * max(s).
     * @param work Workspace array of dimension lwork.
     * @param lwork The dimension of the array work. lwork >= min(m, n)*10 + 2*min(m, n)*NRHS.
     * @param iwork Integer workspace array of dimension at least 3 * min(m, n) * nlvl + 11 * min(m, n), where nlvl = max(0, int(log_2(min(m, n)/(smlsiz+1))) + 1) and smlsiz is a constant defined in LAPACK.
     * @param info
     *              - = 0: successful exit.
     *              - < 0: if info = -i, the i-th argument had an illegal value.
     *              - > 0: the algorithm failed to converge; info > 0 specifies how many superdiagonals of an intermediate bidiagonal form did not converge to zero.
     */
    void dgelsd_(const int* m, const int* n, const int* nrhs, double* a, const int* lda,
                 double* b, const int* ldb, double* s, const double* rcond, int* rank,
                 double* work, const int* lwork, int* iwork, int* info);
    /** the Cholesky factorization of a real symmetric
     *  positive definite matrix a.*/
    void dpotrf_(char *uplo, int *n, double *a, int *lda, int *info);

    /** Compute inverse using the Cholesky factorisation from dpotrf */
    void dpotri_(char *uplo, int *n, double *a, int *lda, int *info);

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

    void dgemm_(char *transa, char *transb, int *m, int *n,
            int *k, double *alpha, double *a, int *lda, double *b, int *ldb,
            double *beta, double *c, int *ldc);

    /** Balance a general real matrix a */
    void dgebal_(char *job, int *n, double *a, int *lda, int *ilo, int *ihi,
            double *scale, int *info);

    void dposv_(char *uplo, int *n, int *nrhs, double *a, int *lda, double *b,
            int *ldb, int *info);

    void dpocon_(char *uplo, int *n, double *a, int *lda, double *anorm,
            double *rcond, double *work, int *iwork, int *info);

    double dlange_(char *norm, int *m, int *n, double *a, int *lda, double *work);

}


// Some usefull operations on vectors and matrices
aed_type T_dgemv(Matrix &mat, aed_type &vec, char trans='N');
aed_type T_MDMT_diag(const Matrix &M, const Matrix &D);
double norm_X(Matrix &A, char norm='1');
aed_type solve_posv(Matrix &A, aed_type /*&*/T, char uplo='U');
double condition_number(Matrix &A, char uplo='U');

#endif
