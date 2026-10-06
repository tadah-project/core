#include "catch2/catch.hpp"
#include <tadah/core/utils.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("Strict indices expand lists ranges and strides", "[utils][pc_dataset]") {
    CHECK(tadah::core::parse_indices_strict("1, 3-7:2, 9-10")
          == std::vector<std::size_t>{1,3,5,7,9,10});
    CHECK(tadah::core::parse_indices_strict("3, 1, 0") == std::vector<std::size_t>{3,1,0});
    const auto max = std::numeric_limits<std::size_t>::max();
    const auto s = std::to_string(max);
    CHECK(tadah::core::parse_indices_strict(s + "-" + s) == std::vector<std::size_t>{max});
    CHECK(tadah::core::parse_indices(s + "-" + s) == std::vector<std::size_t>{max});
}

TEST_CASE("Strict indices reject partial input and unsafe expansion", "[utils][pc_dataset]") {
    for (const auto* input : {"", " ", ",", "1,", ",1", "1,,2", "x,1", "1x", "-1",
                             "1-", "3-1", "1-3:0", "1-3:", "1-3:x", "1:2", "1-2-3",
                             "1-3:2:1", "1,1", "1-3,2", "184467440737095516160"}) {
        INFO(input);
        CHECK_THROWS(tadah::core::parse_indices_strict(input));
    }
    CHECK_THROWS(tadah::core::parse_indices("1-3:0"));
}

/* Pins: parse_dataset_indices, the 1-based index list of the dataset
 * commands (tadah data sample and data balance -i), on a dataset of 10:
 *  - the values form one list, as the command line and a task file give
 *    them: '1, 3' arrives as the values '1,' and '3'; spaces around '-' and
 *    ':' and between indices are allowed; repeated indices are removed;
 *  - an empty segment, a descending range, a step of 0, a token that is not
 *    an index, index 0 and an index above 10 are refused, naming the
 *    offending text, and a range is checked against the bounds before it is
 *    expanded (1-1000000000 is refused at once). */
TEST_CASE("parse_dataset_indices: one 1-based list, checked before expansion",
          "[utils][indices]") {
    using tadah::core::parse_dataset_indices;
    using V = std::vector<std::size_t>;
    using S = std::vector<std::string>;
    CHECK(parse_dataset_indices(S{"1,", "3"}, 10, "t") == V{1, 3});
    CHECK(parse_dataset_indices(S{"1, 3"}, 10, "t") == V{1, 3});
    CHECK(parse_dataset_indices(S{"1 - 3"}, 10, "t") == V{1, 2, 3});
    CHECK(parse_dataset_indices(S{"1-5 : 2"}, 10, "t") == V{1, 3, 5});
    CHECK(parse_dataset_indices(S{"1,3", "5"}, 10, "t") == V{1, 3, 5});
    CHECK(parse_dataset_indices(S{"1,3 5"}, 10, "t") == V{1, 3, 5});
    CHECK(parse_dataset_indices(S{"1,1,2"}, 10, "t") == V{1, 2});
    CHECK(parse_dataset_indices(S{"10", "1-10:9"}, 10, "t") == V{10, 1});
    const std::vector<std::pair<S, std::string>> bad = {
        {S{"1,,2"}, "1,,2"}, {S{",1"}, ",1"}, {S{"1,"}, "1,"}, {S{"5-3"}, "'5-3'"},
        {S{"1-3:0"}, "'1-3:0'"}, {S{"x"}, "'x'"}, {S{"0"}, "index 0"},
        {S{"11"}, "index 11"}, {S{"1-1000000000"}, "index 1000000000"},
        {S{"184467440737095516160"}, "'184467440737095516160'"}};
    for (const auto &b : bad) {
        INFO(b.first[0]);
        try {
            (void)parse_dataset_indices(b.first, 10, "tadah data sample");
            FAIL("no exception");
        } catch (const std::runtime_error &e) {
            CHECK_THAT(e.what(), Catch::Contains(b.second));
            CHECK_THAT(e.what(), Catch::Contains("tadah data sample"));
        }
    }
}
