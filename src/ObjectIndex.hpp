/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "AffectList.hpp"
#include "Types.hpp"
#include <array>
#include <string>
#include <vector>

struct AREA_DATA;
struct ExtraDescription;
/*
 * Prototype for an object.
 */
struct ObjectIndex {
    ObjectIndex *next{};
    std::vector<ExtraDescription> extra_descr;
    AffectList affected{};
    std::string name;
    std::string short_descr;
    std::string description;
    sh_int vnum{};
    sh_int reset_num{};
    sh_int material{};
    sh_int item_type{};
    unsigned int extra_flags{};
    int wear_flags{};
    std::string wear_string;
    sh_int level{};
    sh_int condition{};
    sh_int count{};
    sh_int weight{};
    int cost{};
    std::array<int, 5> value{};

    AREA_DATA *area{};
};
