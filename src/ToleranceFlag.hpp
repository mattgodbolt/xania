/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

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
