/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
enum class AffectFlag : unsigned int {
    Blind = A,
    Invisible = B,
    DetectEvil = C,
    DetectInvis = D,
    DetectMagic = E,
    DetectHidden = F,
    Talon = G,
    Sanctuary = H,
    FaerieFire = I,
    Infrared = J,
    Curse = K,
    ProtectionEvil = L,
    Poison = M,
    ProtectionGood = N,
    // O unused currently
    Sneak = P,
    Hide = Q,
    Sleep = R,
    Charm = S,
    Flying = T,
    PassDoor = U,
    Haste = V,
    Calm = W,
    Plague = X,
    Weaken = Y,
    DarkVision = Z,
    Berserk = aa,
    Swim = bb,
    Regeneration = cc,
    OctarineFire = dd,
    Lethargy = ee
};

[[nodiscard]] constexpr auto to_int(const AffectFlag flag) noexcept {
    return magic_enum::enum_integer<AffectFlag>(flag);
}
