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
        if (is_name(name, liquid.short_name))
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

constexpr std::array<struct Material, magic_enum::enum_count<Material::Type>()> Materials{{
    // clang-format off
    {Material::Type::None, "none", 0},
    {Material::Type::Default, "default", 40},
    {Material::Type::Adamantite, "adamantite", 90},
    {Material::Type::Iron, "iron", 70},
    {Material::Type::Glass, "glass", 15},
    {Material::Type::Bronze, "bronze", 71},
    {Material::Type::Cloth, "cloth", 30},
    {Material::Type::Wood, "wood", 35},
    {Material::Type::Paper, "paper", 10},
    {Material::Type::Steel, "steel", 75},
    {Material::Type::Stone, "stone", 85},
    {Material::Type::Food, "food", 0},
    {Material::Type::Silver, "silver", 55},
    {Material::Type::Gold, "gold", 55},
    {Material::Type::Leather, "leather", 30},
    {Material::Type::Vellum, "vellum", 20},
    {Material::Type::China, "china", 5},
    {Material::Type::Clay, "clay", 10},
    {Material::Type::Brass, "brass", 75},
    {Material::Type::Bone, "bone", 45},
    {Material::Type::Platinum, "platinum", 82},
    {Material::Type::Pearl, "pearl", 40},
    {Material::Type::Mithril, "mithril", 65},
    {Material::Type::Octarine, "octarine", 100}
    // clang-format on
}};

const Material *Material::lookup_with_default(std::string_view name) {
    for (const auto &material : Materials) {
        if (is_name(name, material.material_name))
            return &material;
    }
    return &Materials[magic_enum::enum_integer<Material::Type>(Material::Type::Default)];
}

sh_int Material::get_magical_resilience(const Material::Type type) {
    return Materials[magic_enum::enum_integer<Material::Type>(type)].magical_resilience;
}