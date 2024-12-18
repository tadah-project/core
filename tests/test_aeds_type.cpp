#define CATCH_AEDS_TYPE_MAIN
#include "catch2/catch.hpp"
#include <tadah/core/aeds_type.h>

TEST_CASE("aeds_type2 default constructor") {
    aeds_type2 array;
    REQUIRE(array.size() == 0);
    REQUIRE(array.cols() == 0);
    REQUIRE(array.rows() == 0);
}

TEST_CASE("aeds_type2 parameterized constructor") {
    size_t dim = 4;
    size_t natoms = 3;
    aeds_type2 array(dim, natoms);
    REQUIRE(array.size() == dim * natoms);
    REQUIRE(array.cols() == natoms);
    REQUIRE(array.rows() == dim);
    
    for (size_t i = 0; i < dim; ++i) {
        for (size_t j = 0; j < natoms; ++j) {
            REQUIRE(array(i, j) == 0.0);
        }
    }
}

TEST_CASE("aeds_type2 copy constructor") {
    size_t dim = 4;
    size_t natoms = 3;
    aeds_type2 array1(dim, natoms);
    array1(1, 1) = 5.0;
    
    aeds_type2 array2(array1);
    REQUIRE(array2.size() == array1.size());
    REQUIRE(array2.cols() == array1.cols());
    REQUIRE(array2.rows() == array1.rows());
    REQUIRE(array2(1, 1) == 5.0);
}

TEST_CASE("aeds_type2 move constructor") {
    size_t dim = 4;
    size_t natoms = 3;
    aeds_type2 array1(dim, natoms);
    array1(1, 1) = 5.0;
    
    aeds_type2 array2(std::move(array1));
    REQUIRE(array2.size() == dim * natoms);
    REQUIRE(array2(1, 1) == 5.0);
}

TEST_CASE("aeds_type2 copy assignment operator") {
    size_t dim = 4;
    size_t natoms = 3;
    aeds_type2 array1(dim, natoms);
    aeds_type2 array2;
    array1(1, 1) = 5.0;
    
    array2 = array1;
    REQUIRE(array2.size() == array1.size());
    REQUIRE(array2(1, 1) == 5.0);
}

TEST_CASE("aeds_type2 move assignment operator") {
    size_t dim = 4;
    size_t natoms = 3;
    aeds_type2 array1(dim, natoms);
    aeds_type2 array2;
    array1(1, 1) = 5.0;
    
    array2 = std::move(array1);
    REQUIRE(array2.size() == dim * natoms);
    REQUIRE(array2(1, 1) == 5.0);
}

TEST_CASE("aeds_type2 resize and set_zero") {
    size_t dim = 4;
    size_t natoms = 3;
    aeds_type2 array(dim, natoms);
    array(0, 0) = 3.5;
    
    array.resize(2, 2);
    REQUIRE(array.size() == 4);
    REQUIRE(array.cols() == 2);
    REQUIRE(array.rows() == 2);

    array.set_zero();
    for (size_t i = 0; i < array.rows(); ++i) {
        for (size_t j = 0; j < array.cols(); ++j) {
            REQUIRE(array(i, j) == 0.0);
        }
    }
}

