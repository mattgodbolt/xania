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
