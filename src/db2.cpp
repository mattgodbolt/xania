/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db2.c: extra database utility functions                              */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "Logging.hpp"
#include "db.h"
#include "lookup.h"
#include "merc.h"
#include "string_utils.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

void mprog_read_programs(FILE *fp, MOB_INDEX_DATA *pMobIndex);

/* Sets vnum range for area when loading its constituent mobs/objects/rooms */
void assign_area_vnum(int vnum) {
    if (area_last->lvnum == 0 || area_last->uvnum == 0)
        area_last->lvnum = area_last->uvnum = vnum;
    if (vnum != URANGE(area_last->lvnum, vnum, area_last->uvnum)) {
        if (vnum < area_last->lvnum)
            area_last->lvnum = vnum;
        else
            area_last->uvnum = vnum;
    }
}

/* values for db2.c */
struct social_type social_table[MAX_SOCIALS];
int social_count = 0;

/* snarf a socials file */
void load_socials(FILE *fp) {
    for (;;) {
        struct social_type social;
        char *temp;
        /* clear social */
        social.char_no_arg = nullptr;
        social.others_no_arg = nullptr;
        social.char_found = nullptr;
        social.others_found = nullptr;
        social.vict_found = nullptr;
        social.char_auto = nullptr;
        social.others_auto = nullptr;

        temp = fread_word(fp);
        if (!strcmp(temp, "#0"))
            return; /* done */

        strcpy(social.name, temp);
        fread_to_eol(fp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_no_arg = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_no_arg = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_found = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_found = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.others_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.vict_found = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.vict_found = temp;

        temp = fread_string_eol(fp);
        // MRG char_not_found wasn't used anywhere
        if (!strcmp(temp, "$")) {
            /*social.char_not_found = nullptr*/;
        } else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_auto = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.char_auto = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_auto = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
    }
}

/*
 * Snarf a mob section.  new style
 */
void load_mobiles(FILE *fp) {
    if (!area_last) {
        bug("Load_mobiles: no #AREA seen yet!");
        exit(1);
    }
    for (;;) {
        sh_int vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_mobiles: # not found.");
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        if (get_mob_index(vnum) != nullptr) {
            bug("Load_mobiles: vnum {} duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        auto *pMobIndex = new MOB_INDEX_DATA();
        pMobIndex->vnum = vnum;
        pMobIndex->area = area_last;
        newmobs++;
        pMobIndex->player_name = fread_string(fp);
        pMobIndex->short_descr = lower_case_articles(fread_stdstring(fp));
        pMobIndex->long_descr = upper_first_character(fread_stdstring(fp));
        pMobIndex->description = upper_first_character(fread_stdstring(fp));
        pMobIndex->race = race_lookup(fread_string(fp));

        pMobIndex->act = fread_flag(fp) | ACT_IS_NPC | race_table[pMobIndex->race].act;
        pMobIndex->affected_by = fread_flag(fp) | race_table[pMobIndex->race].aff;
        pMobIndex->pShop = nullptr;
        pMobIndex->alignment = fread_number(fp);

        pMobIndex->group = fread_number(fp);

        pMobIndex->level = fread_number(fp);
        pMobIndex->hitroll = fread_number(fp);

        /* read hit dice */
        pMobIndex->hit[DICE_NUMBER] = fread_number(fp);
        /* 'd'          */ fread_letter(fp);
        pMobIndex->hit[DICE_TYPE] = fread_number(fp);
        /* '+'          */ fread_letter(fp);
        pMobIndex->hit[DICE_BONUS] = fread_number(fp);

        /* read mana dice */
        pMobIndex->mana[DICE_NUMBER] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->mana[DICE_TYPE] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->mana[DICE_BONUS] = fread_number(fp);

        /* read damage dice */
        pMobIndex->damage[DICE_NUMBER] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->damage[DICE_TYPE] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->damage[DICE_BONUS] = fread_number(fp);
        pMobIndex->dam_type = attack_lookup(fread_word(fp));

        /* read armor class */
        pMobIndex->ac[AC_PIERCE] = fread_number(fp) * 10;
        pMobIndex->ac[AC_BASH] = fread_number(fp) * 10;
        pMobIndex->ac[AC_SLASH] = fread_number(fp) * 10;
        pMobIndex->ac[AC_EXOTIC] = fread_number(fp) * 10;

        /* read flags and add in data from the race table */
        pMobIndex->off_flags = fread_flag(fp) | race_table[pMobIndex->race].off;
        pMobIndex->imm_flags = fread_flag(fp) | race_table[pMobIndex->race].imm;
        pMobIndex->res_flags = fread_flag(fp) | race_table[pMobIndex->race].res;
        pMobIndex->vuln_flags = fread_flag(fp) | race_table[pMobIndex->race].vuln;

        /* vital statistics */
        pMobIndex->start_pos = position_lookup(fread_word(fp));
        pMobIndex->default_pos = position_lookup(fread_word(fp));
        pMobIndex->sex = sex_lookup(fread_word(fp));
        pMobIndex->gold = fread_number(fp);

        pMobIndex->form = fread_flag(fp) | race_table[pMobIndex->race].form;
        pMobIndex->parts = fread_flag(fp) | race_table[pMobIndex->race].parts;
        /* size */
        pMobIndex->size = size_lookup(fread_word(fp));
        pMobIndex->material = material_lookup(fread_word(fp));

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'F') {
                char *word;
                long vector;

                word = fread_word(fp);
                vector = fread_flag(fp);

                if (!str_prefix(word, "act")) {
                    REMOVE_BIT(pMobIndex->act, vector);
                } else if (!str_prefix(word, "aff")) {
                    REMOVE_BIT(pMobIndex->affected_by, vector);

                } else if (!str_prefix(word, "off")) {
                    REMOVE_BIT(pMobIndex->off_flags, vector);
                } else if (!str_prefix(word, "imm")) {
                    REMOVE_BIT(pMobIndex->imm_flags, vector);
                } else if (!str_prefix(word, "res")) {
                    REMOVE_BIT(pMobIndex->res_flags, vector);
                } else if (!str_prefix(word, "vul")) {
                    REMOVE_BIT(pMobIndex->vuln_flags, vector);
                } else if (!str_prefix(word, "for")) {
                    REMOVE_BIT(pMobIndex->form, vector);
                } else if (!str_prefix(word, "par")) {

                    REMOVE_BIT(pMobIndex->parts, vector);
                } else {
                    bug("Flag remove: flag '{}' not found.", word);
                    exit(1);
                }
            } else {
                {
                    ungetc(letter, fp);
                    break;
                }
            }

            if (letter != 'S') {
                bug("Load_mobiles: vnum {} non-S.", vnum);
                exit(1);
            }
        }

        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        letter = fread_letter(fp);
        if (letter == '>') {
            ungetc(letter, fp);
            mprog_read_programs(fp, pMobIndex);
        } else
            ungetc(letter, fp);

        iHash = vnum % MAX_KEY_HASH;
        pMobIndex->next = mob_index_hash[iHash];
        mob_index_hash[iHash] = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
        assign_area_vnum(vnum);
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL - 1)].number++;
    }
}

/*
 * Snarf an obj section. new style
 */
void load_objects(FILE *fp) {
    OBJ_INDEX_DATA *pObjIndex;
    char temp; /* Used for Death's Wear Strings bit */

    if (!area_last) {
        bug("Load_objects: no #AREA section found yet!");
        exit(1);
    }

    for (;;) {
        sh_int vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_objects: # not found.");
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        if (get_obj_index(vnum) != nullptr) {
            bug("Load_objects: vnum {} duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pObjIndex = new OBJ_INDEX_DATA;
        pObjIndex->vnum = vnum;
        pObjIndex->area = area_last;
        pObjIndex->reset_num = 0;
        newobjs++;
        pObjIndex->name = fread_stdstring(fp);
        /*
         * MG added - snarf short descrips to kill:
         * You hit The beastly fido
         */
        // str_dup is temporary until short_descr of objects is a std::string
        pObjIndex->short_descr = str_dup(lower_case_articles(fread_stdstring(fp)).c_str());
        pObjIndex->description = fread_stdstring(fp);
        if (pObjIndex->description.empty()) {
            bug("Load_objects: empty long description in object {}.", vnum);
        }
        pObjIndex->material = material_lookup(fread_string(fp));

        pObjIndex->item_type = item_lookup(fread_word(fp));

        pObjIndex->extra_flags = fread_flag(fp);
        pObjIndex->wear_flags = fread_flag(fp);

        temp = fread_letter(fp);
        if (temp == ',') {
            pObjIndex->wear_string = fread_stdstring(fp);
        } else {
            ungetc(temp, fp);
        }

        switch (pObjIndex->item_type) {
        case ITEM_WEAPON:
            pObjIndex->value[0] = weapon_type(fread_word(fp));
            pObjIndex->value[1] = fread_number(fp);
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = attack_lookup(fread_word(fp));
            pObjIndex->value[4] = fread_flag(fp);
            break;
        case ITEM_CONTAINER:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_flag(fp);
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = fread_number(fp);
            pObjIndex->value[4] = fread_number(fp);
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_number(fp);
            pObjIndex->value[2] = liq_lookup(fread_word(fp));
            pObjIndex->value[3] = fread_number(fp);
            pObjIndex->value[4] = fread_number(fp);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_number(fp);
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = fread_spnumber(fp);
            pObjIndex->value[4] = fread_number(fp);
            break;
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_BOMB:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_spnumber(fp);
            pObjIndex->value[2] = fread_spnumber(fp);
            pObjIndex->value[3] = fread_spnumber(fp);
            pObjIndex->value[4] = fread_spnumber(fp);
            break;
        default:
            pObjIndex->value[0] = fread_flag(fp);
            pObjIndex->value[1] = fread_flag(fp);
            pObjIndex->value[2] = fread_flag(fp);
            pObjIndex->value[3] = fread_flag(fp);
            pObjIndex->value[4] = fread_flag(fp);
            break;
        }

        pObjIndex->level = fread_number(fp);
        pObjIndex->weight = fread_number(fp);
        pObjIndex->cost = fread_number(fp);

        /* condition */
        letter = fread_letter(fp);
        switch (letter) {
        case ('P'): pObjIndex->condition = 100; break;
        case ('G'): pObjIndex->condition = 90; break;
        case ('A'): pObjIndex->condition = 75; break;
        case ('W'): pObjIndex->condition = 50; break;
        case ('D'): pObjIndex->condition = 25; break;
        case ('B'): pObjIndex->condition = 10; break;
        case ('R'): pObjIndex->condition = 0; break;
        default: pObjIndex->condition = 100; break;
        }

        for (;;) {
            char letter;

            letter = fread_letter(fp);

            if (letter == 'A') {
                AFFECT_DATA af;
                af.type = -1;
                af.level = pObjIndex->level;
                af.duration = -1;
                af.location = static_cast<AffectLocation>(fread_number(fp));
                af.modifier = fread_number(fp);
                pObjIndex->affected.add(af);
                top_affect++;
            }

            else if (letter == 'E') {
                EXTRA_DESCR_DATA *ed;

                ed = static_cast<EXTRA_DESCR_DATA *>(alloc_perm(sizeof(*ed)));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
                top_ed++;
            }

            else {
                ungetc(letter, fp);
                break;
            }
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch (pObjIndex->item_type) {
        case ITEM_BOMB:
            pObjIndex->value[4] = slot_lookup(pObjIndex->value[4]);
            // fall through
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            pObjIndex->value[1] = slot_lookup(pObjIndex->value[1]);
            pObjIndex->value[2] = slot_lookup(pObjIndex->value[2]);
            pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
            break;

        case ITEM_STAFF:
        case ITEM_WAND: pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]); break;
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
        assign_area_vnum(vnum);
    }
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
