#include "Char.hpp"

#include "Room.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch.hpp>

#include <string_view>

using namespace std::literals;

TEST_CASE("Character tests", "[Char]") {
    Room some_room;
    Char bob;
    bob.in_room = &some_room;
    bob.name = "Bob";
    bob.description = "Bob the blacksmith";

    SECTION("should describe correctly") {
        Char other;
        other.in_room = &some_room;
        other.name = "Other";
        other.description = "Other the other char";
        SECTION("when characters can see each other") {
            CHECK(bob.describe(bob) == "Bob"sv);
            CHECK(other.describe(bob) == "Bob"sv);
            CHECK(bob.describe(other) == "Other"sv);
        }
        SECTION("when one character can't see the other") {
            set_enum_bit(bob.affected_by, AffectFlag::Blind);
            CHECK(bob.describe(bob) == "Bob"sv);
            CHECK(other.describe(bob) == "Bob"sv);
            CHECK(bob.describe(other) == "someone"sv);
        }
    }
}