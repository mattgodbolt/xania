/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "InjuredPart.hpp"
#include "BodySize.hpp"
#include "Char.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "handler.hpp"

#include <catch2/catch_test_macros.hpp>

#include "MockMud.hpp"

namespace {
void set_form(Char &ch, std::string_view race_name, const BodySize body_size) {
    ch.race = race_lookup(race_name);
    ch.body_size = body_size;
}

test::MockMud mock_mud{};

}

TEST_CASE("injured part creation") {
    Char ch{mock_mud};
    set_form(ch, "human", BodySize::Medium);
    Char victim{mock_mud};
    set_form(victim, "human", BodySize::Medium);
    AttackType atk_type = Attacks::at(1); // slice
    KnuthRng rng(0xdeadbaff); // a magic number that'll consistently select the body parts below.
    const auto right_bicep = InjuredPart{"right bicep", "$n's arm is sliced from $s dead body.", 14};
    const auto head = InjuredPart{"head", "$n's severed head plops on the ground.", 12};

    SECTION("headbutt to the face") {
        atk_type = &skill_table[gsn_headbutt];

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(head == part);
    }
    SECTION("equal body size right bicep") {

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(right_bicep == part);
    }
    SECTION("greater body size right bicep") {
        set_form(ch, "human", BodySize::Giant);

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(right_bicep == part);
    }
    SECTION("smaller body size right bicep") {
        set_form(ch, "human", BodySize::Tiny);

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(right_bicep == part);
    }
    SECTION("exotic race with wings") {
        const auto wings = InjuredPart{"wings", "$n's wing is sliced off and lands with a crunch.", 18};
        set_form(victim, "song bird", BodySize::Medium);

        const auto part = InjuredPart::random_from_victim(&ch, &victim, atk_type, rng);

        CHECK(wings == part);
    }
}
