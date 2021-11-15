/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Attacks.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

TEST_CASE("Attacks") {
    SECTION("at 0") {
        const auto atk = Attacks::at(0);

        CHECK(atk->name == "none");
        CHECK(atk->verb == "hit");
        CHECK(atk->damage_type == DamageType::None);
    }
    SECTION("at 41") {
        const auto atk = Attacks::at(41);

        CHECK(atk->name == "hit");
        CHECK(atk->verb == "hit");
        CHECK(atk->damage_type == DamageType::Bash);
    }
    SECTION("at 42 out of bounds") {
        const auto atk = Attacks::at(42);

        CHECK(!atk);
    }
    SECTION("index of") {
        CHECK(Attacks::index_of("none") == 0);
        CHECK(Attacks::index_of("slice") == 1);
        CHECK(Attacks::index_of("SLICE") == 1);
        CHECK(Attacks::index_of("Stab") == 2);
        CHECK(Attacks::index_of("slas") == 3);
        CHECK(Attacks::index_of("maul") == 40);
        CHECK(Attacks::index_of("hit") == 41);
        CHECK(Attacks::index_of("poke") == -1);
        CHECK(Attacks::index_of("") == -1);
    }
}