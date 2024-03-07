#include "catch2/catch.hpp"
#include "../utils/utils.h"
#include "../core_types.h"
#include <iomanip>

TEST_CASE( "Testing logspace() 1", "[utils]" ) {
    REQUIRE_THROWS(logspace(0,2,10,10));
}
TEST_CASE( "Testing logspace() 2", "[utils]" ) {
    v_type v = logspace(1,10,4, 10);
    REQUIRE_THAT( v[0], Catch::Matchers::WithinAbs(1 ,1e-12) );
    REQUIRE_THAT( v[1], Catch::Matchers::WithinAbs(5.8550197495911 ,1e-12) );
    REQUIRE_THAT( v[2], Catch::Matchers::WithinAbs(34.2812562681019 ,1e-12) );
    REQUIRE_THAT( v[3], Catch::Matchers::WithinAbs(200.71743249053 ,1e-12) );
}
TEST_CASE( "Testing logspace() 3", "[utils]" ) {
    v_type v = logspace(1,4,4, log(10));
    REQUIRE_THAT( v[0], Catch::Matchers::WithinAbs(1 ,1e-12) );
    REQUIRE_THAT( v[1], Catch::Matchers::WithinAbs(1.4702093789085, 1e-12) );
    REQUIRE_THAT( v[2], Catch::Matchers::WithinAbs(2.1615156178305, 1e-12) );
    REQUIRE_THAT( v[3], Catch::Matchers::WithinAbs(3.1778805339916, 1e-12) );
}
TEST_CASE( "Testing number_of_digits()", "[utils]" ) {
    REQUIRE( number_of_digits(123456789)==9);
    REQUIRE( number_of_digits(9)==1);
    REQUIRE( number_of_digits(0)==1);
    REQUIRE( number_of_digits(10)==2);
}
TEST_CASE( "Testing max_number()", "[utils]" ) {
    REQUIRE( max_number(0)==0);
    REQUIRE( max_number(1)==9);
    REQUIRE( max_number(2)==99);
    REQUIRE( max_number(9)==999999999);
}
TEST_CASE( "Testing vec_tp_string()", "[utils]" ) {
    v_type v = {1.12, 1.13, 1.145};
    REQUIRE_THAT(vec_to_string(v), Catch::Matchers::Equals("1.12 1.13 1.145"));
    REQUIRE_THAT(vec_to_string(v,","), Catch::Matchers::Equals("1.12,1.13,1.145"));
}
TEST_CASE( "Testing vec_to_upper()", "[utils]" ) {
    REQUIRE_THAT(to_upper("this is test"), Catch::Matchers::Equals("THIS IS TEST"));
    //REQUIRE_THAT(to_upper("This IS tEst"), Catch::Matchers::Equals("THIS IS TEST"));
}
TEST_CASE( "Testing operator<<(vec<T>)", "[utils]" ) {
    v_type v = {1.12, 1.13, 1.145};
    std::stringstream ss;
    ss << v;
    REQUIRE_THAT(ss.str(), Catch::Matchers::Equals("1.12 1.13 1.145 "));
}
TEST_CASE( "Testing linspace", "[utils]" ) {
    v_type v = linspace(0, 10, 11);
    REQUIRE(v[0]==0);
    REQUIRE(v[1]==1);
    REQUIRE(v[9]==9);
    REQUIRE(v[10]==10);
    REQUIRE(linspace(0, 10, 0).size()==0);
    REQUIRE(linspace(1.1, 3.0, 1.0).size()==1);
    REQUIRE(linspace(1.1, 3.0, 1.0)[0]==1.1);
}
TEST_CASE( "Testing to_string<T>(v,n)", "[utils]" ) {
    REQUIRE_THAT(to_string(1.123456789,3), Catch::Matchers::Equals("1.12"));
    REQUIRE_THAT(to_string(1.123456789,5), Catch::Matchers::Equals("1.1235"));
    REQUIRE_THAT(to_string(1.123456789,9), Catch::Matchers::Equals("1.12345679"));
    REQUIRE_THAT(to_string(1.123456789,10), Catch::Matchers::Equals("1.123456789"));
}
TEST_CASE( "Testing to_string<T>(v)", "[utils]" ) {
    REQUIRE_THAT(to_string(1.123456789), Catch::Matchers::Equals("1.12346"));
}

double f(double x) {
    return 3*x*x;
}

TEST_CASE( "Testing central_difference", "[utils]" ) {
    REQUIRE_THAT(central_difference(f,0,0.01),
            Catch::Matchers::WithinAbs(0,1e-12) );
    REQUIRE_THAT(central_difference(f,1,0.01),
            Catch::Matchers::WithinAbs(6,1e-12) );
    REQUIRE_THAT(central_difference(f,-1,0.01),
            Catch::Matchers::WithinAbs(-6 ,1e-12) );
}
TEST_CASE( "Testing random", "[utils]" ) {
    double d1 = random(0.0, 1.0);
    double d2 = random(0.0, 1.0);
    REQUIRE_THAT(d1, !Catch::Matchers::WithinAbs(d2 ,1e-12));
}
TEST_CASE( "Testing get_index", "[utils]" ) {
    v_type v = {1.12, 1.13, 1.55};
    REQUIRE(get_index(v, 1.12)==0);
    REQUIRE(get_index(v, 1.13)==1);
    REQUIRE(get_index(v, 1.55)==2);
}
