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

#include <magic_enum.hpp>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

// Support function: checks if a string is numeric and in 0<=x<max
// Returns number or -1 if not
int numeric_lookup_check(std::string_view name, const int max) {
    if (!is_number(name))
        return -1;
    const auto retVal = parse_number(name);
    if (retVal >= 0 && retVal < max)
        return retVal;
    else
        return -1;
}

int attack_lookup(const char *name) {
    int att;

    for (att = 0; attack_table[att].name != nullptr; att++) {
        if (tolower(name[0]) == tolower(attack_table[att].name[0]) && !str_prefix(name, attack_table[att].name))
            return att;
    }
    bug("Load_objects: Unknown attack: {}", name);
    exit(1);
}

/*
 * Lookup a skill by name.
 */
int skill_lookup(const char *name) {
    int sn;

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        if (tolower(name[0]) == tolower(skill_table[sn].name[0]) && !str_prefix(name, skill_table[sn].name))
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
