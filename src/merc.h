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

#include "common/BitOps.hpp"
#include "common/StandardBits.hpp"

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
 * Character macros.
 */
#define IS_AFFECTED(ch, sn) (check_bit((ch)->affected_by, (sn)))

#define IS_OUTSIDE(ch) (!check_bit((ch)->in_room->room_flags, ROOM_INDOORS))

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part) (check_bit((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat) (check_bit((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (check_bit((obj)->value[4], (stat)))
