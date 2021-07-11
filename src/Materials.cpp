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
constexpr std::array<struct liq_type, magic_enum::enum_count<Liquid>()> liq_table{{
    // clang-format off
    {Liquid::Water,            "water",            "clear",     {0, 1, 10}},
    {Liquid::Beer,             "beer",             "amber",     {3, 2, 5}},
    {Liquid::Wine,             "wine",             "rose",      {5, 2, 5}},
    {Liquid::Ale,              "ale",              "brown",     {2, 2, 5}},
    {Liquid::DarkAle,          "dark ale",         "dark",      {1, 2, 5}},
    {Liquid::Whisky,           "whisky",           "golden",    {6, 1, 4}},
    {Liquid::Lemonade,         "lemonade",         "pink",      {0, 1, 8}},
    {Liquid::Firebreather,     "firebreather",     "boiling",   {10, 0, 0}},
    {Liquid::LocalSpecialty,   "local specialty",  "everclear", {3, 3, 3}},
    {Liquid::SlimeMoldJuice,   "slime mold juice", "green",     {0, 4, -8}},
    {Liquid::Milk,             "milk",             "white",     {0, 3, 6}},
    {Liquid::Tea,              "tea",              "tan",       {0, 1, 6}},
    {Liquid::Coffee,           "coffee",           "black",     {0, 1, 6}},
    {Liquid::Blood,            "blood",            "red",       {0, 2, -1}},
    {Liquid::SaltWater,        "salt water",       "clear",     {0, 1, -2}},
    {Liquid::Cola,             "cola",             "cherry",    {0, 1, 5}},
    {Liquid::RedWine,          "red wine",         "red",       {5, 2, 5}}
    // clang-format on
}};

std::optional<Liquid> Liquids::try_lookup(std::string_view name) {
    for (const auto &liquid : liq_table) {
        if (is_name(liquid.liq_name, name))
            return liquid.liquid;
    }
    return std::nullopt;
}

const liq_type *Liquids::get_liq_type(const int index) {
    if (index < 0 || static_cast<size_t>(index) >= liq_table.size()) {
        // In theory this should never happen, at least w.r.t. liquids defined in area files as they
        // are validated on startup. However, imms can use the 'set obj' command to customize object attributes
        // and there's insufficient input validation there.
        return nullptr;
    } else {
        return &liq_table[index];
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