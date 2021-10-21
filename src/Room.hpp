/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "BitsRoomState.hpp"
#include "Constants.hpp"
#include "Direction.hpp"
#include "ExtraDescription.hpp"
#include "Flag.hpp"
#include "GenericList.hpp"
#include "SectorType.hpp"
#include "Types.hpp"

#include <array>
#include <vector>

struct Object;
struct Area;
struct Exit;
struct Flag;
struct ResetData;

/*
 * Room type.
 */
struct Room {
    Room *next{};
    GenericList<Char *> people;
    GenericList<Object *> contents;
    std::vector<ExtraDescription> extra_descr{};
    Area *area{};
    PerDirection<Exit *> exit{};
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

    inline static constexpr std::array<Flag, 13> AllStateFlags = {{
        // clang-format off
        {ROOM_DARK, 0, "dark"},
        {ROOM_NO_MOB, 0, "nomob"},
        {ROOM_INDOORS, 0, "indoors"},
        {ROOM_PRIVATE, 0, "private"},
        {ROOM_SAFE, 0, "safe"},
        {ROOM_SOLITARY, 0, "solitary"},
        {ROOM_PET_SHOP, 0, "petshop"},
        {ROOM_NO_RECALL, 0, "recall"},
        {ROOM_IMP_ONLY, MAX_LEVEL, "imponly"},
        {ROOM_GODS_ONLY, LEVEL_IMMORTAL, "godonly"},
        {ROOM_HEROES_ONLY, 0, "heronly"},
        {ROOM_NEWBIES_ONLY, 0, "newbieonly"},
        {ROOM_LAW, 0, "law"},
        // clang-format on
    }};
};
