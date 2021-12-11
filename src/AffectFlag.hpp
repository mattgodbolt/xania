/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "FlagFormat.hpp"
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

namespace AffectFlags {

constexpr auto NumAffectFlags = 30;
constexpr std::array<Flag<AffectFlag>, NumAffectFlags> AllAffectFlags = {{
    // clang-format off
    {AffectFlag::Blind, "blind"},
    {AffectFlag::Invisible, "invisible"},
    {AffectFlag::DetectEvil, "detect_evil"},
    {AffectFlag::DetectInvis, "detect_invis"},
    {AffectFlag::DetectMagic, "detect_magic"},
    {AffectFlag::DetectHidden, "detect_hidden"},
    {AffectFlag::Talon, "talon"},
    {AffectFlag::Sanctuary, "sanctuary"},
    {AffectFlag::FaerieFire, "faerie_fire"},
    {AffectFlag::Infrared, "infrared"},
    {AffectFlag::Curse, "curse"},
    {AffectFlag::ProtectionEvil, "protection_evil"},
    {AffectFlag::Poison, "poison"},
    {AffectFlag::ProtectionGood, "protection_good"},
    {AffectFlag::Sneak, "sneak"},
    {AffectFlag::Hide, "hide"},
    {AffectFlag::Sleep, "sleep"},
    {AffectFlag::Charm, "charm"},
    {AffectFlag::Flying, "flying"},
    {AffectFlag::PassDoor, "pass_door"},
    {AffectFlag::Haste, "haste"},
    {AffectFlag::Calm, "calm"},
    {AffectFlag::Plague, "plague"},
    {AffectFlag::Weaken, "weaken"},
    {AffectFlag::DarkVision, "dark_vision"},
    {AffectFlag::Berserk, "berserk"},
    {AffectFlag::Swim, "swim"},
    {AffectFlag::Regeneration, "regeneration"},
    {AffectFlag::OctarineFire, "octarine_fire"},
    {AffectFlag::Lethargy, "lethargy"}
    // clang-format on
}};

// Returns a string containing the names of all flag bits that are set on current_val.
template <typename Bits>
std::string format(const Bits current_val) {
    return format_set_flags(AllAffectFlags, current_val);
}

}