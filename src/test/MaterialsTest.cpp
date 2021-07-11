/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Materials.hpp"

#include <catch2/catch.hpp>
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
            make_tuple(Liquid::Type::DarkAle, "'dark ale'"),
            make_tuple(Liquid::Type::Whisky, "whisky"),
            make_tuple(Liquid::Type::Lemonade, "lemonade"),
            make_tuple(Liquid::Type::Firebreather, "firebreather"),
            make_tuple(Liquid::Type::LocalSpecialty, "'local specialty'"),
            make_tuple(Liquid::Type::SlimeMoldJuice, "'slime mold juice'"),
            make_tuple(Liquid::Type::Milk, "milk"),
            make_tuple(Liquid::Type::Tea, "tea"),
            make_tuple(Liquid::Type::Coffee, "coffee"),
            make_tuple(Liquid::Type::Blood, "blood"),
            make_tuple(Liquid::Type::SaltWater, "'salt water'"),
            make_tuple(Liquid::Type::Cola, "cola"),
            make_tuple(Liquid::Type::RedWine, "'red wine'"),
        }));

        SECTION("ok") {
            const auto *result = Liquid::try_lookup(name);

            CHECK(result == &Liquids[magic_enum::enum_integer<Liquid::Type>(expected)]);
        }
    }
    SECTION("invalid") {
        const auto *result = Liquid::try_lookup("redbull");

        CHECK(!result);
    }
}

TEST_CASE("get liq type") {
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