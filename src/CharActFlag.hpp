
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

using CF = Flag<CharActFlag>;
constexpr auto Npc = CF{CharActFlag::Npc, "npc"};
constexpr auto Sentinel = CF{CharActFlag::Sentinel, "sentinel"};
constexpr auto Scavenger = CF{CharActFlag::Scavenger, "scavenger"};
constexpr auto Aggressive = CF{CharActFlag::Aggressive, "aggressive"};
constexpr auto StayArea = CF{CharActFlag::StayArea, "stay_area"};
constexpr auto Wimpy = CF{CharActFlag::Wimpy, "wimpy"};
constexpr auto Pet = CF{CharActFlag::Pet, "pet"};
constexpr auto Train = CF{CharActFlag::Train, "train"};
constexpr auto Practice = CF{CharActFlag::Practice, "practice"};
constexpr auto Sentient = CF{CharActFlag::Sentient, "sentient"};
constexpr auto Talkative = CF{CharActFlag::Talkative, "talkative"};
constexpr auto Undead = CF{CharActFlag::Undead, "undead"};
constexpr auto Cleric = CF{CharActFlag::Cleric, "cleric"};
constexpr auto Mage = CF{CharActFlag::Mage, "mage"};
constexpr auto Thief = CF{CharActFlag::Thief, "thief"};
constexpr auto Warrior = CF{CharActFlag::Warrior, "warrior"};
constexpr auto NoAlign = CF{CharActFlag::NoAlign, "no_align"};
constexpr auto NoPurge = CF{CharActFlag::NoPurge, "no_purge"};
constexpr auto Healer = CF{CharActFlag::Healer, "healer"};
constexpr auto Gain = CF{CharActFlag::Gain, "skill_train"};
constexpr auto UpdateAlways = CF{CharActFlag::UpdateAlways, "update_always"};
constexpr auto CanBeRidden = CF{CharActFlag::CanBeRidden, "rideable"};

constexpr auto NumCharActFlags = 22;
constexpr std::array<CF, NumCharActFlags> AllCharActFlags = {
    {Npc,    Sentinel, Scavenger, Aggressive, StayArea, Wimpy,   Pet,     Train,  Practice, Sentient,     Talkative,
     Undead, Cleric,   Mage,      Thief,      Warrior,  NoAlign, NoPurge, Healer, Gain,     UpdateAlways, CanBeRidden}};

}
