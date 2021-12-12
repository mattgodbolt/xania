
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

using WF = Flag<WeaponFlag>;
constexpr auto Flaming = WF{WeaponFlag::Flaming, "flaming"};
constexpr auto Frost = WF{WeaponFlag::Frost, "frost"};
constexpr auto Vampiric = WF{WeaponFlag::Vampiric, "vampiric"};
constexpr auto Sharp = WF{WeaponFlag::Sharp, "sharp"};
constexpr auto Vorpal = WF{WeaponFlag::Vorpal, "vorpal"};
constexpr auto TwoHands = WF{WeaponFlag::TwoHands, "two-handed"};
constexpr auto Poisoned = WF{WeaponFlag::Poisoned, "poisoned"};
constexpr auto Plagued = WF{WeaponFlag::Plagued, "plagued"};
constexpr auto Lightning = WF{WeaponFlag::Lightning, "lightning"};
constexpr auto Acid = WF{WeaponFlag::Acid, "acid"};

constexpr auto NumWeaponFlags = 10;
constexpr std::array<Flag<WeaponFlag>, NumWeaponFlags> AllWeaponFlags = {
    {Flaming, Frost, Vampiric, Sharp, Vorpal, TwoHands, Poisoned, Plagued, Lightning, Acid}};

}