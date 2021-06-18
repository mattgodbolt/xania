/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "AttackType.hpp"
#include "InjuredPart.hpp"

// Combines damage related values used when generating damage messages sent to a player.
struct DamageContext {
    const int damage;
    const AttackType atk_type;
    const int dam_type;
    const bool immune;
    const InjuredPart &injured_part;
};