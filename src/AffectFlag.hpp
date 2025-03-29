/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "FlagFormat.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

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

using AF = Flag<AffectFlag>;
constexpr auto Blind = AF{AffectFlag::Blind, "blind"};
constexpr auto Invisible = AF{AffectFlag::Invisible, "invisible"};
constexpr auto DetectEvil = AF{AffectFlag::DetectEvil, "detect_evil"};
constexpr auto DetectInvis = AF{AffectFlag::DetectInvis, "detect_invis"};
constexpr auto DetectMagic = AF{AffectFlag::DetectMagic, "detect_magic"};
constexpr auto DetectHidden = AF{AffectFlag::DetectHidden, "detect_hidden"};
constexpr auto Talon = AF{AffectFlag::Talon, "talon"};
constexpr auto Sanctuary = AF{AffectFlag::Sanctuary, "sanctuary"};
constexpr auto FaerieFire = AF{AffectFlag::FaerieFire, "faerie_fire"};
constexpr auto Infrared = AF{AffectFlag::Infrared, "infrared"};
constexpr auto Curse = AF{AffectFlag::Curse, "curse"};
constexpr auto ProtectionEvil = AF{AffectFlag::ProtectionEvil, "protection_evil"};
constexpr auto Poison = AF{AffectFlag::Poison, "poison"};
constexpr auto ProtectionGood = AF{AffectFlag::ProtectionGood, "protection_good"};
constexpr auto Sneak = AF{AffectFlag::Sneak, "sneak"};
constexpr auto Hide = AF{AffectFlag::Hide, "hide"};
constexpr auto Sleep = AF{AffectFlag::Sleep, "sleep"};
constexpr auto Charm = AF{AffectFlag::Charm, "charm"};
constexpr auto Flying = AF{AffectFlag::Flying, "flying"};
constexpr auto PassDoor = AF{AffectFlag::PassDoor, "pass_door"};
constexpr auto Haste = AF{AffectFlag::Haste, "haste"};
constexpr auto Calm = AF{AffectFlag::Calm, "calm"};
constexpr auto Plague = AF{AffectFlag::Plague, "plague"};
constexpr auto Weaken = AF{AffectFlag::Weaken, "weaken"};
constexpr auto DarkVision = AF{AffectFlag::DarkVision, "dark_vision"};
constexpr auto Berserk = AF{AffectFlag::Berserk, "berserk"};
constexpr auto Swim = AF{AffectFlag::Swim, "swim"};
constexpr auto Regeneration = AF{AffectFlag::Regeneration, "regeneration"};
constexpr auto OctarineFire = AF{AffectFlag::OctarineFire, "octarine_fire"};
constexpr auto Lethargy = AF{AffectFlag::Lethargy, "lethargy"};

constexpr auto NumAffectFlags = 30;

constexpr std::array<AF, NumAffectFlags> AllAffectFlags = {
    {Blind,      Invisible, DetectEvil, DetectInvis,    DetectMagic,  DetectHidden,   Talon,  Sanctuary,
     FaerieFire, Infrared,  Curse,      ProtectionEvil, Poison,       ProtectionGood, Sneak,  Hide,
     Sleep,      Charm,     Flying,     PassDoor,       Haste,        Calm,           Plague, Weaken,
     DarkVision, Berserk,   Swim,       Regeneration,   OctarineFire, Lethargy}};

// Returns a string containing the names of all flag bits that are set on current_val.
template <typename Bits>
std::string format(const Bits current_val) {
    return format_set_flags(AllAffectFlags, current_val);
}

}