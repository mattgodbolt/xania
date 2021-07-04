/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

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
