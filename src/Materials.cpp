/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Materials.hpp"
#include "string_utils.hpp"

/*
 * Liquid properties.
 */
constexpr std::array<struct Liquid, magic_enum::enum_count<Liquid::Type>()> Liquids{{
    // clang-format off
    {Liquid::Type::Water,            "water",            "clear",     {0, 1, 10}},
    {Liquid::Type::Beer,             "beer",             "amber",     {3, 2, 5}},
    {Liquid::Type::Wine,             "wine",             "rose",      {5, 2, 5}},
    {Liquid::Type::Ale,              "ale",              "brown",     {2, 2, 5}},
    {Liquid::Type::DarkAle,          "dark ale",         "dark",      {1, 2, 5}},
    {Liquid::Type::Whisky,           "whisky",           "golden",    {6, 1, 4}},
    {Liquid::Type::Lemonade,         "lemonade",         "pink",      {0, 1, 8}},
    {Liquid::Type::Firebreather,     "firebreather",     "boiling",   {10, 0, 0}},
    {Liquid::Type::LocalSpecialty,   "local specialty",  "everclear", {3, 3, 3}},
    {Liquid::Type::SlimeMoldJuice,   "slime mold juice", "green",     {0, 4, -8}},
    {Liquid::Type::Milk,             "milk",             "white",     {0, 3, 6}},
    {Liquid::Type::Tea,              "tea",              "tan",       {0, 1, 6}},
    {Liquid::Type::Coffee,           "coffee",           "black",     {0, 1, 6}},
    {Liquid::Type::Blood,            "blood",            "red",       {0, 2, -1}},
    {Liquid::Type::SaltWater,        "salt water",       "clear",     {0, 1, -2}},
    {Liquid::Type::Cola,             "cola",             "cherry",    {0, 1, 5}},
    {Liquid::Type::RedWine,          "red wine",         "red",       {5, 2, 5}}
    // clang-format on
}};

const Liquid *Liquid::try_lookup(std::string_view name) {
    for (const auto &liquid : Liquids) {
        if (is_name(liquid.name, name))
            return &liquid;
    }
    return nullptr;
}

const Liquid *Liquid::get_by_index(const int index) {
    if (index < 0 || static_cast<size_t>(index) >= Liquids.size()) {
        // In theory this should never happen, at least w.r.t. liquids defined in area files as they
        // are validated on startup. However, imms can use the 'set obj' command to customize object attributes
        // and there's insufficient input validation there.
        return nullptr;
    } else {
        return &Liquids[index];
    }
}

const struct materials_type material_table[] = {
    /* { percentage resilience, name } */
    {0, "none"},      {40, "default"}, {90, "adamantite"}, {70, "iron"},      {15, "glass"},
    {71, "bronze"},   {30, "cloth"},   {35, "wood"},       {10, "paper"},     {75, "steel"},
    {85, "stone"},    {0, "food"},     {55, "silver"},     {55, "gold"},      {30, "leather"},
    {20, "vellum"},   {5, "china"},    {10, "clay"},       {75, "brass"},     {45, "bone"},
    {82, "platinum"}, {40, "pearl"},   {65, "mithril"},    {100, "octarine"}, {0, nullptr}};

Material material_lookup(std::string_view name) {
    for (auto count = 0; material_table[count].material_name; count++) {
        if (is_name(material_table[count].material_name, name))
            return magic_enum::enum_value<Material>(count);
    }
    return Material::Default;
}