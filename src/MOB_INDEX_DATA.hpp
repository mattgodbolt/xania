#pragma once

#include "Types.hpp"

#include <string>

struct SHOP_DATA;
struct MPROG_DATA;
struct AREA_DATA;

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct MOB_INDEX_DATA {
    MOB_INDEX_DATA *next;
    SpecialFunc spec_fun;
    SHOP_DATA *pShop;
    sh_int vnum;
    sh_int count;
    sh_int killed;
    std::string player_name;
    std::string short_descr;
    std::string long_descr;
    std::string description;
    unsigned long act;
    unsigned long affected_by;
    sh_int alignment;
    sh_int group; /* rom-2.4 style mob groupings */
    sh_int level;
    sh_int hitroll;
    sh_int hit[3];
    sh_int mana[3];
    sh_int damage[3];
    sh_int ac[4];
    sh_int dam_type;
    long off_flags;
    long imm_flags;
    long res_flags;
    long vuln_flags;
    sh_int start_pos;
    sh_int default_pos;
    sh_int sex;
    sh_int race;
    long gold;
    long form;
    long parts;
    sh_int size;
    sh_int material;
    MPROG_DATA *mobprogs; /* Used by MOBprogram */
    int progtypes; /* Used by MOBprogram */

    AREA_DATA *area;
};
