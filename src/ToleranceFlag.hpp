/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

// Character damage tolerance bits (immunity, resistance, vulnerability).
enum class ToleranceFlag : unsigned long {
    None = 0,
    Summon = A,
    Charm = B,
    Magic = C,
    Weapon = D,
    Bash = E,
    Pierce = F,
    Slash = G,
    Fire = H,
    Cold = I,
    Lightning = J,
    Acid = K,
    Poison = L,
    Negative = M,
    Holy = N,
    Energy = O,
    Mental = P,
    Disease = Q,
    Drowning = R,
    Light = S,
    // T, U V, W unused at the moment.
    Wood = X,
    Silver = Y,
    Iron = Z
};

[[nodiscard]] constexpr auto to_int(const ToleranceFlag flag) noexcept {
    return magic_enum::enum_integer<ToleranceFlag>(flag);
}

namespace ToleranceFlags {

constexpr auto NumToleranceFlags = 23;
constexpr std::array<Flag, NumToleranceFlags> AllToleranceFlags = {{
    // clang-format off
    {to_int(ToleranceFlag::None), "none"},
    {to_int(ToleranceFlag::Summon), "summon"},
    {to_int(ToleranceFlag::Charm), "charm"},
    {to_int(ToleranceFlag::Magic), "magic"},
    {to_int(ToleranceFlag::Weapon), "weapon"},
    {to_int(ToleranceFlag::Bash), "bash"},
    {to_int(ToleranceFlag::Pierce), "pierce"},
    {to_int(ToleranceFlag::Slash), "slash"},
    {to_int(ToleranceFlag::Fire), "fire"},
    {to_int(ToleranceFlag::Cold), "cold"},
    {to_int(ToleranceFlag::Lightning), "lightning"},
    {to_int(ToleranceFlag::Acid), "acid"},
    {to_int(ToleranceFlag::Poison), "poison"},
    {to_int(ToleranceFlag::Negative), "negative"},
    {to_int(ToleranceFlag::Holy), "holy"},
    {to_int(ToleranceFlag::Energy), "energy"},
    {to_int(ToleranceFlag::Mental), "mental"},
    {to_int(ToleranceFlag::Disease), "disease"},
    {to_int(ToleranceFlag::Drowning), "drowning"},
    {to_int(ToleranceFlag::Light), "light"},
    {to_int(ToleranceFlag::Wood), "wood"},
    {to_int(ToleranceFlag::Silver), "silver"},
    {to_int(ToleranceFlag::Iron), "iron"}
    // clang-format on
}};

}