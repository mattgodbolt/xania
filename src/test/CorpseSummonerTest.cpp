#include "CorpseSummoner.hpp"
#include "BitsCharAct.hpp"
#include "ExtraDescription.hpp"
#include "Object.hpp"
#include "ObjectType.hpp"
#include "Room.hpp"
#include "merc.h"

#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>

using trompeloeil::_;

namespace {

struct MockDependencies : public CorpseSummoner::Dependencies {

    MAKE_MOCK2(interpret, void(Char *, std::string), override);
    MAKE_MOCK6(act,
               void(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, To to,
                    const Position::Type position),
               override);
    MAKE_MOCK5(act, void(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, To to), override);
    MAKE_MOCK1(obj_from_char, void(Object *obj), override);
    MAKE_MOCK2(obj_to_char, void(Object *obj, Char *ch), override);
    MAKE_MOCK1(obj_from_room, void(Object *obj), override);
    MAKE_MOCK2(obj_to_room, void(Object *obj, Room *room), override);
    MAKE_MOCK1(extract_obj, void(Object *obj), override);
    MAKE_MOCK2(affect_to_char, void(Char *ch, const AFFECT_DATA &paf), override);
    MAKE_MOCK0(object_list, GenericList<Object *> &(), override);
    MAKE_CONST_MOCK0(spec_fun_summoner, SpecialFunc(), override);
    MAKE_CONST_MOCK0(weaken_sn, int(), override);
};

Object make_test_obj(Room *room, std::string_view descr, const ObjectType type) {
    Object obj;
    obj.in_room = room;
    obj.short_descr = descr;
    obj.type = type;
    return obj;
}
}

TEST_CASE("summoner awaits") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Char player{};
    SECTION("does nothing if timer has not expired") {
        FORBID_CALL(mock, interpret(_, _));

        summoner.summoner_awaits(&player, 30);
    }

    SECTION("speaks if timer overdue") {
        REQUIRE_CALL(
            mock,
            interpret(&player,
                      "say If you wish to summon your corpse, purchase a shard that is powerful enough for your level "
                      "and give it to me. Please read the sign for more details."));

        summoner.summoner_awaits(&player, 31);
    }
}

TEST_CASE("summoner preconditions") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Char player{};
    Char mob{};
    SpecialFunc spec_fun_stub{[](Char *) { return true; }};

    SECTION("player is npc") {
        FORBID_CALL(mock, spec_fun_summoner());
        set_bit(player.act, ACT_IS_NPC);

        CHECK(!summoner.check_summoner_preconditions(&player, &mob));
    }

    SECTION("mob is pc") {
        FORBID_CALL(mock, spec_fun_summoner());

        CHECK(!summoner.check_summoner_preconditions(&player, &mob));
    }

    SECTION("mob does not have spec_summoner") {
        set_bit(mob.act, ACT_IS_NPC);
        REQUIRE_CALL(mock, spec_fun_summoner()).RETURN(spec_fun_stub);

        CHECK(!summoner.check_summoner_preconditions(&player, &mob));
    }

    SECTION("mob has spec_summoner") {
        mob.spec_fun = spec_fun_stub;
        set_bit(mob.act, ACT_IS_NPC);
        REQUIRE_CALL(mock, spec_fun_summoner()).RETURN(spec_fun_stub);

        CHECK(summoner.check_summoner_preconditions(&player, &mob));
    }
}

TEST_CASE("is catalyst valid") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Char player{};
    Object catalyst{};

    SECTION("item is not pc corpse summoner") {
        auto msg = summoner.is_catalyst_invalid(&player, &catalyst);

        CHECK(*msg == "Sorry, this item cannot be used to summon your corpse."sv);
    }

    SECTION("catalyst is below level range") {
        player.level = 12;
        set_bit(catalyst.extra_flags, ITEM_SUMMON_CORPSE);
        catalyst.level = player.level - 10;

        auto msg = summoner.is_catalyst_invalid(&player, &catalyst);

        CHECK(*msg == "Sorry, this item is not powerful enough to summon your corpse."sv);
    }

    SECTION("catalyst is valid") {
        player.level = 12;
        set_bit(catalyst.extra_flags, ITEM_SUMMON_CORPSE);
        catalyst.level = player.level - 9;

        auto msg = summoner.is_catalyst_invalid(&player, &catalyst);

        CHECK(!msg);
    }
}

TEST_CASE("check catalyst") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Char player{};
    Char mob{};
    Object catalyst{};

    SECTION("with invalid catalyst") {
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock,
                     act("|C$n tells you '$t|C'|w", &mob, "Sorry, this item cannot be used to summon your corpse."sv,
                         &player, To::Vict, Position::Type::Dead))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_from_char(&catalyst)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_to_char(&catalyst, &player)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("$n gives $p to $N.", &mob, _, &player, To::NotVict)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("$n gives you $p.", &mob, _, &player, To::Vict)).IN_SEQUENCE(seq);

        CHECK(!summoner.check_catalyst(&player, &mob, &catalyst));
    }

    SECTION("with valid catalyst") {
        player.level = 12;
        set_bit(catalyst.extra_flags, ITEM_SUMMON_CORPSE);
        catalyst.level = player.level - 9;
        FORBID_CALL(mock, obj_from_char(&catalyst));

        CHECK(summoner.check_catalyst(&player, &mob, &catalyst));
    }
}

TEST_CASE("get pc corpse world") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Char player{};
    Char mob{};
    Room object_room{};
    Room player_room{};
    auto tests_corpse_desc{"corpse of Test"};

    SECTION("no pc corpse in world") {
        Object weapon = make_test_obj(&object_room, "", ObjectType::Weapon);
        auto obj_list = GenericList<Object *>::of(&weapon);
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(!found);
    }

    SECTION("ignore corpse owned by another player") {
        char descr[] = "corpse of Sinbad";
        Object corpse = make_test_obj(&object_room, descr, ObjectType::Pccorpse);
        auto obj_list = GenericList<Object *>::of(&corpse);
        player.in_room = &player_room;
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(!found);
    }

    SECTION("ignore player's corpse in same room as summoner") {
        char descr[] = "corpse of Sinbad";
        Object corpse = make_test_obj(&player_room, descr, ObjectType::Pccorpse);
        auto obj_list = GenericList<Object *>::of(&corpse);
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(!found);
    }

    SECTION("found player's corpse") {
        char descr[] = "corpse of Test";
        Object corpse = make_test_obj(&object_room, descr, ObjectType::Pccorpse);
        auto obj_list = GenericList<Object *>::of(&corpse);
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(*found == &corpse);
    }
}
/**
 * End to end test of the happy paths.
 */
TEST_CASE("summon corpse") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Room object_room{};
    Room player_room{};
    Char player{};
    player.name = "Test";
    player.level = 12;
    player.in_room = &player_room;
    Char mob{};
    mob.in_room = &player_room;
    Object catalyst;
    catalyst.level = 12;
    set_bit(catalyst.extra_flags, ITEM_SUMMON_CORPSE);
    const auto weaken_sn{68};

    SECTION("successful summmon") {
        char descr[] = "corpse of Test";
        Object corpse = make_test_obj(&object_room, descr, ObjectType::Pccorpse);
        auto obj_list = GenericList<Object *>::of(&corpse);
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock, act("$n clutches $p between $s bony fingers and begins to whisper."sv, &mob, &catalyst,
                               nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("The runes on the summoning stone begin to glow more brightly!"sv, &mob, &catalyst,
                               nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_from_room(&corpse)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_to_room(&corpse, &player_room)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("|BThere is a flash of light and a corpse materialises on the ground before you!|w"sv,
                               &mob, nullptr, nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, weaken_sn()).RETURN(weaken_sn).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, affect_to_char(&player, _))
            .WITH(_2.location == AffectLocation::Str && _2.type == weaken_sn)
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("$n is knocked off $s feet!"sv, &player, nullptr, nullptr, To::Room)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, extract_obj(&catalyst)).IN_SEQUENCE(seq);

        summoner.summon_corpse(&player, &mob, &catalyst);

        CHECK(player.position == Position::Type::Resting);
    }

    SECTION("failed summmon as corpse cannot be found") {
        Object objects{};
        auto obj_list = GenericList<Object *>::of(&objects);
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock, act("$n clutches $p between $s bony fingers and begins to whisper."sv, &mob, &catalyst,
                               nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("The runes on the summoning stone begin to glow more brightly!"sv, &mob, &catalyst,
                               nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock,
                     act("The runes dim and the summoner tips $s head in shame."sv, &mob, nullptr, nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, extract_obj(&catalyst)).IN_SEQUENCE(seq);

        summoner.summon_corpse(&player, &mob, &catalyst);
    }
}
