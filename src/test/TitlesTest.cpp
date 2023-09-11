/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2023 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Titles.hpp"
#include "Char.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

TEST_CASE("default title") {
    Char player{};
    using std::make_tuple;
    using namespace std::literals;
    sh_int level;
    sh_int class_num;
    Sex sex;
    std::string_view expected_title;
    std::tie(level, class_num, sex, expected_title) = GENERATE(table<sh_int, sh_int, Sex, std::string_view>({
        // clang-format off
        make_tuple(0, 0, Sex::male(), "Apprentice of Magic"sv),
        make_tuple(0, 0, Sex::female(), "Apprentice of Magic"sv),
        make_tuple(1, 0, Sex::male(), "Apprentice of Magic"sv),
        make_tuple(10, 0, Sex::male(), "Delver in Spells"sv),
        make_tuple(10, 0, Sex::female(), "Delveress in Spells"sv),
        make_tuple(100, 0, Sex::male(), "Implementor"sv),
        make_tuple(100, 0, Sex::female(), "Implementress"sv),
        make_tuple(101, 0, Sex::female(), "Implementress"sv),
        make_tuple(1, 1, Sex::male(), "Believer"sv),
        make_tuple(20, 1, Sex::male(), "Deacon"sv),
        make_tuple(20, 1, Sex::female(), "Deaconess"sv),
        make_tuple(100, 1, Sex::male(), "Implementor"sv),
        make_tuple(100, 1, Sex::female(), "Implementress"sv),
        make_tuple(1, 2, Sex::male(), "Swordpupil"sv),
        make_tuple(20, 2, Sex::male(), "Veteran"sv),
        make_tuple(20, 2, Sex::female(), "Veteran"sv),
        make_tuple(100, 2, Sex::male(), "Implementor"sv),
        make_tuple(100, 2, Sex::female(), "Implementress"sv),
        make_tuple(1, 3, Sex::male(), "Axeman"sv),
        make_tuple(20, 3, Sex::male(), "Cut-throat"sv),
        make_tuple(20, 3, Sex::female(), "Cut-throat"sv),
        make_tuple(100, 3, Sex::male(), "Implementor"sv),
        make_tuple(100, 3, Sex::female(), "Implementress"sv),
    }));
    // clang-format on
    SECTION("expected for class/level/sex combo") {
        player.sex = sex;
        player.level = level;
        player.class_num = class_num;
        CHECK(Titles::default_title(player) == expected_title);
    }
}
