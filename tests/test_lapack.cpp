#include "catch2/catch.hpp"
#include "../lapack.h"
#include "../core_types.h"
#include <iomanip>

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

    aed_type2 a(2);
    a(0)=11.11;
    a(1)=21.12;
    aed_type2 res=solve_posv(m,a,'U');
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

    aed_type2 a(2);
    a(0)=11.11;
    a(1)=21.12;
    aed_type2 res=solve_posv(m,a,'U');
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
    Matrix d(2,2);
    d(0,0)=1.0;
    d(0,1)=2.0;
    d(1,0)=3.0;
    d(1,1)=4.0;

    Matrix m(3,2);
    m(0,0)=1.0;
    m(0,1)=2.0;
    m(1,0)=3.0;
    m(1,1)=4.0;
    m(2,0)=5.0;
    m(2,1)=6.0;
    aed_type2 res =  T_MDMT_diag(m,d);
    REQUIRE(res(0)==17);
    REQUIRE(res(1)==73);
    REQUIRE(res(2)==169);
}
TEST_CASE( "Testing T_dgemv()", "[lapack]" ) {
    Matrix m(2,2);
    m(0,0)=2;
    m(0,1)=6;
    m(1,0)=6;
    m(1,1)=14;

    aed_type2 a(2);
    a(0)=11.11;
    a(1)=21.12;
    aed_type2 res=T_dgemv(m,a,'T');
    REQUIRE_THAT( res(0), Catch::Matchers::WithinAbs(148.94, 1e-12) );
    REQUIRE_THAT( res(1), Catch::Matchers::WithinAbs(362.34, 1e-12) );
}
