/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

// A headbutt that lands can draw blood that runs into the victim's eyes, blinding them.
namespace Headbutt {

// The level-adjusted roll must come in under this for the victim to be blinded.
inline constexpr int BlindingThreshold = 5;

// Whether a headbutt that landed with the percentile roll hit_roll blinds the victim. The roll is adjusted by the
// difference in level, so an attacker above the victim's level is more likely to blind them, and one below is less
// likely. A victim who's already blind can't be blinded again, so they never get a second hitroll penalty.
[[nodiscard]] constexpr bool blinds(const int hit_roll, const int attacker_level, const int victim_level,
                                    const bool victim_already_blind) {
    return !victim_already_blind && hit_roll - attacker_level + victim_level < BlindingThreshold;
}

}
