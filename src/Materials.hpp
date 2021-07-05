/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Types.hpp"

#include <string_view>

// Material types. Used by objects and mobiles.
enum class Material {
    None = 0,
    Default = 1,
    Adamantite = 2,
    Iron = 3,
    Glass = 4,
    Bronze = 5,
    Cloth = 6,
    Wood = 7,
    Paper = 8,
    Steel = 9,
    Stone = 10,
    Food = 11,
    Silver = 12,
    Gold = 13,
    Leather = 14,
    Vellum = 15,
    China = 16,
    Clay = 17,
    Brass = 18,
    Bone = 19,
    Platinum = 20,
    Pearl = 21,
    Mithril = 22,
    Octarine = 23
};

// TODO: constants
#define LIQ_WATER 0
#define LIQ_MAX 17

struct liq_type {
    const char *liq_name;
    const char *liq_color;
    sh_int liq_affect[3];
};

extern const struct liq_type liq_table[];

struct materials_type {
    sh_int magical_resilience;
    const char *material_name;
};

extern const struct materials_type material_table[];

Material material_lookup(std::string_view name);
