/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Types.hpp"

#include <array>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

struct Liquid {
    enum class Type {
        Water = 0,
        Beer = 1,
        Wine = 2,
        Ale = 3,
        DarkAle = 4,
        Whisky = 5,
        Lemonade = 6,
        Firebreather = 7,
        LocalSpecialty = 8,
        SlimeMoldJuice = 9,
        Milk = 10,
        Tea = 11,
        Coffee = 12,
        Blood = 13,
        SaltWater = 14,
        Cola = 15,
        RedWine = 16
    };
    const Type liquid;
    std::string_view name;
    std::string_view color;
    std::array<sh_int, 3> affect;

    [[nodiscard]] static const Liquid *try_lookup(std::string_view name);
    [[nodiscard]] static const Liquid *get_by_index(const int index);
};

extern const std::array<struct Liquid, magic_enum::enum_count<Liquid::Type>()> Liquids;

struct Material {
    // Used by objects and mobiles.
    enum class Type {
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
    const Material::Type material;
    std::string_view material_name;
    const sh_int magical_resilience;

    [[nodiscard]] static const Material *lookup_with_default(std::string_view name);
    [[nodiscard]] static sh_int get_magical_resilience(const Material::Type type);
};

extern const std::array<struct Material, magic_enum::enum_count<Material::Type>()> Materials;
