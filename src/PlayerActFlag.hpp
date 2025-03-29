/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

// ACT bits for players.
// IMPORTANT: These bits are flipped on Char.act just as the
// act bits in CharActFlag.hpp are. However, these ones are only intended for player characters.
// TODO: The two sets of bits should either be consolidated, or perhaps we should store
// player-only bits in an bitfield in PcData.
enum class PlayerActFlag : unsigned long {
    PlrNpc = A, // never set on players
    PlrBoughtPet = B,
    PlrAutoAssist = C,
    PlrAutoExit = D,
    PlrAutoLoot = E,
    PlrAutoSac = F,
    PlrAutoGold = G,
    PlrAutoSplit = H,
    /* 5 Bits Reserved, I-M */
    PlrHolyLight = N,
    PlrWizInvis = O,
    PlrCanLoot = P,
    PlrNoSummon = Q,
    PlrNoFollow = R,
    PlrAfk = S,
    /* 3 Bits Reserved, T-V */
    PlrLog = W,
    PlrDeny = X,
    PlrFreeze = Y,
    PlrThief = Z,
    PlrKiller = aa,
    PlrAutoPeek = bb,
    PlrProwl = cc
};

[[nodiscard]] constexpr auto to_int(const PlayerActFlag flag) noexcept {
    return magic_enum::enum_integer<PlayerActFlag>(flag);
}

namespace PlayerActFlags {

using PF = Flag<PlayerActFlag>;
constexpr auto PlrNpc = PF{PlayerActFlag::PlrNpc, "npc"};
constexpr auto PlrBoughtPet = PF{PlayerActFlag::PlrBoughtPet, "owner"};
constexpr auto PlrAutoAssist = PF{PlayerActFlag::PlrAutoAssist, "autoassist"};
constexpr auto PlrAutoExit = PF{PlayerActFlag::PlrAutoExit, "autoexit"};
constexpr auto PlrAutoLoot = PF{PlayerActFlag::PlrAutoLoot, "autoloot"};
constexpr auto PlrAutoSac = PF{PlayerActFlag::PlrAutoSac, "autosac"};
constexpr auto PlrAutoGold = PF{PlayerActFlag::PlrAutoGold, "autogold"};
constexpr auto PlrAutoSplit = PF{PlayerActFlag::PlrAutoSplit, "autosplit"};
constexpr auto PlrHolyLight = PF{PlayerActFlag::PlrHolyLight, "holy_light"};
constexpr auto PlrWizInvis = PF{PlayerActFlag::PlrWizInvis, "wizinvis"};
constexpr auto PlrCanLoot = PF{PlayerActFlag::PlrCanLoot, "loot_corpse"};
constexpr auto PlrNoSummon = PF{PlayerActFlag::PlrNoSummon, "no_summon"};
constexpr auto PlrNoFollow = PF{PlayerActFlag::PlrNoFollow, "no_follow"};
constexpr auto PlrAfk = PF{PlayerActFlag::PlrAfk, "afk"};
constexpr auto PlrLog = PF{PlayerActFlag::PlrLog, "log"};
constexpr auto PlrDeny = PF{PlayerActFlag::PlrDeny, "deny"};
constexpr auto PlrFreeze = PF{PlayerActFlag::PlrFreeze, "freeze"};
constexpr auto PlrThief = PF{PlayerActFlag::PlrThief, "thief"};
constexpr auto PlrKiller = PF{PlayerActFlag::PlrKiller, "killer"};
constexpr auto PlrAutoPeek = PF{PlayerActFlag::PlrAutoPeek, "autopeek"};
constexpr auto PlrProwl = PF{PlayerActFlag::PlrProwl, "prowl"};

constexpr auto NumPlayerActFlags = 21;
constexpr std::array<PF, NumPlayerActFlags> AllPlayerActFlags = {
    {PlrNpc,       PlrBoughtPet, PlrAutoAssist, PlrAutoExit, PlrAutoLoot, PlrAutoSac,  PlrAutoGold,
     PlrAutoSplit, PlrHolyLight, PlrWizInvis,   PlrCanLoot,  PlrNoSummon, PlrNoFollow, PlrAfk,
     PlrLog,       PlrDeny,      PlrFreeze,     PlrThief,    PlrKiller,   PlrAutoPeek, PlrProwl}};

}