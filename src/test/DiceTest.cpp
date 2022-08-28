#include "Dice.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_version_macros.hpp>

#include "CatchFormatters.hpp"
#include "MemFile.hpp"
#include "MockRng.hpp"

TEST_CASE("Dice tests") {
    SECTION("should be zero by default") {
        Dice dice;
        CHECK(dice.number() == 0);
        CHECK(dice.type() == 0);
        CHECK(dice.bonus() == 0);
    }
    SECTION("should construct correctly") {
        Dice dice(2, 3, 4);
        CHECK(dice.number() == 2);
        CHECK(dice.type() == 3);
        CHECK(dice.bonus() == 4);
    }
    SECTION("should display appropriately") {
        CHECK(fmt::to_string(Dice(1, 2, 3)) == "1d2+3");
        CHECK(fmt::to_string(Dice(5, 6)) == "5d6");
    }
    SECTION("should roll correctly") {
        test::MockRng rng;
        REQUIRE_CALL(rng, dice(5, 6)).RETURN(13);
        CHECK(Dice(5, 6, 2).roll(rng) == 13 + 2);
    }
    SECTION("should read from files") {
        SECTION("with d") {
            test::MemFile file("1d10+2");
            CHECK(Dice::from_file(file.file()) == Dice(1, 10, 2));
        }
        SECTION("with D") {
            test::MemFile file("6D13+0");
            CHECK(Dice::from_file(file.file()) == Dice(6, 13, 0));
        }
    }
}
