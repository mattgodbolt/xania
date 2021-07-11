/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "BodySize.hpp"

#include <catch2/catch.hpp>
#include <tuple>

TEST_CASE("body size try lookup") {
    SECTION("valid cases") {
        using std::make_tuple;
        BodySize expected;
        std::string_view name;
        std::tie(expected, name) = GENERATE(table<BodySize, std::string_view>({
            make_tuple(BodySize::Tiny, "tiny"),
            make_tuple(BodySize::Small, "small"),
            make_tuple(BodySize::Medium, "medium"),
            make_tuple(BodySize::Large, "large"),
            make_tuple(BodySize::Huge, "huge"),
            make_tuple(BodySize::Giant, "giant"),
        }));

        SECTION("ok") {
            const auto result = BodySizes::try_lookup(name);

            CHECK(result == expected);
        }
    }
    SECTION("substring valid") {
        const auto result = BodySizes::try_lookup("tin");

        CHECK(result == BodySize::Tiny);
    }
    SECTION("case insensitive valid") {
        const auto result = BodySizes::try_lookup("TINY");

        CHECK(result == BodySize::Tiny);
    }
    SECTION("invalid") {
        const auto result = BodySizes::try_lookup("tinyy");

        CHECK(!result);
    }
}
TEST_CASE("body size size diff") {
    SECTION("slightly bigger") {
        const auto result = BodySizes::size_diff(BodySize::Giant, BodySize::Huge);

        CHECK(result == 1);
    }
    SECTION("much bigger") {
        const auto result = BodySizes::size_diff(BodySize::Giant, BodySize::Large);

        CHECK(result == 2);
    }
    SECTION("slightly smaller") {
        const auto result = BodySizes::size_diff(BodySize::Tiny, BodySize::Small);

        CHECK(result == -1);
    }
    SECTION("much smaller") {
        const auto result = BodySizes::size_diff(BodySize::Tiny, BodySize::Medium);

        CHECK(result == -2);
    }
    SECTION("same ") {
        const auto result = BodySizes::size_diff(BodySize::Medium, BodySize::Medium);

        CHECK(result == 0);
    }
}
TEST_CASE("body size mob str bonus") {
    using std::make_tuple;
    BodySize size;
    sh_int expected;
    std::tie(size, expected) = GENERATE(table<BodySize, sh_int>({
        make_tuple(BodySize::Tiny, -2),
        make_tuple(BodySize::Small, -1),
        make_tuple(BodySize::Medium, 0),
        make_tuple(BodySize::Large, 1),
        make_tuple(BodySize::Huge, 2),
        make_tuple(BodySize::Giant, 3),
    }));
    SECTION("expected bonus") {
        const auto result = BodySizes::get_mob_str_bonus(size);

        CHECK(result == expected);
    }
}
TEST_CASE("body size mob con bonus") {
    using std::make_tuple;
    BodySize size;
    sh_int expected;
    std::tie(size, expected) = GENERATE(table<BodySize, sh_int>({
        make_tuple(BodySize::Tiny, -1),
        make_tuple(BodySize::Small, 0),
        make_tuple(BodySize::Medium, 0),
        make_tuple(BodySize::Large, 0),
        make_tuple(BodySize::Huge, 1),
        make_tuple(BodySize::Giant, 1),
    }));
    SECTION("expected bonus") {
        const auto result = BodySizes::get_mob_con_bonus(size);

        CHECK(result == expected);
    }
}