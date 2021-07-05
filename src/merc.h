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
