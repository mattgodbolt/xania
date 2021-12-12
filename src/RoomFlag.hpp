/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
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

namespace RoomFlags {

using RF = Flag<RoomFlag>;
constexpr auto Dark = RF{RoomFlag::Dark, "dark"};
constexpr auto NoMob = RF{RoomFlag::NoMob, "nomob"};
constexpr auto Indoors = RF{RoomFlag::Indoors, "indoors"};
constexpr auto Private = RF{RoomFlag::Private, "private"};
constexpr auto Safe = RF{RoomFlag::Safe, "safe"};
constexpr auto Solitary = RF{RoomFlag::Solitary, "solitary"};
constexpr auto PetShop = RF{RoomFlag::PetShop, "petshop"};
constexpr auto NoRecall = RF{RoomFlag::NoRecall, "recall"};
constexpr auto ImpOnly = RF{RoomFlag::ImpOnly, "imponly", MAX_LEVEL};
constexpr auto GodsOnly = RF{RoomFlag::GodsOnly, "godonly", LEVEL_IMMORTAL};
constexpr auto HeroesOnly = RF{RoomFlag::HeroesOnly, "heroonly"};
constexpr auto NewbiesOnly = RF{RoomFlag::NewbiesOnly, "newbieonly"};
constexpr auto Law = RF{RoomFlag::Law, "law"};

constexpr auto NumRoomFlags = 13;
constexpr std::array<RF, NumRoomFlags> AllRoomFlags = {{Dark, NoMob, Indoors, Private, Safe, Solitary, PetShop,
                                                        NoRecall, ImpOnly, GodsOnly, HeroesOnly, NewbiesOnly, Law}};

}