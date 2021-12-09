
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
constexpr std::array<Flag, NumWeaponFlags> AllWeaponFlags = {{
    // clang-format off
    {to_int(WeaponFlag::Flaming), "flaming"},
    {to_int(WeaponFlag::Frost), "frost"},
    {to_int(WeaponFlag::Vampiric), "vampiric"},
    {to_int(WeaponFlag::Sharp), "sharp"},
    {to_int(WeaponFlag::Vorpal), "vorpal"},
    {to_int(WeaponFlag::TwoHands), "two-handed"},
    {to_int(WeaponFlag::Poisoned), "poisoned"},
    {to_int(WeaponFlag::Plagued), "plagued"},
    {to_int(WeaponFlag::Lightning), "lightning"},
    {to_int(WeaponFlag::Acid), "acid"}
    // clang-format on
}};

}