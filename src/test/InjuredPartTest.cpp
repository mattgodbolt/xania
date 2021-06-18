/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "InjuredPart.hpp"
#include "handler.hpp"
#include "lookup.h"
#include "merc.h"

#include <catch2/catch.hpp>

namespace {
void set_form(Char &ch, std::string_view race_name, const char *size) {
    ch.race = race_lookup(race_name);
    ch.size = size_lookup(size);
}
}

TEST_CASE("injured part creation") {
    Char ch{};
    set_form(ch, "human", "medium");
    Char victim{};
    set_form(victim, "human", "medium");
    AttackType atk_type = &attack_table[1]; // slice
    KnuthRng rng(0xdeadbaff); // a magic number that'll consistently select the body parts below.
    const auto left_bicep = InjuredPart{PART_ARMS, "left bicep"};
    const auto head = InjuredPart{PART_HEAD, "head"};

    SECTION("headbutt to the face") {
        atk_type = &skill_table[gsn_headbutt];

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(head == part);
    }
    SECTION("equal body size left bicep") {

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(left_bicep == part);
    }
    SECTION("greater body size left bicep") {
        set_form(ch, "human", "giant");

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(left_bicep == part);
    }
    SECTION("smaller body size left bicep") {
        set_form(ch, "human", "tiny");

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(left_bicep == part);
    }
    SECTION("exotic race with wings") {
        const auto wings = InjuredPart{PART_WINGS, "wings"};
        set_form(victim, "song bird", "medium");

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(wings == part);
    }
}