/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
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

namespace OffensiveFlags {

constexpr auto NumOffensiveFlags = 23;
constexpr std::array<Flag, NumOffensiveFlags> AllOffensiveFlags = {{
    // clang-format off
    {to_int(OffensiveFlag::AreaAttack), "area_attack"},
    {to_int(OffensiveFlag::Backstab), "backstab"},
    {to_int(OffensiveFlag::Bash), "bash"},
    {to_int(OffensiveFlag::Berserk), "berserk"},
    {to_int(OffensiveFlag::Disarm), "disarm"},
    {to_int(OffensiveFlag::Dodge), "dodge"},
    {to_int(OffensiveFlag::Fade), "fade"},
    {to_int(OffensiveFlag::Fast), "fast"},
    {to_int(OffensiveFlag::Slow), "slow"},
    {to_int(OffensiveFlag::Kick), "kick"},
    {to_int(OffensiveFlag::KickDirt), "kick_dirt"},
    {to_int(OffensiveFlag::Parry), "parry"},
    {to_int(OffensiveFlag::Rescue), "rescue"},
    {to_int(OffensiveFlag::Tail), "tail"},
    {to_int(OffensiveFlag::Trip), "trip"},
    {to_int(OffensiveFlag::Crush), "crush"},
    {to_int(OffensiveFlag::AssistAll), "assist_all"},
    {to_int(OffensiveFlag::AssistAlign), "assist_align"},
    {to_int(OffensiveFlag::AssistRace), "assist_race"},
    {to_int(OffensiveFlag::AssistPlayers), "assist_players"},
    {to_int(OffensiveFlag::AssistGuard), "assist_guard"},
    {to_int(OffensiveFlag::AssistVnum), "assist_vnum"},
    {to_int(OffensiveFlag::Headbutt), "headbutt"}
    // clang-format on
}};

}