
/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
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

namespace CharActFlags {

constexpr auto NumCharActFlags = 22;
constexpr std::array<Flag, NumCharActFlags> AllCharActFlags = {{
    // clang-format off
    {to_int(CharActFlag::Npc), "npc"},
    {to_int(CharActFlag::Sentinel), "sentinel"},
    {to_int(CharActFlag::Scavenger), "scavenger"},
    {to_int(CharActFlag::Aggressive), "aggressive"},
    {to_int(CharActFlag::StayArea), "stay_area"},
    {to_int(CharActFlag::Wimpy), "wimpy"},
    {to_int(CharActFlag::Pet), "pet"},
    {to_int(CharActFlag::Train), "train"},
    {to_int(CharActFlag::Practice), "practice"},
    {to_int(CharActFlag::Sentient), "sentient"},
    {to_int(CharActFlag::Talkative), "talkative"},
    {to_int(CharActFlag::Undead), "undead"},
    {to_int(CharActFlag::Cleric), "cleric"},
    {to_int(CharActFlag::Mage), "mage"},
    {to_int(CharActFlag::Thief), "thief"},
    {to_int(CharActFlag::Warrior), "warrior"},
    {to_int(CharActFlag::NoAlign), "no_align"},
    {to_int(CharActFlag::NoPurge), "no_purge"},
    {to_int(CharActFlag::Healer), "healer"},
    {to_int(CharActFlag::Gain), "skill_train"},
    {to_int(CharActFlag::UpdateAlways), "update_always"},
    {to_int(CharActFlag::CanBeRidden), "rideable"}
    // clang-format on
}};

}
