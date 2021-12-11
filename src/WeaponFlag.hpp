
/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class WeaponFlag : unsigned int {
    Flaming = A,
    Frost = B,
    Vampiric = C,
    Sharp = D,
    Vorpal = E,
    TwoHands = F,
    Poisoned = G,
    Plagued = I,
    Lightning = J,
    Acid = K
};

[[nodiscard]] constexpr auto to_int(const WeaponFlag flag) noexcept {
    return magic_enum::enum_integer<WeaponFlag>(flag);
}

namespace WeaponFlags {

constexpr auto NumWeaponFlags = 10;
constexpr std::array<Flag<WeaponFlag>, NumWeaponFlags> AllWeaponFlags = {{
    // clang-format off
    {WeaponFlag::Flaming, "flaming"},
    {WeaponFlag::Frost, "frost"},
    {WeaponFlag::Vampiric, "vampiric"},
    {WeaponFlag::Sharp, "sharp"},
    {WeaponFlag::Vorpal, "vorpal"},
    {WeaponFlag::TwoHands, "two-handed"},
    {WeaponFlag::Poisoned, "poisoned"},
    {WeaponFlag::Plagued, "plagued"},
    {WeaponFlag::Lightning, "lightning"},
    {WeaponFlag::Acid, "acid"}
    // clang-format on
}};

}