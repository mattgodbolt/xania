/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Direction.hpp"
#include "GenericList.hpp"
#include "SectorType.hpp"
#include "Types.hpp"

struct Object;
struct ExtraDescription;
struct AREA_DATA;
struct Exit;
struct ResetData;

/*
 * Room type.
 */
struct Room {
    Room *next{};
    GenericList<Char *> people;
    GenericList<Object *> contents;
    std::vector<ExtraDescription> extra_descr{};
    AREA_DATA *area{};
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
};
