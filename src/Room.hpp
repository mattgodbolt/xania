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
#include "GenericList.hpp"
#include "ResetData.hpp"
#include "RoomFlag.hpp"
#include "SectorType.hpp"
#include "Types.hpp"

#include <array>
#include <vector>

struct Object;
struct Area;

/*
 * Room type.
 */
struct Room {
    GenericList<Char *> people;
    GenericList<Object *> contents;
    std::vector<ExtraDescription> extra_descr{};
    Area *area{};
    PerDirection<std::optional<Exit>> exits{};
    std::string name{};
    std::string description{};
    sh_int vnum{};
    unsigned int room_flags{};
    sh_int light{};
    SectorType sector_type{SectorType::Inside};

    std::vector<ResetData> resets{};

    [[nodiscard]] bool is_outside() const;
    [[nodiscard]] bool is_inside() const;
};
