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
constexpr std::array<Flag<OffensiveFlag>, NumOffensiveFlags> AllOffensiveFlags = {{
    // clang-format off
    {OffensiveFlag::AreaAttack, "area_attack"},
    {OffensiveFlag::Backstab, "backstab"},
    {OffensiveFlag::Bash, "bash"},
    {OffensiveFlag::Berserk, "berserk"},
    {OffensiveFlag::Disarm, "disarm"},
    {OffensiveFlag::Dodge, "dodge"},
    {OffensiveFlag::Fade, "fade"},
    {OffensiveFlag::Fast, "fast"},
    {OffensiveFlag::Slow, "slow"},
    {OffensiveFlag::Kick, "kick"},
    {OffensiveFlag::KickDirt, "kick_dirt"},
    {OffensiveFlag::Parry, "parry"},
    {OffensiveFlag::Rescue, "rescue"},
    {OffensiveFlag::Tail, "tail"},
    {OffensiveFlag::Trip, "trip"},
    {OffensiveFlag::Crush, "crush"},
    {OffensiveFlag::AssistAll, "assist_all"},
    {OffensiveFlag::AssistAlign, "assist_align"},
    {OffensiveFlag::AssistRace, "assist_race"},
    {OffensiveFlag::AssistPlayers, "assist_players"},
    {OffensiveFlag::AssistGuard, "assist_guard"},
    {OffensiveFlag::AssistVnum, "assist_vnum"},
    {OffensiveFlag::Headbutt, "headbutt"}
    // clang-format on
}};

}