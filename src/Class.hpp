/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Constants.hpp"
#include "Stats.hpp"
#include "Types.hpp"

#include <array>
#include <string_view>

// Constants for player character classes.
struct Class {
    sh_int id;
    std::string_view name;
    std::string_view who_name;
    Stat primary_stat;
    sh_int first_weapon_vnum;
    std::array<sh_int, MAX_GUILD_ROOMS> guild_room_vnums;
    sh_int adept_skill_rating;
    sh_int to_hit_ac_level0; // to hit armour class for level
    sh_int to_hit_ac_level32; // to hit armour class for level 32
    sh_int min_hp_gain_on_level;
    sh_int max_hp_gain_on_level;
    sh_int mana_gain_on_level_factor;
    std::string_view base_skill_group;
    std::string_view default_skill_group;

    [[nodiscard]] static const Class *by_id(sh_int index);
    [[nodiscard]] static const Class *by_name(std::string_view name);
    [[nodiscard]] static std::string names_csv();
    [[nodiscard]] static const Class *mage();
    [[nodiscard]] static const Class *cleric();
    [[nodiscard]] static const Class *knight();
    [[nodiscard]] static const Class *barbarian();
    [[nodiscard]] static const std::array<Class const *, MAX_CLASS> &table();
};
