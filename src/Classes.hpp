/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Constants.hpp"
#include "Stats.hpp"
#include "Types.hpp"

/*
 * Per-class stuff.
 */
static constexpr inline auto MAX_GUILD = 3;
struct class_type {
    const char *name;
    char who_name[4];
    Stat primary_stat;
    sh_int first_weapon_vnum;
    sh_int guild_room_vnums[MAX_GUILD];
    sh_int adept_skill_rating;
    sh_int to_hit_ac_level0; // to hit armour class for level
    sh_int to_hit_ac_level32; // to hit armour class for level 32
    sh_int min_hp_gain_on_level;
    sh_int max_hp_gain_on_level;
    sh_int mana_gain_on_level_factor;
    const char *base_skill_group;
    const char *default_skill_group;
};

extern const struct class_type class_table[];
