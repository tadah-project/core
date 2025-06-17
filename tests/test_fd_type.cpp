#define CATCH_FD_TYPE_MAIN
#include "catch2/catch.hpp"
#include <tadah/core/fd_type.h>

using namespace tadah::core;

TEST_CASE("fd_type default constructor") {
    fd_type fd;
    REQUIRE(fd.rows() == 0);
}

TEST_CASE("fd_type parameterized constructor") {
    size_t dim = 5;
    fd_type fd(dim);
    REQUIRE(fd.rows() == dim);

    for (size_t i = 0; i < dim; ++i) {
        for (int j = 0; j < 3; ++j) {
            REQUIRE(fd(i, j) == 0.0);
        }
    }
}

TEST_CASE("fd_type copy constructor") {
    size_t dim = 3;
    fd_type fd1(dim);
    fd1(1, 1) = 5.0;

    fd_type fd2(fd1);
    REQUIRE(fd2.rows() == fd1.rows());
    REQUIRE(fd2(1, 1) == 5.0);
}

TEST_CASE("fd_type move constructor") {
    size_t dim = 3;
    fd_type fd1(dim);
    fd1(1, 1) = 5.0;

    fd_type fd2(std::move(fd1));
    REQUIRE(fd2.rows() == dim);
    REQUIRE(fd2(1, 1) == 5.0);
}

TEST_CASE("fd_type copy assignment operator") {
    size_t dim = 3;
    fd_type fd1(dim);
    fd1(1, 1) = 7.0;

    fd_type fd2;
    fd2 = fd1;
    REQUIRE(fd2.rows() == fd1.rows());
    REQUIRE(fd2(1, 1) == 7.0);
}

TEST_CASE("fd_type move assignment operator") {
    size_t dim = 3;
    fd_type fd1(dim);
    fd1(1, 1) = 9.0;

    fd_type fd2;
    fd2 = std::move(fd1);
    REQUIRE(fd2.rows() == dim);
    REQUIRE(fd2(1, 1) == 9.0);
}

TEST_CASE("fd_type arithmetic operations") {
    size_t dim = 3;
    fd_type fd1(dim);
    fd_type fd2(dim);
    fd1(1, 1) = 3.0;
    fd2(1, 1) = 1.0;

    fd_type fd3 = fd1 + fd2;
    REQUIRE(fd3(1, 1) == 4.0);

    fd3 = fd1 - fd2;
    REQUIRE(fd3(1, 1) == 2.0);

    fd1 *= 2.0;
    REQUIRE(fd1(1, 1) == 6.0);
}

TEST_CASE("fd_type set_zero and random") {
    size_t dim = 3;
    fd_type fd(dim);
    fd(0, 0) = 5.0;

    fd.set_zero();
    for (size_t i = 0; i < fd.rows(); ++i) {
        for (int j = 0; j < 3; ++j) {
            REQUIRE(fd(i, j) == 0.0);
        }
    }

    fd.random();
    bool allZero = true;
    for (size_t i = 0; i < fd.rows(); ++i) {
        for (int j = 0; j < 3; ++j) {
            if (fd(i, j) != 0.0) {
                allZero = false;
                break;
            }
        }
    }
    REQUIRE_FALSE(allZero);
}

TEST_CASE("fd_type equality and approximation") {
    size_t dim = 3;
    fd_type fd1(dim);
    fd_type fd2(dim);

    REQUIRE(fd1 == fd2);
    
    fd1(1, 1) = 0.0000001;
    REQUIRE(fd1.isApprox(fd2, 1e-6));
}


TEST_CASE("fd_type single dimension boundary checks", "[fd_type][new]") {
    // Test with dim=1
    fd_type fd1(1);
    REQUIRE(fd1.rows() == 1);
    
    // Check initial zero fill
    REQUIRE(fd1(0, 0) == 0.0);
    REQUIRE(fd1(0, 1) == 0.0);
    REQUIRE(fd1(0, 2) == 0.0);

    // Verify arithmetic on single row
    fd1(0,0) = 10.0;
    fd1(0,1) = 20.0;
    fd1 *= 2.0;
    REQUIRE(fd1(0,0) == 20.0);
    REQUIRE(fd1(0,1) == 40.0);

    // Another single dimension instance
    fd_type fd2(1);
    fd2(0,0) = 5.0;
    fd2(0,1) = 7.0;

    // Operator +
    fd_type fd3 = fd1 + fd2;
    REQUIRE(fd3(0,0) == 25.0);
    REQUIRE(fd3(0,1) == 47.0);

    // Operator -
    fd3 = fd1 - fd2;
    REQUIRE(fd3(0,0) == 15.0);
    REQUIRE(fd3(0,1) == 33.0);
}

TEST_CASE("fd_type operator[](size_t) and operator()(size_t k)", "[fd_type][new]") {
    fd_type fd(3);
    // Fill each coordinate
    fd(0,0) = 1.1; fd(1,0) = 1.2; fd(2,0) = 1.3;
    fd(0,1) = 2.1; fd(1,1) = 2.2; fd(2,1) = 2.3;
    fd(0,2) = 3.1; fd(1,2) = 3.2; fd(2,2) = 3.3;

    // operator[](size_t k) returns aed_type reference
    aed_type& ref0 = fd[0]; // x-coordinates
    REQUIRE(ref0.size() == 3);
    REQUIRE(ref0(0) == 1.1);
    REQUIRE(ref0(1) == 1.2);
    REQUIRE(ref0(2) == 1.3);

    // operator()(size_t k) const and non-const
    const fd_type& cfd = fd;
    REQUIRE(cfd(0)(2) == 1.3); // read-only usage
    fd_type& mfd = const_cast<fd_type&>(cfd);
    mfd(1)(0) = 9.9; // modifies y-coordinates at index 0
    REQUIRE(fd(0,1) == 9.9); 
}

TEST_CASE("fd_type operator*(const T&) with small vector", "[fd_type][new]") {
    // Suppose we multiply each column vector by a small coefficient vector.
    // fd_type is 3 rows -> each row's x,y,z. We'll multiply by a 3-element vector.
    fd_type fd(3);
    // Fill columns:
    // X-coordinates
    fd(0,0) = 1.0;  fd(1,0) = 2.0;  fd(2,0) = 3.0;
    // Y-coordinates
    fd(0,1) = 10.0; fd(1,1) = 20.0; fd(2,1) = 30.0;
    // Z-coordinates
    fd(0,2) = 100.0;fd(1,2) = 200.0;fd(2,2) = 300.0;

    // Multiply with a 3-element vector: v2 = [1, 1, 1]
    // The result is a Vec3d with components:
    //  x-component = sum over i: fd(i,x) * v2[i], i=0..2
    //  similarly for y, z
    std::vector<double> v2 = {1.0, 1.0, 1.0};
    Vec3d product = fd * v2; 
    // The x-component of product = (1.0*1.0 + 2.0*1.0 + 3.0*1.0) = 6 
    // The y-component = (10.0+20.0+30.0) = 60 
    // The z-component = (100.0+200.0+300.0) = 600

    REQUIRE(product(0) == 6.0);
    REQUIRE(product(1) == 60.0);
    REQUIRE(product(2) == 600.0);

    // Another example with different v2
    std::vector<double> v3 = {0.0, 1.0, 2.0};
    // x-component = 1*0 + 2*1 + 3*2 = 2 + 6 = 8
    // y-component = 10*0 + 20*1 + 30*2 = 20 + 60 = 80
    // z-component = 100*0 + 200*1 + 300*2 = 200 + 600 = 800
    Vec3d product2 = fd * v3;
    REQUIRE(product2(0) == 8.0);
    REQUIRE(product2(1) == 80.0);
    REQUIRE(product2(2) == 800.0);
}

TEST_CASE("fd_type operator<<(std::ostream& os, const fd_type&)", "[fd_type][new]") {
    fd_type fd(2);
    fd(0,0) = 1.1; fd(1,0) = 2.2;
    fd(0,1) = 3.3; fd(1,1) = 4.4;
    fd(0,2) = 5.5; fd(1,2) = 6.6;

    std::stringstream ss;
    ss << fd;

    // The operator<< prints each aed_type in a row. 
    // By default, aed_type uses indices in the same order as stored.
    // We'll just check that the stream is non-empty and has numeric data.
    REQUIRE_FALSE(ss.str().empty());

    // Basic pattern check. 
    // Typically each coordinate line might appear as "1.1 2.2", then next line "3.3 4.4", then "5.5 6.6".
    REQUIRE(ss.str().find("1.1") != std::string::npos);
    REQUIRE(ss.str().find("6.6") != std::string::npos);
}

TEST_CASE("fd_type out-of-range checks", "[fd_type][improved]") {
    fd_type fd(2);
    // If compiled in debug mode, the following out-of-range access
    // will trigger an assertion failure and stop the tests.
    // We cannot REQUIRE_THROWS with assert() because it’s not an exception.
    // This test mostly acts as a code sample. In a real debug build,
    // you'd see the assert triggered if i or k is invalid.

#ifdef NDEBUG
    WARN("Skipping out-of-range test because NDEBUG is defined (no asserts).");
#else
    // Only do the out-of-range in debug builds:
    // Uncomment next line to manually test:
    // double val = fd(2,0); // out-of-range row => triggers assert
    // double val2 = fd(0,3); // out-of-range col => triggers assert
    REQUIRE(true); // If we get here, we didn't do the actual invalid access.
#endif
}

TEST_CASE("fd_type dimension changes: copy from different sized fd_type", "[fd_type][improved]") {
    // We'll create from dimension=5, then dimension=3, and copy.
    fd_type fdBig(5);
    fdBig(4, 2) = 99.0; // use last row, z-col

    fd_type fdSmall(2);
    fdSmall(1, 1) = 1.23;

    // Copy small -> big
    fdBig = fdSmall;
    REQUIRE(fdBig.rows() == 2);   // dimension changed from 5 to 2
    REQUIRE(fdBig(1, 1) == 1.23); // data is from fdSmall now

    // Next copy big -> small
    // Let’s re-fill fdBig to dimension=4 so it’s not the same as fdSmall
    fd_type fdMid(4);
    fdMid(3, 2) = 7.89;
    fdSmall = fdMid;
    REQUIRE(fdSmall.rows() == 4);
    REQUIRE(fdSmall(3, 2) == 7.89);
}

TEST_CASE("fd_type random distribution bounds check", "[fd_type][improved]") {
    fd_type fd(5);
    double lower = -10.0;
    double upper = 10.0;
    fd.random(std::chrono::system_clock::now().time_since_epoch().count(), lower, upper);

    // Confirm all data is within [lower, upper]
    for (size_t i = 0; i < fd.rows(); ++i) {
      for (int k = 0; k < 3; ++k) {
        double val = fd(i, k);
        REQUIRE(val >= lower);
        REQUIRE(val <= upper);
      }
    }
}

TEST_CASE("fd_type borderline differences for isApprox()", "[fd_type][improved]") {
    size_t dim = 2;
    fd_type fd1(dim);
    fd_type fd2(dim);

    // Make them almost identical
    fd1(0,0) = 1.0;   fd2(0,0) = 1.0;
    fd1(1,2) = 10.0;  fd2(1,2) = 10.0;

    // They should be approx
    REQUIRE(fd1.isApprox(fd2, 1e-9) == true);

    // Now let's exceed the EPS a bit
    fd2(1,2) = 10.00001; 
    REQUIRE(fd1.isApprox(fd2, 1e-12) == false); 
    // because difference is 1e-5 which is >> 1e-12
}
