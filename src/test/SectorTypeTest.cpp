#include "SectorType.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("sector types") {
    SECTION("should format") { CHECK(fmt::format("{}", SectorType::Air) == "air"); }
}
