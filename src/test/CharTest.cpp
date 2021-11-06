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
    SECTION("extra flags") {
        Descriptor ch_desc(0);
        bob.desc = &ch_desc;
        ch_desc.character(&bob);
        bob.pcdata = std::make_unique<PcData>();
        SECTION("format extra flags") {
            SECTION("no flags set") {
                const auto result = bob.format_extra_flags();

                CHECK(result == "");
            }
            SECTION("all flags set") {
                for (auto i = 0; i < 3; i++)
                    bob.extra_flags[i] = ~(0ul);
                const auto result = bob.format_extra_flags();

                CHECK(result == "wnet wn_debug wn_mort wn_imm wn_bug permit wn_tick info_mes tip_wiz tip_adv");
            }
        }
        SECTION("serialize extra flags") {
            SECTION("no flags set") {
                const auto all_off = std::string(64u, '0');
                const auto result = bob.serialize_extra_flags();

                CHECK(result == all_off);
            }
            SECTION("all flags set") {
                const auto all_on = std::string(64u, '1');
                for (auto i = 0; i < 3; i++)
                    bob.extra_flags[i] = ~(0ul);
                const auto result = bob.serialize_extra_flags();

                CHECK(result == all_on);
            }
        }
    }
}