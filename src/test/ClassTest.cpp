/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2023 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Class.hpp"
#include "VnumObjects.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Class static functions") {
    using namespace std::literals;
    SECTION("by_name pointer equivalence") {
        CHECK(Class::by_name("mage") == Class::mage());
        CHECK(Class::by_name("ma") == Class::mage());
        CHECK(Class::by_name("Ma") == Class::mage());
        CHECK(Class::by_name("cleric") == Class::cleric());
        CHECK(Class::by_name("knight") == Class::knight());
        CHECK(Class::by_name("barbarian") == Class::barbarian());
        CHECK(Class::by_name("foo") == nullptr);
        CHECK(Class::by_name("") == nullptr);
    }
    SECTION("by_id pointer equivalence") {
        CHECK(Class::by_id(0) == Class::mage());
        CHECK(Class::by_id(1) == Class::cleric());
        CHECK(Class::by_id(2) == Class::knight());
        CHECK(Class::by_id(3) == Class::barbarian());
        CHECK(Class::by_id(-1) == nullptr);
        CHECK(Class::by_id(4) == nullptr);
    }
    SECTION("name_csv") { CHECK(Class::names_csv() == "mage, cleric, knight, barbarian"); }
    SECTION("table") {
        auto &table = Class::table();
        CHECK(table.size() == 4);
        CHECK(table[0] == Class::mage());
        CHECK(table[1] == Class::cleric());
        CHECK(table[2] == Class::knight());
        CHECK(table[3] == Class::barbarian());
    }
}
TEST_CASE("Classes") {
    SECTION("mage") {
        auto *cl = Class::mage();
        CHECK(cl->id == 0);
        CHECK(cl->name == "mage");
        CHECK(cl->who_name == "Mag");
        CHECK(cl->primary_stat == Stat::Int);
        CHECK(cl->first_weapon_vnum == Objects::SchoolDagger);
        CHECK(cl->guild_room_vnums == std::array<sh_int, MAX_GUILD_ROOMS>{3018, 9618, 10074});
        CHECK(cl->adept_skill_rating == 75);
        CHECK(cl->to_hit_ac_level0 == -1);
        CHECK(cl->to_hit_ac_level32 == -47);
        CHECK(cl->min_hp_gain_on_level == 6);
        CHECK(cl->max_hp_gain_on_level == 8);
        CHECK(cl->mana_gain_on_level_factor == 10);
        CHECK(cl->base_skill_group == "mage basics");
        CHECK(cl->default_skill_group == "mage default");
    }
    SECTION("cleric") {
        auto *cl = Class::cleric();
        CHECK(cl->id == 1);
        CHECK(cl->name == "cleric");
        CHECK(cl->who_name == "Cle");
        CHECK(cl->primary_stat == Stat::Wis);
        CHECK(cl->first_weapon_vnum == Objects::SchoolMace);
        CHECK(cl->guild_room_vnums == std::array<sh_int, MAX_GUILD_ROOMS>{3003, 9619, 10114});
        CHECK(cl->adept_skill_rating == 75);
        CHECK(cl->to_hit_ac_level0 == -1);
        CHECK(cl->to_hit_ac_level32 == -51);
        CHECK(cl->min_hp_gain_on_level == 7);
        CHECK(cl->max_hp_gain_on_level == 10);
        CHECK(cl->mana_gain_on_level_factor == 8);
        CHECK(cl->base_skill_group == "cleric basics");
        CHECK(cl->default_skill_group == "cleric default");
    }
    SECTION("knight") {
        auto *cl = Class::knight();
        CHECK(cl->id == 2);
        CHECK(cl->name == "knight");
        CHECK(cl->who_name == "Kni");
        CHECK(cl->primary_stat == Stat::Str);
        CHECK(cl->first_weapon_vnum == Objects::SchoolSword);
        CHECK(cl->guild_room_vnums == std::array<sh_int, MAX_GUILD_ROOMS>{3028, 9639, 10086});
        CHECK(cl->adept_skill_rating == 75);
        CHECK(cl->to_hit_ac_level0 == -1);
        CHECK(cl->to_hit_ac_level32 == -61);
        CHECK(cl->min_hp_gain_on_level == 10);
        CHECK(cl->max_hp_gain_on_level == 14);
        CHECK(cl->mana_gain_on_level_factor == 6);
        CHECK(cl->base_skill_group == "knight basics");
        CHECK(cl->default_skill_group == "knight default");
    }
    SECTION("barbarian") {
        auto *cl = Class::barbarian();
        CHECK(cl->id == 3);
        CHECK(cl->name == "barbarian");
        CHECK(cl->who_name == "Bar");
        CHECK(cl->primary_stat == Stat::Con);
        CHECK(cl->first_weapon_vnum == Objects::SchoolAxe);
        CHECK(cl->guild_room_vnums == std::array<sh_int, MAX_GUILD_ROOMS>{3022, 9633, 10073});
        CHECK(cl->adept_skill_rating == 75);
        CHECK(cl->to_hit_ac_level0 == -1);
        CHECK(cl->to_hit_ac_level32 == -63);
        CHECK(cl->min_hp_gain_on_level == 13);
        CHECK(cl->max_hp_gain_on_level == 17);
        CHECK(cl->mana_gain_on_level_factor == 3);
        CHECK(cl->base_skill_group == "barbarian basics");
        CHECK(cl->default_skill_group == "barbarian default");
    }
}
