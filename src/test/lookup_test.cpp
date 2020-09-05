#include "lookup.h"
#include "merc.h"

#include <catch2/catch.hpp>

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
}