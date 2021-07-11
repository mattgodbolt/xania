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
        Liquid expected;
        std::string_view name;
        std::tie(expected, name) = GENERATE(table<Liquid, std::string_view>({
            make_tuple(Liquid::Water, "water"),
            make_tuple(Liquid::Beer, "beer"),
            make_tuple(Liquid::Wine, "wine"),
            make_tuple(Liquid::Ale, "ale"),
            make_tuple(Liquid::DarkAle, "'dark ale'"),
            make_tuple(Liquid::Whisky, "whisky"),
            make_tuple(Liquid::Lemonade, "lemonade"),
            make_tuple(Liquid::Firebreather, "firebreather"),
            make_tuple(Liquid::LocalSpecialty, "'local specialty'"),
            make_tuple(Liquid::SlimeMoldJuice, "'slime mold juice'"),
            make_tuple(Liquid::Milk, "milk"),
            make_tuple(Liquid::Tea, "tea"),
            make_tuple(Liquid::Coffee, "coffee"),
            make_tuple(Liquid::Blood, "blood"),
            make_tuple(Liquid::SaltWater, "'salt water'"),
            make_tuple(Liquid::Cola, "cola"),
            make_tuple(Liquid::RedWine, "'red wine'"),
        }));

        SECTION("ok") {
            const auto result = Liquids::try_lookup(name);

            CHECK(result == expected);
        }
    }
    SECTION("invalid") {
        const auto result = Liquids::try_lookup("redbull");

        CHECK(!result);
    }
}

TEST_CASE("get liq type") {
    SECTION("water") {
        const auto result = Liquids::get_liq_type(0);

        CHECK(result);
        CHECK(result->liq_name == "water");
        CHECK(result->liq_color == "clear");
    }
    SECTION("red wine") {
        const auto result = Liquids::get_liq_type(16);

        CHECK(result);
        CHECK(result->liq_name == "red wine");
        CHECK(result->liq_color == "red");
    }
    SECTION("out of range") {
        const auto result = Liquids::get_liq_type(17);

        CHECK(!result);
    }
    SECTION("negative") {
        const auto result = Liquids::get_liq_type(-1);

        CHECK(!result);
    }
}