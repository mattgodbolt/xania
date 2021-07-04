/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageTolerance.hpp"
#include "BitsDamageTolerance.hpp"
#include "Char.hpp"
#include "DamageClass.hpp"
#include "common/BitOps.hpp"

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.cpp */

DamageTolerance check_damage_tolerance(const Char *ch, const int dam_type) {
    int bit;
    if (dam_type == DAM_NONE)
        return DamageTolerance::None;

    if (dam_type <= 3) { // TODO sort this out.
        // DMG_TOL_WEAPON is a more potent version of DMG_TOL_BASH/PIERCE/SLASH as it represents
        // the tolerance of all three melee damage classes. So it's checked first.
        // TODO: I think we should ditch this catch all case and apply specific tolerance bits to
        // all of the NPCs that need it. And likewise DMG_TOL_MAGIC should probably be removed.
        if (check_bit(ch->imm_flags, DMG_TOL_WEAPON))
            return DamageTolerance::Immune;
        else if (check_bit(ch->res_flags, DMG_TOL_WEAPON))
            return DamageTolerance::Resistant;
        else if (check_bit(ch->vuln_flags, DMG_TOL_WEAPON))
            return DamageTolerance::Vulnerable;
    }
    switch (dam_type) {
    case DAM_BASH: bit = DMG_TOL_BASH; break;
    case DAM_PIERCE: bit = DMG_TOL_PIERCE; break;
    case DAM_SLASH: bit = DMG_TOL_SLASH; break;
    case DAM_FIRE: bit = DMG_TOL_FIRE; break;
    case DAM_COLD: bit = DMG_TOL_COLD; break;
    case DAM_LIGHTNING: bit = DMG_TOL_LIGHTNING; break;
    case DAM_ACID: bit = DMG_TOL_ACID; break;
    case DAM_POISON: bit = DMG_TOL_POISON; break;
    case DAM_NEGATIVE: bit = DMG_TOL_NEGATIVE; break;
    case DAM_HOLY: bit = DMG_TOL_HOLY; break;
    case DAM_ENERGY: bit = DMG_TOL_ENERGY; break;
    case DAM_MENTAL: bit = DMG_TOL_MENTAL; break;
    case DAM_DISEASE: bit = DMG_TOL_DISEASE; break;
    case DAM_DROWNING: bit = DMG_TOL_DROWNING; break;
    case DAM_LIGHT: bit = DMG_TOL_LIGHT; break;
    case DAM_OTHER: // everything else is considered magic
    case DAM_HARM:
    default: bit = DMG_TOL_MAGIC; break;
    }
    if (check_bit(ch->imm_flags, bit))
        return DamageTolerance::Immune;
    else if (check_bit(ch->res_flags, bit))
        return DamageTolerance::Resistant;
    else if (check_bit(ch->vuln_flags, bit))
        return DamageTolerance::Vulnerable;
    else
        return DamageTolerance::None;
}