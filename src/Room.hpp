/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Constants.hpp"
#include "Direction.hpp"
#include "Exit.hpp"
#include "ExtraDescription.hpp"
#include "Flag.hpp"
#include "GenericList.hpp"
#include "RoomFlag.hpp"
#include "SectorType.hpp"
#include "Types.hpp"

#include <array>
#include <vector>

struct Object;
struct Area;
struct Flag;
struct ResetData;

/*
 * Room type.
 */
struct Room {
    GenericList<Char *> people;
    GenericList<Object *> contents;
    std::vector<ExtraDescription> extra_descr{};
    Area *area{};
    PerDirection<std::optional<Exit>> exit{};
    char *name{};
    char *description{};
    sh_int vnum{};
    unsigned int room_flags{};
    sh_int light{};
    SectorType sector_type{SectorType::Inside};

    ResetData *reset_first{};
    ResetData *reset_last{};

    [[nodiscard]] bool is_outside() const;
    [[nodiscard]] bool is_inside() const;

    inline static constexpr std::array<Flag, 13> AllFlags = {{
        // clang-format off
        {to_int(RoomFlag::Dark), 0, "dark"},
        {to_int(RoomFlag::NoMob), 0, "nomob"},
        {to_int(RoomFlag::Indoors), 0, "indoors"},
        {to_int(RoomFlag::Private), 0, "private"},
        {to_int(RoomFlag::Safe), 0, "safe"},
        {to_int(RoomFlag::Solitary), 0, "solitary"},
        {to_int(RoomFlag::PetShop), 0, "petshop"},
        {to_int(RoomFlag::NoRecall), 0, "recall"},
        {to_int(RoomFlag::ImpOnly), MAX_LEVEL, "imponly"},
        {to_int(RoomFlag::GodsOnly), LEVEL_IMMORTAL, "godonly"},
        {to_int(RoomFlag::HeroesOnly), 0, "heronly"},
        {to_int(RoomFlag::NewbiesOnly), 0, "newbieonly"},
        {to_int(RoomFlag::Law), 0, "law"},
        // clang-format on
    }};
};
