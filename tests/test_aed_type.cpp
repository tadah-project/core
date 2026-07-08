#define CATCH_AED_TYPE_MAIN
#include "catch2/catch.hpp"
#include <tadah/core/aed_type.h>
#include <sstream>

using namespace tadah::core;

TEST_CASE("aed_type default constructor") {
    aed_type vec;
    REQUIRE(vec.size() == 0);
}

TEST_CASE("aed_type parameterized constructor") {
    size_t size = 5;
    aed_type vec(size);
    REQUIRE(vec.size() == size);
    for (size_t i = 0; i < size; ++i) {
        REQUIRE(vec[i] == 0.0);
    }
}

TEST_CASE("aed_type from array constructor") {
    double data[] = {1.1, 2.2, 3.3};
    aed_type vec(data, 3);
    REQUIRE(vec.size() == 3);
    REQUIRE(vec[0] == 1.1);
    REQUIRE(vec[1] == 2.2);
    REQUIRE(vec[2] == 3.3);
}

TEST_CASE("aed_type copy constructor") {
    double data[] = {4.4, 5.5};
    aed_type vec1(data, 2);
    aed_type vec2(vec1);
    REQUIRE(vec2.size() == vec1.size());
    REQUIRE(vec2[0] == vec1[0]);
    REQUIRE(vec2[1] == vec1[1]);
}

TEST_CASE("aed_type move constructor") {
    aed_type vec1(3);
    vec1[0] = 1.1; vec1[1] = 2.2; vec1[2] = 3.3;
    aed_type vec2(std::move(vec1));
    REQUIRE(vec2.size() == 3);
}

TEST_CASE("aed_type arithmetic operations") {
    aed_type vec(3);
    vec[0] = 1.0; vec[1] = 2.0; vec[2] = 3.0;
    vec += 1.0;
    vec *= 2.0;
    vec -= 1.0;

    REQUIRE(vec[0] == 3.0);
    REQUIRE(vec[1] == 5.0);
    REQUIRE(vec[2] == 7.0);

    aed_type vec2 = vec + 3.0;
    REQUIRE(vec2[0] == 6.0);

    vec2 = vec - 1.0;
    REQUIRE(vec2[0] == 2.0);
}

TEST_CASE("aed_type dot product") {
    double data[] = {1.0, 2.0};
    aed_type vec1(data, 2);
    aed_type vec2 = vec1;
    double result = vec1 * vec2;
    REQUIRE(result == 5.0);  // 1*1 + 2*2
}

TEST_CASE("aed_type equality check") {
    double data[] = {1.0, 2.0, 3.0};
    aed_type vec1(data, 3);
    aed_type vec2(vec1);
    REQUIRE(vec1 == vec2);
    REQUIRE_FALSE(vec1 != vec2);
}

TEST_CASE("aed_type set zero and resize") {
    aed_type vec(3);
    vec[0] = 1.0; vec[1] = 2.0; vec[2] = 3.0;
    vec.set_zero();
    for (size_t i = 0; i < vec.size(); ++i) {
        REQUIRE(vec[i] == 0.0);
    }

    vec.resize(5);
    REQUIRE(vec.size() == 5);
    for (size_t i = 0; i < vec.size(); ++i) {
        REQUIRE(vec[i] == 0.0);
    }
}

TEST_CASE("aed_type random generation") {
    aed_type vec(5);
    vec.random();
    bool allZero = true;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] != 0.0) {
            allZero = false;
            break;
        }
    }
    REQUIRE_FALSE(allZero);
}

TEST_CASE("aed_type mean and std deviation") {
    double data[] = {1.0, 2.0, 3.0, 4.0};
    aed_type vec(data, 4);

    double mean = vec.mean();
    REQUIRE(mean == Approx(2.5));

    double stddev = vec.std_dev(mean, 4);
    REQUIRE(stddev == Approx(1.11803));
}

TEST_CASE("aed_type copy method") {
    double data[] = {1.0, 2.0, 3.0};
    aed_type vec(data, 3);

    aed_type vec_copy = vec.copy(2);

    REQUIRE(vec_copy.size() == 2);
    REQUIRE(vec_copy[0] == 1.0);
    REQUIRE(vec_copy[1] == 2.0);
}

TEST_CASE("aed_type copy() out-of-range throws", "[aed_type][new]") {
    // copy(size_t nelements, size_t start) checks if (nelements + start > size())
    aed_type vec({10.0, 20.0, 30.0});
    REQUIRE_THROWS_AS(vec.copy(2, 2), std::runtime_error);
    // because 2 + 2 = 4 > size 3 => "Out of range"

    REQUIRE_NOTHROW(vec.copy(3, 0)); // 3 + 0 = 3 => valid
}

TEST_CASE("aed_type copy assignment self-assignment", "[aed_type][new]") {
    // Confirm that self-assignment won't break anything
    aed_type vec({1.1, 2.2, 3.3});
    aed_type &vecRef = vec;
    REQUIRE_NOTHROW(vec = vecRef); // self-assignment
    REQUIRE(vec.size() == 3);
    REQUIRE(vec[1] == 2.2);
}

TEST_CASE("aed_type move assignment self-assignment", "[aed_type][new]") {
    aed_type vec({1.1, 2.2, 3.3});
    aed_type &vecRef = vec;
    REQUIRE_NOTHROW(vec = std::move(vecRef)); 
    // Technically self move-assignment is discouraged. We test that it won’t crash 
    // though behavior can be arbitrary. Just for coverage.
}

TEST_CASE("aed_type operator*(T) with size mismatch (potential UB)", "[aed_type][new]") {
    // The code does not do a size check before summation => potential out-of-bounds
    // If using dynamically allocated arrays, this can crash or read random memory.
    // Note it’s undefined behavior (not an exception).
    aed_type vec1({1.0, 2.0});
    aed_type vec2({3.0});
    
    // Because there's no check, we can't guarantee a result.
    // We'll show the call in a REQUIRE_NOTHROW if the implementation doesn't throw:
    REQUIRE_NOTHROW(static_cast<void>(vec1 * vec2));
    // The result is likely out-of-bounds for the second element. 
}

TEST_CASE("aed_type operator<<(std::ostream&)", "[aed_type][new]") {
    aed_type vec({5.5, 6.6, 7.7});
    std::stringstream ss;
    ss << vec;
    // We expect something like "5.5  6.6  7.7"
    REQUIRE_FALSE(ss.str().empty());
    REQUIRE(ss.str().find("5.5") != std::string::npos);
    REQUIRE(ss.str().find("6.6") != std::string::npos);
    REQUIRE(ss.str().find("7.7") != std::string::npos);
}

TEST_CASE("aed_type::resize shrink then grow", "[aed_type][resize]") {
    // Regression: shrinking used to call std::fill with first > last
    // (fill range computed from the OLD size on the reallocated,
    // smaller block), writing zeros past the end of the heap buffer.
    // Exercised by the FIXWEIGHT partial-fix path, where the model
    // weight vector shrinks from K to K - |FIXINDEX| between trainings.
    aed_type v(14);
    for (size_t i = 0; i < v.size(); ++i) v[i] = 1.0 + double(i);

    v.resize(12);  // shrink: must not write beyond the new block
    REQUIRE(v.size() == 12);
    for (size_t i = 0; i < 12; ++i) REQUIRE(v[i] == 1.0 + double(i));

    v.resize(14);  // grow again: new tail must be zero-initialised
    REQUIRE(v.size() == 14);
    for (size_t i = 0; i < 12; ++i) REQUIRE(v[i] == 1.0 + double(i));
    REQUIRE(v[12] == 0.0);
    REQUIRE(v[13] == 0.0);

    v.resize(14);  // no-op resize keeps contents
    REQUIRE(v[5] == 6.0);
}
