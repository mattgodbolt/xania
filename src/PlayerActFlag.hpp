/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

// ACT bits for players.
// IMPORTANT: These bits are flipped on Char.act just as the
// act bits in harActFlag.hpp are. However, these ones are only intended for player characters.
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

constexpr auto NumPlayerActFlags = 21;
constexpr std::array<Flag, NumPlayerActFlags> AllPlayerActFlags = {{
    // clang-format off
    {to_int(PlayerActFlag::PlrNpc), "npc"}, // Only set for NPCs
    {to_int(PlayerActFlag::PlrBoughtPet), "owner"},
    {to_int(PlayerActFlag::PlrAutoAssist), "autoassist"},
    {to_int(PlayerActFlag::PlrAutoExit), "autoexit"},
    {to_int(PlayerActFlag::PlrAutoLoot), "autoloot"},
    {to_int(PlayerActFlag::PlrAutoSac), "autosac"},
    {to_int(PlayerActFlag::PlrAutoGold), "autogold"},
    {to_int(PlayerActFlag::PlrAutoSplit), "autosplit"},
    {to_int(PlayerActFlag::PlrHolyLight), "holy_light"},
    {to_int(PlayerActFlag::PlrWizInvis), "wizinvis"},
    {to_int(PlayerActFlag::PlrCanLoot), "loot_corpse"},
    {to_int(PlayerActFlag::PlrNoSummon), "no_summon"},
    {to_int(PlayerActFlag::PlrNoFollow), "no_follow"},
    {to_int(PlayerActFlag::PlrAfk), "afk"},
    {to_int(PlayerActFlag::PlrLog), "log"},
    {to_int(PlayerActFlag::PlrDeny), "deny"},
    {to_int(PlayerActFlag::PlrFreeze), "freeze"},
    {to_int(PlayerActFlag::PlrThief), "thief"},
    {to_int(PlayerActFlag::PlrKiller), "killer"},
    {to_int(PlayerActFlag::PlrAutoPeek), "autopeek"},
    {to_int(PlayerActFlag::PlrProwl), "prowl"}
    // clang-format on
}};

}