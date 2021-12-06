/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

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
