/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "AttackType.hpp"

#include <string>

class Char;
class Rng;

// Each time damage is delivered to a victim it strikes a semi-randomly selected
// body part, represented by this struct. Currently this is purely cosmetic and
// there's no notion of sustained damage to a part. Nor is there a notion of worn
// armour or spell effects protecting specific body parts. Those protections are
// used in calculations that benefit the entire body.
struct InjuredPart {

    // Selects a victim body part semi-randomly taking in relative size of the combatants into account
    // and returns the description and body part flag.
    // If a headbutt lands it always hits the victim's head! And if the attacker hits itself
    // the caller never reports the affected body part anyway.
    [[nodiscard]] static InjuredPart random_from_victim(const Char *ch, const Char *victim, const AttackType atk_type,
                                                        Rng &rng);

    const unsigned long part_flag;
    const std::string description;
};
