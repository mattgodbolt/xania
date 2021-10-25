/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

enum class RoomFlag : unsigned int {
    Dark = A,
    NoMob = C,
    Indoors = D,
    Private = J,
    Safe = K,
    Solitary = L,
    PetShop = M,
    NoRecall = N,
    ImpOnly = O,
    GodsOnly = P,
    HeroesOnly = Q,
    NewbiesOnly = R,
    Law = S
};

[[nodiscard]] constexpr auto to_int(const RoomFlag flag) noexcept { return magic_enum::enum_integer<RoomFlag>(flag); }
