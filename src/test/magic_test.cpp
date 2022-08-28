/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "lookup.h"
#include "magic.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <fmt/format.h>
#include <tuple>

std::pair<std::string, std::string> casting_messages(const int sn);

TEST_CASE("say spell > casting messages") {
    using std::make_tuple;
    std::string_view spell;
    std::string_view to_other_class;
    SECTION("misc spells") {
        std::tie(spell, to_other_class) = GENERATE(table<std::string_view, std::string_view>(
            {make_tuple("harm", "pabraw"), make_tuple("cure poison", "judicandus sausabru"),
             make_tuple("cure blindness", "judicandus noselacri"), make_tuple("frenzy", "ycandusikl"),
             make_tuple("armor", "abrazak"), make_tuple("protection good", "sfainfrauai oaae")}));
        SECTION("expected messages") {
            auto messages = casting_messages(skill_lookup(spell));

            CHECK(messages.first == fmt::format("|y$n utters the words '{}'.|w", spell));
            CHECK(messages.second == fmt::format("|y$n utters the words '{}'.|w", to_other_class));
        }
    }
}
