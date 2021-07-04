/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

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