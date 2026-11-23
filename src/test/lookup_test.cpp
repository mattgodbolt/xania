#include "SkillTables.hpp"
#include "lookup.h"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

TEST_CASE("looks up skills") {
    auto check_spell = [](const char *x, std::string_view y) {
        INFO(fmt::format("'{}' should map to '{}'", x, y));
        auto sn = skill_lookup(x);
        REQUIRE(sn != -1);
        CHECK(skill_table[sn].name == y);
    };

    // Some common abbreviations.
    check_spell("word", "word of recall");
    check_spell("mag", "magic missile");
    check_spell("acid", "acid blast");
    check_spell("chill", "chill touch");
    check_spell("cure", "cure blindness"); // probably less than ideal, but existing behaviour
    check_spell("cure l", "cure light");
    check_spell("cure s", "cure serious");

    // A few important things used internally.
    check_spell("word of recall", "word of recall");
    check_spell("cure poison", "cure poison");
    check_spell("cure disease", "cure disease");
    check_spell("cure light", "cure light");
    check_spell("cure serious", "cure serious");

    // Check every skill maps back to its own name.
    for (auto &x : skill_table) {
        if (x.name) // Can be removed if/when we get rid of the nullptr end thing.
            check_spell(x.name, x.name);
    }
}

TEST_CASE("an empty skill name matches nothing") {
    // "practice ''" passes an empty view: this read name[0] past its end (debug builds aborted).
    CHECK(skill_lookup("") == -1);
    CHECK(skill_lookup(std::string_view{}) == -1);
    constexpr std::string_view quoted{"''"};
    CHECK(skill_lookup(quoted.substr(1, 0)) == -1); // an empty view into a longer string, as ArgParser makes it
    // A one-letter prefix still works.
    CHECK(skill_lookup("s") != -1);
}
