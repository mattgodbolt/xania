/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Constants.hpp"
#include "Stats.hpp"
#include "Types.hpp"

#include <string_view>

/*
 * Per-class stuff.
 */
struct class_type {
    std::string_view name;
    std::string_view who_name;
    Stat primary_stat;
    sh_int first_weapon_vnum;
    sh_int guild_room_vnums[MAX_GUILD_ROOMS];
    sh_int adept_skill_rating;
    sh_int to_hit_ac_level0; // to hit armour class for level
    sh_int to_hit_ac_level32; // to hit armour class for level 32
    sh_int min_hp_gain_on_level;
    sh_int max_hp_gain_on_level;
    sh_int mana_gain_on_level_factor;
    std::string_view base_skill_group;
    std::string_view default_skill_group;
};

extern const struct class_type class_table[];
