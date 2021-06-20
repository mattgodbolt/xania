/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  merc.h:  the core header file, most stuff needs to refer to this!    */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "Char.hpp"
#include "Constants.hpp"
#include "Direction.hpp"
#include "SectorType.hpp"
#include "Stats.hpp"
#include "Types.hpp"
#include "clan.h"
#include "common/Time.hpp"
#include "version.h"

#include <crypt.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

/* Buffer structure */
typedef struct _BUFFER BUFFER;

/*
 * Structure types.
 */
struct AREA_DATA;
typedef struct ban_data BAN_DATA;
class Descriptor;
typedef struct exit_data EXIT_DATA;
struct EXTRA_DESCR_DATA;
typedef struct kill_data KILL_DATA;
struct OBJ_DATA;
struct OBJ_INDEX_DATA;
typedef struct program PROGRAM;
typedef struct reset_data RESET_DATA;
struct ROOM_INDEX_DATA;
struct SHOP_DATA;

/*
 * Site ban structure.
 */
struct ban_data {
    BAN_DATA *next;
    char *name;
    int ban_flags;
    int level;
};

/* Ban defines */
#define BAN_PERMANENT 1
#define BAN_NEWBIES 2
#define BAN_PERMIT 4
#define BAN_PREFIX 8
#define BAN_SUFFIX 16
#define BAN_ALL 32

/**
 * Indexes into attack_table that are used by specific weapon enchantment spells
 * like spell_acid_wash and spell_tame_lightning. When a weapon is enchanted,
 * an index into that table is written to the object's value[3] attribute.
 * These constants are not plain damage types (DAM_*), the attack_table
 * is actually superset of the damage type enum.
 * See:
 * - enchanting weapon in magic.c
 * - generating the damage message in fight.c
 */
#define ATTACK_TABLE_INDEX_TAME_LIGHTNING 28
#define ATTACK_TABLE_INDEX_ACID_WASH 31

/*
 * Attribute bonus structures.
 */
struct str_app_type {
    sh_int tohit;
    sh_int todam;
    sh_int carry;
    sh_int wield;
};

struct materials_type {
    sh_int magical_resilience;
    const char *material_name;
};

struct int_app_type {
    sh_int learn;
};

struct wis_app_type {
    sh_int practice;
};

struct dex_app_type {
    sh_int defensive;
};

struct con_app_type {
    sh_int hitp;
    sh_int shock;
};

/*
 * Shop types.
 */
#define MAX_TRADE 5

struct SHOP_DATA {
    SHOP_DATA *next; /* Next shop in list            */
    sh_int keeper; /* Vnum of shop keeper mob      */
    sh_int buy_type[MAX_TRADE]; /* Item types shop will buy     */
    sh_int profit_buy; /* Cost multiplier for buying   */
    sh_int profit_sell; /* Cost multiplier for selling  */
    unsigned int open_hour; /* First opening hour           */
    unsigned int close_hour; /* First closing hour           */
};

/*
 * Per-class stuff.
 */
static constexpr inline auto MAX_GUILD = 3;
struct class_type {
    const char *name; /* the full name of the class */
    char who_name[4]; /* Three-letter name for 'who'  */
    Stat attr_prime; /* Prime attribute              */
    sh_int weapon; /* First weapon                 */
    sh_int guild[MAX_GUILD]; /* Vnum of guild rooms          */
    sh_int skill_adept; /* Maximum skill level          */
    sh_int thac0_00; /* Thac0 for level  0           */
    sh_int thac0_32; /* Thac0 for level 32           */
    sh_int hp_min; /* Min hp gained on leveling    */
    sh_int hp_max; /* Max hp gained on leveling    */
    sh_int fMana; /* Class gains mana on level    */
    const char *base_group; /* base skills gained           */
    const char *default_group; /* default skills gained        */
};

struct attack_type {
    const char *name; /* message in the area file */
    const char *verb; /* message in the mud */
    int damage; /* damage class */
};

#define MAX_DAM 18 /* this should really be down below with dam types*/

struct race_type {
    const char *name; /* call name of the race */
    bool pc_race; /* can be chosen by pcs */
    unsigned long act; /* act bits for the race */
    unsigned long aff; /* aff bits for the race */
    unsigned long off; /* off bits for the race */
    unsigned long imm; /* imm bits for the race */
    unsigned long res; /* res bits for the race */
    unsigned long vuln; /* vuln bits for the race */
    unsigned long form; /* default form flag for the race */
    unsigned long parts; /* default parts for the race */
};

struct pc_race_type /* additional data for pc races */
{
    const char *name; /* MUST be in race_type */
    char who_name[6];
    sh_int points; /* cost in points of the race */
    sh_int class_mult[MAX_CLASS]; /* exp multiplier for class, * 100 */
    std::array<const char *, 5> skills{}; /* bonus skills for the race */
    Stats stats; /* starting stats */
    Stats max_stats; /* maximum stats */
    sh_int size; /* aff bits for the race */
};

/*
 * A kill structure (indexed by level).
 */
struct kill_data {
    sh_int number;
    sh_int killed;
};

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/* RT ASCII conversions -- used so we can have letters in this file */

static constexpr unsigned int BIT(unsigned int bit) { return 1u << bit; }
static inline constexpr auto A = BIT(0);
static inline constexpr auto B = BIT(1);
static inline constexpr auto C = BIT(2);
static inline constexpr auto D = BIT(3);
static inline constexpr auto E = BIT(4);
static inline constexpr auto F = BIT(5);
static inline constexpr auto G = BIT(6);
static inline constexpr auto H = BIT(7);

static inline constexpr auto I = BIT(8);
static inline constexpr auto J = BIT(9);
static inline constexpr auto K = BIT(10);
static inline constexpr auto L = BIT(11);
static inline constexpr auto M = BIT(12);
static inline constexpr auto N = BIT(13);
static inline constexpr auto O = BIT(14);
static inline constexpr auto P = BIT(15);

static inline constexpr auto Q = BIT(16);
static inline constexpr auto R = BIT(17);
static inline constexpr auto S = BIT(18);
static inline constexpr auto T = BIT(19);
static inline constexpr auto U = BIT(20);
static inline constexpr auto V = BIT(21);
static inline constexpr auto W = BIT(22);
static inline constexpr auto X = BIT(23);

static inline constexpr auto Y = BIT(24);
static inline constexpr auto Z = BIT(25);
static inline constexpr auto aa = BIT(26);
static inline constexpr auto bb = BIT(27);
static inline constexpr auto cc = BIT(28);
static inline constexpr auto dd = BIT(29);
static inline constexpr auto ee = BIT(30);
static inline constexpr auto ff = BIT(31);

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 * TODO make an actual class to hold bits like this.
 */
#define ACT_IS_NPC (A) /* Auto set for mobs    */
#define ACT_SENTINEL (B) /* Stays in one room    */
#define ACT_SCAVENGER (C) /* Picks up objects     */
#define ACT_AGGRESSIVE (F) /* Attacks PC's         */
#define ACT_STAY_AREA (G) /* Won't leave area     */
#define ACT_WIMPY (H)
#define ACT_PET (I) /* Auto set for pets    */
#define ACT_TRAIN (J) /* Can train PC's       */
#define ACT_PRACTICE (K) /* Can practice PC's    */
#define ACT_SENTIENT (L) /* Is intelligent       */
#define ACT_TALKATIVE (M) /* Will respond to says and emotes */
#define ACT_UNDEAD (O)
#define ACT_CLERIC (Q)
#define ACT_MAGE (R)
#define ACT_THIEF (S)
#define ACT_WARRIOR (T)
#define ACT_NOALIGN (U)
#define ACT_NOPURGE (V)
#define ACT_IS_HEALER (aa)
#define ACT_GAIN (bb)
#define ACT_UPDATE_ALWAYS (cc)
#define ACT_CAN_BE_RIDDEN (dd) /* Can be ridden */

/* damage classes */
#define DAM_NONE 0
#define DAM_BASH 1
#define DAM_PIERCE 2
#define DAM_SLASH 3
#define DAM_FIRE 4
#define DAM_COLD 5
#define DAM_LIGHTNING 6
#define DAM_ACID 7
#define DAM_POISON 8
#define DAM_NEGATIVE 9
#define DAM_HOLY 10
#define DAM_ENERGY 11
#define DAM_MENTAL 12
#define DAM_DISEASE 13
#define DAM_DROWNING 14
#define DAM_LIGHT 15
#define DAM_OTHER 16
#define DAM_HARM 17
#ifndef MAX_DAM
#define MAX_DAM 18 /* should == number of dam types */
#endif
/* DON'T FORGET! to add to the the dam_string_table if you add
   a new type of damage here */

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK (A)
#define OFF_BACKSTAB (B)
#define OFF_BASH (C)
#define OFF_BERSERK (D)
#define OFF_DISARM (E)
#define OFF_DODGE (F)
#define OFF_FADE (G)
#define OFF_FAST (H)
#define OFF_SLOW (W)
#define OFF_KICK (I)
#define OFF_KICK_DIRT (J)
#define OFF_PARRY (K)
#define OFF_RESCUE (L)
#define OFF_TAIL (M)
#define OFF_TRIP (N)
#define OFF_CRUSH (O)
#define ASSIST_ALL (P)
#define ASSIST_ALIGN (Q)
#define ASSIST_RACE (R)
#define ASSIST_PLAYERS (S)
#define ASSIST_GUARD (T)
#define ASSIST_VNUM (U)
#define OFF_HEADBUTT (V)

/* return values for check_imm */
#define IS_NORMAL 0
#define IS_IMMUNE 1
#define IS_RESISTANT 2
#define IS_VULNERABLE 3

/* IMM bits for mobs */
#define IMM_SUMMON (A)
#define IMM_CHARM (B)
#define IMM_MAGIC (C)
#define IMM_WEAPON (D)
#define IMM_BASH (E)
#define IMM_PIERCE (F)
#define IMM_SLASH (G)
#define IMM_FIRE (H)
#define IMM_COLD (I)
#define IMM_LIGHTNING (J)
#define IMM_ACID (K)
#define IMM_POISON (L)
#define IMM_NEGATIVE (M)
#define IMM_HOLY (N)
#define IMM_ENERGY (O)
#define IMM_MENTAL (P)
#define IMM_DISEASE (Q)
#define IMM_DROWNING (R)
#define IMM_LIGHT (S)

/* RES bits for mobs */
#define RES_CHARM (B)
#define RES_MAGIC (C)
#define RES_WEAPON (D)
#define RES_BASH (E)
#define RES_PIERCE (F)
#define RES_SLASH (G)
#define RES_FIRE (H)
#define RES_COLD (I)
#define RES_LIGHTNING (J)
#define RES_ACID (K)
#define RES_POISON (L)
#define RES_NEGATIVE (M)
#define RES_HOLY (N)
#define RES_ENERGY (O)
#define RES_MENTAL (P)
#define RES_DISEASE (Q)
#define RES_DROWNING (R)
#define RES_LIGHT (S)

/* VULN bits for mobs */
#define VULN_MAGIC (C)
#define VULN_WEAPON (D)
#define VULN_BASH (E)
#define VULN_PIERCE (F)
#define VULN_SLASH (G)
#define VULN_FIRE (H)
#define VULN_COLD (I)
#define VULN_LIGHTNING (J)
#define VULN_ACID (K)
#define VULN_POISON (L)
#define VULN_NEGATIVE (M)
#define VULN_HOLY (N)
#define VULN_ENERGY (O)
#define VULN_MENTAL (P)
#define VULN_DISEASE (Q)
#define VULN_DROWNING (R)
#define VULN_LIGHT (S)
#define VULN_WOOD (X)
#define VULN_SILVER (Y)
#define VULN_IRON (Z)

/* body form */
#define FORM_EDIBLE (A)
#define FORM_POISON (B)
#define FORM_MAGICAL (C)
#define FORM_INSTANT_DECAY (D)
#define FORM_OTHER (E) /* defined by material bit */

/* actual form */
#define FORM_ANIMAL (G)
#define FORM_SENTIENT (H)
#define FORM_UNDEAD (I)
#define FORM_CONSTRUCT (J)
#define FORM_MIST (K)
#define FORM_INTANGIBLE (L)

#define FORM_BIPED (M)
#define FORM_CENTAUR (N)
#define FORM_INSECT (O)
#define FORM_SPIDER (P)
#define FORM_CRUSTACEAN (Q)
#define FORM_WORM (R)
#define FORM_BLOB (S)

#define FORM_MAMMAL (V)
#define FORM_BIRD (W)
#define FORM_REPTILE (X)
#define FORM_SNAKE (Y)
#define FORM_DRAGON (Z)
#define FORM_AMPHIBIAN (aa)
#define FORM_FISH (bb)
#define FORM_COLD_BLOOD (cc)

/* body parts */
#define PART_HEAD (A)
#define PART_ARMS (B)
#define PART_LEGS (C)
#define PART_HEART (D)
#define PART_BRAINS (E)
#define PART_GUTS (F)
#define PART_HANDS (G)
#define PART_FEET (H)
#define PART_FINGERS (I)
#define PART_EAR (J)
#define PART_EYE (K)
#define PART_LONG_TONGUE (L)
#define PART_EYESTALKS (M)
#define PART_TENTACLES (N)
#define PART_FINS (O)
#define PART_WINGS (P)
#define PART_TAIL (Q)
/* for combat */
#define PART_CLAWS (U)
#define PART_FANGS (V)
#define PART_HORNS (W)
#define PART_SCALES (X)
#define PART_TUSKS (Y)

/* only change this if you add a new body part!! */
#define MAX_BODY_PARTS 22

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND (A)
#define AFF_INVISIBLE (B)
#define AFF_DETECT_EVIL (C)
#define AFF_DETECT_INVIS (D)
#define AFF_DETECT_MAGIC (E)
#define AFF_DETECT_HIDDEN (F)
#define AFF_TALON (G)
#define AFF_SANCTUARY (H)

#define AFF_FAERIE_FIRE (I)
#define AFF_INFRARED (J)
#define AFF_CURSE (K)
#define AFF_PROTECTION_EVIL (L)
#define AFF_POISON (M)
#define AFF_PROTECTION_GOOD (N)
// O currently unused

#define AFF_SNEAK (P)
#define AFF_HIDE (Q)
#define AFF_SLEEP (R)
#define AFF_CHARM (S)
#define AFF_FLYING (T)
#define AFF_PASS_DOOR (U)
#define AFF_HASTE (V)
#define AFF_CALM (W)
#define AFF_PLAGUE (X)
#define AFF_WEAKEN (Y)
#define AFF_DARK_VISION (Z)
#define AFF_BERSERK (aa)
#define AFF_SWIM (bb)
#define AFF_REGENERATION (cc)
#define AFF_OCTARINE_FIRE (dd)
#define AFF_LETHARGY (ee)

/* AC types */
#define AC_PIERCE 0
#define AC_BASH 1
#define AC_SLASH 2
#define AC_EXOTIC 3

/* size */
#define SIZE_TINY 0
#define SIZE_SMALL 1
#define SIZE_MEDIUM 2
#define SIZE_LARGE 3
#define SIZE_HUGE 4
#define SIZE_GIANT 5

/*
 * Material types.
 * Used in #OBJECTS
 */
#define MATERIAL_NONE 0
#define MATERIAL_DEFAULT 1
#define MATERIAL_ADAMANTITE 2
#define MATERIAL_IRON 3
#define MATERIAL_GLASS 4
#define MATERIAL_BRONZE 5
#define MATERIAL_CLOTH 6
#define MATERIAL_WOOD 7
#define MATERIAL_PAPER 8
#define MATERIAL_STEEL 9
#define MATERIAL_STONE 10
#define MATERIAL_FOOD 11
#define MATERIAL_SILVER 12
#define MATERIAL_GOLD 13
#define MATERIAL_LEATHER 14
#define MATERIAL_VELLUM 15
#define MATERIAL_CHINA 16
#define MATERIAL_CLAY 17
#define MATERIAL_BRASS 18
#define MATERIAL_BONE 19
#define MATERIAL_PLATINUM 20
#define MATERIAL_PEARL 21
#define MATERIAL_MITHRIL 22
#define MATERIAL_OCTARINE 23
/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT 1
#define ITEM_SCROLL 2
#define ITEM_WAND 3
#define ITEM_STAFF 4
#define ITEM_WEAPON 5
#define ITEM_TREASURE 8
#define ITEM_ARMOR 9
#define ITEM_POTION 10
#define ITEM_CLOTHING 11
#define ITEM_FURNITURE 12
#define ITEM_TRASH 13
#define ITEM_CONTAINER 15
#define ITEM_DRINK_CON 17
#define ITEM_KEY 18
#define ITEM_FOOD 19
#define ITEM_MONEY 20
#define ITEM_BOAT 22
#define ITEM_CORPSE_NPC 23
#define ITEM_CORPSE_PC 24
#define ITEM_FOUNTAIN 25
#define ITEM_PILL 26
#define ITEM_MAP 28
#define ITEM_BOMB 29
#define ITEM_PORTAL 30

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW (A)
#define ITEM_HUM (B)
#define ITEM_DARK (C)
#define ITEM_LOCK (D)
#define ITEM_EVIL (E)
#define ITEM_INVIS (F)
#define ITEM_MAGIC (G)
#define ITEM_NODROP (H)
#define ITEM_BLESS (I)
#define ITEM_ANTI_GOOD (J)
#define ITEM_ANTI_EVIL (K)
#define ITEM_ANTI_NEUTRAL (L)
#define ITEM_NOREMOVE (M) // Only weapons are meant to have this, it prevents disarm.
#define ITEM_INVENTORY (N)
#define ITEM_NOPURGE (O)
#define ITEM_ROT_DEATH (P)
#define ITEM_VIS_DEATH (Q)
#define ITEM_PROTECT_CONTAINER (R)
#define ITEM_NO_LOCATE (S)
#define ITEM_SUMMON_CORPSE (T)
#define ITEM_UNIQUE (U)

#define ITEM_EXTRA_FLAGS                                                                                               \
    "glow hum dark lock evil invis magic nodrop bless antigood antievil antineutral "                                  \
    "noremove inventory nopurge rotdeath visdeath protected nolocate summon_corpse unique"

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE (A)
#define ITEM_WEAR_FINGER (B)
#define ITEM_WEAR_NECK (C)
#define ITEM_WEAR_BODY (D)
#define ITEM_WEAR_HEAD (E)
#define ITEM_WEAR_LEGS (F)
#define ITEM_WEAR_FEET (G)
#define ITEM_WEAR_HANDS (H)
#define ITEM_WEAR_ARMS (I)
#define ITEM_WEAR_SHIELD (J)
#define ITEM_WEAR_ABOUT (K)
#define ITEM_WEAR_WAIST (L)
#define ITEM_WEAR_WRIST (M)
#define ITEM_WIELD (N)
#define ITEM_HOLD (O)
#define ITEM_TWO_HANDS (P)
#define ITEM_WEAR_EARS (Q)

#define ITEM_WEAR_FLAGS                                                                                                \
    "take finger neck body head legs feet hands arms shield about waist wrist wield hold twohands ears"

/* weapon class */
#define WEAPON_EXOTIC 0
#define WEAPON_SWORD 1
#define WEAPON_DAGGER 2
#define WEAPON_SPEAR 3
#define WEAPON_MACE 4
#define WEAPON_AXE 5
#define WEAPON_FLAIL 6
#define WEAPON_WHIP 7
#define WEAPON_POLEARM 8

/* weapon types */
#define WEAPON_FLAMING (A)
#define WEAPON_FROST (B)
#define WEAPON_VAMPIRIC (C)
#define WEAPON_SHARP (D)
#define WEAPON_VORPAL (E)
#define WEAPON_TWO_HANDS (F)
#define WEAPON_POISONED (G)
#define WEAPON_PLAGUED (I)
#define WEAPON_LIGHTNING (J)
#define WEAPON_ACID (K)

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE 1
#define CONT_PICKPROOF 2
#define CONT_CLOSED 4
#define CONT_LOCKED 8

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK (A)
#define ROOM_NO_MOB (C)
#define ROOM_INDOORS (D)

#define ROOM_PRIVATE (J)
#define ROOM_SAFE (K)
#define ROOM_SOLITARY (L)
#define ROOM_PET_SHOP (M)
#define ROOM_NO_RECALL (N)
#define ROOM_IMP_ONLY (O)
#define ROOM_GODS_ONLY (P)
#define ROOM_HEROES_ONLY (Q)
#define ROOM_NEWBIES_ONLY (R)
#define ROOM_LAW (S)

/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR (A)
#define EX_CLOSED (B)
#define EX_LOCKED (C)
#define EX_PICKPROOF (F)
#define EX_PASSPROOF (G)

#define EX_EASY (H)
#define EX_HARD (I)
#define EX_INFURIATING (J)
#define EX_NOCLOSE (K)
#define EX_NOLOCK (L)
#define EX_TRAP (M)
#define EX_TRAP_DARTS (N)
#define EX_TRAP_POISON (O)
#define EX_TRAP_EXPLODING (P)
#define EX_TRAP_SLEEPGAS (Q)
#define EX_TRAP_DEATH (R)
#define EX_SECRET (S)

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE -1
#define WEAR_LIGHT 0
#define WEAR_FINGER_L 1
#define WEAR_FINGER_R 2
#define WEAR_NECK_1 3
#define WEAR_NECK_2 4
#define WEAR_BODY 5
#define WEAR_HEAD 6
#define WEAR_LEGS 7
#define WEAR_FEET 8
#define WEAR_HANDS 9
#define WEAR_ARMS 10
#define WEAR_SHIELD 11
#define WEAR_ABOUT 12
#define WEAR_WAIST 13
#define WEAR_WRIST_L 14
#define WEAR_WRIST_R 15
#define WEAR_WIELD 16
#define WEAR_HOLD 17
#define WEAR_EARS 18
#define MAX_WEAR 19

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/
/*
 * ACT bits for players.
 */
#define PLR_IS_NPC (A) /* Don't EVER set.      */
#define PLR_BOUGHT_PET (B)

/* RT auto flags */
#define PLR_AUTOASSIST (C)
#define PLR_AUTOEXIT (D)
#define PLR_AUTOLOOT (E)
#define PLR_AUTOSAC (F)
#define PLR_AUTOGOLD (G)
#define PLR_AUTOSPLIT (H)
#define PLR_AUTOPEEK (bb)
/* 5 bits reserved, I-M */

/* RT personal flags */
#define PLR_HOLYLIGHT (N)
#define PLR_WIZINVIS (O)
#define PLR_CANLOOT (P)
#define PLR_NOSUMMON (Q)
#define PLR_NOFOLLOW (R)
#define PLR_AFK (S)
/* 3 bits reserved, T-V */
/* XT personal flags */
#define PLR_PROWL (cc)

/* penalty flags */

#define PLR_LOG (W)
#define PLR_DENY (X)
#define PLR_FREEZE (Y)
#define PLR_THIEF (Z)
#define PLR_KILLER (aa)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET (A)
#define COMM_DEAF (B)
#define COMM_NOWIZ (C)
#define COMM_NOAUCTION (D)
#define COMM_NOGOSSIP (E)
#define COMM_NOQUESTION (F)
#define COMM_NOMUSIC (G)
#define COMM_NOGRATZ (H)
#define COMM_NOANNOUNCE (I)
#define COMM_NOPHILOSOPHISE (J)
#define COMM_NOQWEST (K)
/* all four spare letters now taken up - think about adding more!*/

/* display flags */
#define COMM_COMPACT (L)
#define COMM_BRIEF (M)
#define COMM_PROMPT (N)
#define COMM_COMBINE (O)
//#define COMM_TELNET_GA (P) -- no longer used, probably should be retired after a pfile sweep
#define COMM_SHOWAFK (Q)
#define COMM_SHOWDEFENCE (R)
#define COMM_AFFECT (Y)
/* 1 flag reserved, S */

/* penalties */
#define COMM_NOEMOTE (T)
#define COMM_NOSHOUT (U)
#define COMM_NOTELL (V)
#define COMM_NOCHANNELS (W)

/* spare flags used for new channels */
#define COMM_NOALLEGE (X)

/* Data for generating characters -- only used during generation */
struct GEN_DATA {
    GEN_DATA *next;
    bool skill_chosen[MAX_SKILL];
    bool group_chosen[MAX_GROUP];
    int points_chosen;
};

/* Rohan's finger_info structure to cache all char's info data */
struct FingerInfo {
    std::string name;
    std::string info_message;
    std::string last_login_at;
    std::string last_login_from;
    sh_int invis_level{};
    bool i_message{};
    explicit FingerInfo(std::string_view name) : name(name) {}
    FingerInfo(std::string_view name, std::string_view info_message, std::string_view last_login_at,
               std::string_view last_login_from, sh_int invis_level, bool i_message)
        : name(name), info_message(info_message), last_login_at(last_login_at), last_login_from(last_login_from),
          invis_level(invis_level), i_message(i_message) {}
};

/*
 * Liquids.
 */
#define LIQ_WATER 0
#define LIQ_MAX 17

struct liq_type {
    const char *liq_name;
    const char *liq_color;
    sh_int liq_affect[3];
};

// Extra description data for a room or object.
struct EXTRA_DESCR_DATA {
    std::string keyword; // Keyword in look/examine
    std::string description; // What to see
};

/*
 * Prototype for an object.
 */
struct OBJ_INDEX_DATA {
    OBJ_INDEX_DATA *next{};
    std::vector<EXTRA_DESCR_DATA> extra_descr;
    AffectList affected{};
    std::string name;
    std::string short_descr;
    std::string description;
    sh_int vnum{};
    sh_int reset_num{};
    sh_int material{};
    sh_int item_type{};
    unsigned int extra_flags{};
    int wear_flags{};
    std::string wear_string;
    sh_int level{};
    sh_int condition{};
    sh_int count{};
    sh_int weight{};
    int cost{};
    std::array<int, 5> value{};

    AREA_DATA *area{};
};

/*
 * One object.
 */
struct OBJ_DATA {
    GenericList<OBJ_DATA *> contains;
    OBJ_DATA *in_obj{};
    Char *carried_by{};
    std::vector<EXTRA_DESCR_DATA> extra_descr;
    AffectList affected{};
    OBJ_INDEX_DATA *pIndexData{};
    ROOM_INDEX_DATA *in_room{};
    bool enchanted{};
    std::string owner;
    std::string name;
    std::string short_descr;
    std::string description;
    sh_int item_type{};
    unsigned int extra_flags{};
    int wear_flags{};
    std::string wear_string;
    int wear_loc{};
    sh_int weight{};
    int cost{};
    sh_int level{};
    sh_int condition{};
    sh_int material{};
    sh_int timer{};
    std::array<int, 5> value{};
    ROOM_INDEX_DATA *destination{};
};

/*
 * Exit data.
 */
struct exit_data {
    union {
        ROOM_INDEX_DATA *to_room;
        sh_int vnum;
    } u1;
    sh_int exit_info;
    sh_int key;
    char *keyword;
    char *description;

    EXIT_DATA *next;
    int rs_flags;
    bool is_one_way;
};

// If an exit's vnum is this value the exit goes nowhere but it can be looked at
static constexpr inline auto EXIT_VNUM_COSMETIC = -1;

/**
 * Commands used in #RESETS section of area files
 */
#define RESETS_MOB_IN_ROOM 'M' /* place mob in a room */
#define RESETS_EQUIP_OBJ_MOB 'E' /* equip and item on a mobile */
#define RESETS_GIVE_OBJ_MOB 'G' /* give an item to a mobile's inventory */
#define RESETS_OBJ_IN_ROOM 'O' /* place a static object in a room */
#define RESETS_PUT_OBJ_OBJ 'P' /* place a static object in another object */
#define RESETS_EXIT_FLAGS 'D' /* set exit closed/locked flags */
#define RESETS_RANDOMIZE_EXITS 'R' /* randomize room exits */
#define RESETS_COMMENT '*' /* comment line */
#define RESETS_END_SECTION 'S' /* end of the resets section */

/*
 * Area-reset definition.
 */
struct reset_data {
    RESET_DATA *next;
    char command;
    sh_int arg1;
    sh_int arg2;
    sh_int arg3;
    sh_int arg4;
};

/*
 * Room type.
 */
struct ROOM_INDEX_DATA {
    ROOM_INDEX_DATA *next{};
    GenericList<Char *> people;
    GenericList<OBJ_DATA *> contents;
    std::vector<EXTRA_DESCR_DATA> extra_descr{};
    AREA_DATA *area{};
    PerDirection<EXIT_DATA *> exit{};
    char *name{};
    char *description{};
    sh_int vnum{};
    unsigned int room_flags{};
    sh_int light{};
    SectorType sector_type{SectorType::Inside};

    RESET_DATA *reset_first{};
    RESET_DATA *reset_last{};
};

/*
 *  Target types.
 */
#define TAR_IGNORE 0
#define TAR_CHAR_OFFENSIVE 1
#define TAR_CHAR_DEFENSIVE 2
#define TAR_CHAR_SELF 3
#define TAR_OBJ_INV 4
#define TAR_CHAR_OBJECT 5
#define TAR_CHAR_OTHER 6

/*
 *  Skill rating magic numbers.
 */
#define SKILL_UNATTAINABLE 0
#define SKILL_ATTAINABLE -1
#define SKILL_ASSASSIN -2 /* Hacky kludge, ohyes. */

/*
 * Skills include spells as a particular case.
 */
struct skill_type {
    const char *name; /* Name of skill                */
    sh_int skill_level[MAX_CLASS]; /* Level needed by class        */
    sh_int rating[MAX_CLASS]; /* How hard it is to learn      */

    SpellFunc spell_fun; /* Spell function pointer  */
    sh_int target; /* Legal targets                */
    Position::Type minimum_position;
    sh_int *pgsn; /* Pointer to associated gsn    */
    sh_int slot; /* Slot for #OBJECT loading     */
    sh_int min_mana; /* Minimum mana used            */
    sh_int beats; /* Waiting time after use       */
    const char *verb; /* Damage message               */
    const char *msg_off; /* Wear off message for the wearer  */
    DispelMagicFunc dispel_fun;
    /* If a spell effect can be dispelled or cancelled, set this to contain the
     * message sent to others in the room about a victim */
    const char *dispel_victim_msg_to_room;
    /* AFFECT bit if this is a dispellable affect that can permanently affect an NPC
     * i.e. it's a spell applied in the area file that is not going to appear
     * as an entry in an AffectList.
     */
    const unsigned int dispel_npc_perm_affect_bit;
};

struct group_type {
    const char *name;
    sh_int rating[MAX_CLASS];
    const char *spells[MAX_IN_GROUP];
};

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern sh_int gsn_sharpen;
extern sh_int gsn_backstab;
extern sh_int gsn_dodge;
extern sh_int gsn_hide;
extern sh_int gsn_peek;
extern sh_int gsn_pick_lock;
extern sh_int gsn_sneak;
extern sh_int gsn_steal;

extern sh_int gsn_disarm;
extern sh_int gsn_enhanced_damage;
extern sh_int gsn_kick;
extern sh_int gsn_parry;
extern sh_int gsn_rescue;
extern sh_int gsn_second_attack;
extern sh_int gsn_third_attack;

extern sh_int gsn_blindness;
extern sh_int gsn_charm_person;
extern sh_int gsn_curse;
extern sh_int gsn_invis;
extern sh_int gsn_mass_invis;
extern sh_int gsn_plague;
extern sh_int gsn_poison;
extern sh_int gsn_sleep;

/* new gsns */
extern sh_int gsn_axe;
extern sh_int gsn_dagger;
extern sh_int gsn_flail;
extern sh_int gsn_mace;
extern sh_int gsn_polearm;
extern sh_int gsn_shield_block;
extern sh_int gsn_spear;
extern sh_int gsn_sword;
extern sh_int gsn_whip;

extern sh_int gsn_bash;
extern sh_int gsn_berserk;
extern sh_int gsn_dirt;
extern sh_int gsn_hand_to_hand;
extern sh_int gsn_trip;

extern sh_int gsn_fast_healing;
extern sh_int gsn_haggle;
extern sh_int gsn_lore;
extern sh_int gsn_meditation;

extern sh_int gsn_scrolls;
extern sh_int gsn_staves;
extern sh_int gsn_wands;
extern sh_int gsn_recall;

/* a few extra xania skilloys */

/* commented out for time being --Fara
extern sh_int  gsn_raise_dead; */

extern sh_int gsn_headbutt;
extern sh_int gsn_ride;
extern sh_int gsn_throw;
extern sh_int gsn_octarine_fire;
extern sh_int gsn_insanity;
extern sh_int gsn_bless;

/*
 * Utility macros.
 */
//#define UMIN(a, b)              ((a) < (b) ? (a) : (b))
//#define UMAX(a, b)              ((a) > (b) ? (a) : (b))
#define UMIN(a, b)                                                                                                     \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a < _b ? _a : _b;                                                                                             \
    })
#define UMAX(a, b)                                                                                                     \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a > _b ? _a : _b;                                                                                             \
    })
#define URANGE(a, b, c) ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c) ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define IS_SET(flag, bit) (((flag) & (bit)) ? true : false)
#define SET_BIT(var, bit) ((var) |= (bit))
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))

/*
 * Character macros.
 */
#define IS_AFFECTED(ch, sn) (IS_SET((ch)->affected_by, (sn)))

#define GET_AC(ch, type)                                                                                               \
    ((ch)->armor[type] + (ch->is_pos_awake() ? dex_app[get_curr_stat(ch, Stat::Dex)].defensive : 0))

#define IS_OUTSIDE(ch) (!IS_SET((ch)->in_room->room_flags, ROOM_INDOORS))

#define WAIT_STATE(ch, npulse) ((ch)->wait = UMAX((ch)->wait, (npulse)))

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part) (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat) (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (IS_SET((obj)->value[4], (stat)))

/*
 * Structure for a social in the socials table.
 */
struct social_type {
    char name[20];
    const char *char_no_arg;
    const char *others_no_arg;
    const char *char_found;
    const char *others_found;
    const char *vict_found;
    const char *char_auto;
    const char *others_auto;
};

/*
 * Global constants.
 */
extern const struct str_app_type str_app[26];
extern const struct int_app_type int_app[26];
extern const struct wis_app_type wis_app[26];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];

extern const struct materials_type material_table[];

extern const struct class_type class_table[MAX_CLASS];
extern const struct attack_type attack_table[];
extern const struct race_type race_table[];
extern const struct pc_race_type pc_race_table[];
extern const struct liq_type liq_table[LIQ_MAX + 1];
extern const struct skill_type skill_table[MAX_SKILL];
extern const struct group_type group_table[MAX_GROUP];
extern struct social_type social_table[MAX_SOCIALS];
extern const char *title_table[MAX_CLASS][MAX_LEVEL + 1][2];

struct flag_type {
    const char *name;
    int bit;
    bool settable;
};

extern const struct flag_type door_resets[];
extern const struct flag_type room_flags[];
extern const struct flag_type sector_flags[];
extern const struct flag_type extra_flags[];
extern const struct flag_type wear_flags[];

/*
 * Global variables.
 */
extern SHOP_DATA *shop_first;

extern BAN_DATA *ban_list;
extern GenericList<Char *> char_list;
extern GenericList<OBJ_DATA *> object_list;

extern bool fLogAll;
extern KILL_DATA kill_table[];

/* Moog added stuff */
extern std::string deity_name;

/* Added by Rohan, to try to make count work properly */
extern size_t max_on;

/* act_comm.c */
void add_follower(Char *ch, Char *master);
void stop_follower(Char *ch);
void nuke_pets(Char *ch);
void die_follower(Char *ch);
bool is_same_group(const Char *ach, const Char *bch);
void thrown_off(Char *ch, Char *pet);
void fallen_off_mount(Char *ch);

/* act_move.c */
void move_char(Char *ch, Direction door);
void unride_char(Char *ch, Char *pet);
/* act_obj.c */
bool can_loot(const Char *ch, const OBJ_DATA *obj);
void get_obj(Char *ch, OBJ_DATA *obj, OBJ_DATA *container);
bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, OBJ_DATA *moving_obj, OBJ_DATA *obj_to);
bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, OBJ_DATA *moving_obj,
                                  GenericList<OBJ_DATA *> &objs_to);

/* act_wiz.c */

/* ban.c */
void save_bans();
void load_bans();
bool check_ban(const char *site, int type);
void ban_site(Char *ch, const char *site, bool fType);

/* db.c */
void boot_db();
void area_update();
Char *create_mobile(MobIndexData *pMobIndex);
void clone_mobile(Char *parent, Char *clone);
OBJ_DATA *create_object(OBJ_INDEX_DATA *pObjIndex);
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone);
const char *get_extra_descr(std::string_view name, const std::vector<EXTRA_DESCR_DATA> &ed);
OBJ_INDEX_DATA *get_obj_index(int vnum);
ROOM_INDEX_DATA *get_room_index(int vnum);
char *fread_word(FILE *fp);
void *alloc_mem(int sMem);
void *alloc_perm(int sMem);
void free_mem(void *pMem, int sMem);
char *str_dup(const char *str);
void free_string(char *pstr);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent();
int number_bits(int width);
int number_mm();
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
void append_file(Char *ch, const char *file, const char *str);

/* handler.c */
int check_immune(Char *ch, int dam_type);
int material_lookup(std::string_view name);
int class_lookup(const char *name);
int get_skill(const Char *ch, int sn);
int get_weapon_sn(Char *ch);
int get_weapon_skill(Char *ch, int sn);
int get_age(const Char *ch);
int get_curr_stat(const Char *ch, Stat stat);
int get_max_train(Char *ch, Stat stat);
int can_carry_n(Char *ch);
int can_carry_w(Char *ch);
void affect_to_char(Char *ch, const AFFECT_DATA &af);
void affect_to_obj(OBJ_DATA *obj, const AFFECT_DATA &af);
void affect_remove(Char *ch, const AFFECT_DATA &af);
void affect_remove_obj(OBJ_DATA *obj, const AFFECT_DATA &af);
void affect_strip(Char *ch, int sn);
bool is_affected(const Char *ch, int sn);
AFFECT_DATA *find_affect(Char *ch, int sn);
void affect_join(Char *ch, const AFFECT_DATA &af);
void char_from_room(Char *ch);
void char_to_room(Char *ch, ROOM_INDEX_DATA *pRoomIndex);
void obj_to_char(OBJ_DATA *obj, Char *ch);
void obj_from_char(OBJ_DATA *obj);
int apply_ac(OBJ_DATA *obj, int iWear, int type);
OBJ_DATA *get_eq_char(Char *ch, int iWear);
void equip_char(Char *ch, OBJ_DATA *obj, int iWear);
void unequip_char(Char *ch, OBJ_DATA *obj);
int count_obj_list(OBJ_INDEX_DATA *obj, const GenericList<OBJ_DATA *> &list);
void obj_from_room(OBJ_DATA *obj);
void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex);
void obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to);
void obj_from_obj(OBJ_DATA *obj);
void extract_obj(OBJ_DATA *obj);

/* MRG added */
bool check_sub_issue(OBJ_DATA *obj, Char *ch);

Char *get_mob_by_vnum(sh_int vnum);
OBJ_DATA *get_obj_type(OBJ_INDEX_DATA *pObjIndexData);
OBJ_DATA *get_obj_list(const Char *ch, std::string_view argument, GenericList<OBJ_DATA *> &list);
OBJ_DATA *get_obj_carry(Char *ch, const char *argument);
OBJ_DATA *get_obj_wear(Char *ch, const char *argument);
OBJ_DATA *get_obj_here(const Char *ch, std::string_view argument);
OBJ_DATA *get_obj_world(Char *ch, std::string_view argument);
OBJ_DATA *create_money(int amount);
int get_obj_number(OBJ_DATA *obj);
int get_obj_weight(OBJ_DATA *obj);
bool room_is_dark(ROOM_INDEX_DATA *pRoomIndex);
bool room_is_private(ROOM_INDEX_DATA *pRoomIndex);
bool can_see(const Char *ch, const Char *victim);
inline std::string_view pers(const Char *ch, const Char *looker) {
    if (!looker->can_see(*ch))
        return "someone";
    return ch->is_npc() ? ch->short_descr : ch->name;
}
bool can_see_obj(const Char *ch, const OBJ_DATA *obj);
bool can_see_room(const Char *ch, const ROOM_INDEX_DATA *pRoomIndex);
bool can_drop_obj(Char *ch, OBJ_DATA *obj);
const char *item_type_name(OBJ_DATA *obj);
const char *item_index_type_name(OBJ_INDEX_DATA *obj);
std::string affect_bit_name(unsigned int vector);
std::string extra_bit_name(unsigned int extra_flags);
const char *wear_bit_name(int wear_flags);
const char *act_bit_name(int act_flags);
const char *off_bit_name(int off_flags);
const char *imm_bit_name(int imm_flags);
const char *form_bit_name(int form_flags);
const char *part_bit_name(int part_flags);
const char *weapon_bit_name(int weapon_flags);
const char *comm_bit_name(int comm_flags);

/* interp.c */
void interpret(Char *ch, const char *argument);
const char *one_argument(const char *argument, char *arg_first);
char *one_argument(char *argument, char *arg_first); // TODO(MRG) get rid of this as soon as we can.

/* magic.c */
int mana_cost(Char *ch, int min_mana, int level);
int slot_lookup(int slot);
bool saves_spell(int level, const Char *victim);
void obj_cast_spell(int sn, int level, Char *ch, Char *victim, OBJ_DATA *obj);

/* skills.c */
bool parse_gen_groups(Char *ch, const char *argument);
void list_group_costs(Char *ch);
unsigned int exp_per_level(const Char *ch, int points);
void check_improve(Char *ch, int sn, bool success, int multiplier);
int group_lookup(const char *name);
void gn_add(Char *ch, int gn);
void gn_remove(Char *ch, int gn);
void group_add(Char *ch, const char *name, bool deduct);
void group_remove(Char *ch, const char *name);

/* special.c */
SpecialFunc spec_lookup(const char *name);

/* update.c */
void advance_level(Char *ch);
void gain_exp(Char *ch, int gain);
void update_handler();
bool is_safe_sentient(Char *ch, Char *wch);

/* xania.c - a mishmash */
void check_xania();
int get_skill_level(const Char *ch, int gsn);
int get_skill_difficulty(Char *ch, int gsn);
int get_skill_trains(Char *ch, int gsn);
int get_group_trains(Char *ch, int gsn);
int get_group_level(Char *ch, int gsn);
int check_material_vulnerability(Char *ch, OBJ_DATA *object);

void load_tipfile(); /* tip wizard - Faramir 21 Sep 1998 */
void tip_players();

#define MACRO_STRINGIFY(s) MACRO_STRINGIFY_(s)
#define MACRO_STRINGIFY_(s) #s

#include "Logging.hpp" // TODO remove with haste!

#define bug_snprintf(...)                                                                                              \
    do {                                                                                                               \
        if (snprintf(__VA_ARGS__) < 0)                                                                                 \
            bug("Buffer too small at " __FILE__ ":" MACRO_STRINGIFY(__LINE__) " - message was truncated");             \
    } while (0)
