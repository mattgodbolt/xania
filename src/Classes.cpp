/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Classes.hpp"
#include "Constants.hpp"
#include "Stats.hpp"
#include "VnumObjects.hpp"

/*
 * Class table.
 */
const struct class_type class_table[MAX_CLASS] = {{"mage",
                                                   "Mag",
                                                   Stat::Int,
                                                   Objects::SchoolDagger,
                                                   {3018, 9618, 10074},
                                                   75,
                                                   -1,
                                                   -47,
                                                   6,
                                                   8,
                                                   10,
                                                   "mage basics",
                                                   "mage default"},

                                                  {"cleric",
                                                   "Cle",
                                                   Stat::Wis,
                                                   Objects::SchoolMace,
                                                   {3003, 9619, 10114},
                                                   75,
                                                   -1,
                                                   -51,
                                                   7,
                                                   10,
                                                   8,
                                                   "cleric basics",
                                                   "cleric default"},

                                                  {"knight",
                                                   "Kni",
                                                   Stat::Str,
                                                   Objects::SchoolSword,
                                                   {3028, 9639, 10086},
                                                   75,
                                                   -1,
                                                   -61,
                                                   10,
                                                   14,
                                                   6,
                                                   "knight basics",
                                                   "knight default"},

                                                  {"barbarian",
                                                   "Bar",
                                                   Stat::Con,
                                                   Objects::SchoolAxe,
                                                   {3022, 9633, 10073},
                                                   75,
                                                   -1,
                                                   -63,
                                                   13,
                                                   17,
                                                   3,
                                                   "barbarian basics",
                                                   "barbarian default"}};
