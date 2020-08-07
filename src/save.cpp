/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  save.c:  data saving routines                                        */
/*                                                                       */
/*************************************************************************/

#include "Descriptor.hpp"
#include "merc.h"
#include "news.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

char *login_from;
char *login_at;

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST 100
static OBJ_DATA *rgObjNest[MAX_NEST];

extern void char_ride(CHAR_DATA *ch, CHAR_DATA *pet);

/*
 * Local functions.
 */
void fwrite_char(CHAR_DATA *ch, FILE *fp);
void fwrite_obj(CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest);
void fwrite_pet(CHAR_DATA *ch, CHAR_DATA *pet, FILE *fp);
void fread_char(CHAR_DATA *ch, FILE *fp);
void fread_pet(CHAR_DATA *ch, FILE *fp);
void fread_obj(CHAR_DATA *ch, FILE *fp);

char *extra_bit_string(CHAR_DATA *ch) {
    static char buf[MAX_STRING_LENGTH]; /* NB not re-entrant :) */
    int n;
    buf[0] = '\0';
    for (n = 0; n < MAX_EXTRA_FLAGS; n++) {
        if (is_set_extra(ch, n)) {
            strcat(buf, "1");
        } else {
            strcat(buf, "0");
        }
    }
    return (char *)&buf;
}

void set_bits_from_pfile(CHAR_DATA *ch, FILE *fp) {
    int n;
    char c;
    for (n = 0; n <= MAX_EXTRA_FLAGS; n++) {
        c = fread_letter(fp);
        if (c == '0')
            continue;
        if (c == '1') {
            set_extra(ch, n);
        } else {
            break;
        }
    }
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj(CHAR_DATA *ch) {
    char strsave[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    if (IS_NPC(ch))
        return;

    if (ch->desc != nullptr && ch->desc->original != nullptr)
        ch = ch->desc->original;

    /* create god log */
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL) {
        fclose(fpReserve);
        snprintf(strsave, sizeof(strsave), "%s%s", GOD_DIR, capitalize(ch->name));
        if ((fp = fopen(strsave, "w")) == nullptr) {
            bug("Save_char_obj: fopen");
            perror(strsave);
        }

        fprintf(fp, "Lev %2d Trust %2d  %s%s\n", ch->level, get_trust(ch), ch->name, ch->pcdata->title);
        fclose(fp);
        fpReserve = fopen(NULL_FILE, "r");
    }

    fclose(fpReserve);
    snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, capitalize(ch->name));
    if ((fp = fopen(PLAYER_TEMP, "w")) == nullptr) {
        bug("Save_char_obj: fopen");
        perror(strsave);
    } else {
        fwrite_char(ch, fp);
        if (ch->carrying != nullptr)
            fwrite_obj(ch, ch->carrying, fp, 0);
        /* save the pets */
        if (ch->pet != nullptr && ch->pet->in_room == ch->in_room)
            fwrite_pet(ch, ch->pet, fp);
        fprintf(fp, "#END\n");
    }
    fclose(fp);
    /* move the file */
    snprintf(buf, sizeof(buf), "mv %s %s", PLAYER_TEMP, strsave);
    if (system(buf) != 0) {
        bug("Unable to move temporary player name %s!! save failed!", strsave);
    }
    fpReserve = fopen(NULL_FILE, "r");
}

/*
 * Write the char.
 */
void fwrite_char(CHAR_DATA *ch, FILE *fp) {
    AFFECT_DATA *paf;
    int sn, gn;
    char hostbuf[MAX_MASKED_HOSTNAME];

    fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", ch->name);
    fprintf(fp, "Vers %d\n", 3);
    if (ch->short_descr[0] != '\0')
        fprintf(fp, "ShD  %s~\n", ch->short_descr);
    if (ch->long_descr[0] != '\0')
        fprintf(fp, "LnD  %s~\n", ch->long_descr);
    if (ch->description[0] != '\0')
        fprintf(fp, "Desc %s~\n", ch->description);
    fprintf(fp, "Race %s~\n", pc_race_table[ch->race].name);
    fprintf(fp, "Sex  %d\n", ch->sex);
    fprintf(fp, "Cla  %d\n", ch->class_num);
    fprintf(fp, "Levl %d\n", ch->level);
    if ((ch->pcdata) && (ch->pcdata->pcclan)) {
        fprintf(fp, "Clan %d\n", (int)ch->pcdata->pcclan->clan->clanchar);
        fprintf(fp, "CLevel %d\n", ch->pcdata->pcclan->clanlevel);
        fprintf(fp, "CCFlags %d\n", ch->pcdata->pcclan->channelflags);
    }
    if (ch->trust != 0)
        fprintf(fp, "Tru  %d\n", ch->trust);
    fprintf(fp, "Plyd %d\n", ch->played + (int)(current_time - ch->logon));
    fprintf(fp, "Note %d\n", (int)ch->last_note);
    fprintf(fp, "Scro %d\n", ch->lines);
    fprintf(fp, "Room %d\n",
            (ch->in_room == get_room_index(ROOM_VNUM_LIMBO) && ch->was_in_room != nullptr)
                ? ch->was_in_room->vnum
                : ch->in_room == nullptr ? 3001 : ch->in_room->vnum);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move);
    if (ch->gold > 0)
        fprintf(fp, "Gold %ld\n", ch->gold);
    else
        fprintf(fp, "Gold %d\n", 0);
    fprintf(fp, "Exp  %ld\n", ch->exp);
    if (ch->act != 0)
        fprintf(fp, "Act  %ld\n", ch->act);
    if (ch->affected_by != 0)
        fprintf(fp, "AfBy %d\n", ch->affected_by);
    fprintf(fp, "Comm %ld\n", ch->comm);
    if (ch->invis_level != 0)
        fprintf(fp, "Invi %d\n", ch->invis_level);
    fprintf(fp, "Pos  %d\n", ch->position == POS_FIGHTING ? POS_STANDING : ch->position);
    if (ch->practice != 0)
        fprintf(fp, "Prac %d\n", ch->practice);
    if (ch->train != 0)
        fprintf(fp, "Trai %d\n", ch->train);
    if (ch->saving_throw != 0)
        fprintf(fp, "Save  %d\n", ch->saving_throw);
    fprintf(fp, "Alig  %d\n", ch->alignment);
    if (ch->hitroll != 0)
        fprintf(fp, "Hit   %d\n", ch->hitroll);
    if (ch->damroll != 0)
        fprintf(fp, "Dam   %d\n", ch->damroll);
    fprintf(fp, "ACs %d %d %d %d\n", ch->armor[0], ch->armor[1], ch->armor[2], ch->armor[3]);
    if (ch->wimpy != 0)
        fprintf(fp, "Wimp  %d\n", ch->wimpy);
    fprintf(fp, "Attr %d %d %d %d %d\n", ch->perm_stat[STAT_STR], ch->perm_stat[STAT_INT], ch->perm_stat[STAT_WIS],
            ch->perm_stat[STAT_DEX], ch->perm_stat[STAT_CON]);

    fprintf(fp, "AMod %d %d %d %d %d\n", ch->mod_stat[STAT_STR], ch->mod_stat[STAT_INT], ch->mod_stat[STAT_WIS],
            ch->mod_stat[STAT_DEX], ch->mod_stat[STAT_CON]);

    if (IS_NPC(ch)) {
        fprintf(fp, "Vnum %d\n", ch->pIndexData->vnum);
    } else {
        fprintf(fp, "Pass %s~\n", ch->pcdata->pwd);
        if (ch->pcdata->bamfin[0] != '\0')
            fprintf(fp, "Bin  %s~\n", ch->pcdata->bamfin);
        if (ch->pcdata->bamfout[0] != '\0')
            fprintf(fp, "Bout %s~\n", ch->pcdata->bamfout);
        fprintf(fp, "Titl %s~\n", ch->pcdata->title);
        fprintf(fp, "Afk %s~\n", ch->pcdata->afk);
        fprintf(fp, "Colo %d\n", ch->pcdata->colour);
        fprintf(fp, "Prmt %s~\n", ch->pcdata->prompt);
        fprintf(fp, "Pnts %d\n", ch->pcdata->points);
        fprintf(fp, "TSex %d\n", ch->pcdata->true_sex);
        fprintf(fp, "LLev %d\n", ch->pcdata->last_level);
        fprintf(fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit, ch->pcdata->perm_mana, ch->pcdata->perm_move);
        /* Rohan: Save info data */
        fprintf(fp, "Info_message %s~\n", ch->pcdata->info_message);
        if (ch->desc) {
            fprintf(fp, "LastLoginFrom %s~\n", get_masked_hostname(hostbuf, ch->desc->host));
            fprintf(fp, "LastLoginAt %s~\n", ch->desc->logintime);
        }

        /* save prefix PCFN 19-05-97 */
        fprintf(fp, "Prefix %s~\n", ch->pcdata->prefix);

        /* Save time ;-)  PCFN 24-05-97 */
        fprintf(fp, "HourOffset %d\n", ch->pcdata->houroffset);
        fprintf(fp, "MinOffset %d\n", ch->pcdata->minoffset);

        fprintf(fp, "ExtraBits %s~\n", extra_bit_string(ch));

        fprintf(fp, "Cond %d %d %d\n", ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2]);

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != nullptr && ch->pcdata->learned[sn] > 0) // NOT get_skill_learned
            {
                fprintf(fp, "Sk %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn].name); // NOT get_skill_learned
            }
        }

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name != nullptr && ch->pcdata->group_known[gn]) {
                fprintf(fp, "Gr '%s'\n", group_table[gn].name);
            }
        }
    }

    for (paf = ch->affected; paf != nullptr; paf = paf->next) {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        fprintf(fp, "AffD '%s' %3d %3d %3d %3d %10d\n", skill_table[paf->type].name, paf->level, paf->duration,
                paf->modifier, paf->location, paf->bitvector);
    }

    {
        THREAD *t;
        ARTICLE *a;
        fprintf(fp, "NewsRead ");
        for (t = thread_head; t; t = t->next)
            for (a = t->articles; a; a = a->next)
                if (has_read_before(ch, a->msg_id))
                    fprintf(fp, "%d ", a->msg_id);
        fprintf(fp, "\n");
    }

    fprintf(fp, "End\n\n");
}

/* write a pet */
void fwrite_pet(CHAR_DATA *ch, CHAR_DATA *pet, FILE *fp) {
    AFFECT_DATA *paf;

    fprintf(fp, "#PET\n");

    fprintf(fp, "Vnum %d\n", pet->pIndexData->vnum);

    fprintf(fp, "Name %s~\n", pet->name);
    if (pet->short_descr != pet->pIndexData->short_descr)
        fprintf(fp, "ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
        fprintf(fp, "LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
        fprintf(fp, "Desc %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
        fprintf(fp, "Race %s~\n", race_table[pet->race].name);
    fprintf(fp, "Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
        fprintf(fp, "Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n", pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0)
        fprintf(fp, "Gold %ld\n", pet->gold);
    if (pet->exp > 0)
        fprintf(fp, "Exp  %ld\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
        fprintf(fp, "Act  %ld\n", pet->act);
    if (pet->affected_by != pet->pIndexData->affected_by)
        fprintf(fp, "AfBy %d\n", pet->affected_by);
    if (pet->comm != 0)
        fprintf(fp, "Comm %ld\n", pet->comm);
    fprintf(fp, "Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
        fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
        fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
        fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
        fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n", pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d\n", pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT], pet->perm_stat[STAT_WIS],
            pet->perm_stat[STAT_DEX], pet->perm_stat[STAT_CON]);
    fprintf(fp, "AMod %d %d %d %d %d\n", pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT], pet->mod_stat[STAT_WIS],
            pet->mod_stat[STAT_DEX], pet->mod_stat[STAT_CON]);

    for (paf = pet->affected; paf != nullptr; paf = paf->next) {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        fprintf(fp, "AffD '%s' %3d %3d %3d %3d %10d\n", skill_table[paf->type].name, paf->level, paf->duration,
                paf->modifier, paf->location, paf->bitvector);
    }

    if (ch->riding == pet) {
        fprintf(fp, "Ridden\n");
    }

    if (pet->carrying)
        fwrite_obj(pet, pet->carrying, fp, 0);

    fprintf(fp, "End\n");
}

/*
 * Write an object and its contents.
 */
void fwrite_obj(CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest) {
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if (obj->next_content != nullptr)
        fwrite_obj(ch, obj->next_content, fp, iNest);

    /*
     * Castrate storage characters.
     */
    if ((ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER) || obj->item_type == ITEM_KEY
        || (obj->item_type == ITEM_MAP && !obj->value[0]))
        return;

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %d\n", obj->pIndexData->vnum);
    if (!obj->pIndexData->new_format)
        fprintf(fp, "Oldstyle\n");
    if (obj->enchanted)
        fprintf(fp, "Enchanted\n");
    fprintf(fp, "Nest %d\n", iNest);

    /* these data are only used if they do not match the defaults */

    if (obj->name != obj->pIndexData->name)
        fprintf(fp, "Name %s~\n", obj->name);
    if (obj->short_descr != obj->pIndexData->short_descr)
        fprintf(fp, "ShD  %s~\n", obj->short_descr);
    if (obj->description != obj->pIndexData->description)
        fprintf(fp, "Desc %s~\n", obj->description);
    if (obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf(fp, "ExtF %d\n", obj->extra_flags);
    if (obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf(fp, "WeaF %d\n", obj->wear_flags);
    if (obj->wear_string != obj->pIndexData->wear_string)
        fprintf(fp, "WStr %s~\n", obj->wear_string);
    if (obj->item_type != obj->pIndexData->item_type)
        fprintf(fp, "Ityp %d\n", obj->item_type);
    if (obj->weight != obj->pIndexData->weight)
        fprintf(fp, "Wt   %d\n", obj->weight);

    /* variable data */

    fprintf(fp, "Wear %d\n", obj->wear_loc);
    if (obj->level != 0)
        fprintf(fp, "Lev  %d\n", obj->level);
    if (obj->timer != 0)
        fprintf(fp, "Time %d\n", obj->timer);
    fprintf(fp, "Cost %d\n", obj->cost);
    if (obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1]
        || obj->value[2] != obj->pIndexData->value[2] || obj->value[3] != obj->pIndexData->value[3]
        || obj->value[4] != obj->pIndexData->value[4])
        fprintf(fp, "Val  %d %d %d %d %d\n", obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);

    switch (obj->item_type) {
    case ITEM_POTION:
    case ITEM_SCROLL:
        if (obj->value[1] > 0) {
            fprintf(fp, "Spell 1 '%s'\n", skill_table[obj->value[1]].name);
        }

        if (obj->value[2] > 0) {
            fprintf(fp, "Spell 2 '%s'\n", skill_table[obj->value[2]].name);
        }

        if (obj->value[3] > 0) {
            fprintf(fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name);
        }

        break;

    case ITEM_PILL:
    case ITEM_STAFF:
    case ITEM_WAND:
        if (obj->value[3] > 0) {
            fprintf(fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name);
        }

        break;
    }

    for (paf = obj->affected; paf != nullptr; paf = paf->next) {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;
        fprintf(fp, "AffD '%s' %d %d %d %d %d\n", skill_table[paf->type].name, paf->level, paf->duration, paf->modifier,
                paf->location, paf->bitvector);
    }

    for (ed = obj->extra_descr; ed != nullptr; ed = ed->next) {
        fprintf(fp, "ExDe %s~ %s~\n", ed->keyword, ed->description);
    }

    fprintf(fp, "End\n\n");

    if (obj->contains != nullptr)
        fwrite_obj(ch, obj->contains, fp, iNest + 1);
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj(Descriptor *d, const char *name) {
    static PC_DATA pcdata_zero;
    char strsave[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH * 2];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;

    if (char_free == nullptr) {
        ch = static_cast<CHAR_DATA *>(alloc_perm(sizeof(*ch)));
    } else {
        ch = char_free;
        char_free = char_free->next;
    }
    clear_char(ch);

    if (pcdata_free == nullptr) {
        ch->pcdata = static_cast<PC_DATA *>(alloc_perm(sizeof(*ch->pcdata)));
    } else {
        ch->pcdata = pcdata_free;
        pcdata_free = pcdata_free->next;
    }
    *ch->pcdata = pcdata_zero;

    d->character = ch;
    ch->desc = d;
    ch->name = str_dup(name);
    ch->version = 0;
    ch->race = race_lookup("human");
    ch->affected_by = 0;

    ch->act = PLR_AUTOPEEK | PLR_AUTOASSIST | PLR_AUTOEXIT | PLR_AUTOGOLD | PLR_AUTOLOOT | PLR_AUTOSAC | PLR_NOSUMMON;

    ch->comm = COMM_COMBINE | COMM_PROMPT | COMM_SHOWAFK | COMM_SHOWDEFENCE;
    ch->invis_level = 0;
    ch->practice = 0;
    ch->train = 0;
    ch->hitroll = 0;
    ch->damroll = 0;
    ch->trust = 0;
    ch->wimpy = 0;
    ch->riding = nullptr;
    ch->ridden_by = nullptr;
    ch->saving_throw = 0;
    ch->thread = thread_head;
    if (ch->thread) {
        ch->article = thread_head->articles;
    } else {
        ch->article = nullptr;
    }
    ch->articlenum = 1;
    ch->newsbuffer = nullptr;
    ch->newsreply = nullptr;
    ch->newssubject = nullptr;
    /* prefix added 19-05-97 PCFN  */
    ch->pcdata->prefix = str_dup("");
    ch->pcdata->pcclan = (PCCLAN *)0;
    ch->pcdata->points = 0;
    ch->pcdata->confirm_delete = false;
    ch->pcdata->pwd = str_dup("");
    ch->pcdata->bamfin = str_dup("");
    ch->pcdata->bamfout = str_dup("");
    ch->pcdata->title = str_dup("");
    ch->pcdata->prompt = str_dup("");
    ch->pcdata->afk = str_dup("");
    ch->pcdata->info_message = str_dup("");
    ch->pcdata->colour = 0;
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat] = 13;
    ch->pcdata->perm_hit = 0;
    ch->pcdata->perm_mana = 0;
    ch->pcdata->perm_move = 0;
    ch->pcdata->true_sex = 0;
    ch->pcdata->last_level = 0;
    ch->pcdata->minoffset = 0;
    ch->pcdata->houroffset = 0;
    ch->pcdata->condition[COND_THIRST] = 48;

    ch->pcdata->condition[COND_FULL] = 48;

    found = false;
    fclose(fpReserve);

    /* decompress if .gz file exists */
    snprintf(strsave, sizeof(strsave), "%s%s%s", PLAYER_DIR, capitalize(name), ".gz");
    if ((fp = fopen(strsave, "r")) != nullptr) {
        FILE *q;
        char t2[MAX_STRING_LENGTH * 2];
        fclose(fp);
        snprintf(t2, sizeof(t2), "%s%s", PLAYER_DIR, capitalize(name));
        if ((q = fopen(t2, "r")) == nullptr) {
            fclose(q);
            snprintf(buf, sizeof(buf), "gzip -dfq %s", strsave);
            if (system(buf) != 0) {
                bug("Unable to decompress %s", strsave);
            }
        }
    }

    snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, capitalize(name));
    if ((fp = fopen(strsave, "r")) != nullptr) {
        int iNest;

        for (iNest = 0; iNest < MAX_NEST; iNest++)
            rgObjNest[iNest] = nullptr;

        found = true;
        for (;;) {
            char letter;
            char *word;

            letter = fread_letter(fp);
            if (letter == '*') {
                fread_to_eol(fp);
                continue;
            }

            if (letter != '#') {
                bug("Load_char_obj: # not found.");
                break;
            }

            word = fread_word(fp);
            if (!str_cmp(word, "PLAYER")) {
                fread_char(ch, fp);
                affect_strip(ch, gsn_ride);
            } else if (!str_cmp(word, "OBJECT"))
                fread_obj(ch, fp);
            else if (!str_cmp(word, "O"))
                fread_obj(ch, fp);
            else if (!str_cmp(word, "PET"))
                fread_pet(ch, fp);
            else if (!str_cmp(word, "END"))
                break;
            else {
                bug("Load_char_obj: bad section.");
                break;
            }
        }
        fclose(fp);
    }

    fpReserve = fopen(NULL_FILE, "r");

    /* initialize race */
    if (found) {
        int i;

        if (ch->race == 0)
            ch->race = race_lookup("human");

        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = 17; /*punch */

        for (i = 0; i < 5; i++) {
            if (pc_race_table[ch->race].skills[i] == nullptr)
                break;
            group_add(ch, pc_race_table[ch->race].skills[i], false);
        }
        ch->affected_by = (int)(ch->affected_by | race_table[ch->race].aff);
        ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
        ch->form = race_table[ch->race].form;
        ch->parts = race_table[ch->race].parts;
    }

    /* RT initialize skills */

    if (found && ch->version < 2) /* need to add the new skills */
    {
        group_add(ch, "rom basics", false);
        group_add(ch, class_table[ch->class_num].base_group, false);
        group_add(ch, class_table[ch->class_num].default_group, true);
        ch->pcdata->learned[gsn_recall] = 50;
    }

    return found;
}

/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)                                                                                     \
    if (!str_cmp(word, literal)) {                                                                                     \
        field = value;                                                                                                 \
        fMatch = true;                                                                                                 \
        break;                                                                                                         \
    }

void fread_char(CHAR_DATA *ch, FILE *fp) {
    char buf[MAX_STRING_LENGTH];
    const char *word;
    bool fMatch;

    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
        case '*':
            fMatch = true;
            fread_to_eol(fp);
            break;

        case 'A':
            KEY("Act", ch->act, fread_number(fp));
            KEY("AffectedBy", ch->affected_by, fread_number(fp));
            KEY("AfBy", ch->affected_by, fread_number(fp));
            KEY("Afk", ch->pcdata->afk, fread_string(fp));
            KEY("Alignment", ch->alignment, fread_number(fp));
            KEY("Alig", ch->alignment, fread_number(fp));

            if (!str_cmp(word, "AC") || !str_cmp(word, "Armor")) {
                fread_to_eol(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "ACs")) {
                int i;

                for (i = 0; i < 4; i++)
                    ch->armor[i] = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Affect") || !str_cmp(word, "Aff") || !str_cmp(word, "AffD")) {
                AFFECT_DATA *paf;

                if (affect_free == nullptr) {
                    paf = static_cast<AFFECT_DATA *>(alloc_perm(sizeof(*paf)));
                } else {
                    paf = affect_free;
                    affect_free = affect_free->next;
                }

                if (!str_cmp(word, "AffD")) {
                    int sn;
                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_char: unknown skill.");
                    else
                        paf->type = sn;
                } else /* old form */
                    paf->type = fread_number(fp);
                if (ch->version == 0)
                    paf->level = ch->level;
                else
                    paf->level = fread_number(fp);
                paf->duration = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->location = fread_number(fp);
                paf->bitvector = fread_number(fp);
                paf->next = ch->affected;
                ch->affected = paf;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AttrMod") || !str_cmp(word, "AMod")) {
                int stat;
                for (stat = 0; stat < MAX_STATS; stat++)
                    ch->mod_stat[stat] = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AttrPerm") || !str_cmp(word, "Attr")) {
                int stat;

                for (stat = 0; stat < MAX_STATS; stat++)
                    ch->perm_stat[stat] = fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'B':
            KEY("Bamfin", ch->pcdata->bamfin, fread_string(fp));
            KEY("Bamfout", ch->pcdata->bamfout, fread_string(fp));
            KEY("Bin", ch->pcdata->bamfin, fread_string(fp));
            KEY("Bout", ch->pcdata->bamfout, fread_string(fp));
            break;

        case 'C':
            if (!str_cmp(word, "Clan")) {
                int clannum = fread_number(fp);
                int count;
                PCCLAN *newpcclan;
                for (count = 0; count < NUM_CLANS; count++) {
                    if (clantable[count].clanchar == (char)clannum) {
                        newpcclan = (PCCLAN *)alloc_mem(sizeof(*newpcclan));
                        newpcclan->clan = (CLAN *)&clantable[count];
                        newpcclan->clanlevel = CLAN_MEMBER;
                        newpcclan->channelflags = CLANCHANNEL_ON;
                        ch->pcdata->pcclan = newpcclan;
                    }
                }
                fMatch = true;
                break;
            }

            KEY("Class", ch->class_num, fread_number(fp));
            KEY("Cla", ch->class_num, fread_number(fp));
            if (!str_cmp(word, "CLevel")) {
                if (ch->pcdata->pcclan) {
                    ch->pcdata->pcclan->clanlevel = fread_number(fp);
                } else {
                    bug("fread_char: CLAN level with no clan");
                    fread_to_eol(fp);
                }
                fMatch = true;
                break;
            }
            if (!str_cmp(word, "CCFlags")) {
                if (ch->pcdata->pcclan) {
                    ch->pcdata->pcclan->channelflags = fread_number(fp);
                } else {
                    bug("fread_char: CLAN channelflags with no clan");
                    fread_to_eol(fp);
                }
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Condition") || !str_cmp(word, "Cond")) {
                ch->pcdata->condition[0] = fread_number(fp);
                ch->pcdata->condition[1] = fread_number(fp);
                ch->pcdata->condition[2] = fread_number(fp);
                fMatch = true;
                break;
            }
            KEY("Colo", ch->pcdata->colour, fread_number(fp));
            KEY("Comm", ch->comm, fread_number(fp));

            break;

        case 'D':
            KEY("Damroll", ch->damroll, fread_number(fp));
            KEY("Dam", ch->damroll, fread_number(fp));
            KEY("Description", ch->description, fread_string(fp));
            KEY("Desc", ch->description, fread_string(fp));
            break;

        case 'E':
            if (!str_cmp(word, "End"))
                return;
            KEY("Exp", ch->exp, fread_number(fp));
            if (!str_cmp(word, "ExtraBits")) {
                set_bits_from_pfile(ch, fp);
                fread_to_eol(fp);
                fMatch = true;
                break;
            }
            break;

        case 'G':
            KEY("Gold", ch->gold, fread_number(fp));
            if (!str_cmp(word, "Group") || !str_cmp(word, "Gr")) {
                int gn;
                char *temp;

                temp = fread_word(fp);
                gn = group_lookup(temp);
                /* gn    = group_lookup( fread_word( fp ) ); */
                if (gn < 0) {
                    fprintf(stderr, "%s", temp);
                    bug("Fread_char: unknown group.");
                } else
                    gn_add(ch, gn);
                fMatch = true;
            }
            break;

        case 'H':
            KEY("Hitroll", ch->hitroll, fread_number(fp));
            KEY("Hit", ch->hitroll, fread_number(fp));
            KEY("HourOffset", ch->pcdata->houroffset, fread_number(fp));

            if (!str_cmp(word, "HpManaMove") || !str_cmp(word, "HMV")) {
                ch->hit = fread_number(fp);
                ch->max_hit = fread_number(fp);
                ch->mana = fread_number(fp);
                ch->max_mana = fread_number(fp);
                ch->move = fread_number(fp);
                ch->max_move = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "HpManaMovePerm") || !str_cmp(word, "HMVP")) {
                ch->pcdata->perm_hit = fread_number(fp);
                ch->pcdata->perm_mana = fread_number(fp);
                ch->pcdata->perm_move = fread_number(fp);
                fMatch = true;
                break;
            }

            break;

        case 'I':
            KEY("InvisLevel", ch->invis_level, fread_number(fp));
            KEY("Invi", ch->invis_level, fread_number(fp));
            KEY("Info_message", ch->pcdata->info_message, fread_string(fp));
            break;

        case 'L':
            KEY("LastLevel", ch->pcdata->last_level, fread_number(fp));
            KEY("LLev", ch->pcdata->last_level, fread_number(fp));
            KEY("Level", ch->level, fread_number(fp));
            KEY("Lev", ch->level, fread_number(fp));
            KEY("Levl", ch->level, fread_number(fp));
            KEY("LongDescr", ch->long_descr, fread_string(fp));
            KEY("LnD", ch->long_descr, fread_string(fp));
            KEY("LastLoginFrom", login_from, fread_string(fp));
            KEY("LastLoginAt", login_at, fread_string(fp));
            break;

        case 'M': KEY("MinOffset", ch->pcdata->minoffset, fread_number(fp)); break;

        case 'N':
            KEY("Name", ch->name, fread_string(fp));
            KEY("Note", ch->last_note, fread_number(fp));
            if (!str_cmp(word, "NewsRead")) {
                int c;
                char buf[MAX_STRING_LENGTH];
                fMatch = true;
                for (;;) {
                    do {
                        c = getc(fp);
                    } while (c == ' ');
                    if (c == '\n')
                        break;
                    ungetc(c, fp);
                    strcpy(buf, fread_word(fp));
                    if (strlen(buf) == 0)
                        break;
                    c = atoi(buf);
                    if (article_exists(c))
                        mark_as_read(ch, c);
                }
            }
            break;

        case 'P':
            KEY("Password", ch->pcdata->pwd, fread_string(fp));
            KEY("Pass", ch->pcdata->pwd, fread_string(fp));
            KEY("Played", ch->played, fread_number(fp));
            KEY("Plyd", ch->played, fread_number(fp));
            KEY("Points", ch->pcdata->points, fread_number(fp));
            KEY("Pnts", ch->pcdata->points, fread_number(fp));
            KEY("Position", ch->position, fread_number(fp));
            KEY("Pos", ch->position, fread_number(fp));
            KEY("Practice", ch->practice, fread_number(fp));
            KEY("Prac", ch->practice, fread_number(fp));
            KEY("Prompt", ch->pcdata->prompt, fread_string(fp));
            KEY("Prmt", ch->pcdata->prompt, fread_string(fp));
            KEY("Prefix", ch->pcdata->prefix, fread_string(fp));
            break;

        case 'R':
            KEY("Race", ch->race, race_lookup(fread_string(fp)));

            if (!str_cmp(word, "Room")) {
                ch->in_room = get_room_index(fread_number(fp));
                if (ch->in_room == nullptr)
                    ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
                fMatch = true;
                break;
            }

            break;

        case 'S':
            KEY("SavingThrow", ch->saving_throw, fread_number(fp));
            KEY("Save", ch->saving_throw, fread_number(fp));
            KEY("Scro", ch->lines, fread_number(fp));
            /* kludge to avoid memory bug */
            if (ch->lines == 0 || ch->lines > 52)
                ch->lines = 52;
            KEY("Sex", ch->sex, fread_number(fp));
            KEY("ShortDescr", ch->short_descr, fread_string(fp));
            KEY("ShD", ch->short_descr, fread_string(fp));

            if (!str_cmp(word, "Skill") || !str_cmp(word, "Sk")) {
                int sn;
                int value;
                char *temp;

                value = fread_number(fp);
                temp = fread_word(fp);
                sn = skill_lookup(temp);
                /* sn    = skill_lookup( fread_word( fp ) ); */
                if (sn < 0) {
                    fprintf(stderr, "%s", temp);
                    bug("Fread_char: unknown skill.");
                } else
                    ch->pcdata->learned[sn] = value;
                fMatch = true;
            }

            break;

        case 'T':
            KEY("TrueSex", ch->pcdata->true_sex, fread_number(fp));
            KEY("TSex", ch->pcdata->true_sex, fread_number(fp));
            KEY("Trai", ch->train, fread_number(fp));
            KEY("Trust", ch->trust, fread_number(fp));
            KEY("Tru", ch->trust, fread_number(fp));

            if (!str_cmp(word, "Title") || !str_cmp(word, "Titl")) {
                ch->pcdata->title = fread_string(fp);
                if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ',' && ch->pcdata->title[0] != '!'
                    && ch->pcdata->title[0] != '?') {
                    snprintf(buf, sizeof(buf), " %s", ch->pcdata->title);
                    free_string(ch->pcdata->title);
                    ch->pcdata->title = str_dup(buf);
                }
                fMatch = true;
                break;
            }

            break;

        case 'V':
            KEY("Version", ch->version, fread_number(fp));
            KEY("Vers", ch->version, fread_number(fp));
            if (!str_cmp(word, "Vnum")) {
                ch->pIndexData = get_mob_index(fread_number(fp));
                fMatch = true;
                break;
            }
            break;

        case 'W':
            KEY("Wimpy", ch->wimpy, fread_number(fp));
            KEY("Wimp", ch->wimpy, fread_number(fp));
            break;
        }

        if (!fMatch) {
            bug("Fread_char: no match.");
            fread_to_eol(fp);
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet(CHAR_DATA *ch, FILE *fp) {
    const char *word;
    CHAR_DATA *pet;
    bool fMatch;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word, "Vnum")) {
        int vnum;

        vnum = fread_number(fp);
        if (get_mob_index(vnum) == nullptr) {
            bug("Fread_pet: bad vnum %d.", vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        } else
            pet = create_mobile(get_mob_index(vnum));
    } else {
        bug("Fread_pet: no vnum in file.");
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }

    for (;;) {
        word = feof(fp) ? "END" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
        case '#':
            fMatch = true;
            fread_obj(pet, fp);
            break;

        case '*':
            fMatch = true;
            fread_to_eol(fp);
            break;

        case 'A':
            KEY("Act", pet->act, fread_number(fp));
            KEY("AfBy", pet->affected_by, fread_number(fp));
            KEY("Alig", pet->alignment, fread_number(fp));

            if (!str_cmp(word, "ACs")) {
                int i;

                for (i = 0; i < 4; i++)
                    pet->armor[i] = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AffD")) {
                AFFECT_DATA *paf;
                int sn;

                if (affect_free == nullptr)
                    paf = static_cast<AFFECT_DATA *>(alloc_perm(sizeof(*paf)));
                else {
                    paf = affect_free;
                    affect_free = affect_free->next;
                }

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.");
                else
                    paf->type = sn;

                paf->level = fread_number(fp);
                paf->duration = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->location = fread_number(fp);
                paf->bitvector = fread_number(fp);
                paf->next = pet->affected;
                pet->affected = paf;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AMod")) {
                int stat;

                for (stat = 0; stat < MAX_STATS; stat++)
                    pet->mod_stat[stat] = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Attr")) {
                int stat;

                for (stat = 0; stat < MAX_STATS; stat++)
                    pet->perm_stat[stat] = fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'C': KEY("Comm", pet->comm, fread_number(fp)); break;

        case 'D':
            KEY("Dam", pet->damroll, fread_number(fp));
            KEY("Desc", pet->description, fread_string(fp));
            break;

        case 'E':
            if (!str_cmp(word, "End")) {
                pet->leader = ch;
                pet->master = ch;
                ch->pet = pet;
                return;
            }
            KEY("Exp", pet->exp, fread_number(fp));
            break;

        case 'G': KEY("Gold", pet->gold, fread_number(fp)); break;

        case 'H':
            KEY("Hit", pet->hitroll, fread_number(fp));

            if (!str_cmp(word, "HMV")) {
                pet->hit = fread_number(fp);
                pet->max_hit = fread_number(fp);
                pet->mana = fread_number(fp);
                pet->max_mana = fread_number(fp);
                pet->move = fread_number(fp);
                pet->max_move = fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'L':
            KEY("Levl", pet->level, fread_number(fp));
            KEY("LnD", pet->long_descr, fread_string(fp));
            break;

        case 'N': KEY("Name", pet->name, fread_string(fp)); break;

        case 'P': KEY("Pos", pet->position, fread_number(fp)); break;

        case 'R': KEY("Race", pet->race, race_lookup(fread_string(fp))); break;

        case 'S':
            KEY("Save", pet->saving_throw, fread_number(fp));
            KEY("Sex", pet->sex, fread_number(fp));
            KEY("ShD", pet->short_descr, fread_string(fp));
            break;

            if (!fMatch) {
                bug("Fread_pet: no match.");
                fread_to_eol(fp);
            }
        }
    }
}

void fread_obj(CHAR_DATA *ch, FILE *fp) {
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;
    const char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format; /* to prevent errors */
    bool make_new; /* update object */

    fVnum = false;
    obj = nullptr;
    first = true; /* used to counter fp offset */
    new_format = false;
    make_new = false;

    word = feof(fp) ? "End" : fread_word(fp);
    if (!str_cmp(word, "Vnum")) {
        int vnum;
        first = false; /* fp will be in right place */

        vnum = fread_number(fp);
        if (get_obj_index(vnum) == nullptr) {
            bug("Fread_obj: bad vnum %d.", vnum);
        } else {
            obj = create_object(get_obj_index(vnum), -1);
            new_format = true;
        }
    }

    if (obj == nullptr) /* either not found or old style */
    {
        if (obj_free == nullptr) {
            obj = static_cast<OBJ_DATA *>(alloc_perm(sizeof(*obj)));
        } else {
            obj = obj_free;
            obj_free = obj_free->next;
        }

        *obj = obj_zero;
        obj->name = str_dup("");
        obj->short_descr = str_dup("");
        obj->description = str_dup("");
        obj->wear_string = str_dup("");
    }

    fNest = false;
    fVnum = true;
    iNest = 0;

    for (;;) {
        if (first)
            first = false;
        else
            word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
        case '*':
            fMatch = true;
            fread_to_eol(fp);
            break;

        case 'A':
            if (!str_cmp(word, "Affect") || !str_cmp(word, "Aff") || !str_cmp(word, "AffD")) {
                AFFECT_DATA *paf;

                if (affect_free == nullptr) {
                    paf = static_cast<AFFECT_DATA *>(alloc_perm(sizeof(*paf)));
                } else {
                    paf = affect_free;
                    affect_free = affect_free->next;
                }

                if (!str_cmp(word, "AffD")) {
                    int sn;
                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_obj: unknown skill.");
                    else
                        paf->type = sn;
                } else /* old form */
                    paf->type = fread_number(fp);
                if (ch->version == 0)
                    paf->level = 20;
                else
                    paf->level = fread_number(fp);
                paf->duration = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->location = fread_number(fp);
                paf->bitvector = fread_number(fp);
                paf->next = obj->affected;
                obj->affected = paf;
                fMatch = true;
                break;
            }
            break;

        case 'C': KEY("Cost", obj->cost, fread_number(fp)); break;

        case 'D':
            KEY("Description", obj->description, fread_string(fp));
            KEY("Desc", obj->description, fread_string(fp));
            break;

        case 'E':

            if (!str_cmp(word, "Enchanted")) {
                obj->enchanted = true;
                fMatch = true;
                break;
            }

            KEY("ExtraFlags", obj->extra_flags, fread_number(fp));
            KEY("ExtF", obj->extra_flags, fread_number(fp));

            if (!str_cmp(word, "ExtraDescr") || !str_cmp(word, "ExDe")) {
                EXTRA_DESCR_DATA *ed;

                if (extra_descr_free == nullptr) {
                    ed = static_cast<EXTRA_DESCR_DATA *>(alloc_perm(sizeof(*ed)));
                } else {
                    ed = extra_descr_free;
                    extra_descr_free = extra_descr_free->next;
                }

                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = obj->extra_descr;
                obj->extra_descr = ed;
                fMatch = true;
            }

            if (!str_cmp(word, "End")) {
                if (!fNest || !fVnum || obj->pIndexData == nullptr) {
                    bug("Fread_obj: incomplete object.");
                    /*        free_string( obj->name        );
                              free_string( obj->description );
                              free_string( obj->short_descr );
                              free_string( obj->wear_string );
                              obj->next = obj_free;
                              obj_free  = obj; */
                    extract_obj(obj); /* does it better than above */
                    return;
                } else {
                    if (!new_format) {
                        obj->next = object_list;
                        object_list = obj;
                        obj->pIndexData->count++;
                    }

                    if (!obj->pIndexData->new_format && obj->item_type == ITEM_ARMOR && obj->value[1] == 0) {
                        obj->value[1] = obj->value[0];
                        obj->value[2] = obj->value[0];
                    }
                    if (make_new) {
                        int wear;

                        wear = obj->wear_loc;
                        extract_obj(obj);

                        obj = create_object(obj->pIndexData, 0);
                        obj->wear_loc = wear;
                    }
                    if (iNest == 0 || rgObjNest[iNest] == nullptr)
                        obj_to_char(obj, ch);
                    else
                        obj_to_obj(obj, rgObjNest[iNest - 1]);
                    return;
                }
            }
            break;

        case 'I':
            KEY("ItemType", obj->item_type, fread_number(fp));
            KEY("Ityp", obj->item_type, fread_number(fp));
            break;

        case 'L':
            KEY("Level", obj->level, fread_number(fp));
            KEY("Lev", obj->level, fread_number(fp));
            break;

        case 'N':
            KEY("Name", obj->name, fread_string(fp));

            if (!str_cmp(word, "Nest")) {
                iNest = fread_number(fp);
                if (iNest < 0 || iNest >= MAX_NEST) {
                    bug("Fread_obj: bad nest %d.", iNest);
                } else {
                    rgObjNest[iNest] = obj;
                    fNest = true;
                }
                fMatch = true;
            }
            break;

        case 'O':
            if (!str_cmp(word, "Oldstyle")) {
                if (obj->pIndexData != nullptr && obj->pIndexData->new_format)
                    make_new = true;
                fMatch = true;
            }
            break;

        case 'S':
            KEY("ShortDescr", obj->short_descr, fread_string(fp));
            KEY("ShD", obj->short_descr, fread_string(fp));

            if (!str_cmp(word, "Spell")) {
                int iValue;
                int sn;

                iValue = fread_number(fp);
                sn = skill_lookup(fread_word(fp));
                if (iValue < 0 || iValue > 3) {
                    bug("Fread_obj: bad iValue %d.", iValue);
                } else if (sn < 0) {
                    bug("Fread_obj: unknown skill.");
                } else {
                    obj->value[iValue] = sn;
                }
                fMatch = true;
                break;
            }

            break;

        case 'T':
            KEY("Timer", obj->timer, fread_number(fp));
            KEY("Time", obj->timer, fread_number(fp));
            break;

        case 'V':
            if (!str_cmp(word, "Values") || !str_cmp(word, "Vals")) {
                obj->value[0] = fread_number(fp);
                obj->value[1] = fread_number(fp);
                obj->value[2] = fread_number(fp);
                obj->value[3] = fread_number(fp);
                if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                    obj->value[0] = obj->pIndexData->value[0];
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Val")) {
                obj->value[0] = fread_number(fp);
                obj->value[1] = fread_number(fp);
                obj->value[2] = fread_number(fp);
                obj->value[3] = fread_number(fp);
                obj->value[4] = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Vnum")) {
                int vnum;

                vnum = fread_number(fp);
                if ((obj->pIndexData = get_obj_index(vnum)) == nullptr)
                    bug("Fread_obj: bad vnum %d.", vnum);
                else
                    fVnum = true;
                fMatch = true;
                break;
            }
            break;

        case 'W':
            KEY("WearFlags", obj->wear_flags, fread_number(fp));
            KEY("WeaF", obj->wear_flags, fread_number(fp));
            KEY("WearLoc", obj->wear_loc, fread_number(fp));
            KEY("Wear", obj->wear_loc, fread_number(fp));
            KEY("Weight", obj->weight, fread_number(fp));
            KEY("WStr", obj->wear_string, fread_string(fp));
            KEY("Wt", obj->weight, fread_number(fp));
            break;
        }

        if (!fMatch) {
            bug("Fread_obj: no match.");
            fread_to_eol(fp);
        }
    }
}
