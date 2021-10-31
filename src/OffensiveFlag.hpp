/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class OffensiveFlag {
    AreaAttack = A,
    Backstab = B,
    Bash = C,
    Berserk = D,
    Disarm = E,
    Dodge = F,
    Fade = G,
    Fast = H,
    Slow = W,
    Kick = I,
    KickDirt = J,
    Parry = K,
    Rescue = L,
    Tail = M,
    Trip = N,
    Crush = O,
    AssistAll = P,
    AssistAlign = Q,
    AssistRace = R,
    AssistPlayers = S,
    AssistGuard = T,
    AssistVnum = U,
    Headbutt = V
};

[[nodiscard]] constexpr auto to_int(const OffensiveFlag flag) noexcept {
    return magic_enum::enum_integer<OffensiveFlag>(flag);
}
