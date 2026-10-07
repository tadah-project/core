#include "catch2/catch.hpp"
#include <tadah/core/lapack.h>
#include <tadah/core/core_types.h>
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
