#include "nth.hpp"

#include <catch2/catch.hpp>

TEST_CASE("nth formatting") {
    CHECK(fmt::to_string(nth(1)) == "1st");
    CHECK(fmt::to_string(nth(2)) == "2nd");
    CHECK(fmt::to_string(nth(3)) == "3rd");
    CHECK(fmt::to_string(nth(4)) == "4th");
    CHECK(fmt::to_string(nth(5)) == "5th");
    CHECK(fmt::to_string(nth(101)) == "101st");
    CHECK(fmt::to_string(nth(999)) == "999th");
    CHECK(fmt::to_string(nth(0)) == "0th");
    CHECK(fmt::to_string(nth(-1)) == "-1st");
}
