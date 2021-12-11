
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
constexpr std::array<Flag<CharActFlag>, NumCharActFlags> AllCharActFlags = {{
    // clang-format off
    {CharActFlag::Npc, "npc"},
    {CharActFlag::Sentinel, "sentinel"},
    {CharActFlag::Scavenger, "scavenger"},
    {CharActFlag::Aggressive, "aggressive"},
    {CharActFlag::StayArea, "stay_area"},
    {CharActFlag::Wimpy, "wimpy"},
    {CharActFlag::Pet, "pet"},
    {CharActFlag::Train, "train"},
    {CharActFlag::Practice, "practice"},
    {CharActFlag::Sentient, "sentient"},
    {CharActFlag::Talkative, "talkative"},
    {CharActFlag::Undead, "undead"},
    {CharActFlag::Cleric, "cleric"},
    {CharActFlag::Mage, "mage"},
    {CharActFlag::Thief, "thief"},
    {CharActFlag::Warrior, "warrior"},
    {CharActFlag::NoAlign, "no_align"},
    {CharActFlag::NoPurge, "no_purge"},
    {CharActFlag::Healer, "healer"},
    {CharActFlag::Gain, "skill_train"},
    {CharActFlag::UpdateAlways, "update_always"},
    {CharActFlag::CanBeRidden, "rideable"}
    // clang-format on
}};

}
