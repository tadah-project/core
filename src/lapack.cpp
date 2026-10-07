#include <tadah/core/lapack.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace tadah {
namespace core {

namespace {
std::string shape(const Matrix &A) {
  return std::to_string(A.rows()) + " x " + std::to_string(A.cols());
}
}  // namespace

// Some useful operations on vectors and matrices
aed_type T_dgemv(Matrix &mat, aed_type &vec, char trans) {
  int m = mat.rows();
  int n = mat.cols();
  double alpha = 1.0;
  int lda = m;
  double *a = &mat.data()[0];
  double *x = &vec.data()[0];
  int incx = 1;
  double beta = 0.0;

  aed_type res(trans=='N' ? mat.rows() : mat.cols());
  double *y = &res.data()[0];
  int incy = 1;

  dgemv_(&trans, &m, &n, &alpha, a, &lda, x, &incx, &beta, y, &incy);
  return res;
}
/** Computes diagonal entries of (M D M^T) where M is m by n
 * and D is n by n diagonal matrix
 *
 * [m,n][n,n][n,m]=[m,m]
 *
 */
aed_type T_MDMT_diag(const Matrix &M, const Matrix &D) {
  if (D.rows() != M.cols() || D.cols() != M.cols())
    throw std::invalid_argument(
        "T_MDMT_diag: D must be N x N for an M of N columns; M is " +
        shape(M) + ", D is " + shape(D));

  aed_type res(M.rows());
  res.set_zero();
  for (size_t i=0; i<M.rows(); ++i) {
    for (size_t j=0; j<M.cols(); ++j) {
      res[i] += D(j,j)*M(i,j)*M(i,j);
    }
  }

  return res;
}
aed_type T_MDMT_sym(const Matrix &M, const Matrix &S) {
  if (S.rows() != M.cols() || S.cols() != M.cols())
    throw std::invalid_argument(
        "T_MDMT_sym: S must be N x N for an M of N columns; M is " +
        shape(M) + ", S is " + shape(S));
  const size_t m = M.rows(), n = M.cols();
  aed_type res(m);
  res.set_zero();
  if (m == 0 || n == 0) return res;
  // Row blocks of M: P_b = M_b S (dgemm), then res_i = sum_j P_b(i,j) M(i,j).
  // M is column-major with leading dimension m, so the block of rows
  // i0 .. i0+b-1 starts at M.data() + i0 with the same leading dimension.
  const size_t block = std::max<size_t>(1, std::min<size_t>(m, 1024));
  std::vector<double> P(block * n);
  char transa = 'N', transb = 'N';
  double alpha = 1.0, beta = 0.0;
  int nn = static_cast<int>(n), lda = static_cast<int>(m);
  // dgemm_ takes non-const pointers; it does not write A or B.
  double *Mp = const_cast<Matrix &>(M).ptr();
  double *Sp = const_cast<Matrix &>(S).ptr();
  for (size_t i0 = 0; i0 < m; i0 += block) {
    const size_t b = std::min(block, m - i0);
    int bb = static_cast<int>(b);
    dgemm_(&transa, &transb, &bb, &nn, &nn, &alpha, Mp + i0, &lda, Sp, &nn,
           &beta, P.data(), &bb);
    for (size_t j = 0; j < n; ++j) {
      const double *pj = P.data() + j * b;
      const double *mj = Mp + j * m + i0;
      for (size_t r = 0; r < b; ++r) res[i0 + r] += pj[r] * mj[r];
    }
  }
  return res;
}
double norm_X(Matrix &A, char norm) {
  double *a = A.ptr();
  int m=(int)A.rows();
  int n=(int)A.cols();
  double *work = NULL;
  if (norm=='I' || norm=='i') work = new double[n];

  double res = dlange_(&norm, &m, &n, a, &m, work);

  if (norm=='I' || norm=='i') delete [] work;

  return 1.0/res;
}
aed_type solve_posv(Matrix &A, aed_type /*&*/T, char uplo) {
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

} // namespace core
} // namespace tadah

