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
    const char *name; /* the full name of the class */
    char who_name[4]; /* Three-letter name for 'who'  */
    Stat attr_prime; /* Prime attribute              */
    sh_int weapon; /* First weapon                 */
    sh_int guild[MAX_GUILD]; /* Vnum of guild rooms          */
    sh_int skill_adept; /* Maximum skill level          */
    sh_int to_hit_ac_level0; /* to hit armour class for level  0 */
    sh_int to_hit_ac_level32; /* to hit armour class for for level 32 */
    sh_int hp_min; /* Min hp gained on leveling    */
    sh_int hp_max; /* Max hp gained on leveling    */
    sh_int fMana; /* Class gains mana on level    */
    const char *base_group; /* base skills gained           */
    const char *default_group; /* default skills gained        */
};

extern const struct class_type class_table[];
