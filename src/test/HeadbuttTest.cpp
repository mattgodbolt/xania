/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Headbutt.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

TEST_CASE("headbutt blinds") {
    using std::make_tuple;
    SECTION("a victim who can see, by the level-adjusted roll") {
        int hit_roll, attacker_level, victim_level;
        bool expected;
        std::tie(hit_roll, attacker_level, victim_level, expected) = GENERATE(table<int, int, int, bool>({
            make_tuple(1, 50, 50, true), // the same level: rolls under 5 blind
            make_tuple(4, 50, 50, true), // the highest roll that blinds
            make_tuple(5, 50, 50, false), // the lowest that doesn't
            make_tuple(60, 50, 50, false), // blinded every time before
            make_tuple(24, 50, 30, true), // 20 levels above the victim: rolls under 25 blind
            make_tuple(25, 50, 30, false), // and 25 doesn't
            make_tuple(1, 30, 50, false), // 20 levels below the victim: never
        }));
        CHECK(Headbutt::blinds(hit_roll, attacker_level, victim_level, false) == expected);
    }
    SECTION("never a victim who's already blind, so the hitroll penalty doesn't stack") {
        const auto hit_roll = GENERATE(range(1, 100));
        CHECK_FALSE(Headbutt::blinds(hit_roll, 50, 1, true));
    }
}
