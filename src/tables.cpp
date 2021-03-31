/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  handler.c:  a core module with a huge amount of utility functions    */
/*                                                                       */
/*************************************************************************/
/***************************************************************************
 * ROM 2.4 is copyright 1993-1996 Russ Taylor
 * ROM has been brought to you by the ROM consortium
 *     Russ Taylor (rtaylor@pacinfo.com)
 *     Gabrielle Taylor (gtaylor@pacinfo.com)
 *     Brian Moore (rom@rom.efn.org)
 * By using this code, you have agreed to follow the terms of the
 * ROM license, in the file Rom24/doc/rom.license
 ***************************************************************************/

#include "tables.h"
#include "VnumObjects.hpp"
#include "merc.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

struct flag_stat_type {
    const struct flag_type *structure;
    bool stat;
};

/* for position */
const struct position_type position_table[] = {{"dead", "dead"},           {"mortally wounded", "mort"},
                                               {"incapacitated", "incap"}, {"stunned", "stun"},
                                               {"sleeping", "sleep"},      {"resting", "rest"},
                                               {"sitting", "sit"},         {"fighting", "fight"},
                                               {"standing", "stand"},      {nullptr, nullptr}};

/* for sizes */
const struct size_type size_table[] = {{"tiny"},
                                       {"small"},
                                       {"medium"},
                                       {"large"},
                                       {
                                           "huge",
                                       },
                                       {"giant"},
                                       {nullptr}};

const struct item_type item_table[] = {{ITEM_LIGHT, "light"},
                                       {ITEM_SCROLL, "scroll"},
                                       {ITEM_WAND, "wand"},
                                       {ITEM_STAFF, "staff"},
                                       {ITEM_WEAPON, "weapon"},
                                       {ITEM_TREASURE, "treasure"},
                                       {ITEM_ARMOR, "armor"},
                                       {ITEM_POTION, "potion"},
                                       {ITEM_CLOTHING, "clothing"},
                                       {ITEM_FURNITURE, "furniture"},
                                       {ITEM_TRASH, "trash"},
                                       {ITEM_CONTAINER, "container"},
                                       {ITEM_DRINK_CON, "drink"},
                                       {ITEM_KEY, "key"},
                                       {ITEM_FOOD, "food"},
                                       {ITEM_MONEY, "money"},
                                       {ITEM_BOAT, "boat"},
                                       {ITEM_CORPSE_NPC, "npc_corpse"},
                                       {ITEM_CORPSE_PC, "pc_corpse"},
                                       {ITEM_FOUNTAIN, "fountain"},
                                       {ITEM_PILL, "pill"},
                                       {ITEM_MAP, "map"},
                                       {ITEM_PORTAL, "portal"},
                                       {ITEM_BOMB, "bomb"},
                                       {0, nullptr}};

const struct weapon_type weapon_table[] = {{"sword", objects::SchoolSword, WEAPON_SWORD, &gsn_sword},
                                           {"mace", objects::SchoolMace, WEAPON_MACE, &gsn_mace},
                                           {"dagger", objects::SchoolDagger, WEAPON_DAGGER, &gsn_dagger},
                                           {"axe", objects::SchoolAxe, WEAPON_AXE, &gsn_axe},

                                           {"exotic", objects::SchoolSword, WEAPON_EXOTIC, nullptr},
                                           {"spear", objects::SchoolMace, WEAPON_SPEAR, &gsn_spear},
                                           {"flail", objects::SchoolDagger, WEAPON_FLAIL, &gsn_flail},
                                           {"whip", objects::SchoolAxe, WEAPON_WHIP, &gsn_whip},
                                           {"polearm", objects::SchoolAxe, WEAPON_POLEARM, &gsn_polearm},
                                           {nullptr, 0, 0, nullptr}};
