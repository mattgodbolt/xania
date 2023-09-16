#include "Learning.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "common/BitOps.hpp"
#include "lookup.h"
#include <catch2/catch_test_macros.hpp>

#include "MockRng.hpp"

TEST_CASE("learning") {
    Char bob{};
    bob.level = 1;
    const auto dagger = skill_lookup("dagger");
    test::MockRng rng;
    SECTION("pc") {
        using trompeloeil::_;
        Descriptor bob_desc(0);
        bob.desc = &bob_desc;
        bob_desc.character(&bob);
        bob.pcdata = std::make_unique<PcData>();
        SECTION("can't learn unknown skill") {
            bob.pcdata->learned[dagger] = 0;
            FORBID_CALL(rng, number_range(_, _));
            FORBID_CALL(rng, number_percent());

            const auto learning = Learning(&bob, dagger, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
        SECTION("can't learn skill requiring greater level") {
            const auto acid_blast = skill_lookup("acid blast");
            FORBID_CALL(rng, number_range(_, _));
            FORBID_CALL(rng, number_percent());

            const auto learning = Learning(&bob, acid_blast, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
        SECTION("can't learn skill maxed out") {
            bob.pcdata->learned[dagger] = 100;
            FORBID_CALL(rng, number_range(_, _));
            FORBID_CALL(rng, number_percent());

            const auto learning = Learning(&bob, dagger, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
        SECTION("didn't learn too difficult") {
            bob.pcdata->learned[dagger] = 1;
            REQUIRE_CALL(rng, number_range(_, _)).RETURN(1);
            FORBID_CALL(rng, number_percent());

            const auto learning = Learning(&bob, dagger, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
        SECTION("didn't learn despite success") {
            bob.pcdata->learned[dagger] = 95;
            REQUIRE_CALL(rng, number_range(1, 1000)).RETURN(1000);
            REQUIRE_CALL(rng, number_percent()).RETURN(5); // Would require a roll of 4 to learn.

            const auto learning = Learning(&bob, dagger, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
        SECTION("didn't learn despite failure") {
            bob.pcdata->learned[dagger] = 95;
            REQUIRE_CALL(rng, number_range(1, 1000)).RETURN(1000);
            REQUIRE_CALL(rng, number_percent()).RETURN(30); // Would require a roll of 29 to learn.

            const auto learning = Learning(&bob, dagger, false, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
        SECTION("learned from success") {
            bob.pcdata->learned[dagger] = 95;
            REQUIRE_CALL(rng, number_range(1, 1000)).RETURN(1000);
            REQUIRE_CALL(rng, number_percent()).RETURN(4);

            const auto learning = Learning(&bob, dagger, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == true);
            const auto points = learning.gainable_points();
            CHECK(points == 1);
            bob.apply_skill_points(dagger, points);
            CHECK(learning.learning_message() == "|WYou have become better at |Cdagger|W! (96)|w\n\r");
            CHECK(learning.experience_reward() == 4);
        }
        SECTION("learned from failure") {
            const auto expected_points = 3;
            bob.pcdata->learned[dagger] = 95;
            REQUIRE_CALL(rng, number_range(1, 1000)).RETURN(1000);
            REQUIRE_CALL(rng, number_percent()).RETURN(29);
            REQUIRE_CALL(rng, number_range(1, 3)).RETURN(expected_points);

            const auto learning = Learning(&bob, dagger, false, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == true);
            const auto points = learning.gainable_points();
            CHECK(points == expected_points);
            bob.apply_skill_points(dagger, points);
            CHECK(learning.learning_message()
                  == "|WYou learn from your mistakes, and your |Cdagger|W skill improves. (98)|w\n\r");
            CHECK(learning.experience_reward() == 4);
        }
    }
    SECTION("npc") {
        using trompeloeil::_;
        set_enum_bit(bob.act, CharActFlag::Npc);
        SECTION("never learns") {
            FORBID_CALL(rng, number_range(_, _));
            FORBID_CALL(rng, number_percent());

            const auto learning = Learning(&bob, dagger, true, 1, rng);
            const auto result = learning.has_learned();

            CHECK(result == false);
        }
    }
}
