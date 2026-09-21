#include "catch2/catch.hpp"
#include <tadah/core/utils.h>
#include <limits>

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
