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

constexpr auto NumRoomFlags = 13;
constexpr std::array<Flag<RoomFlag>, NumRoomFlags> AllRoomFlags = {{
    // clang-format off
    {RoomFlag::Dark, "dark"},
    {RoomFlag::NoMob, "nomob"},
    {RoomFlag::Indoors, "indoors"},
    {RoomFlag::Private, "private"},
    {RoomFlag::Safe, "safe"},
    {RoomFlag::Solitary, "solitary"},
    {RoomFlag::PetShop, "petshop"},
    {RoomFlag::NoRecall, "recall"},
    {RoomFlag::ImpOnly, "imponly", MAX_LEVEL},
    {RoomFlag::GodsOnly, "godonly", LEVEL_IMMORTAL},
    {RoomFlag::HeroesOnly, "heroonly"},
    {RoomFlag::NewbiesOnly, "newbieonly"},
    {RoomFlag::Law, "law"},
    // clang-format on
}};

}