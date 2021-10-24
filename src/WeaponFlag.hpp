
/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

enum class WeaponFlag : unsigned int {
    Flaming = A,
    Frost = B,
    Vampiric = C,
    Sharp = D,
    Vorpal = E,
    TwoHands = F,
    Poisoned = G,
    Plagued = I,
    Lightning = J,
    Acid = K
};
