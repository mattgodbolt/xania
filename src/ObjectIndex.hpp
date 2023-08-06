/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "AffectList.hpp"
#include "ExtraDescription.hpp"
#include "Materials.hpp"
#include "Types.hpp"
#include <array>
#include <string>
#include <vector>

enum class ObjectType;
class Area;

/*
 * Prototype for an object.
 */
struct ObjectIndex {
    std::vector<ExtraDescription> extra_descr{};
    AffectList affected{};
    std::string name{};
    std::string short_descr{};
    std::string description{};
    sh_int vnum{};
    sh_int reset_num{};
    Material::Type material{};
    ObjectType type{};
    unsigned int extra_flags{};
    unsigned int wear_flags{};
    std::string wear_string{};
    sh_int level{};
    sh_int condition{};
    sh_int count{};
    sh_int weight{};
    int cost{};
    std::array<int, 5> value{};
    Area *area{};

    [[nodiscard]] std::string type_name() const;

    // A subset of the object extra attribute checkers available on Object.
    [[nodiscard]] bool is_no_remove() const;
};
