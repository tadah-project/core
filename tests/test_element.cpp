#include "catch2/catch.hpp"
#include "../element.h"
#include <sstream>

TEST_CASE( "Testing Element class constructor", "[element]" ) {

    Element empty();

    char symbol1[]="Ti";
    int Z1 = 22;
    Element e1(symbol1,Z1);
    Element e2(symbol1,Z1);

    REQUIRE(e1==e2);

    char symbol3[]="Nb";
    int Z3 = 41;
    Element e3(symbol3,Z3);
    REQUIRE(!(e1==e3));
    REQUIRE(!(e2==e3));
}
TEST_CASE( "Testing Element class copy operator", "[element]" ) {
    char symbol1[]="Ti";
    int Z1 = 22;
    Element e1(symbol1,Z1);
    Element e2=e1;
    REQUIRE(e1==e2);
}
TEST_CASE( "Testing Element class methods", "[element]" ) {
    char symbol1[]="Ti";
    int Z1 = 22;
    Element e1(symbol1,Z1);

    char symbol2[]="N";
    int Z2 = 7;
    Element e2(symbol2,Z2);
    REQUIRE(e2<e1);
    REQUIRE(e1>e2);
}
TEST_CASE( "Testing Element ostream", "[element]" ) {
    char symbol1[]="Ti";
    int Z1 = 22;
    Element e1(symbol1,Z1);
    std::stringstream ss;
    ss << e1;
    REQUIRE_THAT(ss.str(), Catch::Matchers::Equals("Symbol: Ti\nAtomic Number: 22\n"));
}
