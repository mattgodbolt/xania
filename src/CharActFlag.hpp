
/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>
/*
 * Character act bits.
 * Used in #MOBILES section of area files.
 */
enum class CharActFlag : unsigned long {
    Npc = A, // Auto set for non-player characters
    Sentinel = B, // Stays in one room
    Scavenger = C, // Picks up objects
    Aggressive = F, // Attacks PCs
    StayArea = G, // Won't leave area
    Wimpy = H,
    Pet = I, // Auto set for pets
    Train = J, // Can train PCs
    Practice = K, // Can practice PCs
    Sentient = L, // Intelligence: remembers who attacked it
    Talkative = M, // Will respond to says and emotes
    Undead = O,
    Cleric = Q,
    Mage = R,
    Thief = S,
    Warrior = T,
    NoAlign = U,
    NoPurge = V,
    Healer = aa,
    Gain = bb,
    UpdateAlways = cc,
    CanBeRidden = dd
};

[[nodiscard]] constexpr auto to_int(const CharActFlag flag) noexcept {
    return magic_enum::enum_integer<CharActFlag>(flag);
}
