/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  lookup.c: lookup functions                                           */
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

#include "Attacks.hpp"
#include "Logging.hpp"
#include "Materials.hpp"
#include "ObjectType.hpp"
#include "Position.hpp"
#include "SkillTables.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <magic_enum/magic_enum.hpp>

/*
 * Lookup a skill by name.
 */
int skill_lookup(std::string_view name) {
    int sn;

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        if (tolower(name[0]) == tolower(skill_table[sn].name[0]) && matches_start(name, skill_table[sn].name))
            return sn;
    }

    // skill_lookup relies on -1 being returned - so no moogification here

    return -1;
}

/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup(int slot) {
    extern bool fBootDb;
    int sn;

    if (slot <= 0)
        return -1;

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (slot == skill_table[sn].slot)
            return sn;
    }

    if (fBootDb) {
        bug("Slot_lookup: bad slot {}.", slot);
        abort();
    }

    return -1;
}
