/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

// "Extra" flags for objects.
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
