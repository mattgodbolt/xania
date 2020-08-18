/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db.c: file in/out, area database handler                             */
/*                                                                       */
/*************************************************************************/

#include "db.h"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "buffer.h"
#include "comm.hpp"
#include "interp.h"
#include "merc.h"
#include "note.h"

#include <range/v3/iterator/operations.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

/* Externally referenced functions. */
void wiznet_initialise();

/*
 * Globals.
 */
HELP_DATA *help_first;
HELP_DATA *help_last;

SHOP_DATA *shop_first;
SHOP_DATA *shop_last;

CHAR_DATA *char_free;
EXTRA_DESCR_DATA *extra_descr_free = nullptr;
OBJ_DATA *obj_free;
PC_DATA *pcdata_free;

char bug_buf[2 * MAX_INPUT_LENGTH];
CHAR_DATA *char_list;
char *help_greeting;
char log_buf[LOG_BUF_SIZE];
KILL_DATA kill_table[MAX_LEVEL];
OBJ_DATA *object_list;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;

sh_int gsn_backstab;
sh_int gsn_dodge;
sh_int gsn_hide;
sh_int gsn_peek;
sh_int gsn_pick_lock;
sh_int gsn_sneak;
sh_int gsn_steal;

sh_int gsn_disarm;
sh_int gsn_enhanced_damage;
sh_int gsn_kick;
sh_int gsn_parry;
sh_int gsn_rescue;
sh_int gsn_second_attack;
sh_int gsn_third_attack;

sh_int gsn_blindness;
sh_int gsn_charm_person;
sh_int gsn_curse;
sh_int gsn_invis;
sh_int gsn_mass_invis;
sh_int gsn_poison;
sh_int gsn_plague;
sh_int gsn_sleep;

/* new gsns */

sh_int gsn_axe;
sh_int gsn_dagger;
sh_int gsn_flail;
sh_int gsn_mace;
sh_int gsn_polearm;
sh_int gsn_shield_block;
sh_int gsn_spear;
sh_int gsn_sword;
sh_int gsn_whip;

sh_int gsn_bash;
sh_int gsn_berserk;
sh_int gsn_dirt;
sh_int gsn_hand_to_hand;
sh_int gsn_trip;

sh_int gsn_fast_healing;
sh_int gsn_haggle;
sh_int gsn_lore;
sh_int gsn_meditation;

sh_int gsn_scrolls;
sh_int gsn_staves;
sh_int gsn_wands;
sh_int gsn_recall;

/* a bunch of xania spells and skills */
sh_int gsn_headbutt;
sh_int gsn_ride;
sh_int gsn_throw;

sh_int gsn_sharpen;

sh_int gsn_raise_dead;
sh_int gsn_octarine_fire;
sh_int gsn_insanity;
sh_int gsn_bless;

/* Locals. */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
char *string_hash[MAX_KEY_HASH];

AREA_DATA *area_first;
AREA_DATA *area_last;

char *string_space;
char *top_string;
char str_empty[1];

int top_affect;
int top_area;
int top_ed;
int top_exit;
int top_help;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;
int mobile_count = 0;
int newmobs = 0;
int newobjs = 0;
int top_vnum_room;
int top_vnum_mob;
int top_vnum_obj;

/*
 * Merc-2.2 MOBprogram locals - Faramir 31/8/1998
 */

int mprog_name_to_type(char *name);
MPROG_DATA *mprog_file_read(char *f, MPROG_DATA *mprg, MOB_INDEX_DATA *pMobIndex);
void load_mobprogs(FILE *fp);
void mprog_read_programs(FILE *fp, MOB_INDEX_DATA *pMobIndex);

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define MAX_STRING 2650976
#define MAX_PERM_BLOCK 131072
#define MAX_MEM_LIST 14

void *rgFreeList[MAX_MEM_LIST];
const int rgSizeList[MAX_MEM_LIST] = {
    /*   16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768-64 */
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144};

int nAllocString;
int sAllocString;
int nAllocPerm;
int sAllocPerm;

/* Semi-locals. */
bool fBootDb;
FILE *fpArea;
char strArea[MAX_INPUT_LENGTH];
static bool area_header_found;

void copy_areaname(char *dest) { strcpy(dest, strArea); }

/* Local booting procedures. */
void init_mm();
void load_area(FILE *fp);
void load_helps(FILE *fp);
void load_old_mob(FILE *fp);
void load_old_mob_race(FILE *fp);
void load_mobiles(FILE *fp);
void load_old_obj(FILE *fp);
void load_objects(FILE *fp);

void load_resets(FILE *fp);
void new_reset(ROOM_INDEX_DATA *, RESET_DATA *);
void validate_resets();
void assign_area_vnum(int vnum);

void load_rooms(FILE *fp);

void load_shops(FILE *fp);
void load_socials(FILE *fp);
void load_specials(FILE *fp);

void fix_exits();

void reset_area(AREA_DATA *pArea);

/* RT max open files fix */

void maxfilelimit() {
    struct rlimit r;

    getrlimit(RLIMIT_NOFILE, &r);
    r.rlim_cur = r.rlim_max;
    setrlimit(RLIMIT_NOFILE, &r);
}

/* Big mama top level function. */
void boot_db() {

    /* open file fix */
    maxfilelimit();

    /* Init some data space stuff. */
    if ((string_space = static_cast<char *>(calloc(1, MAX_STRING))) == nullptr) {
        bug("Boot_db: can't alloc %d string space.", MAX_STRING);
        exit(1);
    }
    top_string = string_space;
    fBootDb = true;

    /* Init random number generator. */
    init_mm();

    /* Set time and weather. */
    {
        long lhour, lday, lmonth;

        lhour = (current_time - 650336715) / (PULSE_TICK / PULSE_PER_SECOND);
        time_info.hour = lhour % 24;
        lday = lhour / 24;
        time_info.day = lday % 35;
        lmonth = lday / 35;
        time_info.month = lmonth % 17;
        time_info.year = lmonth / 17;

        if (time_info.hour < 5)
            weather_info.sunlight = SUN_DARK;
        else if (time_info.hour < 6)
            weather_info.sunlight = SUN_RISE;
        else if (time_info.hour < 19)
            weather_info.sunlight = SUN_LIGHT;
        else if (time_info.hour < 20)
            weather_info.sunlight = SUN_SET;
        else
            weather_info.sunlight = SUN_DARK;

        weather_info.change = 0;
        weather_info.mmhg = 960;
        if (time_info.month >= 7 && time_info.month <= 12)
            weather_info.mmhg += number_range(1, 50);
        else
            weather_info.mmhg += number_range(1, 80);

        if (weather_info.mmhg <= 980)
            weather_info.sky = SKY_LIGHTNING;
        else if (weather_info.mmhg <= 1000)
            weather_info.sky = SKY_RAINING;
        else if (weather_info.mmhg <= 1020)
            weather_info.sky = SKY_CLOUDY;
        else
            weather_info.sky = SKY_CLOUDLESS;
    }

    /* Assign gsn's for skills which have them. */
    {
        int sn;

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].pgsn != nullptr)
                *skill_table[sn].pgsn = sn;
        }
    }

    /* Read in all the area files. */
    {
        FILE *fpList;

        if ((fpList = fopen(AREA_LIST, "r")) == nullptr) {
            perror(AREA_LIST);
            exit(1);
        }

        for (;;) {
            strcpy(strArea, fread_word(fpList));
            if (strArea[0] == '$')
                break;

            if (strArea[0] == '-') {
                fpArea = stdin;
            } else if ((fpArea = fopen(strArea, "r")) == nullptr) {
                perror(strArea);
                exit(1);
            }

            for (;;) {
                char *word;
                if (fread_letter(fpArea) != '#') {
                    bug("Boot_db: # not found.");
                    exit(1);
                }

                word = fread_word(fpArea);

                if (word[0] == '$')
                    break;
                else if (!str_cmp(word, "AREA"))
                    load_area(fpArea);
                else if (!str_cmp(word, "HELPS"))
                    load_helps(fpArea);
                else if (!str_cmp(word, "MOBOLD"))
                    load_old_mob(fpArea);
                else if (!str_cmp(word, "MOBRACE"))
                    load_old_mob_race(fpArea);
                else if (!str_cmp(word, "MOBILES"))
                    load_mobiles(fpArea);
                else if (!str_cmp(word, "OBJOLD"))
                    load_old_obj(fpArea);
                else if (!str_cmp(word, "OBJECTS"))
                    load_objects(fpArea);
                else if (!str_cmp(word, "RESETS"))
                    load_resets(fpArea);
                else if (!str_cmp(word, "ROOMS"))
                    load_rooms(fpArea);
                else if (!str_cmp(word, "SHOPS"))
                    load_shops(fpArea);
                else if (!str_cmp(word, "SOCIALS"))
                    load_socials(fpArea);
                else if (!str_cmp(word, "SPECIALS"))
                    load_specials(fpArea);
                else if (!str_cmp(word, "MOBPROGS"))
                    load_mobprogs(fpArea);
                else {
                    bug("Boot_db: bad section name.");
                    exit(1);
                }
            }

            if (fpArea != stdin)
                fclose(fpArea);
            fpArea = nullptr;
        }
        fclose(fpList);
    }
    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the notes file.
     */
    fix_exits();
    fBootDb = false;
    area_update();
    note_initialise();
    wiznet_initialise();
    interp_initialise();
}

/* Snarf an 'area' header line. */
void load_area(FILE *fp) {
    AREA_DATA *pArea;

    pArea = static_cast<AREA_DATA *>(alloc_perm(sizeof(*pArea)));

    fread_string(fp); /* filename */
    pArea->areaname = fread_string(fp);
    pArea->name = fread_string(fp);

    pArea->lvnum = fread_number(fp);
    pArea->uvnum = fread_number(fp);

    pArea->area_flags = AREA_LOADING;
    pArea->vnum = top_area;
    pArea->filename = str_dup(strArea);

    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->empty = false;

    if (area_first == nullptr)
        area_first = pArea;
    if (area_last != nullptr) {
        area_last->next = pArea;
        REMOVE_BIT(area_last->area_flags, AREA_LOADING);
    }
    area_last = pArea;
    pArea->next = nullptr;
    area_header_found = true;
    top_area++;
}

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

/* Snarf a help section. */
void load_helps(FILE *fp) {
    HELP_DATA *pHelp;

    for (;;) {
        pHelp = static_cast<HELP_DATA *>(alloc_perm(sizeof(*pHelp)));

        if (area_header_found)
            pHelp->area = area_last ? area_last : nullptr;
        else
            pHelp->area = nullptr;

        pHelp->level = fread_number(fp);
        pHelp->keyword = fread_string(fp);
        if (pHelp->keyword[0] == '$')
            break;
        pHelp->text = fread_string(fp);

        if (!str_cmp(pHelp->keyword, "greeting"))
            help_greeting = pHelp->text;

        if (help_first == nullptr)
            help_first = pHelp;
        if (help_last != nullptr)
            help_last->next = pHelp;

        help_last = pHelp;
        pHelp->next = nullptr;
        top_help++;
    }
}

/* Snarf a mob section.  old style */
void load_old_mob(FILE *fp) {
    MOB_INDEX_DATA *pMobIndex;
    /* for race updating */
    int race;
    char name[MAX_STRING_LENGTH];

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
            bug("Load_mobiles: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pMobIndex = static_cast<MOB_INDEX_DATA *>(alloc_perm(sizeof(*pMobIndex)));
        pMobIndex->vnum = vnum;
        pMobIndex->new_format = false;
        pMobIndex->player_name = fread_string(fp);
        pMobIndex->short_descr = fread_string(fp);
        pMobIndex->long_descr = fread_string(fp);
        pMobIndex->description = fread_string(fp);

        pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

        pMobIndex->act = fread_flag(fp) | ACT_IS_NPC;
        pMobIndex->affected_by = fread_flag(fp);
        pMobIndex->pShop = nullptr;
        pMobIndex->alignment = fread_number(fp);
        letter = fread_letter(fp);
        pMobIndex->level = number_fuzzy(fread_number(fp));

        /* The unused stuff is for imps who want to use the old-style
         * stats-in-files method.
         */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* 'd' Unused */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* '+' Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* 'd' Unused */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* '+' Unused */
        fread_number(fp); /* Unused */
        pMobIndex->gold = fread_number(fp); /* Unused */
        fread_number(fp); /* xp can't be used! Unused */
        pMobIndex->start_pos = fread_number(fp); /* Unused */
        pMobIndex->default_pos = fread_number(fp); /* Unused */

        if (pMobIndex->start_pos < POS_SLEEPING)
            pMobIndex->start_pos = POS_STANDING;
        if (pMobIndex->default_pos < POS_SLEEPING)
            pMobIndex->default_pos = POS_STANDING;

        /* Back to meaningful values. */
        pMobIndex->sex = fread_number(fp);

        /* compute the race BS */
        one_argument(pMobIndex->player_name, name);

        if (name[0] == '\0' || (race = race_lookup(name)) == 0) {
            /* fill in with blanks */
            pMobIndex->race = race_lookup("human");
            pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_VNUM;
            pMobIndex->imm_flags = 0;
            pMobIndex->res_flags = 0;
            pMobIndex->vuln_flags = 0;
            pMobIndex->form = FORM_EDIBLE | FORM_SENTIENT | FORM_BIPED | FORM_MAMMAL;
            pMobIndex->parts = PART_HEAD | PART_ARMS | PART_LEGS | PART_HEART | PART_BRAINS | PART_GUTS;
        } else {
            pMobIndex->race = race;
            pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_RACE | race_table[race].off;
            pMobIndex->imm_flags = race_table[race].imm;
            pMobIndex->res_flags = race_table[race].res;
            pMobIndex->vuln_flags = race_table[race].vuln;
            pMobIndex->form = race_table[race].form;
            pMobIndex->parts = race_table[race].parts;
        }

        if (letter != 'S') {
            bug("Load_mobiles: vnum %d non-S.", vnum);
            exit(1);
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
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL - 1)].number++;
    }
}

/* Snarf a mob section.  old style with race */
void load_old_mob_race(FILE *fp) {
    MOB_INDEX_DATA *pMobIndex;
    /* for race updating */

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
            bug("Load_mobiles: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pMobIndex = static_cast<MOB_INDEX_DATA *>(alloc_perm(sizeof(*pMobIndex)));
        pMobIndex->vnum = vnum;
        pMobIndex->new_format = false;
        pMobIndex->player_name = fread_string(fp);
        pMobIndex->short_descr = fread_string(fp);
        pMobIndex->long_descr = fread_string(fp);
        pMobIndex->description = fread_string(fp);

        pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

        pMobIndex->act = fread_flag(fp) | ACT_IS_NPC;
        pMobIndex->affected_by = fread_flag(fp);
        pMobIndex->pShop = nullptr;
        pMobIndex->alignment = fread_number(fp);
        letter = fread_letter(fp);
        pMobIndex->level = number_fuzzy(fread_number(fp));

        /*
         * The unused stuff is for imps who want to use the old-style
         * stats-in-files method.
         */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* 'd'      */ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* '+'      */ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* 'd'      */ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* '+'      */ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        pMobIndex->gold = fread_number(fp); /* Unused */
        /* xp can't be used! */ fread_number(fp); /* Unused */
        pMobIndex->start_pos = fread_number(fp); /* Unused */
        pMobIndex->default_pos = pMobIndex->start_pos;

        if (pMobIndex->start_pos < POS_SLEEPING)
            pMobIndex->start_pos = POS_STANDING;
        if (pMobIndex->default_pos < POS_SLEEPING)
            pMobIndex->default_pos = POS_STANDING;

        pMobIndex->race = race_lookup(fread_string(fp));

        /*
         * Back to meaningful values.
         */
        pMobIndex->sex = fread_number(fp);

        {
            pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_RACE | race_table[pMobIndex->race].off;
            pMobIndex->imm_flags = race_table[pMobIndex->race].imm;
            pMobIndex->res_flags = race_table[pMobIndex->race].res;
            pMobIndex->vuln_flags = race_table[pMobIndex->race].vuln;
            pMobIndex->form = race_table[pMobIndex->race].form;
            pMobIndex->parts = race_table[pMobIndex->race].parts;
        }

        if (letter != 'S') {
            bug("Load_mobiles: vnum %d non-S.", vnum);
            exit(1);
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
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL - 1)].number++;
    }
}

/*
 * Snarf an obj section.  old style
 */
void load_old_obj(FILE *fp) {
    OBJ_INDEX_DATA *pObjIndex;
    char temp; /* Used for looking for ',' after wear flags
                * Indicating a 'wear string'.
                */

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
            bug("Load_objects: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pObjIndex = static_cast<OBJ_INDEX_DATA *>(alloc_perm(sizeof(*pObjIndex)));
        pObjIndex->vnum = vnum;
        pObjIndex->new_format = false;
        pObjIndex->reset_num = 0;
        pObjIndex->name = fread_string(fp);
        pObjIndex->short_descr = fread_string(fp);
        pObjIndex->description = fread_string(fp);
        pObjIndex->material = material_lookup(fread_string(fp));

        /* Mg's 'orrible hack to guess material type */
        if (pObjIndex->material == MATERIAL_DEFAULT)
            pObjIndex->material = material_guess(pObjIndex->name);

        pObjIndex->short_descr[0] = LOWER(pObjIndex->short_descr[0]);
        pObjIndex->description[0] = UPPER(pObjIndex->description[0]);

        pObjIndex->item_type = fread_number(fp);
        pObjIndex->extra_flags = fread_flag(fp);
        pObjIndex->wear_flags = fread_flag(fp);

        /* Death.  Wear Strings for objects 18/3/96 */
        temp = getc(fp);
        if (temp == ',')
            pObjIndex->wear_string = fread_string(fp);
        else {
            ungetc(temp, fp);
            pObjIndex->wear_string = nullptr;
        }

        pObjIndex->value[0] = fread_spnumber(fp);
        pObjIndex->value[1] = fread_spnumber(fp);
        pObjIndex->value[2] = fread_spnumber(fp);
        pObjIndex->value[3] = fread_spnumber(fp);
        pObjIndex->value[4] = 0;
        pObjIndex->level = 0;
        pObjIndex->condition = 100;
        pObjIndex->weight = fread_number(fp);
        pObjIndex->cost = fread_number(fp); /* Unused */
        /* Cost per day */ fread_number(fp);

        if (pObjIndex->item_type == ITEM_WEAPON) {
            if (is_name("two", pObjIndex->name) || is_name("two-handed", pObjIndex->name)
                || is_name("claymore", pObjIndex->name))
                SET_BIT(pObjIndex->value[4], WEAPON_TWO_HANDS);
        }

        for (;;) {
            char letter;

            letter = fread_letter(fp);

            if (letter == 'A') {
                AFFECT_DATA *paf;

                paf = static_cast<AFFECT_DATA *>(alloc_perm(sizeof(*paf)));
                paf->type = -1;
                paf->level = 20; /* RT temp fix */
                paf->duration = -1;
                paf->location = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->bitvector = 0;
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == RESETS_EQUIP_OBJ_MOB) {
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

        /* fix armors */
        if (pObjIndex->item_type == ITEM_ARMOR) {
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[1];
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch (pObjIndex->item_type) {
        case ITEM_PILL:
        case ITEM_BOMB:
        case ITEM_POTION:
        case ITEM_SCROLL:
            pObjIndex->value[1] = slot_lookup(pObjIndex->value[1]);
            pObjIndex->value[2] = slot_lookup(pObjIndex->value[2]);
            pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
            pObjIndex->value[4] = slot_lookup(pObjIndex->value[4]);
            break;

        case ITEM_DRINK_CON:
            if (pObjIndex->value[2] > LIQ_MAX) {
                bug("Liquid number out of range!");
                exit(1);
            }
            // falls through
        case ITEM_STAFF:
        case ITEM_WAND: pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]); break;
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
    }
}

/*
 * Adds a reset to a room.
 */
void new_reset(ROOM_INDEX_DATA *pR, RESET_DATA *pReset) {
    RESET_DATA *pr;

    if (!pR) {
        return;
    }
    pr = pR->reset_last;

    if (!pr) {
        pR->reset_first = pReset;
        pR->reset_last = pReset;
    } else {
        pR->reset_last->next = pReset;
        pR->reset_last = pReset;
        pR->reset_last->next = nullptr;
    }

    top_reset++;
}

void load_resets(FILE *fp) {
    RESET_DATA *pReset;
    ROOM_INDEX_DATA *pRoomIndex;
    EXIT_DATA *pexit;
    int iLastRoom = 0;
    int iLastObj = 0;

    if (area_last == nullptr) {
        bug("Load_resets: no #AREA seen yet.");
        exit(1);
    }

    for (;;) {

        char letter;
        if ((letter = fread_letter(fp)) == RESETS_END_SECTION)
            break;

        if (letter == RESETS_COMMENT) {
            fread_to_eol(fp);
            continue;
        }

        pReset = static_cast<RESET_DATA *>(alloc_perm(sizeof(*pReset)));
        pReset->command = letter;
        /* if_flag */ fread_number(fp);
        pReset->arg1 = fread_number(fp);

        pReset->arg2 = fread_number(fp);

        pReset->arg3 = (letter == RESETS_GIVE_OBJ_MOB || letter == RESETS_RANDOMIZE_EXITS) ? 0 : fread_number(fp);

        if (letter == RESETS_PUT_OBJ_OBJ || letter == RESETS_MOB_IN_ROOM) {
            pReset->arg4 = fread_number(fp);
        } else
            pReset->arg4 = 0;

        fread_to_eol(fp);

        /* Don't validate now, do it after all area's have been loaded */
        /* Stuff to add reset to the correct room */
        switch (letter) {
        default:
            bug("Load_resets: bad command '%c'.", letter);
            exit(1);
            break;
        case RESETS_MOB_IN_ROOM:
            if ((pRoomIndex = get_room_index(pReset->arg3))) {
                new_reset(pRoomIndex, pReset);
                iLastRoom = pReset->arg3;
            }
            break;
        case RESETS_OBJ_IN_ROOM:
            if ((pRoomIndex = get_room_index(pReset->arg3))) {
                new_reset(pRoomIndex, pReset);
                iLastObj = pReset->arg3;
            }
            break;
        case RESETS_PUT_OBJ_OBJ:
            if ((pRoomIndex = get_room_index(iLastObj)))
                new_reset(pRoomIndex, pReset);
            break;
        case RESETS_GIVE_OBJ_MOB:
        case RESETS_EQUIP_OBJ_MOB:
            if ((pRoomIndex = get_room_index(iLastRoom))) {
                new_reset(pRoomIndex, pReset);
                iLastObj = iLastRoom;
            }
            break;
        case RESETS_EXIT_FLAGS:
            pRoomIndex = get_room_index(pReset->arg1);

            if (pReset->arg2 < 0 || pReset->arg2 > 5 || !pRoomIndex
                || (pexit = pRoomIndex->exit[pReset->arg2]) == nullptr || !IS_SET(pexit->rs_flags, EX_ISDOOR)) {
                bug("Load_resets: 'D': exit %d not door.", pReset->arg2);
                exit(1);
            }

            switch (pReset->arg3) {
            default: bug("Load_resets: 'D': bad 'locks': %d.", pReset->arg3);
            case 0: break;
            case 1: SET_BIT(pexit->rs_flags, EX_CLOSED); break;
            case 2: SET_BIT(pexit->rs_flags, EX_CLOSED | EX_LOCKED); break;
            }
            break;
        case RESETS_RANDOMIZE_EXITS:
            if (pReset->arg2 < 0 || pReset->arg2 > 6) {
                bug("Load_resets: 'R': bad exit %d.", pReset->arg2);
                exit(1);
            }

            if ((pRoomIndex = get_room_index(pReset->arg1)))
                new_reset(pRoomIndex, pReset);

            break;
        }

        pReset->next = nullptr;
        top_reset++;
    }
}

void validate_resets() {
    AREA_DATA *pArea;
    OBJ_INDEX_DATA *temp_index;
    ROOM_INDEX_DATA *pRoom;
    RESET_DATA *pReset, *pReset_next, *pReset_last;
    int vnum;
    bool Okay, oldBoot;
    char buf[MAX_STRING_LENGTH];

    for (pArea = area_first; pArea != nullptr; pArea = pArea->next) {
        for (vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++) {
            oldBoot = fBootDb;
            fBootDb = false;
            pRoom = get_room_index(vnum);
            fBootDb = oldBoot;

            pReset_last = nullptr;
            if (pRoom) {
                for (pReset = pRoom->reset_first; pReset; pReset = pReset_next) {
                    pReset_next = pReset->next;
                    Okay = true;

                    /*
                     * Validate parameters.
                     * We're calling the index functions for the side effect.
                     */
                    switch (pReset->command) {
                    case RESETS_MOB_IN_ROOM:
                        if (!(get_mob_index(pReset->arg1))) {
                            Okay = false;
                            snprintf(buf, sizeof(buf), "Get_mob_index: bad vnum %d in reset in room %d.", pReset->arg1,
                                     pRoom->vnum);
                            bug(buf, 0);
                        }
                        break;
                    case RESETS_OBJ_IN_ROOM:
                        temp_index = get_obj_index(pReset->arg1);
                        if (temp_index)
                            temp_index->reset_num++;
                        else {
                            Okay = false;
                            snprintf(buf, sizeof(buf), "Get_obj_index: bad vnum %d in reset in room %d.", pReset->arg1,
                                     pRoom->vnum);
                            bug(buf, 0);
                        }
                        break;
                    case RESETS_PUT_OBJ_OBJ:
                        temp_index = get_obj_index(pReset->arg1);
                        if (temp_index)
                            temp_index->reset_num++;
                        else {
                            Okay = false;
                            snprintf(buf, sizeof(buf), "Get_obj_index: bad vnum %d in reset in room %d.", pReset->arg1,
                                     pRoom->vnum);
                            bug(buf, 0);
                        }
                        break;
                    case RESETS_GIVE_OBJ_MOB:
                    case RESETS_EQUIP_OBJ_MOB:
                        temp_index = get_obj_index(pReset->arg1);
                        if (temp_index)
                            temp_index->reset_num++;
                        else {
                            Okay = false;
                            snprintf(buf, sizeof(buf), "Get_obj_index: bad vnum %d in reset in room %d.", pReset->arg1,
                                     pRoom->vnum);
                            bug(buf, 0);
                        }
                        break;
                    case RESETS_EXIT_FLAGS:
                    case RESETS_RANDOMIZE_EXITS: break;
                    }

                    if (!Okay) {
                        /* This is a wrong reset. Remove it. */
                        if (pReset_last)
                            pReset_last->next = pReset_next;
                        else
                            pRoom->reset_first = pReset_next;
                        /* free_reset_data(pReset); TODO: This is old code, figure out if it's needed... */
                    } else
                        pReset_last = pReset;
                }
            }
        }
    }
}

/* Snarf a room section. */
void load_rooms(FILE *fp) {
    ROOM_INDEX_DATA *pRoomIndex;

    if (area_last == nullptr) {
        bug("Load_resets: no #AREA seen yet.");
        exit(1);
    }

    for (;;) {
        sh_int vnum;
        char letter;
        int door;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_rooms: # not found.");
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        if (get_room_index(vnum) != nullptr) {
            bug("Load_rooms: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pRoomIndex = static_cast<ROOM_INDEX_DATA *>(alloc_perm(sizeof(*pRoomIndex)));
        pRoomIndex->people = nullptr;
        pRoomIndex->contents = nullptr;
        pRoomIndex->extra_descr = nullptr;
        pRoomIndex->area = area_last;
        pRoomIndex->vnum = vnum;
        pRoomIndex->name = fread_string(fp);
        pRoomIndex->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        pRoomIndex->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            SET_BIT(pRoomIndex->room_flags, ROOM_LAW);
        pRoomIndex->sector_type = fread_number(fp);
        pRoomIndex->light = 0;
        for (door = 0; door <= 5; door++)
            pRoomIndex->exit[door] = nullptr;

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S')
                break;

            if (letter == 'D') {
                EXIT_DATA *pexit;
                int locks;

                door = fread_number(fp);
                if (door < 0 || door > 5) {
                    bug("Fread_rooms: vnum %d has bad door number.", vnum);
                    exit(1);
                }

                pexit = static_cast<EXIT_DATA *>(alloc_perm(sizeof(*pexit)));
                pexit->description = fread_string(fp);
                pexit->keyword = fread_string(fp);
                pexit->exit_info = 0;
                locks = fread_number(fp);
                pexit->key = fread_number(fp);
                pexit->u1.vnum = fread_number(fp);

                pexit->rs_flags = 0;

                switch (locks) {

                    /* the following statements assign rs_flags, replacing
                            exit_info which is what used to get set. */
                case 1: pexit->rs_flags = EX_ISDOOR; break;
                case 2: pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF; break;
                case 3: pexit->rs_flags = EX_ISDOOR | EX_PASSPROOF; break;
                case 4: pexit->rs_flags = EX_ISDOOR | EX_PASSPROOF | EX_PICKPROOF; break;
                }

                pRoomIndex->exit[door] = pexit;
                top_exit++;
            } else if (letter == 'E') {
                EXTRA_DESCR_DATA *ed;

                ed = static_cast<EXTRA_DESCR_DATA *>(alloc_perm(sizeof(*ed)));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pRoomIndex->extra_descr;
                pRoomIndex->extra_descr = ed;
                top_ed++;
            } else {
                bug("Load_rooms: vnum %d has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pRoomIndex->next = room_index_hash[iHash];
        room_index_hash[iHash] = pRoomIndex;
        top_room++;
    }
}

/* Snarf a shop section. */
void load_shops(FILE *fp) {
    SHOP_DATA *pShop;

    for (;;) {
        MOB_INDEX_DATA *pMobIndex;
        int iTrade;

        pShop = static_cast<SHOP_DATA *>(alloc_perm(sizeof(*pShop)));
        pShop->keeper = fread_number(fp);
        if (pShop->keeper == 0)
            break;
        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
            pShop->buy_type[iTrade] = fread_number(fp);
        pShop->profit_buy = fread_number(fp);
        pShop->profit_sell = fread_number(fp);
        pShop->open_hour = fread_number(fp);
        pShop->close_hour = fread_number(fp);
        fread_to_eol(fp);
        pMobIndex = get_mob_index(pShop->keeper);
        pMobIndex->pShop = pShop;

        if (shop_first == nullptr)
            shop_first = pShop;
        if (shop_last != nullptr)
            shop_last->next = pShop;

        shop_last = pShop;
        pShop->next = nullptr;
        top_shop++;
    }
}

/* Snarf spec proc declarations. */
void load_specials(FILE *fp) {
    for (;;) {
        MOB_INDEX_DATA *pMobIndex;
        char letter;

        switch (letter = fread_letter(fp)) {
        default: bug("Load_specials: letter '%c' not *, M or S.", letter); exit(1);

        case 'S': return;

        case '*': break;

        case 'M':
            pMobIndex = get_mob_index(fread_number(fp));
            pMobIndex->spec_fun = spec_lookup(fread_word(fp));
            if (pMobIndex->spec_fun == 0) {
                bug("Load_specials: 'M': vnum %d.", pMobIndex->vnum);
                exit(1);
            }
            break;
        }

        fread_to_eol(fp);
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits() {
    extern const sh_int rev_dir[];
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *pRoomIndex;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;
    int iHash;
    int door;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (pRoomIndex = room_index_hash[iHash]; pRoomIndex != nullptr; pRoomIndex = pRoomIndex->next) {
            bool fexit;

            fexit = false;
            for (door = 0; door <= 5; door++) {
                if ((pexit = pRoomIndex->exit[door]) != nullptr) {
                    if (pexit->u1.vnum <= 0 || get_room_index(pexit->u1.vnum) == nullptr)
                        pexit->u1.to_room = nullptr;
                    else {
                        fexit = true;
                        pexit->u1.to_room = get_room_index(pexit->u1.vnum);
                    }
                }
            }
            if (!fexit)
                SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
        }
    }

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (pRoomIndex = room_index_hash[iHash]; pRoomIndex != nullptr; pRoomIndex = pRoomIndex->next) {
            for (door = 0; door <= 5; door++) {
                if ((pexit = pRoomIndex->exit[door]) != nullptr && (to_room = pexit->u1.to_room) != nullptr
                    && (pexit_rev = to_room->exit[rev_dir[door]]) != nullptr && pexit_rev->u1.to_room != pRoomIndex
                    && (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299)) {
                    snprintf(buf, sizeof(buf), "Fix_exits: %d:%d -> %d:%d -> %d.", pRoomIndex->vnum, door,
                             to_room->vnum, rev_dir[door],
                             (pexit_rev->u1.to_room == nullptr) ? 0 : pexit_rev->u1.to_room->vnum);
                    bug(buf, 0);
                }
            }
        }
    }
}

/* Repopulate areas periodically. */
void area_update() {
    AREA_DATA *pArea;

    for (pArea = area_first; pArea != nullptr; pArea = pArea->next) {

        if (++pArea->age < 3)
            continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ((!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15)) || pArea->age >= 31) {
            ROOM_INDEX_DATA *pRoomIndex;

            reset_area(pArea);
            pArea->age = number_range(0, 3);
            pRoomIndex = get_room_index(ROOM_VNUM_SCHOOL);
            if (pRoomIndex != nullptr && pArea == pRoomIndex->area)
                pArea->age = 15 - 2;
            else if (pArea->nplayer == 0)
                pArea->empty = true;
        }
    }
}

/*
 * Reset one room.  Called by reset_area.
 */
void reset_room(ROOM_INDEX_DATA *pRoom) {
    extern const sh_int rev_dir[];
    RESET_DATA *pReset;
    CHAR_DATA *pMob;
    OBJ_DATA *pObj;
    CHAR_DATA *LastMob = nullptr;
    OBJ_DATA *LastObj = nullptr;
    int iExit;
    int level = 0;
    bool last;

    if (!pRoom)
        return;

    pMob = nullptr;
    last = false;

    for (iExit = 0; iExit < MAX_DIR; iExit++) {
        EXIT_DATA *pExit;
        if ((pExit = pRoom->exit[iExit])) {
            pExit->exit_info = pExit->rs_flags;
            if ((pExit->u1.to_room != nullptr) && ((pExit = pExit->u1.to_room->exit[rev_dir[iExit]]))) {
                /* nail the other side */
                pExit->exit_info = pExit->rs_flags;
            }
        }
    }

    for (pReset = pRoom->reset_first; pReset != nullptr; pReset = pReset->next) {
        MOB_INDEX_DATA *pMobIndex;
        OBJ_INDEX_DATA *pObjIndex;
        OBJ_INDEX_DATA *pObjToIndex;
        ROOM_INDEX_DATA *pRoomIndex;
        int count, limit;

        switch (pReset->command) {
        default: bug("Reset_room: bad command %c.", pReset->command); break;

        case RESETS_MOB_IN_ROOM:
            if (!(pMobIndex = get_mob_index(pReset->arg1))) {
                bug("Reset_room: 'M': bad vnum %d.", pReset->arg1);
                continue;
            }
            if (pMobIndex->count >= pReset->arg2) {
                last = false;
                break;
            }
            count = 0;
            for (pMob = pRoom->people; pMob != nullptr; pMob = pMob->next_in_room) {
                if (pMob->pIndexData == pMobIndex) {
                    count++;
                    if (count >= pReset->arg4) {
                        last = false;
                        break;
                    }
                }
            }

            if (count >= pReset->arg4)
                break;

            pMob = create_mobile(pMobIndex);

            /*
             * Pet shop mobiles get ACT_PET set.
             */
            {
                ROOM_INDEX_DATA *pRoomIndexPrev;

                pRoomIndexPrev = get_room_index(pRoom->vnum - 1);
                if (pRoomIndexPrev && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                    SET_BIT(pMob->act, ACT_PET);
            }

            char_to_room(pMob, pRoom);

            LastMob = pMob;
            level = URANGE(0, pMob->level - 2, LEVEL_HERO - 1);
            last = true;
            break;

        case RESETS_OBJ_IN_ROOM:
            if (!(pObjIndex = get_obj_index(pReset->arg1))) {
                bug("Reset_room: 'O': bad vnum %d.", pReset->arg1);
                continue;
            }

            if (!(pRoomIndex = get_room_index(pReset->arg3))) {
                bug("Reset_room: 'O': bad vnum %d.", pReset->arg3);
                continue;
            }

            if (pRoom->area->nplayer > 0 || count_obj_list(pObjIndex, pRoom->contents) > 0) {
                last = false;
                break;
            }

            pObj = create_object(pObjIndex, number_fuzzy(level));
            pObj->cost = 0;
            obj_to_room(pObj, pRoom);
            break;

        case RESETS_PUT_OBJ_OBJ:
            if (!(pObjIndex = get_obj_index(pReset->arg1))) {
                bug("Reset_room: 'P': bad vnum %d.", pReset->arg1);
                continue;
            }

            if (!(pObjToIndex = get_obj_index(pReset->arg3))) {
                bug("Reset_room: 'P': bad vnum %d.", pReset->arg3);
                continue;
            }

            if (pReset->arg2 > 20) /* old format reduced from 50! */
                limit = 6;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

            if (pRoom->area->nplayer > 0 || (LastObj = get_obj_type(pObjToIndex)) == nullptr
                || (LastObj->in_room == nullptr && !last) || (pObjIndex->count >= limit && number_range(0, 4) != 0)
                || (count = count_obj_list(pObjIndex, LastObj->contains)) > pReset->arg4) {
                last = false;
                break;
            }

            while (count < pReset->arg4) {
                pObj = create_object(pObjIndex, number_fuzzy(LastObj->level));
                obj_to_obj(pObj, LastObj);
                count++;
                if (pObjIndex->count >= limit)
                    break;
            }

            /*
             * Ensure that the container gets reset.
             */
            if (LastObj->item_type == ITEM_CONTAINER) {
                LastObj->value[1] = LastObj->pIndexData->value[1];
            }
            last = true;
            break;

        case RESETS_GIVE_OBJ_MOB:
        case RESETS_EQUIP_OBJ_MOB:
            if (!(pObjIndex = get_obj_index(pReset->arg1))) {
                bug("Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1);
                continue;
            }

            if (!last)
                break;

            if (!LastMob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum %d.", pReset->arg1);
                last = false;
                break;
            }

            if (LastMob->pIndexData->pShop) /* Shop-keeper? */
            {
                int olevel = 0, i, j;

                if (!pObjIndex->new_format)
                    switch (pObjIndex->item_type) {
                    case ITEM_PILL:
                    case ITEM_POTION:
                    case ITEM_SCROLL:
                        olevel = 53;
                        for (i = 1; i < 5; i++) {
                            if (pObjIndex->value[i] > 0) {
                                for (j = 0; j < MAX_CLASS; j++) {
                                    olevel = UMIN(olevel, skill_table[pObjIndex->value[i]].skill_level[j]);
                                }
                            }
                        }

                        olevel = UMAX(0, (olevel * 3 / 4) - 2);
                        break;
                    case ITEM_WAND: olevel = number_range(10, 20); break;
                    case ITEM_STAFF: olevel = number_range(15, 25); break;
                    case ITEM_ARMOR: olevel = number_range(5, 15); break;
                    case ITEM_WEAPON: olevel = number_range(5, 15); break;
                    case ITEM_TREASURE: olevel = number_range(10, 20); break;
                    }

                pObj = create_object(pObjIndex, olevel);
                SET_BIT(pObj->extra_flags, ITEM_INVENTORY);
            }

            else {
                if (pReset->arg2 > 50) /* old format */
                    limit = 6;
                else if (pReset->arg2 == -1) /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;

                if (pObjIndex->count < limit || number_range(0, 4) == 0) {
                    pObj = create_object(pObjIndex, UMIN(number_fuzzy(level), LEVEL_HERO - 1));
#ifdef notdef /* Hack for object levels */
                    /* error message if it is too high */
                    if (pObj->level > LastMob->level + 3
                        || (pObj->item_type == ITEM_WEAPON && pReset->command == 'E' && pObj->level < LastMob->level - 5
                            && pObj->level < 45))
                        fprintf(stderr, "Err: obj %s (%d) -- %d, mob %s (%d) -- %d\n", pObj->short_descr,
                                pObj->pIndexData->vnum, pObj->level, LastMob->short_descr, LastMob->pIndexData->vnum,
                                LastMob->level);
#endif
                } else
                    break;
            }

            obj_to_char(pObj, LastMob);
            if (pReset->command == RESETS_EQUIP_OBJ_MOB)
                equip_char(LastMob, pObj, pReset->arg3);
            last = true;
            break;

        case RESETS_EXIT_FLAGS: break;

        case RESETS_RANDOMIZE_EXITS:
            if (!(pRoomIndex = get_room_index(pReset->arg1))) {
                bug("Reset_room: 'R': bad vnum %d.", pReset->arg1);
                continue;
            }

            {
                EXIT_DATA *pExit;
                int d0;
                int d1;

                for (d0 = 0; d0 < pReset->arg2 - 1; d0++) {
                    d1 = number_range(d0, pReset->arg2 - 1);
                    pExit = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = pExit;
                }
            }
            break;
        }
    }
}

/*
 * Reset one area.
 */
void reset_area(AREA_DATA *pArea) {
    ROOM_INDEX_DATA *pRoom;
    int vnum;

    for (vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++) {
        if ((pRoom = get_room_index(vnum)))
            reset_room(pRoom);
    }
}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile(MOB_INDEX_DATA *pMobIndex) {
    CHAR_DATA *mob;
    int i;

    mobile_count++;

    if (pMobIndex == nullptr) {
        bug("Create_mobile: nullptr pMobIndex.");
        exit(1);
    }

    if (char_free == nullptr) {
        mob = static_cast<CHAR_DATA *>(alloc_perm(sizeof(*mob)));
    } else {
        mob = char_free;
        char_free = char_free->next;
    }

    clear_char(mob);
    mob->pIndexData = pMobIndex;

    mob->name = str_dup(pMobIndex->player_name);
    mob->short_descr = str_dup(pMobIndex->short_descr);
    mob->long_descr = str_dup(pMobIndex->long_descr);
    mob->description = str_dup(pMobIndex->description);
    mob->spec_fun = pMobIndex->spec_fun;

    if (pMobIndex->new_format)
    /* load in new style */
    {
        /* read from prototype */
        mob->act = pMobIndex->act;
        mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
        mob->affected_by = pMobIndex->affected_by;
        mob->alignment = pMobIndex->alignment;
        mob->level = pMobIndex->level;
        mob->hitroll = pMobIndex->hitroll;
        mob->damroll = pMobIndex->damage[DICE_BONUS];
        mob->max_hit = dice(pMobIndex->hit[DICE_NUMBER], pMobIndex->hit[DICE_TYPE]) + pMobIndex->hit[DICE_BONUS];
        mob->hit = mob->max_hit;
        mob->max_mana = dice(pMobIndex->mana[DICE_NUMBER], pMobIndex->mana[DICE_TYPE]) + pMobIndex->mana[DICE_BONUS];
        mob->mana = mob->max_mana;
        mob->damage[DICE_NUMBER] = pMobIndex->damage[DICE_NUMBER];
        mob->damage[DICE_TYPE] = pMobIndex->damage[DICE_TYPE];
        mob->dam_type = pMobIndex->dam_type;
        for (i = 0; i < 4; i++)
            mob->armor[i] = pMobIndex->ac[i];
        mob->off_flags = pMobIndex->off_flags;
        mob->imm_flags = pMobIndex->imm_flags;
        mob->res_flags = pMobIndex->res_flags;
        mob->vuln_flags = pMobIndex->vuln_flags;
        mob->start_pos = pMobIndex->start_pos;
        mob->default_pos = pMobIndex->default_pos;
        mob->sex = pMobIndex->sex;
        if (mob->sex == 3) /* random sex */
            mob->sex = number_range(1, 2);
        mob->race = pMobIndex->race;
        if (pMobIndex->gold == 0)
            mob->gold = 0;
        else
            mob->gold = number_range(pMobIndex->gold / 2, pMobIndex->gold * 3 / 2);
        mob->form = pMobIndex->form;
        mob->parts = pMobIndex->parts;
        mob->size = pMobIndex->size;
        mob->material = pMobIndex->material;

        /* computed on the spot */

        for (i = 0; i < MAX_STATS; i++)
            mob->perm_stat[i] = UMIN(25, 11 + mob->level / 4);

        if (IS_SET(mob->act, ACT_WARRIOR)) {
            mob->perm_stat[STAT_STR] += 3;
            mob->perm_stat[STAT_INT] -= 1;
            mob->perm_stat[STAT_CON] += 2;
        }

        if (IS_SET(mob->act, ACT_THIEF)) {
            mob->perm_stat[STAT_DEX] += 3;
            mob->perm_stat[STAT_INT] += 1;
            mob->perm_stat[STAT_WIS] -= 1;
        }

        if (IS_SET(mob->act, ACT_CLERIC)) {
            mob->perm_stat[STAT_WIS] += 3;
            mob->perm_stat[STAT_DEX] -= 1;
            mob->perm_stat[STAT_STR] += 1;
        }

        if (IS_SET(mob->act, ACT_MAGE)) {
            mob->perm_stat[STAT_INT] += 3;
            mob->perm_stat[STAT_STR] -= 1;
            mob->perm_stat[STAT_DEX] += 1;
        }

        if (IS_SET(mob->off_flags, OFF_FAST))
            mob->perm_stat[STAT_DEX] += 2;

        mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
        mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;
    } else /* read in old format and convert */
    {
        mob->act = pMobIndex->act | ACT_WARRIOR;
        mob->affected_by = pMobIndex->affected_by;
        mob->alignment = pMobIndex->alignment;
        mob->level = pMobIndex->level;
        mob->hitroll = pMobIndex->hitroll + (pMobIndex->level / 3);
        mob->damroll = pMobIndex->level / 2;
        mob->max_hit = mob->level * 8 + number_range(mob->level * mob->level / 4, mob->level * mob->level);
        mob->max_hit *= .9;
        mob->hit = mob->max_hit;
        mob->max_mana = 100 + dice(mob->level, 10);
        mob->mana = mob->max_mana;
        switch (number_range(1, 3)) {
        case (1): mob->dam_type = 3; break; /* slash */
        case (2): mob->dam_type = 7; break; /* pound */
        case (3): mob->dam_type = 11; break; /* pierce */
        }
        for (i = 0; i < 3; i++)
            mob->armor[i] = interpolate(mob->level, 100, -100);
        mob->armor[3] = interpolate(mob->level, 100, 0);
        mob->race = pMobIndex->race;
        mob->off_flags = pMobIndex->off_flags;
        mob->imm_flags = pMobIndex->imm_flags;
        mob->res_flags = pMobIndex->res_flags;
        mob->vuln_flags = pMobIndex->vuln_flags;
        mob->start_pos = pMobIndex->start_pos;
        mob->default_pos = pMobIndex->default_pos;
        mob->sex = pMobIndex->sex;
        mob->gold = pMobIndex->gold / 100;
        mob->form = pMobIndex->form;
        mob->parts = pMobIndex->parts;
        mob->size = SIZE_MEDIUM;
        mob->material = 0;

        for (i = 0; i < MAX_STATS; i++)
            mob->perm_stat[i] = 11 + mob->level / 4;
    }

    mob->position = mob->start_pos;

    /* link the mob to the world list */
    mob->next = char_list;
    char_list = mob;
    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone) {
    int i;
    AFFECT_DATA *paf;

    if (parent == nullptr || clone == nullptr || !IS_NPC(parent))
        return;

    /* start fixing values */
    clone->name = str_dup(parent->name);
    clone->version = parent->version;
    clone->short_descr = str_dup(parent->short_descr);
    clone->long_descr = str_dup(parent->long_descr);
    clone->description = str_dup(parent->description);
    clone->sex = parent->sex;
    clone->class_num = parent->class_num;
    clone->race = parent->race;
    clone->level = parent->level;
    clone->trust = 0;
    clone->timer = parent->timer;
    clone->wait = parent->wait;
    clone->hit = parent->hit;
    clone->max_hit = parent->max_hit;
    clone->mana = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move = parent->move;
    clone->max_move = parent->max_move;
    clone->gold = parent->gold;
    clone->exp = parent->exp;
    clone->act = parent->act;
    clone->comm = parent->comm;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affected_by = parent->affected_by;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment = parent->alignment;
    clone->hitroll = parent->hitroll;
    clone->damroll = parent->damroll;
    clone->wimpy = parent->wimpy;
    clone->form = parent->form;
    clone->parts = parent->parts;
    clone->size = parent->size;
    clone->material = parent->material;
    clone->off_flags = parent->off_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos = parent->start_pos;
    clone->default_pos = parent->default_pos;
    clone->spec_fun = parent->spec_fun;

    for (i = 0; i < 4; i++)
        clone->armor[i] = parent->armor[i];

    for (i = 0; i < MAX_STATS; i++) {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i] = parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
        clone->damage[i] = parent->damage[i];

    /* now add the affects */
    for (paf = parent->affected; paf != nullptr; paf = paf->next)
        affect_to_char(clone, paf);
}

/*
 * Create an instance of an object.
 * TheMoog 1/10/2k : fixes up portal objects - value[0] of a portal
 * if non-zero is looked up and then destination set accordingly.
 */
OBJ_DATA *create_object(OBJ_INDEX_DATA *pObjIndex, int level) {
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;

    if (pObjIndex == nullptr) {
        bug("Create_object: nullptr pObjIndex.");
        exit(1);
    }

    if (obj_free == nullptr) {
        obj = static_cast<OBJ_DATA *>(alloc_perm(sizeof(*obj)));
    } else {
        obj = obj_free;
        obj_free = obj_free->next;
    }

    *obj = obj_zero;
    obj->pIndexData = pObjIndex;
    obj->in_room = nullptr;
    obj->enchanted = false;

    if (pObjIndex->new_format)
        obj->level = pObjIndex->level;
    else
        obj->level = UMAX(0, level);
    obj->wear_loc = -1;

    obj->name = str_dup(pObjIndex->name);
    obj->short_descr = str_dup(pObjIndex->short_descr);
    obj->description = str_dup(pObjIndex->description);
    obj->material = pObjIndex->material;
    obj->item_type = pObjIndex->item_type;
    obj->extra_flags = pObjIndex->extra_flags;
    obj->wear_flags = pObjIndex->wear_flags;
    obj->wear_string = pObjIndex->wear_string;
    obj->value[0] = pObjIndex->value[0];
    obj->value[1] = pObjIndex->value[1];
    obj->value[2] = pObjIndex->value[2];
    obj->value[3] = pObjIndex->value[3];
    obj->value[4] = pObjIndex->value[4];
    obj->weight = pObjIndex->weight;

    if (level == -1 || pObjIndex->new_format)
        obj->cost = pObjIndex->cost;
    else
        obj->cost = number_fuzzy(10) * number_fuzzy(level) * number_fuzzy(level);

    /*
     * Mess with object properties.
     */
    switch (obj->item_type) {
    default: bug("Read_object: vnum %d bad type.", pObjIndex->vnum); break;

    case ITEM_LIGHT:
        if (obj->value[2] == 999)
            obj->value[2] = -1;
        break;
    case ITEM_TREASURE:
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_BOMB: break;

    case ITEM_PORTAL:
        if (obj->value[0] != 0) {
            obj->destination = get_room_index(obj->value[0]);
            if (!obj->destination)
                bug("Couldn't find room index %d for a portal (vnum %d)", obj->value[0], pObjIndex->vnum);
            obj->value[0] = 0; // Prevents ppl ever finding the vnum in the obj
        }
        break;

    case ITEM_SCROLL:
        if (level != -1 && !pObjIndex->new_format)
            obj->value[0] = number_fuzzy(obj->value[0]);
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        if (level != -1 && !pObjIndex->new_format) {
            obj->value[0] = number_fuzzy(obj->value[0]);
            obj->value[1] = number_fuzzy(obj->value[1]);
            obj->value[2] = obj->value[1];
        }
        break;

    case ITEM_WEAPON:
        if (level != -1 && !pObjIndex->new_format) {
            obj->value[1] = number_fuzzy(number_fuzzy(1 * level / 4 + 2));
            obj->value[2] = number_fuzzy(number_fuzzy(3 * level / 4 + 6));
        }
        break;

    case ITEM_ARMOR:
        if (level != -1 && !pObjIndex->new_format) {
            obj->value[0] = number_fuzzy(level / 5 + 3);
            obj->value[1] = number_fuzzy(level / 5 + 3);
            obj->value[2] = number_fuzzy(level / 5 + 3);
        }
        break;

    case ITEM_POTION:
    case ITEM_PILL:
        if (level != -1 && !pObjIndex->new_format)
            obj->value[0] = number_fuzzy(number_fuzzy(obj->value[0]));
        break;

    case ITEM_MONEY:
        if (!pObjIndex->new_format)
            obj->value[0] = obj->cost;
        break;
    }

    obj->next = object_list;
    object_list = obj;
    pObjIndex->count++;

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone) {
    int i;
    AFFECT_DATA *paf;
    /*    EXTRA_DESCR_DATA *ed,*ed_new; */

    if (parent == nullptr || clone == nullptr)
        return;

    /* start fixing the object */
    clone->name = str_dup(parent->name);
    clone->short_descr = str_dup(parent->short_descr);
    clone->description = str_dup(parent->description);
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->material = parent->material;
    clone->timer = parent->timer;

    for (i = 0; i < 5; i++)
        clone->value[i] = parent->value[i];

    /* affects */
    clone->enchanted = parent->enchanted;

    for (paf = parent->affected; paf != nullptr; paf = paf->next)
        affect_to_obj(clone, paf);

    /* extended desc */
    /*
        for (ed = parent->extra_descr; ed != nullptr; ed = ed->next);
        {
            ed_new              = alloc_perm( sizeof(*ed_new) );

            ed_new->keyword       = str_dup( ed->keyword);
            ed_new->description     = str_dup( ed->description );
            ed_new->next             = clone->extra_descr;
            clone->extra_descr    = ed_new;
        }
    */
}

/*
 * Clear a new character.
 */
void clear_char(CHAR_DATA *ch) {
    static CHAR_DATA ch_zero;
    int i;

    memset(ch, 0, sizeof(CHAR_DATA)); /* Added by TM */

    *ch = ch_zero;
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->logon = current_time;
    ch->last_note = 0;
    ch->lines = PAGELEN;
    for (i = 0; i < 4; i++)
        ch->armor[i] = 100;
    ch->comm = 0;
    ch->position = POS_STANDING;
    ch->practice = 0;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->riding = nullptr;
    ch->ridden_by = nullptr;
    ch->clipboard = nullptr;
    for (i = 0; i < MAX_STATS; i++) {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }
}

/*
 * Free a character.
 */
void free_char(CHAR_DATA *ch) {
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if (IS_NPC(ch))
        mobile_count--;

    for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
        obj_next = obj->next_content;
        extract_obj(obj);
    }

    for (paf = ch->affected; paf != nullptr; paf = paf_next) {
        paf_next = paf->next;
        affect_remove(ch, paf);
    }

    free_string(ch->name);
    free_string(ch->short_descr);
    free_string(ch->long_descr);
    free_string(ch->description);

    if (ch->clipboard)
        free_string(ch->clipboard);

    if (ch->pcdata != nullptr) {
        free_string(ch->pcdata->pwd);
        free_string(ch->pcdata->bamfin);
        free_string(ch->pcdata->bamfout);
        free_string(ch->pcdata->title);
        free_string(ch->pcdata->prefix); /* PCFN added */
        if (ch->pcdata->pcclan)
            free_mem(ch->pcdata->pcclan, sizeof(*(ch->pcdata->pcclan)));
        ch->pcdata->next = pcdata_free;
        pcdata_free = ch->pcdata;
    }

    ch->next = char_free;
    char_free = ch;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA *ed) {
    for (; ed != nullptr; ed = ed->next) {
        if (is_name((char *)name, ed->keyword))
            return ed->description;
    }
    return nullptr;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index(int vnum) {
    MOB_INDEX_DATA *pMobIndex;

    for (pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH]; pMobIndex != nullptr; pMobIndex = pMobIndex->next) {
        if (pMobIndex->vnum == vnum)
            return pMobIndex;
    }

    if (fBootDb) {
        bug("Get_mob_index: bad vnum %d.", vnum);
        exit(1);
    }

    return nullptr;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index(int vnum) {
    OBJ_INDEX_DATA *pObjIndex;

    for (pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH]; pObjIndex != nullptr; pObjIndex = pObjIndex->next) {
        if (pObjIndex->vnum == vnum)
            return pObjIndex;
    }

    if (fBootDb) {
        bug("Get_obj_index: bad vnum %d.", vnum);
        exit(1);
    }

    return nullptr;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index(int vnum) {
    ROOM_INDEX_DATA *pRoomIndex;

    for (pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex != nullptr; pRoomIndex = pRoomIndex->next) {
        if (pRoomIndex->vnum == vnum)
            return pRoomIndex;
    }

    if (fBootDb) {
        bug("Get_room_index: bad vnum %d.", vnum);
        exit(1);
    }

    return nullptr;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE *fp) {
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    return c;
}

/*
 * Read a number from a file.
 */
int fread_number(FILE *fp) {
    int number;
    bool sign;
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = false;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = true;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        bug("Fread_number: bad format.");
        exit(1);
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if (c != ' ')
        ungetc(c, fp);

    return number;
}
/*
 * Read a number from a file.
 */
int fread_spnumber(FILE *fp) {
    int number;
    bool sign;
    char c;
    char *spell_name;
    char long_spell_name[64];

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = false;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = true;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        if (c == '\'' || c == '"') {
            char endquote = c;
            /* MG> Replaced the elided C with something which works reliably! */
            for (spell_name = long_spell_name; (c = getc(fp)) != endquote; ++spell_name)
                *spell_name = c;
            *spell_name = '\0';
            spell_name = long_spell_name;
        } else {
            ungetc(c, fp);
            spell_name = fread_string(fp);
        }
        if ((number = skill_lookup(spell_name)) <= 0) {
            bug("Fread_spnumber: bad spell.");
            exit(1);
        }
        return skill_table[number].slot;
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if ((c != ' ') && (c != '~'))
        ungetc(c, fp);

    return number;
}

long fread_flag(FILE *fp) {
    int number;
    char c;
    char lastc;

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    if (!isdigit(c)) {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
            lastc = c;
            number += flag_convert(c);
            do {
                c = getc(fp);
            } while (c == lastc); /* bug fix for Ozydoc crap by TheMoog */
        }
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (c == '|') {
        number += fread_flag(fp);
    } else {
        if (c != ' ')
            ungetc(c, fp);
    }

    return number;
}

long flag_convert(char letter) {
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z') {
        bitsum = 1;
        for (i = letter; i > 'A'; i--)
            bitsum *= 2;
    } else if ('a' <= letter && letter <= 'z') {
        bitsum = 67108864; /* 2^26 */
        for (i = letter; i > 'a'; i--)
            bitsum *= 2;
    }

    return bitsum;
}

/* Reads a ~-terminated string from a file into a new buffer. */
BUFFER *fread_string_tobuffer(FILE *fp) {
    char buf[MAX_STRING_LENGTH];
    char c;
    int index = 0;
    BUFFER *buffer = buffer_create();

    if (buffer == nullptr) {
        bug("fread_string_tobuffer: Failed to create new buffer.");
        return nullptr;
    }
    do {
        c = getc(fp);
    } while (isspace(c));
    ungetc(c, fp);
    while (index < MAX_STRING_LENGTH - 2) {
        switch (buf[index] = getc(fp)) {
        default: index++; break;

        case EOF:
            bug("fread_string_tobuffer: EOF found.");
            buffer_shrink(buffer);
            return buffer;

        case '\r': break;

        case '\n':
            buf[index + 1] = '\r';
            buf[index + 2] = '\0';
            buffer_addline(buffer, buf);
            index = 0;
            break;

        case '~':
            buf[index] = '\0';
            buffer_addline(buffer, buf);
            buffer_shrink(buffer);
            return buffer;
        }
    }
    bug("fread_string_tobuffer: String overflow - aborting read.");
    buffer_shrink(buffer);
    return buffer;
}

/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *fread_string(FILE *fp) {
    char *plast;
    char c;

    plast = top_string + sizeof(char *);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
        bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }
    do {
        c = getc(fp);
    } while (isspace(c));

    if ((*plast++ = c) == '~')
        return &str_empty[0];

    for (;;) {
        switch (*plast = getc(fp)) {
        default: plast++; break;

        case EOF:
            /* temp fix */
            bug("Fread_string: EOF");
            return nullptr;
            /* exit( 1 ); */
            break;

        case '\n':
            plast++;
            *plast++ = '\r';
            break;

        case '\r': break;

        case '~':
            plast++;
            {
                union {
                    char *pc;
                    char rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;

                plast[-1] = '\0';
                iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
                for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev) {
                    for (ic = 0; ic < (int)sizeof(char *); ic++)
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash += sizeof(char *);

                    if (top_string[sizeof(char *)] == pHash[0] && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
                        return pHash;
                }

                if (fBootDb) {
                    pString = top_string;
                    top_string = plast;
                    u1.pc = string_hash[iHash];
                    for (ic = 0; ic < (int)sizeof(char *); ic++)
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash] = pString;

                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                } else {
                    return str_dup(top_string + sizeof(char *));
                }
            }
        }
    }
}

char *fread_string_eol(FILE *fp) {
    static bool char_special[256 - EOF];
    char *plast;
    char c;

    if (char_special[EOF - EOF] != true) {
        char_special[EOF - EOF] = true;
        char_special['\n' - EOF] = true;
        char_special['\r' - EOF] = true;
    }

    plast = top_string + sizeof(char *);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
        bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do {
        c = getc(fp);
    } while (isspace(c));

    if ((*plast++ = c) == '\n')
        return &str_empty[0];

    for (;;) {
        if (!char_special[(*plast++ = getc(fp)) - EOF])
            continue;

        switch (plast[-1]) {
        default: break;

        case EOF:
            bug("Fread_string_eol  EOF");
            exit(1);
            break;

        case '\n':
        case '\r': {
            union {
                char *pc;
                char rgc[sizeof(char *)];
            } u1;
            int ic;
            int iHash;
            char *pHash;
            char *pHashPrev;
            char *pString;

            plast[-1] = '\0';
            iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
            for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev) {
                for (ic = 0; ic < (int)sizeof(char *); ic++)
                    u1.rgc[ic] = pHash[ic];
                pHashPrev = u1.pc;
                pHash += sizeof(char *);

                if (top_string[sizeof(char *)] == pHash[0] && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
                    return pHash;
            }

            if (fBootDb) {
                pString = top_string;
                top_string = plast;
                u1.pc = string_hash[iHash];
                for (ic = 0; ic < (int)sizeof(char *); ic++)
                    pString[ic] = u1.rgc[ic];
                string_hash[iHash] = pString;

                nAllocString += 1;
                sAllocString += top_string - pString;
                return pString + sizeof(char *);
            } else {
                return str_dup(top_string + sizeof(char *));
            }
        }
        }
    }
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE *fp) {
    int c;

    do {
        c = getc(fp);
    } while (c != '\n' && c != '\r');

    do {
        c = getc(fp);
    } while (c == '\n' || c == '\r');

    ungetc(c, fp);
}

/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp) {
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do {
        cEnd = getc(fp);
    } while (isspace(cEnd));

    if (cEnd == '\'' || cEnd == '"') {
        pword = word;
    } else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH; pword++) {
        *pword = getc(fp);
        if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd) {
            if (cEnd == ' ')
                ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    bug("Fread_word: word too long.");
    exit(1);
    return nullptr;
}

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *alloc_mem(int sMem) {
    void *pMem;
    int iList;

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Alloc_mem: size %d too large.", sMem);
        exit(1);
    }

    if (rgFreeList[iList] == nullptr) {
        pMem = alloc_perm(rgSizeList[iList]);
    } else {
        pMem = rgFreeList[iList];
        rgFreeList[iList] = *((void **)rgFreeList[iList]);
    }

    return pMem;
}

/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem(void *pMem, int sMem) {
    int iList;

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Free_mem: size %d too large.", sMem);
        exit(1);
    }

    *((void **)pMem) = rgFreeList[iList];
    rgFreeList[iList] = pMem;
}

/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm(int sMem) {
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;

    while (sMem % sizeof(long) != 0)
        sMem++;
    if (sMem > MAX_PERM_BLOCK) {
        bug("Alloc_perm: %d too large.", sMem);
        exit(1);
    }

    if (pMemPerm == nullptr || iMemPerm + sMem > MAX_PERM_BLOCK) {
        iMemPerm = 0;
        if ((pMemPerm = static_cast<char *>(calloc(1, MAX_PERM_BLOCK))) == nullptr) {
            perror("Alloc_perm");
            exit(1);
        }
    }

    pMem = pMemPerm + iMemPerm;
    iMemPerm += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup(const char *str) {
    char *str_new;

    if (str[0] == '\0')
        return &str_empty[0];

    if (str >= string_space && str < top_string)
        return (char *)str;

    str_new = static_cast<char *>(alloc_mem(strlen(str) + 1));
    strcpy(str_new, str);
    return str_new;
}

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char *pstr) {
    if (pstr == nullptr || pstr == &str_empty[0] || (pstr >= string_space && pstr < top_string))
        return;

    free_mem(pstr, strlen(pstr) + 1);
}

// Now takes parameters (TM was 'ere 10/00)
void do_areas(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH], cmdBuf[MAX_STRING_LENGTH];
    const char *pArea1rating, *pArea2rating;
    AREA_DATA *pArea1;
    AREA_DATA *pArea2;
    AREA_DATA *pPtr;
    int minLevel = 0;
    int maxLevel = 100;
    int nFound = 0;
    int charLevel = ch->level;

    if (argument[0] != '\0') {
        argument = one_argument(argument, cmdBuf);
        if (isdigit(cmdBuf[0])) {
            minLevel = atoi(cmdBuf);
        } else {
            send_to_char("You must specify a number for the minimum level.\n\r", ch);
            return;
        }
        if (argument[0] != '\0') {
            if (isdigit(argument[0])) {
                maxLevel = atoi(argument);
            } else {
                send_to_char("You must specify a number for the maximum level.\n\r", ch);
                return;
            }
        }
    }

    if (minLevel > maxLevel) {
        send_to_char("The minimum level must be below the maximum level.\n\r", ch);
        return;
    }

    pArea1 = pArea2 = nullptr;
    for (pPtr = area_first; pPtr; pPtr = pPtr->next) {
        int min = 0, max = 100; // Defaults to 'all'
        const char *cCode = "|w";
        int scanRet = sscanf(pPtr->name, "{%d %d}", &min, &max);
        // Is it outside the requested range?
        if (min > maxLevel || max < minLevel)
            continue;

        // Work out colour code
        if (scanRet != 2) {
            cCode = "|C";
        } else if (min > (charLevel + 3) || max < (charLevel - 3)) {
            cCode = "|w";
        } else {
            cCode = "|W";
        }

        if (pArea1 == nullptr) {
            pArea1 = pPtr;
            pArea1rating = cCode;
            nFound++;
        } else {
            pArea2 = pPtr;
            pArea2rating = cCode;
            nFound++;
            // And shift out
            snprintf(buf, sizeof(buf), "%s%-39s%s%-39s|w\n\r", pArea1rating, pArea1->name, pArea2rating, pArea2->name);
            send_to_char(buf, ch);
            pArea1 = pArea2 = nullptr;
        }
    }
    // Check for any straggling lines
    if (pArea1) {
        snprintf(buf, sizeof(buf), "%s%-39s|w\n\r", pArea1rating, pArea1->name);
        send_to_char(buf, ch);
    }
    if (nFound) {
        snprintf(buf, sizeof(buf), "\n\rAreas found: %d\n\r", nFound);
        send_to_char(buf, ch);
    } else {
        send_to_char("No areas found.\n\r", ch);
    }
}

void do_memory(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "Affects %5d\n\r", top_affect);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Areas   %5d\n\r", top_area);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "ExDes   %5d\n\r", top_ed);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Exits   %5d\n\r", top_exit);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Helps   %5d\n\r", top_help);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Socials %5d\n\r", social_count);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Mobs    %5d(%d new format)\n\r", top_mob_index, newmobs);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "(in use)%5d\n\r", mobile_count);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Objs    %5d(%d new format)\n\r", top_obj_index, newobjs);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Resets  %5d\n\r", top_reset);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Rooms   %5d\n\r", top_room);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Shops   %5d\n\r", top_shop);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Strings %5d strings of %7d bytes (max %d).\n\r", nAllocString, sAllocString,
             MAX_STRING);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Perms   %5d blocks  of %7d bytes.\n\r", nAllocPerm, sAllocPerm);
    send_to_char(buf, ch);
}

void do_dump(CHAR_DATA *ch, const char *argument) {
    (void)ch;
    (void)argument;
    int count, count2, num_pcs, aff_count;
    CHAR_DATA *fch;
    MOB_INDEX_DATA *pMobIndex;
    PC_DATA *pc;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit;
    Descriptor *d;
    AFFECT_DATA *af;
    FILE *fp;
    int vnum, nMatch = 0;

    /* open file */
    fclose(fpReserve);
    fp = fopen("mem.dmp", "w");

    /* report use of data structures */

    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp, "MobProt	%4d (%8ld bytes)\n", top_mob_index, top_mob_index * (sizeof(*pMobIndex)));

    /* mobs */
    count = 0;
    count2 = 0;
    for (fch = char_list; fch != nullptr; fch = fch->next) {
        count++;
        if (fch->pcdata != nullptr)
            num_pcs++;
        for (af = fch->affected; af != nullptr; af = af->next)
            aff_count++;
    }
    for (fch = char_free; fch != nullptr; fch = fch->next)
        count2++;

    fprintf(fp, "Mobs	%4d (%8ld bytes), %2d free (%ld bytes)\n", count, count * (sizeof(*fch)), count2,
            count2 * (sizeof(*fch)));

    /* pcdata */
    count = 0;
    for (pc = pcdata_free; pc != nullptr; pc = pc->next)
        count++;

    fprintf(fp, "Pcdata	%4d (%8ld bytes), %2d free (%ld bytes)\n", num_pcs, num_pcs * (sizeof(*pc)), count,
            count * (sizeof(*pc)));

    /* descriptors */
    count = static_cast<int>(ranges::distance(descriptors().all()));

    fprintf(fp, "Descs	%4d (%8ld bytes)\n", count, count * (sizeof(*d)));

    /* object prototypes */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != nullptr) {
            for (af = pObjIndex->affected; af != nullptr; af = af->next)
                aff_count++;
            nMatch++;
        }

    fprintf(fp, "ObjProt	%4d (%8ld bytes)\n", top_obj_index, top_obj_index * (sizeof(*pObjIndex)));

    /* objects */
    count = 0;
    count2 = 0;
    for (obj = object_list; obj != nullptr; obj = obj->next) {
        count++;
        for (af = obj->affected; af != nullptr; af = af->next)
            aff_count++;
    }
    for (obj = obj_free; obj != nullptr; obj = obj->next)
        count2++;

    fprintf(fp, "Objs	%4d (%8ld bytes), %2d free (%ld bytes)\n", count, count * (sizeof(*obj)), count2,
            count2 * (sizeof(*obj)));

    /* affects */
    count = 0;
    for (af = affect_free; af != nullptr; af = af->next)
        count++;

    fprintf(fp, "Affects	%4d (%8ld bytes), %2d free (%ld bytes)\n", aff_count, aff_count * (sizeof(*af)), count,
            count * (sizeof(*af)));

    /* rooms */
    fprintf(fp, "Rooms	%4d (%8ld bytes)\n", top_room, top_room * (sizeof(*room)));

    /* exits */
    fprintf(fp, "Exits	%4d (%8ld bytes)\n", top_exit, top_exit * (sizeof(*exit)));

    fclose(fp);

    /* start printing out mobile data */
    fp = fopen("mob.dmp", "w");

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
        if ((pMobIndex = get_mob_index(vnum)) != nullptr) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d killed     %s\n", pMobIndex->vnum, pMobIndex->count, pMobIndex->killed,
                    pMobIndex->short_descr);
        }
    fclose(fp);

    /* start printing out object data */
    fp = fopen("obj.dmp", "w");

    fprintf(fp, "\nObject Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != nullptr) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d reset      %s\n", pObjIndex->vnum, pObjIndex->count, pObjIndex->reset_num,
                    pObjIndex->short_descr);
        }

    /* close file */
    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");
}

/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy(int number) {
    switch (number_bits(2)) {
    case 0: number -= 1; break;
    case 3: number += 1; break;
    }

    return UMAX(1, number);
}

/*
 * Generate a random number.
 */
int number_range(int from, int to) {
    int power;
    int number;

    if (from == 0 && to == 0)
        return 0;

    if ((to = to - from + 1) <= 1)
        return from;

    for (power = 2; power < to; power <<= 1)
        ;

    while ((number = number_mm() & (power - 1)) >= to)
        ;

    return from + number;
}

/*
 * Generate a percentile roll.
 */
int number_percent() {
    int percent;

    while ((percent = number_mm() & (128 - 1)) > 99)
        ;

    return 1 + percent;
}

/*
 * Generate a random door.
 */
int number_door() {
    int door;

    while ((door = number_mm() & (8 - 1)) > 5)
        ;

    return door;
}

int number_bits(int width) { return number_mm() & ((1 << width) - 1); }

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int rgiState[2 + 55];

void init_mm() {
    int *piState;
    int iState;

    piState = &rgiState[2];

    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;

    piState[0] = ((int)current_time) & ((1 << 30) - 1);
    piState[1] = 1;
    for (iState = 2; iState < 55; iState++) {
        piState[iState] = (piState[iState - 1] + piState[iState - 2]) & ((1 << 30) - 1);
    }
}

int number_mm() {
    int *piState;
    int iState1;
    int iState2;
    int iRand;

    piState = &rgiState[2];
    iState1 = piState[-2];
    iState2 = piState[-1];
    iRand = (piState[iState1] + piState[iState2]) & ((1 << 30) - 1);
    piState[iState1] = iRand;
    if (++iState1 == 55)
        iState1 = 0;
    if (++iState2 == 55)
        iState2 = 0;
    piState[-2] = iState1;
    piState[-1] = iState2;
    return iRand >> 6;
}

/*
 * Roll some dice.
 */
int dice(int number, int size) {
    int idice;
    int sum;

    switch (size) {
    case 0: return 0;
    case 1: return number;
    }

    for (idice = 0, sum = 0; idice < number; idice++)
        sum += number_range(1, size);

    return sum;
}

/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32) { return value_00 + level * (value_32 - value_00) / 32; }

/*
 * Compare strings, case insensitive.
 * Return true if different
 *   (compatibility with historical functions).
 */
bool str_cmp(const char *astr, const char *bstr) {
    if (astr == nullptr) {
        bug("Str_cmp: null astr.");
        return true;
    }

    if (bstr == nullptr) {
        bug("Str_cmp: null bstr.");
        return true;
    }

    for (; *astr || *bstr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr))
            return true;
    }
    return false;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char *astr, const char *bstr) {
    if (astr == nullptr) {
        bug("Strn_cmp: null astr.");
        return true;
    }

    if (bstr == nullptr) {
        bug("Strn_cmp: null bstr.");
        return true;
    }

    for (; *astr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr))
            return true;
    }

    return false;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix(const char *astr, const char *bstr) {
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ((c0 = LOWER(astr[0])) == '\0')
        return false;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
        if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar))
            return false;
    }
    return true;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix(const char *astr, const char *bstr) {
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
        return false;
    else
        return true;
}

/*
 * Returns an initial-capped string.
 */
char *capitalize(const char *str) {
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for (i = 0; str[i] != '\0'; i++)
        strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}

/*
 * Append a string to a file.
 */
void append_file(CHAR_DATA *ch, const char *file, const char *str) {
    FILE *fp;

    if (IS_NPC(ch) || str[0] == '\0')
        return;

    if ((fp = fopen(file, "a")) == nullptr) {
        perror(file);
        send_to_char("Could not open the file!\n\r", ch);
    } else {
        fprintf(fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str);
        fclose(fp);
    }
}

/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */

/*
 * MOBprogram code block
 */
/* the functions */

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */
int mprog_name_to_type(char *name) {
    if (!str_cmp(name, "in_file_prog"))
        return IN_FILE_PROG;
    if (!str_cmp(name, "act_prog"))
        return ACT_PROG;
    if (!str_cmp(name, "speech_prog"))
        return SPEECH_PROG;
    if (!str_cmp(name, "rand_prog"))
        return RAND_PROG;
    if (!str_cmp(name, "fight_prog"))
        return FIGHT_PROG;
    if (!str_cmp(name, "hitprcnt_prog"))
        return HITPRCNT_PROG;
    if (!str_cmp(name, "death_prog"))
        return DEATH_PROG;
    if (!str_cmp(name, "entry_prog"))
        return ENTRY_PROG;
    if (!str_cmp(name, "greet_prog"))
        return GREET_PROG;
    if (!str_cmp(name, "all_greet_prog"))
        return ALL_GREET_PROG;
    if (!str_cmp(name, "give_prog"))
        return GIVE_PROG;
    if (!str_cmp(name, "bribe_prog"))
        return BRIBE_PROG;
    return (ERROR_PROG);
}

/* This routine reads in scripts of MOBprograms from a file */
MPROG_DATA *mprog_file_read(char *f, MPROG_DATA *mprg, MOB_INDEX_DATA *pMobIndex) {
    MPROG_DATA *mprg2;
    FILE *progfile;
    char letter;
    bool done = false;
    char MOBProgfile[MAX_INPUT_LENGTH];

    snprintf(MOBProgfile, sizeof(MOBProgfile), "%s%s", MOB_DIR, f);
    progfile = fopen(MOBProgfile, "r");
    if (!progfile) {
        bug("Mob:%d couldnt open mobprog file %s", pMobIndex->vnum, MOBProgfile);
        exit(1);
    }
    mprg2 = mprg;
    switch (letter = fread_letter(progfile)) {
    case '>': break;
    case '|':
        bug("empty mobprog file.");
        exit(1);
        break;
    default:
        bug("in mobprog file syntax error.");
        exit(1);
        break;
    }
    while (!done) {
        mprg2->type = mprog_name_to_type(fread_word(progfile));
        switch (mprg2->type) {
        case ERROR_PROG:
            bug("mobprog file type error");
            exit(1);
            break;
        case IN_FILE_PROG:
            bug("mprog file contains a call to file.");
            exit(1);
            break;
        default:
            pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
            mprg2->arglist = fread_string(progfile);
            mprg2->comlist = fread_string(progfile);
            switch (letter = fread_letter(progfile)) {
            case '>':
                mprg2->next = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
                mprg2 = mprg2->next;
                mprg2->next = nullptr;
                break;
            case '|': done = true; break;
            default:
                bug("in mobprog file syntax error.");
                exit(1);
                break;
            }
            break;
        }
    }
    fclose(progfile);
    return mprg2;
}

/* Snarf a MOBprogram section from the area file.
 */
void load_mobprogs(FILE *fp) {
    char letter;
    MOB_INDEX_DATA *iMob;
    int value;
    MPROG_DATA *original;
    MPROG_DATA *working;

    if (area_last == nullptr) {
        bug("Load_mobprogs: no #AREA seen yet!");
        exit(1);
    }

    for (;;) {
        switch (letter = fread_letter(fp)) {
        default:
            bug("Load_mobprogs: bad command '%c'.", letter);
            exit(1);
            break;
        case 'S':
        case 's': fread_to_eol(fp); return;
        case '*': fread_to_eol(fp); break;
        case 'M':
        case 'm':
            value = fread_number(fp);
            if ((iMob = get_mob_index(value)) == nullptr) {
                bug("Load_mobprogs: vnum %d doesnt exist", value);
                exit(1);
            }

            original = iMob->mobprogs;
            if (original != nullptr)
                for (; original->next != nullptr; original = original->next)
                    ;
            working = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
            if (original)
                original->next = working;
            else
                iMob->mobprogs = working;
            working = mprog_file_read(fread_word(fp), working, iMob);
            working->next = nullptr;
            fread_to_eol(fp);
            break;
        }
    }
}

/* This procedure is responsible for reading any in_file MOBprograms.
 */
void mprog_read_programs(FILE *fp, MOB_INDEX_DATA *pMobIndex) {
    MPROG_DATA *mprg;
    bool done = false;
    char letter;
    if ((letter = fread_letter(fp)) != '>') {
        bug("Load_mobiles: vnum %d MOBPROG char", pMobIndex->vnum);
        exit(1);
    }
    pMobIndex->mobprogs = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
    mprg = pMobIndex->mobprogs;
    while (!done) {
        mprg->type = mprog_name_to_type(fread_word(fp));
        switch (mprg->type) {
        case ERROR_PROG:
            bug("Load_mobiles: vnum %d MOBPROG type.", pMobIndex->vnum);
            exit(1);
            break;
        case IN_FILE_PROG:
            mprg = mprog_file_read(fread_string(fp), mprg, pMobIndex);
            fread_to_eol(fp);
            switch (letter = fread_letter(fp)) {
            case '>':
                mprg->next = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
                mprg = mprg->next;
                mprg->next = nullptr;
                break;
            case '|':
                mprg->next = nullptr;
                fread_to_eol(fp);
                done = true;
                break;
            default:
                bug("Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->vnum);
                exit(1);
                break;
            }
            break;
        default:
            pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
            mprg->arglist = fread_string(fp);
            fread_to_eol(fp);
            mprg->comlist = fread_string(fp);
            fread_to_eol(fp);
            switch (letter = fread_letter(fp)) {
            case '>':
                mprg->next = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
                mprg = mprg->next;
                mprg->next = nullptr;
                break;
            case '|':
                mprg->next = nullptr;
                fread_to_eol(fp);
                done = true;
                break;
            default:
                bug("Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->vnum);
                exit(1);
                break;
            }
            break;
        }
    }
}
