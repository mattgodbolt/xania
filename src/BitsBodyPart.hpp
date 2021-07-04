/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

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
