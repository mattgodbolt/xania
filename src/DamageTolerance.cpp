/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageTolerance.hpp"
#include "Char.hpp"
#include "DamageType.hpp"
#include "ToleranceFlag.hpp"
#include "common/BitOps.hpp"

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.cpp */

DamageTolerance check_damage_tolerance(const Char *ch, const DamageType damage_type) {
    ToleranceFlag bit;
    if (damage_type == DamageType::None)
        return DamageTolerance::None;

    if (damage_type <= DamageType::Slash) { // TODO sort this out.
        // ToleranceFlag::Weapon is a more potent version of ToleranceFlag::Bash/Pierce/Slash as it represents
        // the tolerance of all three melee damage types. So it's checked first.
        // TODO: I think we should ditch this catch all case and apply specific tolerance bits to
        // all of the NPCs that need it. And likewise ToleranceFlag::Magic should probably be removed.
        if (check_enum_bit(ch->imm_flags, ToleranceFlag::Weapon))
            return DamageTolerance::Immune;
        else if (check_enum_bit(ch->res_flags, ToleranceFlag::Weapon))
            return DamageTolerance::Resistant;
        else if (check_enum_bit(ch->vuln_flags, ToleranceFlag::Weapon))
            return DamageTolerance::Vulnerable;
    }
    switch (damage_type) {
    case DamageType::Bash: bit = ToleranceFlag::Bash; break;
    case DamageType::Pierce: bit = ToleranceFlag::Pierce; break;
    case DamageType::Slash: bit = ToleranceFlag::Slash; break;
    case DamageType::Fire: bit = ToleranceFlag::Fire; break;
    case DamageType::Cold: bit = ToleranceFlag::Cold; break;
    case DamageType::Lightning: bit = ToleranceFlag::Lightning; break;
    case DamageType::Acid: bit = ToleranceFlag::Acid; break;
    case DamageType::Poison: bit = ToleranceFlag::Poison; break;
    case DamageType::Negative: bit = ToleranceFlag::Negative; break;
    case DamageType::Holy: bit = ToleranceFlag::Holy; break;
    case DamageType::Energy: bit = ToleranceFlag::Energy; break;
    case DamageType::Mental: bit = ToleranceFlag::Mental; break;
    case DamageType::Disease: bit = ToleranceFlag::Disease; break;
    case DamageType::Drowning: bit = ToleranceFlag::Drowning; break;
    case DamageType::Light: bit = ToleranceFlag::Light; break;
    case DamageType::Other: // everything else is considered magic
    case DamageType::Harm:
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