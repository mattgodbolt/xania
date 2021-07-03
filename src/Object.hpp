/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "AffectList.hpp"
#include "GenericList.hpp"
#include "Types.hpp"

#include <array>
#include <string>
#include <vector>

struct Char;
struct ExtraDescription;
struct ObjectIndex;
struct Room;

/*
 * One object.
 */
struct Object {
    GenericList<Object *> contains;
    Object *in_obj{};
    Char *carried_by{};
    std::vector<ExtraDescription> extra_descr;
    AffectList affected{};
    ObjectIndex *objIndex{};
    Room *in_room{};
    bool enchanted{};
    std::string owner;
    std::string name;
    std::string short_descr;
    std::string description;
    sh_int item_type{};
    unsigned int extra_flags{};
    int wear_flags{};
    std::string wear_string;
    int wear_loc{};
    sh_int weight{};
    int cost{};
    sh_int level{};
    sh_int condition{};
    sh_int material{};
    sh_int timer{};
    std::array<int, 5> value{};
    Room *destination{};
};