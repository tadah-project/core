#define CATCH_FD_TYPE_MAIN
#include "catch2/catch.hpp"
#include <tadah/core/fd_type.h>

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
