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

#include "Logging.hpp"
#include "merc.h"
#include "string_utils.hpp"
#include "tables.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

// Support function: checks if a string is numeric and in 0<=x<max
// Returns number or -1 if not
int numeric_lookup_check(const char *name, int max) {
    int retVal;
    if (!isdigit(name[0]))
        return -1;
    retVal = atoi(name);
    if (retVal >= 0 && retVal < max)
        return retVal;
    else
        return -1;
}

/*
 * All lookup functions tweaked by TM cos of bad area designers :)
 */
int position_lookup(const char *name) {
    int pos;

    for (pos = 0; position_table[pos].name != nullptr; pos++) {
        if (LOWER(name[0]) == LOWER(position_table[pos].name[0]) && !str_prefix(name, position_table[pos].name))
            return pos;
    }

    // Check for numerics now (now we know how many are in
    // the table)
    pos = numeric_lookup_check(name, pos);
    if (pos >= 0)
        return pos;

    bug("Unknown position '{}' - defaulting!", name);

    return 0;
}

int sex_lookup(const char *name) {
    int sex;

    for (sex = 0; sex_table[sex].name != nullptr; sex++) {
        if (LOWER(name[0]) == LOWER(sex_table[sex].name[0]) && !str_prefix(name, sex_table[sex].name))
            return sex_table[sex].sex;
    }

    sex = numeric_lookup_check(name, sex);
    if (sex >= 0)
        return sex;

    bug("Unknown sex (oo-er) '{}' - defaulting!", name);
    return 0;
}

int size_lookup(const char *name) {
    int size;

    for (size = 0; size_table[size].name != nullptr; size++) {
        if (LOWER(name[0]) == LOWER(size_table[size].name[0]) && !str_prefix(name, size_table[size].name))
            return size;
    }

    size = numeric_lookup_check(name, size);
    if (size >= 0)
        return size;

    bug("Unknown size type '{}' - defaulting!", name);

    return 0;
}

int attack_lookup(const char *name) {
    int att;

    for (att = 0; attack_table[att].name != nullptr; att++) {
        if (LOWER(name[0]) == LOWER(attack_table[att].name[0]) && !str_prefix(name, attack_table[att].name))
            return att;
    }

    att = numeric_lookup_check(name, att);
    if (att >= 0)
        return att;

    bug("Unknown attack type '{}' - defaulting!", name);

    return 0;
}

int item_lookup_impl(const char *name) {
    int type;
    for (type = 0; item_table[type].name != nullptr; type++) {
        if (LOWER(name[0]) == LOWER(item_table[type].name[0]) && !str_prefix(name, item_table[type].name))
            return item_table[type].type;
    }
    // Match on the item type number instead.
    type = numeric_lookup_check(name, type);
    return type;
}

/**
 * Lookup an item type by its type name or type number.
 * Returns a default type if no match is found, which is a bug
 * when this is being used while loading area or player files so
 * it gets logged.
 */
int item_lookup(const char *name) {
    int type = item_lookup_impl(name);
    if (type >= 0)
        return type;
    bug("Unknown item type '{}' - defaulting!", name);
    return item_table[0].type;
}

/**
 * Lookup an item type by its type name or type number.
 * This is stricter than item_lookup() as it doesn't fallback to a default type
 * and will instead return -1 if no match is found.
 */
int item_lookup_strict(const char *name) { return item_lookup_impl(name); }

int liq_lookup(const char *name) {
    int liq;

    for (liq = 0; liq_table[liq].liq_name != nullptr; liq++) {
        if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0]) && !str_prefix(name, liq_table[liq].liq_name))
            return liq;
    }

    liq = numeric_lookup_check(name, liq);
    if (liq >= 0)
        return liq;

    bug("Unknown liquid type '{}' - defaulting!", name);

    return 0;
}

/*
 * Lookup a skill by name.
 */
int skill_lookup(const char *name) {
    int sn;

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        if (is_name(name, skill_table[sn].name))
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

int weapon_type(const char *name) {
    int type;

    for (type = 0; weapon_table[type].name != nullptr; type++) {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0]) && !str_prefix(name, weapon_table[type].name))
            return weapon_table[type].type;
    }

    type = numeric_lookup_check(name, type);
    if (type >= 0)
        return type;

    bug("Unknown weapon type '{}' - defaulting!", name);

    return 0;
}
