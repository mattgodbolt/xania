/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "VampiricDrain.hpp"
#include "Alignment.hpp"
#include "Char.hpp"
#include "DamageType.hpp"

#include <algorithm>

namespace VampiricDrain {

int amount(const int damage_dealt, const DamageTolerance negative_tolerance) {
    if (damage_dealt <= 0)
        return 0;
    // Rounded rather than truncated, so that low-level hits drain something: at 5%, a hit of 10 drains 1.
    auto drained = (damage_dealt * Percent + 50) / 100;
    switch (negative_tolerance) {
    case DamageTolerance::Immune: return 0;
    case DamageTolerance::Resistant: drained -= drained / 3; break;
    case DamageTolerance::Vulnerable: drained += drained / 2; break;
    case DamageTolerance::None: break;
    }
    return drained;
}

int drain(Char &wielder, Char &victim, const int damage_dealt) {
    const auto drained = amount(damage_dealt, check_damage_tolerance(&victim, DamageType::Negative));
    if (drained == 0)
        return 0;
    victim.hit -= drained;
    // No overheal, and a wielder already above max (from elsewhere) isn't clamped down by a drain.
    if (wielder.hit < wielder.max_hit)
        wielder.hit = std::min<int>(wielder.hit + drained, wielder.max_hit);
    wielder.alignment = std::max<int>(Alignment::Satanic, wielder.alignment - 1);
    return drained;
}

}
