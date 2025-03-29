/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Flag.hpp"

#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

enum class ObjectExtraFlag : unsigned int {
    Glow = A,
    Hum = B,
    Dark = C,
    Lock = D,
    Evil = E,
    Invis = F,
    Magic = G,
    NoDrop = H,
    Bless = I,
    AntiGood = J,
    AntiEvil = K,
    AntiNeutral = L,
    NoRemove = M, // Only Weapons Are Meant To Have This, It Prevents Disarm.
    Inventory = N,
    NoPurge = O,
    RotDeath = P,
    VisDeath = Q,
    ProtectContainer = R,
    NoLocate = S,
    SummonCorpse = T,
    Unique = U
};

[[nodiscard]] constexpr auto to_int(const ObjectExtraFlag flag) noexcept {
    return magic_enum::enum_integer<ObjectExtraFlag>(flag);
}

namespace ObjectExtraFlags {

using OF = Flag<ObjectExtraFlag>;
constexpr auto Glow = OF{ObjectExtraFlag::Glow, "glow"};
constexpr auto Hum = OF{ObjectExtraFlag::Hum, "hum"};
constexpr auto Dark = OF{ObjectExtraFlag::Dark, "dark"};
constexpr auto Lock = OF{ObjectExtraFlag::Lock, "lock"};
constexpr auto Evil = OF{ObjectExtraFlag::Evil, "evil"};
constexpr auto Invis = OF{ObjectExtraFlag::Invis, "invis"};
constexpr auto Magic = OF{ObjectExtraFlag::Magic, "magic"};
constexpr auto NoDrop = OF{ObjectExtraFlag::NoDrop, "nodrop"};
constexpr auto Bless = OF{ObjectExtraFlag::Bless, "bless"};
constexpr auto AntiGood = OF{ObjectExtraFlag::AntiGood, "antigood"};
constexpr auto AntiEvil = OF{ObjectExtraFlag::AntiEvil, "antievil"};
constexpr auto AntiNeutral = OF{ObjectExtraFlag::AntiNeutral, "antineutral"};
constexpr auto NoRemove = OF{ObjectExtraFlag::NoRemove, "noremove"};
constexpr auto Inventory = OF{ObjectExtraFlag::Inventory, "inventory"};
constexpr auto NoPurge = OF{ObjectExtraFlag::NoPurge, "nopurge"};
constexpr auto RotDeath = OF{ObjectExtraFlag::RotDeath, "rotdeath"};
constexpr auto VisDeath = OF{ObjectExtraFlag::VisDeath, "visdeath"};
constexpr auto ProtectContainer = OF{ObjectExtraFlag::ProtectContainer, "protected"};
constexpr auto NoLocate = OF{ObjectExtraFlag::NoLocate, "nolocate"};
constexpr auto SummonCorpse = OF{ObjectExtraFlag::SummonCorpse, "summon_corpse"};
constexpr auto Unique = OF{ObjectExtraFlag::Unique, "unique"};

constexpr auto NumObjectExtraFlags = 21;
constexpr std::array<OF, NumObjectExtraFlags> AllObjectExtraFlags = {
    {Glow,     Hum,          Dark,        Lock,     Evil,      Invis,   Magic,    NoDrop,   Bless,
     AntiGood, AntiEvil,     AntiNeutral, NoRemove, Inventory, NoPurge, RotDeath, VisDeath, ProtectContainer,
     NoLocate, SummonCorpse, Unique}};

}