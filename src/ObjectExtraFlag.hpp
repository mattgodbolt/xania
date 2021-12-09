/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Flag.hpp"

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

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

constexpr auto NumObjectExtraFlags = 21;
constexpr std::array<Flag, NumObjectExtraFlags> AllObjectExtraFlags = {{
    // clang-format off
    {to_int(ObjectExtraFlag::Glow), "glow"},
    {to_int(ObjectExtraFlag::Hum), "hum"},
    {to_int(ObjectExtraFlag::Dark), "dark"},
    {to_int(ObjectExtraFlag::Lock), "lock"},
    {to_int(ObjectExtraFlag::Evil), "evil"},
    {to_int(ObjectExtraFlag::Invis), "invis"},
    {to_int(ObjectExtraFlag::Magic), "magic"},
    {to_int(ObjectExtraFlag::NoDrop), "nodrop"},
    {to_int(ObjectExtraFlag::Bless), "bless"},
    {to_int(ObjectExtraFlag::AntiGood), "antigood"},
    {to_int(ObjectExtraFlag::AntiEvil), "antievil"},
    {to_int(ObjectExtraFlag::AntiNeutral), "antineutral"},
    {to_int(ObjectExtraFlag::NoRemove), "noremove"}, // Only weapons are meant to have this, it prevents disarm.
    {to_int(ObjectExtraFlag::Inventory), "inventory"},
    {to_int(ObjectExtraFlag::NoPurge), "nopurge"},
    {to_int(ObjectExtraFlag::RotDeath), "rotdeath"},
    {to_int(ObjectExtraFlag::VisDeath), "visdeath"},
    {to_int(ObjectExtraFlag::ProtectContainer), "protected"},
    {to_int(ObjectExtraFlag::NoLocate), "nolocate"},
    {to_int(ObjectExtraFlag::SummonCorpse), "summon_corpse"},
    {to_int(ObjectExtraFlag::Unique), "unique"},
    // clang-format on
}};

}