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

using TF = Flag<ToleranceFlag>;
constexpr auto None = TF{ToleranceFlag::None, "none"};
constexpr auto Summon = TF{ToleranceFlag::Summon, "summon"};
constexpr auto Charm = TF{ToleranceFlag::Charm, "charm"};
constexpr auto Magic = TF{ToleranceFlag::Magic, "magic"};
constexpr auto Weapon = TF{ToleranceFlag::Weapon, "weapon"};
constexpr auto Bash = TF{ToleranceFlag::Bash, "bash"};
constexpr auto Pierce = TF{ToleranceFlag::Pierce, "pierce"};
constexpr auto Slash = TF{ToleranceFlag::Slash, "slash"};
constexpr auto Fire = TF{ToleranceFlag::Fire, "fire"};
constexpr auto Cold = TF{ToleranceFlag::Cold, "cold"};
constexpr auto Lightning = TF{ToleranceFlag::Lightning, "lightning"};
constexpr auto Acid = TF{ToleranceFlag::Acid, "acid"};
constexpr auto Poison = TF{ToleranceFlag::Poison, "poison"};
constexpr auto Negative = TF{ToleranceFlag::Negative, "negative"};
constexpr auto Holy = TF{ToleranceFlag::Holy, "holy"};
constexpr auto Energy = TF{ToleranceFlag::Energy, "energy"};
constexpr auto Mental = TF{ToleranceFlag::Mental, "mental"};
constexpr auto Disease = TF{ToleranceFlag::Disease, "disease"};
constexpr auto Drowning = TF{ToleranceFlag::Drowning, "drowning"};
constexpr auto Light = TF{ToleranceFlag::Light, "light"};
constexpr auto Wood = TF{ToleranceFlag::Wood, "wood"};
constexpr auto Silver = TF{ToleranceFlag::Silver, "silver"};
constexpr auto Iron = TF{ToleranceFlag::Iron, "iron"};

constexpr auto NumToleranceFlags = 23;
constexpr std::array<TF, NumToleranceFlags> AllToleranceFlags = {
    {None,   Summon,   Charm, Magic,  Weapon, Bash,    Pierce,   Slash, Fire, Cold,   Lightning, Acid,
     Poison, Negative, Holy,  Energy, Mental, Disease, Drowning, Light, Wood, Silver, Iron}};

}