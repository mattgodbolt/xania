/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

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
