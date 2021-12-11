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
constexpr std::array<Flag<ToleranceFlag>, NumToleranceFlags> AllToleranceFlags = {{
    // clang-format off
    {ToleranceFlag::None, "none"},
    {ToleranceFlag::Summon, "summon"},
    {ToleranceFlag::Charm, "charm"},
    {ToleranceFlag::Magic, "magic"},
    {ToleranceFlag::Weapon, "weapon"},
    {ToleranceFlag::Bash, "bash"},
    {ToleranceFlag::Pierce, "pierce"},
    {ToleranceFlag::Slash, "slash"},
    {ToleranceFlag::Fire, "fire"},
    {ToleranceFlag::Cold, "cold"},
    {ToleranceFlag::Lightning, "lightning"},
    {ToleranceFlag::Acid, "acid"},
    {ToleranceFlag::Poison, "poison"},
    {ToleranceFlag::Negative, "negative"},
    {ToleranceFlag::Holy, "holy"},
    {ToleranceFlag::Energy, "energy"},
    {ToleranceFlag::Mental, "mental"},
    {ToleranceFlag::Disease, "disease"},
    {ToleranceFlag::Drowning, "drowning"},
    {ToleranceFlag::Light, "light"},
    {ToleranceFlag::Wood, "wood"},
    {ToleranceFlag::Silver, "silver"},
    {ToleranceFlag::Iron, "iron"}
    // clang-format on
}};

}