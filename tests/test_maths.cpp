#include "catch2/catch.hpp"
#include <tadah/core/core_types.h>
#include <tadah/core/maths.h>
#include <iomanip>


TEST_CASE("aed_type Constructors") {
    SECTION("Default constructor") {
        aed_type vec;
        REQUIRE(vec.size() == 0);
        REQUIRE(vec.data() == nullptr);
    }

    SECTION("Constructor with size and stride") {
        size_t size = 5;
        size_t stride = 2;
        aed_type vec(size, stride);
        REQUIRE(vec.size() == size);
        REQUIRE(vec.data() != nullptr);
    }
}

TEST_CASE("aed_type Constructors with External Arrays") {
    SECTION("Non-allocating constructor with double*") {
        size_t size = 5;
        double data[] = {1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == data[i]);
        }
    }

    SECTION("Allocating constructor with const double*") {
        size_t size = 5;
        const double data[] = {1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == data[i]);
        }
    }
}
TEST_CASE("aed_type Constructors with 'new' Allocated Arrays") {
    SECTION("Non-allocating constructor with double*") {
        size_t size = 5;
        double* data = new double[size]{1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == data[i]);
        }

        delete[] data; // Clean up
    }

    SECTION("Allocating constructor with const double*") {
        size_t size = 5;
        const double* data = new double[size]{1, 2, 3, 4, 5};
        aed_type vec(data, size);

        REQUIRE(vec.size() == size);
        for (size_t i = 0; i < size; ++i) {
            REQUIRE(vec[i] == data[i]);
        }

        delete[] data; // Clean up
    }
}

TEST_CASE( "Testing copy constructors of aed_type") {
  REQUIRE_NOTHROW(aed_type());
  REQUIRE_NOTHROW(aed_type(5));
  REQUIRE_NOTHROW(aed_type(5,2));
  REQUIRE_NOTHROW(aed_type(6,2));

  // Create an initial object
  aed_type original(3);
  aed_type original2(3);
  original[0]=0.1; original[1]=10; original[2]=0.0001;
  original[0]=0.100000000000001; original[1]=10; original[2]=0.0001;

  // Set up original with specific values
  SECTION("Const copy constructor") {
    const aed_type const_original = original;

    aed_type const_copy = const_original;

    REQUIRE(const_copy != original2);
  }

  SECTION("Non-const copy constructor") {
    aed_type non_const_copy = original;

    REQUIRE(non_const_copy == original);
    REQUIRE(non_const_copy != original2);
  }
}
TEST_CASE("aed_type Assignment Operator") {
    size_t size = 5;
    aed_type original(size);
    for (size_t i = 0; i < size; ++i) {
        original[i] = static_cast<double>(i);
    }

    aed_type copy;
    copy = original;
    REQUIRE(copy == original); // Uses operator==
}

TEST_CASE("aed_type Element Access") {
    size_t size = 3;
    aed_type vec(size);
    vec[0] = 1.0;
    vec[1] = 2.0;
    vec[2] = 3.0;

    REQUIRE(vec[0] == 1.0);
    REQUIRE(vec[1] == 2.0);
    REQUIRE(vec[2] == 3.0);
}

TEST_CASE("aed_type Operations") {
    size_t size = 3;
    aed_type vec(size);
    vec[0] = 1.0;
    vec[1] = 2.0;
    vec[2] = 3.0;

    vec += 1.0;
    REQUIRE(vec[0] == 2.0);
    REQUIRE(vec[1] == 3.0);
    REQUIRE(vec[2] == 4.0);

    vec -= 1.0;
    REQUIRE(vec[0] == 1.0);
    REQUIRE(vec[1] == 2.0);
    REQUIRE(vec[2] == 3.0);

    vec *= 2.0;
    REQUIRE(vec[0] == 2.0);
    REQUIRE(vec[1] == 4.0);
    REQUIRE(vec[2] == 6.0);
}

TEST_CASE("aed_type Copy Method with Range") {
    size_t size = 5;
    aed_type vec(size);
    for (size_t i = 0; i < size; ++i) {
        vec[i] = static_cast<double>(i + 1);
    }

    aed_type subVec = vec.copy(3, 1);
    REQUIRE(subVec.size() == 3);
    REQUIRE(subVec[0] == 2.0);
    REQUIRE(subVec[1] == 3.0);
    REQUIRE(subVec[2] == 4.0);
}

TEST_CASE("aed_type Random Initialization") {
  size_t size = 6;
  SECTION("Default bounds") {
    aed_type vec(size);
    vec.random();

    // Check values are within a specified range
    for (size_t i = 0; i < size; ++i) {
      REQUIRE(vec[i] >= -1e4);
      REQUIRE(vec[i] <= 1e4);
    }
  }

  SECTION("Custom bounds") {
    int seed = 501;
    double low = 1e-5;
    double high = 1e-4;
    aed_type vec2(size);
    vec2.random(seed,low,high);

    // Check values are within a specified range
    for (size_t i = 0; i < size; ++i) {
      REQUIRE(vec2[i] >= low);
      REQUIRE(vec2[i] <= high);
    }
  }
}
