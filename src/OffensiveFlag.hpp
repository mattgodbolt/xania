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

using OF = Flag<OffensiveFlag>;
constexpr auto AreaAttack = OF{OffensiveFlag::AreaAttack, "area_attack"};
constexpr auto Backstab = OF{OffensiveFlag::Backstab, "backstab"};
constexpr auto Bash = OF{OffensiveFlag::Bash, "bash"};
constexpr auto Berserk = OF{OffensiveFlag::Berserk, "berserk"};
constexpr auto Disarm = OF{OffensiveFlag::Disarm, "disarm"};
constexpr auto Dodge = OF{OffensiveFlag::Dodge, "dodge"};
constexpr auto Fade = OF{OffensiveFlag::Fade, "fade"};
constexpr auto Fast = OF{OffensiveFlag::Fast, "fast"};
constexpr auto Slow = OF{OffensiveFlag::Slow, "slow"};
constexpr auto Kick = OF{OffensiveFlag::Kick, "kick"};
constexpr auto KickDirt = OF{OffensiveFlag::KickDirt, "kick_dirt"};
constexpr auto Parry = OF{OffensiveFlag::Parry, "parry"};
constexpr auto Rescue = OF{OffensiveFlag::Rescue, "rescue"};
constexpr auto Tail = OF{OffensiveFlag::Tail, "tail"};
constexpr auto Trip = OF{OffensiveFlag::Trip, "trip"};
constexpr auto Crush = OF{OffensiveFlag::Crush, "crush"};
constexpr auto AssistAll = OF{OffensiveFlag::AssistAll, "assist_all"};
constexpr auto AssistAlign = OF{OffensiveFlag::AssistAlign, "assist_align"};
constexpr auto AssistRace = OF{OffensiveFlag::AssistRace, "assist_race"};
constexpr auto AssistPlayers = OF{OffensiveFlag::AssistPlayers, "assist_players"};
constexpr auto AssistGuard = OF{OffensiveFlag::AssistGuard, "assist_guard"};
constexpr auto AssistVnum = OF{OffensiveFlag::AssistVnum, "assist_vnum"};
constexpr auto Headbutt = OF{OffensiveFlag::Headbutt, "headbutt"};

constexpr auto NumOffensiveFlags = 23;
constexpr std::array<OF, NumOffensiveFlags> AllOffensiveFlags = {
    {AreaAttack, Backstab,    Bash,       Berserk,       Disarm,      Dodge,      Fade,    Fast,
     Slow,       Kick,        KickDirt,   Parry,         Rescue,      Tail,       Trip,    Crush,
     AssistAll,  AssistAlign, AssistRace, AssistPlayers, AssistGuard, AssistVnum, Headbutt}};

}