#include "urange.hpp"

#include <catch2/catch.hpp>

TEST_CASE("urange") {
    SECTION("1 > 2 < 3") { CHECK(urange(1, 2, 3) == 2); }
    SECTION("2 < 1 < 3") { CHECK(urange(2, 1, 3) == 2); }
    SECTION("1 < 3 > 2") { CHECK(urange(1, 3, 2) == 2); }
    SECTION("3 > 2 > 1") { CHECK(urange(3, 2, 1) == 1); } // A weird case for urange to be used.
}