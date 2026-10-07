#include "catch2/catch.hpp"
#include <tadah/core/lapack.h>
#include <tadah/core/core_types.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <utility>

using namespace tadah::core;

TEST_CASE( "Testing condition_number()", "[lapack]" ) {
    Matrix m(2,2);
    m(0,0)=1.0;
    m(0,1)=2.0;
    m(1,0)=3.0;
    m(1,1)=4.0;
    REQUIRE_THAT( condition_number(m,'U'), Catch::Matchers::WithinAbs(0.22916666666667, 1e-12) );
}

TEST_CASE( "Testing solve_posv() 1", "[lapack]" ) {
    Matrix m(2,2);
    m(0,0)=2;
    m(0,1)=6;
    m(1,0)=6;
    m(1,1)=20;

    aed_type a(2);
    a(0)=11.11;
    a(1)=21.12;
    aed_type res=solve_posv(m,a,'U');
    REQUIRE_THAT( res(0), Catch::Matchers::WithinAbs(23.87, 1e-12) );
    REQUIRE_THAT( res(1), Catch::Matchers::WithinAbs(-6.10499999999998, 1e-12) );
}

TEST_CASE( "Testing solve_posv() 2", "[lapack]" ) {
    // matrix is to positive definite
    Matrix m(2,2);
    m(0,0)=2;
    m(0,1)=6;
    m(1,0)=6;
    m(1,1)=14;

    aed_type a(2);
    a(0)=11.11;
    a(1)=21.12;
    aed_type res=solve_posv(m,a,'U');
    REQUIRE_THAT( res(0), Catch::Matchers::WithinAbs(11.11, 1e-12) );
    REQUIRE_THAT( res(1), Catch::Matchers::WithinAbs(21.12, 1e-12) );
}

TEST_CASE( "Testing norm_X()", "[lapack]" ) {
    Matrix m(2,2);
    m(0,0)=1.0;
    m(0,1)=2.0;
    m(1,0)=3.0;
    m(1,1)=4.0;
    REQUIRE_THAT( norm_X(m,'m'), Catch::Matchers::WithinAbs(0.25, 1e-12) );
    REQUIRE_THAT( norm_X(m,'o'), Catch::Matchers::WithinAbs(0.1666666666667, 1e-12) );
    REQUIRE_THAT( norm_X(m,'i'), Catch::Matchers::WithinAbs(0.14285714285714, 1e-12) );
    REQUIRE_THAT( norm_X(m,'e'), Catch::Matchers::WithinAbs(0.18257418583505, 1e-12) );
}
TEST_CASE( "Testing T_MDMT_diag()", "[lapack]" ) {
    Matrix d(2,2);  // must be diagonal
    d(0,0)=1.0;
    d(0,1)=0.0;
    d(1,0)=0.0;
    d(1,1)=4.0;

    Matrix m(3,2);
    m(0,0)=1.0;
    m(0,1)=2.0;
    m(1,0)=3.0;
    m(1,1)=4.0;
    m(2,0)=5.0;
    m(2,1)=6.0;
    aed_type res =  T_MDMT_diag(m,d);
    REQUIRE(res(0)==17);
    REQUIRE(res(1)==73);
    REQUIRE(res(2)==169);
}
/* Pins: T_MDMT_diag(M, D) refuses a D that is not N x N for an M of N
 * columns (std::invalid_argument naming both shapes): a 3 x 2 M with a 4 x
 * 4 D and with a 2 x 3 D; the 2 x 2 D of "Testing T_MDMT_diag()" is
 * accepted.
 * Before the fix: no check; it read D(j,j) for j < N whatever D's shape
 * (with a smaller D, past its end: the out-of-bounds read predict --error
 * made with FIXINDEX, a covariance of the free weights only), and returned
 * a value for both. */
TEST_CASE( "T_MDMT_diag refuses a D of another size than M's columns",
           "[lapack]" ) {
    Matrix m(3,2);
    m.set_zero();
    for (const auto &rc : {std::pair<size_t,size_t>{4,4},
                           std::pair<size_t,size_t>{2,3}}) {
        Matrix d(rc.first, rc.second);
        d.set_zero();
        INFO("D " << rc.first << " x " << rc.second);
        std::string msg;
        try {
            T_MDMT_diag(m, d);
        } catch (const std::invalid_argument &e) {
            msg = e.what();
        }
        INFO("message: " << msg);
        CHECK(msg.find("3 x 2") != std::string::npos);
        CHECK(msg.find(std::to_string(rc.first) + " x " +
                       std::to_string(rc.second)) != std::string::npos);
    }
    Matrix d(2,2);
    d.set_zero();
    CHECK_NOTHROW(T_MDMT_diag(m, d));
}
/* Pins: T_MDMT_sym(M, S) is the diagonal of M S M^T for a non-diagonal
 * symmetric S (the hand values of a 3 x 2 M and S = [[2, 1], [1, 3]]:
 * m_i^T S m_i = 2 a^2 + 2 a b + 3 b^2 for a row (a, b): 18, 90, 218,
 * where T_MDMT_diag, reading the diagonal only, gives 14, 66, 158);
 * the same as the row-by-row quadratic form on 2500 x 3 rows, which takes
 * three blocks of 1024 rows (1e-12 relative); it refuses an S that is not
 * N x N (std::invalid_argument naming both shapes).
 * A new function: there is no before (T_MDMT_diag, which predict --error
 * called with the full covariance, read its diagonal only). */
TEST_CASE( "T_MDMT_sym is the diagonal of M S M^T", "[lapack]" ) {
    Matrix m(3,2);
    m(0,0)=1.0; m(0,1)=2.0;
    m(1,0)=3.0; m(1,1)=4.0;
    m(2,0)=5.0; m(2,1)=6.0;
    Matrix s(2,2);
    s(0,0)=2.0; s(0,1)=1.0;
    s(1,0)=1.0; s(1,1)=3.0;
    aed_type res = T_MDMT_sym(m, s);
    REQUIRE(res.size() == 3);
    CHECK(res(0) == 18.0);
    CHECK(res(1) == 90.0);
    CHECK(res(2) == 218.0);

    Matrix big(2500, 3);
    for (size_t j = 0; j < 3; ++j)
        for (size_t i = 0; i < 2500; ++i)
            big(i, j) = std::sin(0.37 * double(i) + 1.3 * double(j));
    Matrix S(3, 3);
    const double v[3][3] = {{2.0, 0.3, -0.4}, {0.3, 1.5, 0.2}, {-0.4, 0.2, 1.1}};
    for (size_t a = 0; a < 3; ++a)
        for (size_t b = 0; b < 3; ++b) S(a, b) = v[a][b];
    const aed_type q = T_MDMT_sym(big, S);
    REQUIRE(q.size() == 2500);
    double worst = 0.0;
    for (size_t i = 0; i < 2500; ++i) {
        double ref = 0.0;
        for (size_t a = 0; a < 3; ++a)
            for (size_t b = 0; b < 3; ++b) ref += big(i, a) * v[a][b] * big(i, b);
        worst = std::max(worst, std::fabs(q(i) - ref) / std::fabs(ref));
    }
    CHECK(worst < 1e-12);

    Matrix bad(3, 3);
    bad.set_zero();
    std::string msg;
    try {
        T_MDMT_sym(m, bad);
    } catch (const std::invalid_argument &e) {
        msg = e.what();
    }
    INFO("message: " << msg);
    CHECK(msg.find("3 x 2") != std::string::npos);
    CHECK(msg.find("3 x 3") != std::string::npos);
}
TEST_CASE( "Testing T_dgemv()", "[lapack]" ) {
    Matrix m(2,2);
    m(0,0)=2;
    m(0,1)=6;
    m(1,0)=6;
    m(1,1)=14;

    aed_type a(2);
    a(0)=11.11;
    a(1)=21.12;
    aed_type res=T_dgemv(m,a,'T');
    REQUIRE_THAT( res(0), Catch::Matchers::WithinAbs(148.94, 1e-12) );
    REQUIRE_THAT( res(1), Catch::Matchers::WithinAbs(362.34, 1e-12) );
}
