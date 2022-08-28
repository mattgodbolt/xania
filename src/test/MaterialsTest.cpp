/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Materials.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

TEST_CASE("liquid try lookup") {
    SECTION("valid cases match by full name") {
        using std::make_tuple;
        Liquid::Type expected;
        std::string_view name;
        std::tie(expected, name) = GENERATE(table<Liquid::Type, std::string_view>({
            make_tuple(Liquid::Type::Water, "water"),
            make_tuple(Liquid::Type::Beer, "beer"),
            make_tuple(Liquid::Type::Wine, "wine"),
            make_tuple(Liquid::Type::Ale, "ale"),
            make_tuple(Liquid::Type::DarkAle, "dark ale"),
            make_tuple(Liquid::Type::Whisky, "whisky"),
            make_tuple(Liquid::Type::Lemonade, "lemonade"),
            make_tuple(Liquid::Type::Firebreather, "firebreather"),
            make_tuple(Liquid::Type::LocalSpecialty, "local specialty"),
            make_tuple(Liquid::Type::SlimeMoldJuice, "slime mold juice"),
            make_tuple(Liquid::Type::Milk, "milk"),
            make_tuple(Liquid::Type::Tea, "tea"),
            make_tuple(Liquid::Type::Coffee, "coffee"),
            make_tuple(Liquid::Type::Blood, "blood"),
            make_tuple(Liquid::Type::SaltWater, "salt water"),
            make_tuple(Liquid::Type::Cola, "cola"),
            make_tuple(Liquid::Type::RedWine, "red wine"),
        }));

        SECTION("ok") {
            const auto *result = Liquid::try_lookup(name);

            CHECK(result == &Liquids[magic_enum::enum_integer<Liquid::Type>(expected)]);
        }
    }
    SECTION("partial is_name matching") {
        const auto *result = Liquid::try_lookup("s m j");

        CHECK(result == &Liquids[magic_enum::enum_integer<Liquid::Type>(Liquid::Type::SlimeMoldJuice)]);
    }
    SECTION("invalid") {
        const auto *result = Liquid::try_lookup("redbull");

        CHECK(!result);
    }
}

TEST_CASE("liquid get by index") {
    SECTION("water") {
        const auto *result = Liquid::get_by_index(0);

        CHECK(result);
        CHECK(result->name == "water");
        CHECK(result->color == "clear");
    }
    SECTION("red wine") {
        const auto *result = Liquid::get_by_index(16);

        CHECK(result);
        CHECK(result->name == "red wine");
        CHECK(result->color == "red");
    }
    SECTION("out of range") {
        const auto *result = Liquid::get_by_index(17);

        CHECK(!result);
    }
    SECTION("negative") {
        const auto *result = Liquid::get_by_index(-1);

        CHECK(!result);
    }
}

TEST_CASE("material lookup with default") {
    SECTION("valid cases match by full name") {
        using std::make_tuple;
        Material::Type expected;
        std::string_view name;
        std::tie(expected, name) = GENERATE(table<Material::Type, std::string_view>({
            // clang-format off
            make_tuple(Material::Type::None, "none"),
            make_tuple(Material::Type::Default, "default"),
            make_tuple(Material::Type::Adamantite, "adamantite"),
            make_tuple(Material::Type::Iron, "iron"),
            make_tuple(Material::Type::Glass, "glass"),
            make_tuple(Material::Type::Bronze, "bronze"),
            make_tuple(Material::Type::Cloth, "cloth"),
            make_tuple(Material::Type::Wood, "wood"),
            make_tuple(Material::Type::Paper, "paper"),
            make_tuple(Material::Type::Steel, "steel"),
            make_tuple(Material::Type::Stone, "stone"),
            make_tuple(Material::Type::Food, "food"),
            make_tuple(Material::Type::Silver, "silver"),
            make_tuple(Material::Type::Gold, "gold"),
            make_tuple(Material::Type::Leather, "leather"),
            make_tuple(Material::Type::Vellum, "vellum"),
            make_tuple(Material::Type::China, "china"),
            make_tuple(Material::Type::Clay, "clay"),
            make_tuple(Material::Type::Brass, "brass"),
            make_tuple(Material::Type::Bone, "bone"),
            make_tuple(Material::Type::Platinum, "platinum"),
            make_tuple(Material::Type::Pearl, "pearl"),
            make_tuple(Material::Type::Mithril, "mithril"),
            make_tuple(Material::Type::Octarine, "octarine")
            // clang-format on
        }));

        SECTION("ok") {
            const auto *result = Material::lookup_with_default(name);

            CHECK(result == &Materials[magic_enum::enum_integer<Material::Type>(expected)]);
        }
    }
    SECTION("invalid uses default") {
        const auto *result = Material::lookup_with_default("cotton");

        CHECK(result == &Materials[magic_enum::enum_integer<Material::Type>(Material::Type::Default)]);
    }
}

TEST_CASE("material get magical resilience") {
    SECTION("none") {
        const auto result = Material::get_magical_resilience(Material::Type::None);

        CHECK(result == 0);
    }
    SECTION("default") {
        const auto result = Material::get_magical_resilience(Material::Type::Default);

        CHECK(result == 40);
    }
    SECTION("octarine") {
        const auto result = Material::get_magical_resilience(Material::Type::Octarine);

        CHECK(result == 100);
    }
}
