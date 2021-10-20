/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Weapon.hpp"

#include <catch2/catch.hpp>

TEST_CASE("weapon type try from name") {
    SECTION("valid") {
        const auto result = Weapons::try_from_name("sword");

        CHECK(result == Weapon::Sword);
    }
    SECTION("invalid") {
        const auto result = Weapons::try_from_name("banana");

        CHECK(!result);
    }
}
TEST_CASE("weapon type try from integer") {
    SECTION("valid") {
        const auto result = Weapons::try_from_integer(1);

        CHECK(result == Weapon::Sword);
    }
    SECTION("invalid") {
        const auto result = Weapons::try_from_integer(9);

        CHECK(!result);
    }
}
TEST_CASE("weapon type name from integer") {
    SECTION("all names") {
        CHECK(Weapons::name_from_integer(0) == "exotic");
        CHECK(Weapons::name_from_integer(1) == "sword");
        CHECK(Weapons::name_from_integer(2) == "dagger");
        CHECK(Weapons::name_from_integer(3) == "spear");
        CHECK(Weapons::name_from_integer(4) == "mace");
        CHECK(Weapons::name_from_integer(5) == "axe");
        CHECK(Weapons::name_from_integer(6) == "flail");
        CHECK(Weapons::name_from_integer(7) == "whip");
        CHECK(Weapons::name_from_integer(8) == "polearm");
    }
}
