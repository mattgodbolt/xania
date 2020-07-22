/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  lookup.c: lookup functions                                           */
/*                                                                       */
/*************************************************************************/
/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@pacinfo.com)				   *
 *	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
 *	    Brian Moore (rom@rom.efn.org)				   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "tables.h"

/*
 * This function is Russ Taylor's creation.  Thanks Russ!
 * All code copyright (C) Russ Taylor, permission to use and/or distribute
 * has NOT been granted.  Use only in this OLC package has been granted.
 */
/*****************************************************************************
 Name:		flag_lookup( flag, table )
 Purpose:	Returns the value of a single, settable flag from the table.
 Called by:	flag_value and flag_string.
 Note:		This function is local and used only in bit.c.
 ****************************************************************************/
int flag_lookup(const char *name, const struct flag_type *flag_table) {
    int flag;

    for (flag = 0; *flag_table[flag].name; flag++) /* OLC 1.1b */
    {
        if (!str_cmp(name, flag_table[flag].name) && flag_table[flag].settable)
            return flag_table[flag].bit;
    }

    return NO_FLAG;
}

/*****************************************************************************
 Name:		flag_value( table, flag )
 Purpose:	Returns the value of the flags entered.  Multi-flags accepted.
 Called by:	olc.c and olc_act.c.
 ****************************************************************************/
int flag_value(const struct flag_type *flag_table, char *argument) {
    char word[MAX_INPUT_LENGTH];
    int bit;
    int marked = 0;
    bool found = FALSE;

    if (is_stat(flag_table)) {
        one_argument(argument, word);

        if ((bit = flag_lookup(word, flag_table)) != NO_FLAG)
            return bit;
        else
            return NO_FLAG;
    }

    /*
     * Accept multiple flags.
     */
    for (;;) {
        argument = one_argument(argument, word);

        if (word[0] == '\0')
            break;

        if ((bit = flag_lookup(word, flag_table)) != NO_FLAG) {
            SET_BIT(marked, bit);
            found = TRUE;
        }
    }

    if (found)
        return marked;
    else
        return NO_FLAG;
}

/*****************************************************************************
 Name:		flag_string( table, flags/stat )
 Purpose:	Returns string with name(s) of the flags or stat entered.
 Called by:	act_olc.c, olc.c, and olc_save.c.
 ****************************************************************************/
char *flag_string(const struct flag_type *flag_table, int bits) {
    static char buf[512];
    int flag;

    buf[0] = '\0';

    for (flag = 0; *flag_table[flag].name; flag++) /* OLC 1.1b */
    {
        if (!is_stat(flag_table) && IS_SET(bits, flag_table[flag].bit)) {
            strcat(buf, " ");
            strcat(buf, flag_table[flag].name);
        } else if (flag_table[flag].bit == bits) {
            strcat(buf, " ");
            strcat(buf, flag_table[flag].name);
            break;
        }
    }
    return (buf[0] != '\0') ? buf + 1 : "none";
}

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

    for (pos = 0; position_table[pos].name != NULL; pos++) {
        if (LOWER(name[0]) == LOWER(position_table[pos].name[0]) && !str_prefix(name, position_table[pos].name))
            return pos;
    }

    // Check for numerics now (now we know how many are in
    // the table)
    pos = numeric_lookup_check(name, pos);
    if (pos >= 0)
        return pos;

    bug("Unknown position '%s' - defaulting!", name);

    return 0;
}

int sex_lookup(const char *name) {
    int sex;

    for (sex = 0; sex_table[sex].name != NULL; sex++) {
        if (LOWER(name[0]) == LOWER(sex_table[sex].name[0]) && !str_prefix(name, sex_table[sex].name))
            return sex_table[sex].sex;
    }

    sex = numeric_lookup_check(name, sex);
    if (sex >= 0)
        return sex;

    bug("Unknown sex (oo-er) '%s' - defaulting!", name);
    return 0;
}

int size_lookup(const char *name) {
    int size;

    for (size = 0; size_table[size].name != NULL; size++) {
        if (LOWER(name[0]) == LOWER(size_table[size].name[0]) && !str_prefix(name, size_table[size].name))
            return size;
    }

    size = numeric_lookup_check(name, size);
    if (size >= 0)
        return size;

    bug("Unknown size type '%s' - defaulting!", name);

    return 0;
}

int attack_lookup(const char *name) {
    int att;

    for (att = 0; attack_table[att].name != NULL; att++) {
        if (LOWER(name[0]) == LOWER(attack_table[att].name[0]) && !str_prefix(name, attack_table[att].name))
            return att;
    }

    att = numeric_lookup_check(name, att);
    if (att >= 0)
        return att;

    bug("Unknown attack type '%s' - defaulting!", name);

    return 0;
}
int item_lookup(const char *name) {
    int type;

    for (type = 0; item_table[type].name != NULL; type++) {
        if (LOWER(name[0]) == LOWER(item_table[type].name[0]) && !str_prefix(name, item_table[type].name))
            return item_table[type].type;
    }

    type = numeric_lookup_check(name, type);
    if (type >= 0)
        return type;

    bug("Unknown item type '%s' - defaulting!", name);

    return item_table[0].type;
}

int liq_lookup(const char *name) {
    int liq;

    for (liq = 0; liq_table[liq].liq_name != NULL; liq++) {
        if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0]) && !str_prefix(name, liq_table[liq].liq_name))
            return liq;
    }

    liq = numeric_lookup_check(name, liq);
    if (liq >= 0)
        return liq;

    bug("Unknown liquid type '%s' - defaulting!", name);

    return 0;
}

int weapon_lookup(const char *name) {
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++) {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0]) && !str_prefix(name, weapon_table[type].name))
            return type;
    }

    type = numeric_lookup_check(name, type);
    if (type >= 0)
        return type;

    bug("Unknown weapon type '%s' - defaulting!", name);

    return 0;
}

/*
 * Lookup a skill by name.
 */
int skill_lookup(const char *name) {
    int sn;

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == NULL)
            break;
        if (LOWER(name[0]) == LOWER(skill_table[sn].name[0]) && !str_prefix(name, skill_table[sn].name))
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
        bug("Slot_lookup: bad slot %d.", slot);
        abort();
    }

    return -1;
}

int weapon_type(const char *name) {
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++) {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0]) && !str_prefix(name, weapon_table[type].name))
            return weapon_table[type].type;
    }

    type = numeric_lookup_check(name, type);
    if (type >= 0)
        return type;

    bug("Unknown weapon type '%s' - defaulting!", name);

    return 0;
}
