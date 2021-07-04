/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageResistance.hpp"
#include "BitsDamageResistance.hpp"
#include "Char.hpp"
#include "DamageClass.hpp"
#include "common/BitOps.hpp"

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.cpp */

DamageResistance check_immune(const Char *ch, const int dam_type) {
    DamageResistance resistance;
    int bit;

    resistance = DamageResistance::None;

    if (dam_type == DAM_NONE)
        return resistance;

    if (dam_type <= 3) { // TODO sort this out.
        if (check_bit(ch->imm_flags, IMM_WEAPON))
            resistance = DamageResistance::Immune;
        else if (check_bit(ch->res_flags, RES_WEAPON))
            resistance = DamageResistance::Resistant;
        else if (check_bit(ch->vuln_flags, VULN_WEAPON))
            resistance = DamageResistance::Vulnerable;
    } else /* magical attack */
    {
        if (check_bit(ch->imm_flags, IMM_MAGIC))
            resistance = DamageResistance::Immune;
        else if (check_bit(ch->res_flags, RES_MAGIC))
            resistance = DamageResistance::Resistant;
        else if (check_bit(ch->vuln_flags, VULN_MAGIC))
            resistance = DamageResistance::Vulnerable;
    }

    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    switch (dam_type) {
    case (DAM_BASH): bit = IMM_BASH; break;
    case (DAM_PIERCE): bit = IMM_PIERCE; break;
    case (DAM_SLASH): bit = IMM_SLASH; break;
    case (DAM_FIRE): bit = IMM_FIRE; break;
    case (DAM_COLD): bit = IMM_COLD; break;
    case (DAM_LIGHTNING): bit = IMM_LIGHTNING; break;
    case (DAM_ACID): bit = IMM_ACID; break;
    case (DAM_POISON): bit = IMM_POISON; break;
    case (DAM_NEGATIVE): bit = IMM_NEGATIVE; break;
    case (DAM_HOLY): bit = IMM_HOLY; break;
    case (DAM_ENERGY): bit = IMM_ENERGY; break;
    case (DAM_MENTAL): bit = IMM_MENTAL; break;
    case (DAM_DISEASE): bit = IMM_DISEASE; break;
    case (DAM_DROWNING): bit = IMM_DROWNING; break;
    case (DAM_LIGHT): bit = IMM_LIGHT; break;
    default: return resistance;
    }

    if (check_bit(ch->imm_flags, bit))
        resistance = DamageResistance::Immune;
    else if (check_bit(ch->res_flags, bit))
        resistance = DamageResistance::Resistant;
    else if (check_bit(ch->vuln_flags, bit))
        resistance = DamageResistance::Vulnerable;

    return resistance;
}