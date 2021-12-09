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
constexpr std::array<Flag, NumAffectFlags> AllAffectFlags = {{
    // clang-format off
    {to_int(AffectFlag::Blind), "blind"},
    {to_int(AffectFlag::Invisible), "invisible"},
    {to_int(AffectFlag::DetectEvil), "detect_evil"},
    {to_int(AffectFlag::DetectInvis), "detect_invis"},
    {to_int(AffectFlag::DetectMagic), "detect_magic"},
    {to_int(AffectFlag::DetectHidden), "detect_hidden"},
    {to_int(AffectFlag::Talon), "talon"},
    {to_int(AffectFlag::Sanctuary), "sanctuary"},
    {to_int(AffectFlag::FaerieFire), "faerie_fire"},
    {to_int(AffectFlag::Infrared), "infrared"},
    {to_int(AffectFlag::Curse), "curse"},
    {to_int(AffectFlag::ProtectionEvil), "protection_evil"},
    {to_int(AffectFlag::Poison), "poison"},
    {to_int(AffectFlag::ProtectionGood), "protection_good"},
    {to_int(AffectFlag::Sneak), "sneak"},
    {to_int(AffectFlag::Hide), "hide"},
    {to_int(AffectFlag::Sleep), "sleep"},
    {to_int(AffectFlag::Charm), "charm"},
    {to_int(AffectFlag::Flying), "flying"},
    {to_int(AffectFlag::PassDoor), "pass_door"},
    {to_int(AffectFlag::Haste), "haste"},
    {to_int(AffectFlag::Calm), "calm"},
    {to_int(AffectFlag::Plague), "plague"},
    {to_int(AffectFlag::Weaken), "weaken"},
    {to_int(AffectFlag::DarkVision), "dark_vision"},
    {to_int(AffectFlag::Berserk), "berserk"},
    {to_int(AffectFlag::Swim), "swim"},
    {to_int(AffectFlag::Regeneration), "regeneration"},
    {to_int(AffectFlag::OctarineFire), "octarine_fire"},
    {to_int(AffectFlag::Lethargy), "lethargy"}
    // clang-format on
}};

// Returns a string containing the names of all flag bits that are set on current_val.
template <typename Bits>
std::string format(const Bits current_val) {
    return format_set_flags(AllAffectFlags, current_val);
}

}