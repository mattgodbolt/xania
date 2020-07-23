/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  dbextras.c: many utils, largely intended for OLC                     */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "merc.h"

/*
 * Globals.
 */
extern HELP_DATA *help_first;
extern HELP_DATA *help_last;

extern SHOP_DATA *shop_first;
extern SHOP_DATA *shop_last;

extern CHAR_DATA *char_free;
extern EXTRA_DESCR_DATA *extra_descr_free;
extern OBJ_DATA *obj_free;
extern PC_DATA *pcdata_free;

extern char bug_buf[2 * MAX_INPUT_LENGTH];
extern CHAR_DATA *char_list;
extern char *help_greeting;
extern char log_buf[2 * MAX_INPUT_LENGTH];
extern KILL_DATA kill_table[MAX_LEVEL];
extern OBJ_DATA *object_list;
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;

/*
 * Locals.
 */
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern char *string_hash[MAX_KEY_HASH];

extern AREA_DATA *area_first;
extern AREA_DATA *area_last;

extern char *string_space;
extern char *top_string;
extern char str_empty[1];

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;
extern int mobile_count;
extern int newmobs;
extern int newobjs;

/* The offending bits :-) */

int edit_start_vnum;
int edit_end_vnum;
AREA_DATA *edit_area;

void recurse_object_contents(FILE *fp, OBJ_DATA *obj);

char *const lock_doing[] = {"Opening", "Closing", "Closing and locking"};

char *const lock_dir[] = {"north of", "east of", "south of", "west of", "above", "below"};

extern char *const where_name[];

char size_lookup(int size) {
    switch (size) {
    default: return 'M';
    case SIZE_TINY: return 'T';
    case SIZE_SMALL: return 'S';
    case SIZE_MEDIUM: return 'M';
    case SIZE_LARGE: return 'L';
    case SIZE_HUGE: return 'H';
    case SIZE_GIANT: return 'G';
    }
}

char condition_lookup(int condition) {
    switch (condition) {
    default: return 'P';
    case 100: return 'P';
    case 90: return 'G';
    case 75: return 'A';
    case 50: return 'W';
    case 25: return 'D';
    case 10: return 'B';
    case 0: return 'R';
    }
}

char *strip(char *string) {
    static char buf[MAX_STRING_LENGTH], c;
    char *p = buf;

    for (;;) {
        c = *string++;
        if (c != '\r')
            *p++ = c;
        if (c == '\0')
            break;
    }

    return buf;
}

char *print_flags(const int value) {
    static char buf[36];
    char *b = buf;
    int bit;
    static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef";
    buf[0] = '\0';
    if (value == 0) {
        snprintf(buf, sizeof(buf), "0");
        return (char *)&buf;
    }

    for (bit = 0; bit < 32; bit++) {
        if ((value & (1 << bit)) != 0)
            *b++ = table[bit];
    }
    *b = '\0';
    return (char *)&buf;
}

void fpflags(FILE *fp, int value) { fprintf(fp, "%s", print_flags(value)); }

#define fstring(fp, string) fprintf(fp, "%s~\n", strip(string));

/* NOTE: for OLC this has been superceeded now, please look
 * in olc_room.c
 */
int create_new_room(int lower, int higher) {
    int vnum, door, iHash;
    ROOM_INDEX_DATA *pRoomIndex;

    for (vnum = lower; vnum <= higher; vnum++) {
        if (get_room_index(vnum) == NULL)
            break;
    }
    if (get_room_index(vnum) != NULL)
        return 0;

    pRoomIndex = alloc_perm(sizeof(*pRoomIndex));
    pRoomIndex->people = NULL;
    pRoomIndex->contents = NULL;
    pRoomIndex->extra_descr = NULL;
    pRoomIndex->area = area_last;
    pRoomIndex->vnum = vnum;
    pRoomIndex->name = NULL;
    pRoomIndex->description = NULL;
    pRoomIndex->room_flags = 0;
    pRoomIndex->sector_type = 0;
    pRoomIndex->light = 0;
    for (door = 0; door <= 5; door++)
        pRoomIndex->exit[door] = NULL;

    iHash = vnum % MAX_KEY_HASH;
    pRoomIndex->next = room_index_hash[iHash];
    room_index_hash[iHash] = pRoomIndex;

    return vnum;
}

/* Room and exit data is *not* returned to the memory manager when a room 'dies' */
/* for OLC, this has been superceeded, look in olc_room.c */
int destroy_room(int vnum) {
    int iHash;
    ROOM_INDEX_DATA *pRoomIndex;
    ROOM_INDEX_DATA *prev;
    EXTRA_DESCR_DATA *extras;

    if ((pRoomIndex = get_room_index(vnum)) == NULL)
        return 0;

    iHash = vnum % MAX_KEY_HASH;
    prev = room_index_hash[iHash];
    if (prev == pRoomIndex) {
        room_index_hash[iHash] = pRoomIndex->next;
    } else {
        for (; prev; prev = prev->next) {
            if (prev->next == pRoomIndex) {
                prev->next = pRoomIndex->next;
                break;
            }
        }
        if (prev == NULL) {
            return 0;
        }
    }
    pRoomIndex->next = NULL;
    free_string(pRoomIndex->name);
    free_string(pRoomIndex->description);

    if (pRoomIndex->extra_descr != NULL) {
        for (extras = pRoomIndex->extra_descr; extras != NULL; extras = extras->next) {
            free_string(extras->keyword);
            free_string(extras->description);
        }
        pRoomIndex->extra_descr = NULL;
    }

    return 1;
}

AREA_DATA *area_lookup(char *name) {

    AREA_DATA *a = area_first;

    for (; a; a = a->next) {
        if (!str_cmp(name, a->name))
            return a;
    }
    return 0;
}

void find_limits(AREA_DATA *area, int *lower, int *higher) {
    int room_vnum = 0;
    ROOM_INDEX_DATA *room;
    char buf[10], *ptr;

    for (; room_vnum < 32768; room_vnum++) {
        if ((room = get_room_index(room_vnum)) != NULL) {
            if (room->area == area) {
                snprintf(buf, sizeof(buf), "%d", room_vnum);
                for (ptr = (buf + 2); *ptr; ptr++)
                    *ptr = '0';
                *lower = atoi(buf);
                for (ptr = (buf + 2); *ptr; ptr++)
                    *ptr = '9';
                *higher = atoi(buf);
                return;
            }
        }
    }
    *lower = 0;
    *higher = 0;
    return;
}

int save_whole_area(char *area_name, char *filename) {
    /* OK here goes ... */

    AREA_DATA *save_area = area_lookup(area_name);
    FILE *fp;
    int vnum, door, locks;
    ROOM_INDEX_DATA *pRoomIndex;
    OBJ_INDEX_DATA *pObjIndex;
    EXIT_DATA *pExitData;
    EXTRA_DESCR_DATA *extra;
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    AFFECT_DATA *paf;
    int value[5];
    int lower, higher;

    if (save_area == 0)
        return 0;

    find_limits(save_area, &lower, &higher);

    if ((fp = fopen(filename, "w")) == NULL)
        return 0;

    /*
     *  Okay, we'll deal with each section in a predetermined order.
     *  First up is the #AREA section
     */

    fprintf(fp, "#AREA	%s~\n\n\n", strip(save_area->name));

    /*
     *  Theoretically that deals with that section.
     *  Next section to deal with is the static #ROOMS section
     *  No part of the runtime game database is needed for this part
     */

    fprintf(fp, "#ROOMS\n");

    /*
     *  Now loop through all the possible VNUMs and write all the rooms that
     *  actually exist
     */

    for (vnum = lower; vnum <= higher; vnum++) {

        if ((pRoomIndex = get_room_index(vnum)) == NULL)
            continue;

        fprintf(fp, "#%d\n", vnum); /* Write the VNUM being processed */
        fstring(fp, pRoomIndex->name);
        fstring(fp, pRoomIndex->description);
        fprintf(fp, "00 %d %d\n", pRoomIndex->room_flags, pRoomIndex->sector_type);

        /*
         *  Now check each door exit in turn and write the relevant info
         *
         */

        for (door = 0; door <= 5; door++) {

            if ((pExitData = pRoomIndex->exit[door]) == NULL)
                continue;

            switch ((pExitData->exit_info & (EX_ISDOOR | EX_PICKPROOF))) {
            default: locks = 0; break;
            case EX_ISDOOR: locks = 1; break;
            case (EX_ISDOOR | EX_PICKPROOF): locks = 2; break;
            }

            fprintf(fp, "D%d\n", door);
            fstring(fp, pExitData->description);
            fstring(fp, pExitData->keyword);
            fprintf(fp, "%d %d %d\n", locks, pExitData->key,
                    (pExitData->u1.to_room) ? pExitData->u1.to_room->vnum : pExitData->u1.vnum);
        }

        /*
         *  Scan through the linked list of extra descriptions and write these in also
         */

        for (extra = pRoomIndex->extra_descr; extra != NULL; extra = extra->next) {
            fprintf(fp, "E\n");
            fstring(fp, extra->keyword);
            fstring(fp, extra->description);
        }

        /*
         *  End the room with an 'S'
         */

        fprintf(fp, "S\n");
    } /* loop for each room */

    fprintf(fp, "#0\n\n\n"); /* end #ROOMS section */

    /*
     *  And now onto the #OBJECTS section.  Again this section doesn't use
     *  the runtime database.
     */

    fprintf(fp, "#OBJECTS\n");

    for (vnum = lower; vnum <= higher; vnum++) {

        if ((pObjIndex = get_obj_index(vnum)) == NULL)
            continue;

        fprintf(fp, "#%d\n", vnum);

        fstring(fp, pObjIndex->name);
        fstring(fp, pObjIndex->short_descr);
        fstring(fp, pObjIndex->description);
        fprintf(fp, "~\n"); /* Material ignored AT THE MOMENT */

        fprintf(fp, "%d ", pObjIndex->item_type);
        fpflags(fp, pObjIndex->extra_flags);
        fprintf(fp, " ");
        fpflags(fp, pObjIndex->wear_flags);
        fprintf(fp, "\n");

        /*
         *  We now have to convert skill numbers back into slot numbers
         *  for the appropriate objects.
         */

        value[0] = pObjIndex->value[0];
        value[1] = pObjIndex->value[1];
        value[2] = pObjIndex->value[2];
        value[3] = pObjIndex->value[3];
        value[4] = pObjIndex->value[4];

        switch (pObjIndex->item_type) {
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL: /* and bombs ... */
            value[1] = skill_table[value[1]].slot;
            value[2] = skill_table[value[2]].slot;
            value[3] = skill_table[value[3]].slot;
            value[4] = skill_table[value[4]].slot; /* added 'cos bombs use it I think */
            break;

        case ITEM_STAFF:
        case ITEM_WAND: value[3] = skill_table[value[3]].slot; break;
        }

        fprintf(fp, "%d %d %d %d %d\n", value[0], value[1], value[2], value[3], value[4]);

        fprintf(fp, "%d %d %d %c\n", pObjIndex->level, pObjIndex->weight, pObjIndex->weight,
                condition_lookup(pObjIndex->condition));

        /*
         *  Follow linked list of affects on the object
         */

        for (paf = pObjIndex->affected; paf != NULL; paf = paf->next) {
            fprintf(fp, "A\n");
            fprintf(fp, "%d %d\n", paf->location, paf->modifier);
        }

        /*
         * and now the extra descriptions
         */

        for (extra = pObjIndex->extra_descr; extra != NULL; extra = extra->next) {
            fprintf(fp, "E\n");
            fstring(fp, extra->keyword);
            fstring(fp, extra->description);
        }
    }
    fprintf(fp, "#0\n\n\n"); /* End #OBJECTS section */

    /*
     *  Another static section - #MOBILES
     */

    fprintf(fp, "#MOBILES\n");

    for (vnum = lower; vnum <= higher; vnum++) {

        if ((pMobIndex = get_mob_index(vnum)) == NULL)
            continue;

        fprintf(fp, "#%d\n", vnum);
        fstring(fp, pMobIndex->player_name);
        fstring(fp, pMobIndex->short_descr);
        fstring(fp, pMobIndex->long_descr);
        fstring(fp, pMobIndex->description);
        fstring(fp, race_table[pMobIndex->race].name);

        fpflags(fp, (int)(pMobIndex->act & ~(ACT_IS_NPC | race_table[pMobIndex->race].act)));
        fprintf(fp, " ");
        fpflags(fp, (int)(pMobIndex->affected_by & ~(race_table[pMobIndex->race].aff)));

        fprintf(fp, " %d S\n", pMobIndex->alignment);

        fprintf(fp, "%d %d %dd%d+%d %dd%d+%d %dd%d+%d %d\n", pMobIndex->level, pMobIndex->hitroll,
                pMobIndex->hit[DICE_NUMBER], pMobIndex->hit[DICE_TYPE], pMobIndex->hit[DICE_BONUS],
                pMobIndex->mana[DICE_NUMBER], pMobIndex->mana[DICE_TYPE], pMobIndex->mana[DICE_BONUS],
                pMobIndex->damage[DICE_NUMBER], pMobIndex->damage[DICE_TYPE], pMobIndex->damage[DICE_BONUS],
                pMobIndex->dam_type); /* PHEW! */

        fprintf(fp, "%d %d %d %d\n", pMobIndex->ac[AC_PIERCE] / 10, pMobIndex->ac[AC_BASH] / 10,
                pMobIndex->ac[AC_SLASH] / 10, pMobIndex->ac[AC_EXOTIC] / 10);

        fpflags(fp, (int)(pMobIndex->off_flags & ~(race_table[pMobIndex->race].off)));
        fprintf(fp, " ");
        fpflags(fp, (int)(pMobIndex->imm_flags & ~(race_table[pMobIndex->race].imm)));
        fprintf(fp, " ");
        fpflags(fp, (int)(pMobIndex->res_flags & ~(race_table[pMobIndex->race].res)));
        fprintf(fp, " ");
        fpflags(fp, (int)(pMobIndex->vuln_flags & ~(race_table[pMobIndex->race].vuln)));
        fprintf(fp, "\n");

        fprintf(fp, "%d %d %d %d\n", pMobIndex->start_pos, pMobIndex->default_pos, pMobIndex->sex,
                (int)pMobIndex->gold);

        fpflags(fp, (int)(pMobIndex->form & ~(race_table[pMobIndex->race].form)));
        fprintf(fp, " ");
        fpflags(fp, (int)(pMobIndex->parts & ~(race_table[pMobIndex->race].parts)));
        fprintf(fp, " %c 0\n", size_lookup(pMobIndex->size));
    }
    fprintf(fp, "#0\n\n\n");

    /*
     *  And now .......... the #RESETS section - wholly dependant on the
     *  runtime database and therefore a bit more tricky
     */

    fprintf(fp, "#RESETS\n* #MOBILES section:\n");

    /*
     *  Firstly scan through each active mobile ... put it in its room and
     *  equip it accordingly
     */

    for (ch = char_list; ch != NULL; ch = ch->next) {

        if (!IS_NPC(ch))
            continue; /* should only happen with IMMs but you never know :-) */
        if (ch->in_room->area != save_area)
            continue; /* only save data we're interested in, after all! */

        /*
         * So we've found an NPC somewhere in the MUD ... lets put it in its
         * place :
         */

        fprintf(fp, "* %s in %s\n", ch->pIndexData->short_descr, ch->in_room->name);

        fprintf(fp, "M 0 %d %d %d\n", ch->pIndexData->vnum, ch->pIndexData->count, ch->in_room->vnum);

        /*
         * The mob has been loaded in appropriately then ... now let's equip it
         * accordingly :
         */

        for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {

            if (obj->wear_loc == WEAR_NONE) {
                /* G <number> <object-vnum> <limit-number> */
                fprintf(fp, "G 1 %d %d		<in inventory>      %s\n", obj->pIndexData->vnum,
                        obj->pIndexData->count, obj->short_descr);
            } else {
                /* E <number> <object-vnum> <limit-number> <waear_loc-number> */
                fprintf(fp, "E 1 %d %d %d		%s%s\n", obj->pIndexData->vnum, obj->pIndexData->count,
                        obj->wear_loc, where_name[obj->wear_loc], obj->short_descr);
            }
            recurse_object_contents(fp, obj);
        }
        fprintf(fp, "\n"); /* for aesthetic purposes :-) */
    }

    fprintf(fp, "* #OBJECTS section:\n");
    for (obj = object_list; obj != NULL; obj = obj->next) {

        if ((pRoomIndex = obj->in_room) == NULL)
            continue;
        if ((obj->in_room->area != save_area))
            continue;

        /*
         *  Okay this is a 'root' object - place it in a location and check
         *  recursively for any contents
         */

        fprintf(fp, "O 0 %d %d %d		%s in %s", obj->pIndexData->vnum, obj->pIndexData->count,
                obj->in_room->vnum, obj->short_descr, obj->in_room->name);

        recurse_object_contents(fp, obj);
        fprintf(fp, "\n");
    }

    /*
     *  Close and lock relevant doors
     */

    fprintf(fp, "\n* Doors:\n");

    for (vnum = lower; vnum <= higher; vnum++) {

        if ((pRoomIndex = get_room_index(vnum)) == NULL)
            continue;

        for (door = 0; door <= 5; door++) {

            if (pRoomIndex->exit[door] == NULL)
                continue;

            locks = 0;
            if (IS_SET(pRoomIndex->exit[door]->exit_info, EX_CLOSED))
                locks++;
            if (IS_SET(pRoomIndex->exit[door]->exit_info, EX_LOCKED))
                locks++;

            if (locks != 0) {
                fprintf(fp, "D 0 %d %d %d		%s door %s in '%s'\n", vnum, door, locks, lock_doing[locks],
                        lock_dir[door], pRoomIndex->name);
            }
        }
    }
    /* Phew - that's all the implemented stuff so far :-) */
    fprintf(fp, "S\n\n\n");

    fprintf(fp, "#$\n\n\nThis area was generated using XROMArea  1995 Matthew Godbolt\nFor more information contact "
                "M.R.Godbolt@exeter.ac.uk\n");
    fclose(fp);

    return 1;
}

void recurse_object_contents(FILE *fp, OBJ_DATA *obj) {

    OBJ_DATA *inside;

    if (obj->contains == NULL)
        return;

    for (inside = obj->contains; inside != NULL; inside = inside->next_content) {

        fprintf(fp, "P 1 %d %d %d		%s in %s\n", inside->pIndexData->vnum, inside->pIndexData->count,
                obj->pIndexData->vnum, inside->short_descr, inside->short_descr);

        if (obj->contains != NULL)
            recurse_object_contents(fp, inside); /* recursion !!! */
    }
}

/* IMPLEMENTOR NOTES :-

  (1) in int_create_room - update to cater for AAxxx areas.
  (2) No materials or SPEC_FUNs catered for yet
  (3) Bombs need to be put in as do all new spells
  (4) Need to implement ..->pIndexData->count in editor to allow
      restrictions - ..->count must reflect MAXIMUM ALLOWABLE instances of
      an object/mobile

*/

/* Written to eradicate objects from the mud permanently! Faramir */
/* for OLC I have replaced this now....look in olc_obj.c          */

int _delete_obj(int vnum) {

    int iHash;
    OBJ_INDEX_DATA *prev;
    OBJ_INDEX_DATA *pObjIndex;
    EXTRA_DESCR_DATA *extras;

    if ((pObjIndex = get_obj_index(vnum)) == NULL)
        return 0;

    iHash = vnum % MAX_KEY_HASH;
    prev = obj_index_hash[iHash];
    if (prev == pObjIndex) {
        obj_index_hash[iHash] = pObjIndex->next;
    } else {
        for (; prev; prev = prev->next) {
            if (prev->next == pObjIndex) {
                prev->next = pObjIndex->next;
                break;
            }
        }
        if (prev == NULL) {
            return 0;
        }
    }
    pObjIndex->next = NULL;
    free_string(pObjIndex->name);
    free_string(pObjIndex->short_descr);
    free_string(pObjIndex->description);

    if (pObjIndex->extra_descr != NULL) {
        for (extras = pObjIndex->extra_descr; extras != NULL; extras = extras->next) {

            free_string(extras->keyword);
            free_string(extras->description);
        }
        pObjIndex->extra_descr = NULL;
    }

    return 1;
}
