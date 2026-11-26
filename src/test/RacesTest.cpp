#include "Races.hpp"
#include "BodyPartFlag.hpp"
#include "common/BitOps.hpp"
#include "handler.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("race body parts") {
    SECTION("dragons have a tail") {
        const auto dragon = race_lookup("dragon");
        REQUIRE(dragon > 0);
        CHECK(check_enum_bit(race_table[dragon].parts, BodyPartFlag::Tail));
    }
    SECTION("dragons keep their wings and scales") {
        const auto dragon = race_lookup("dragon");
        CHECK(check_enum_bit(race_table[dragon].parts, BodyPartFlag::Wings));
        CHECK(check_enum_bit(race_table[dragon].parts, BodyPartFlag::Scales));
    }
    SECTION("humans have no tail") {
        const auto human = race_lookup("human");
        CHECK(!check_enum_bit(race_table[human].parts, BodyPartFlag::Tail));
    }
}
