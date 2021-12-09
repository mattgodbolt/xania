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
constexpr std::array<Flag, NumRoomFlags> AllRoomFlags = {{
    // clang-format off
    {to_int(RoomFlag::Dark), "dark"},
    {to_int(RoomFlag::NoMob), "nomob"},
    {to_int(RoomFlag::Indoors), "indoors"},
    {to_int(RoomFlag::Private), "private"},
    {to_int(RoomFlag::Safe), "safe"},
    {to_int(RoomFlag::Solitary), "solitary"},
    {to_int(RoomFlag::PetShop), "petshop"},
    {to_int(RoomFlag::NoRecall), "recall"},
    {to_int(RoomFlag::ImpOnly), "imponly", MAX_LEVEL},
    {to_int(RoomFlag::GodsOnly), "godonly", LEVEL_IMMORTAL},
    {to_int(RoomFlag::HeroesOnly), "heroonly"},
    {to_int(RoomFlag::NewbiesOnly), "newbieonly"},
    {to_int(RoomFlag::Law), "law"},
    // clang-format on
}};

}