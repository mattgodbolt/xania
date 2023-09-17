/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2023 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Titles.hpp"
#include "Char.hpp"
#include "Class.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

TEST_CASE("default title") {
    Char player{};
    using std::make_tuple;
    using namespace std::literals;
    sh_int level;
    Class const *class_type;
    Sex sex;
    std::string_view expected_title;
    std::tie(level, class_type, sex, expected_title) = GENERATE(table<sh_int, Class const *, Sex, std::string_view>({
        // clang-format off
        make_tuple(0, Class::mage(), Sex::male(), "Apprentice of Magic"sv),
        make_tuple(0, Class::mage(), Sex::female(), "Apprentice of Magic"sv),
        make_tuple(1, Class::mage(), Sex::male(), "Apprentice of Magic"sv),
        make_tuple(10, Class::mage(), Sex::male(), "Delver in Spells"sv),
        make_tuple(10, Class::mage(), Sex::female(), "Delveress in Spells"sv),
        make_tuple(100, Class::mage(), Sex::male(), "Implementor"sv),
        make_tuple(100, Class::mage(), Sex::female(), "Implementress"sv),
        make_tuple(101, Class::mage(), Sex::female(), "Implementress"sv),
        make_tuple(1, Class::cleric(), Sex::male(), "Believer"sv),
        make_tuple(20, Class::cleric(), Sex::male(), "Deacon"sv),
        make_tuple(20, Class::cleric(), Sex::female(), "Deaconess"sv),
        make_tuple(100, Class::cleric(), Sex::male(), "Implementor"sv),
        make_tuple(100, Class::cleric(), Sex::female(), "Implementress"sv),
        make_tuple(1, Class::knight(), Sex::male(), "Swordpupil"sv),
        make_tuple(20, Class::knight(), Sex::male(), "Veteran"sv),
        make_tuple(20, Class::knight(), Sex::female(), "Veteran"sv),
        make_tuple(100, Class::knight(), Sex::male(), "Implementor"sv),
        make_tuple(100, Class::knight(), Sex::female(), "Implementress"sv),
        make_tuple(1, Class::barbarian(), Sex::male(), "Axeman"sv),
        make_tuple(20, Class::barbarian(), Sex::male(), "Cut-throat"sv),
        make_tuple(20, Class::barbarian(), Sex::female(), "Cut-throat"sv),
        make_tuple(100, Class::barbarian(), Sex::male(), "Implementor"sv),
        make_tuple(100, Class::barbarian(), Sex::female(), "Implementress"sv),
    }));
    // clang-format on
    SECTION("expected for class/level/sex combo") {
        player.sex = sex;
        player.level = level;
        player.class_type = class_type;
        CHECK(Titles::default_title(player) == expected_title);
    }
}
