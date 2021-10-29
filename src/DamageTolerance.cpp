/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageTolerance.hpp"
#include "Char.hpp"
#include "DamageClass.hpp"
#include "ToleranceFlag.hpp"
#include "common/BitOps.hpp"

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.cpp */

DamageTolerance check_damage_tolerance(const Char *ch, const int dam_type) {
    ToleranceFlag bit;
    if (dam_type == DAM_NONE)
        return DamageTolerance::None;

    if (dam_type <= 3) { // TODO sort this out.
        // ToleranceFlag::Weapon is a more potent version of ToleranceFlag::Bash/PIERCE/SLASH as it represents
        // the tolerance of all three melee damage classes. So it's checked first.
        // TODO: I think we should ditch this catch all case and apply specific tolerance bits to
        // all of the NPCs that need it. And likewise ToleranceFlag::Magic should probably be removed.
        if (check_enum_bit(ch->imm_flags, ToleranceFlag::Weapon))
            return DamageTolerance::Immune;
        else if (check_enum_bit(ch->res_flags, ToleranceFlag::Weapon))
            return DamageTolerance::Resistant;
        else if (check_enum_bit(ch->vuln_flags, ToleranceFlag::Weapon))
            return DamageTolerance::Vulnerable;
    }
    switch (dam_type) {
    case DAM_BASH: bit = ToleranceFlag::Bash; break;
    case DAM_PIERCE: bit = ToleranceFlag::Pierce; break;
    case DAM_SLASH: bit = ToleranceFlag::Slash; break;
    case DAM_FIRE: bit = ToleranceFlag::Fire; break;
    case DAM_COLD: bit = ToleranceFlag::Cold; break;
    case DAM_LIGHTNING: bit = ToleranceFlag::Lightning; break;
    case DAM_ACID: bit = ToleranceFlag::Acid; break;
    case DAM_POISON: bit = ToleranceFlag::Poison; break;
    case DAM_NEGATIVE: bit = ToleranceFlag::Negative; break;
    case DAM_HOLY: bit = ToleranceFlag::Holy; break;
    case DAM_ENERGY: bit = ToleranceFlag::Energy; break;
    case DAM_MENTAL: bit = ToleranceFlag::Mental; break;
    case DAM_DISEASE: bit = ToleranceFlag::Disease; break;
    case DAM_DROWNING: bit = ToleranceFlag::Drowning; break;
    case DAM_LIGHT: bit = ToleranceFlag::Light; break;
    case DAM_OTHER: // everything else is considered magic
    case DAM_HARM:
    default: bit = ToleranceFlag::Magic; break;
    }
    if (check_enum_bit(ch->imm_flags, bit))
        return DamageTolerance::Immune;
    else if (check_enum_bit(ch->res_flags, bit))
        return DamageTolerance::Resistant;
    else if (check_enum_bit(ch->vuln_flags, bit))
        return DamageTolerance::Vulnerable;
    else
        return DamageTolerance::None;
}