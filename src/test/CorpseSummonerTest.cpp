#include "CorpseSummoner.hpp"
#include "CharActFlag.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectType.hpp"
#include "Room.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_version_macros.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <vector>

using trompeloeil::_;

namespace {

struct MockDependencies : public CorpseSummoner::Dependencies {

    MAKE_MOCK2(interpret, void(Char *, std::string_view), override);
    MAKE_MOCK7(act,
               void(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To to,
                    const MobTrig mob_trig, const Position::Type position),
               override);
    MAKE_MOCK5(act, void(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To to), override);
    MAKE_MOCK1(obj_from_char, void(Object *obj), override);
    MAKE_MOCK2(obj_to_char, void(Object *obj, Char *ch), override);
    MAKE_MOCK1(obj_from_room, void(Object *obj), override);
    MAKE_MOCK2(obj_to_room, void(Object *obj, Room *room), override);
    MAKE_MOCK1(extract_obj, void(Object *obj), override);
    MAKE_MOCK2(affect_to_char, void(Char *ch, const AFFECT_DATA &paf), override);
    MAKE_MOCK0(object_list, std::vector<std::unique_ptr<Object>> &(), override);
    MAKE_CONST_MOCK0(spec_fun_summoner, SpecialFunc(), override);
    MAKE_CONST_MOCK0(weaken_sn, int(), override);
};

std::unique_ptr<Object> make_test_obj(Room *room, ObjectIndex *obj_idx) {
    auto obj = std::make_unique<Object>(obj_idx);
    obj->in_room = room;
    return obj;
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
                      "and give it to me. Please read the sign for more details."sv));

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
        set_enum_bit(player.act, CharActFlag::Npc);

        CHECK(!summoner.check_summoner_preconditions(&player, &mob));
    }

    SECTION("mob is pc") {
        FORBID_CALL(mock, spec_fun_summoner());

        CHECK(!summoner.check_summoner_preconditions(&player, &mob));
    }

    SECTION("mob does not have spec_summoner") {
        set_enum_bit(mob.act, CharActFlag::Npc);
        REQUIRE_CALL(mock, spec_fun_summoner()).RETURN(spec_fun_stub);

        CHECK(!summoner.check_summoner_preconditions(&player, &mob));
    }

    SECTION("mob has spec_summoner") {
        mob.spec_fun = spec_fun_stub;
        set_enum_bit(mob.act, CharActFlag::Npc);
        REQUIRE_CALL(mock, spec_fun_summoner()).RETURN(spec_fun_stub);

        CHECK(summoner.check_summoner_preconditions(&player, &mob));
    }
}

TEST_CASE("is catalyst valid") {
    MockDependencies mock;
    CorpseSummoner summoner(mock);
    Char player{};
    ObjectIndex catalyst_idx{};
    Object catalyst{&catalyst_idx};

    SECTION("item is not pc corpse summoner") {
        auto msg = summoner.is_catalyst_invalid(&player, &catalyst);

        CHECK(*msg == "Sorry, this item cannot be used to summon your corpse."sv);
    }

    SECTION("catalyst is below level range") {
        player.level = 12;
        set_enum_bit(catalyst.extra_flags, ObjectExtraFlag::SummonCorpse);
        catalyst.level = player.level - 10;

        auto msg = summoner.is_catalyst_invalid(&player, &catalyst);

        CHECK(*msg == "Sorry, this item is not powerful enough to summon your corpse."sv);
    }

    SECTION("catalyst is valid") {
        player.level = 12;
        set_enum_bit(catalyst.extra_flags, ObjectExtraFlag::SummonCorpse);
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
    ObjectIndex obj_idx{};
    Object catalyst{&obj_idx};

    SECTION("with invalid catalyst") {
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock,
                     act("|C$n tells you '$t|C'|w", &mob, "Sorry, this item cannot be used to summon your corpse."sv,
                         &player, To::Vict, MobTrig::Yes, Position::Type::Dead))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_from_char(&catalyst)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_to_char(&catalyst, &player)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("$n gives $p to $N.", &mob, _, &player, To::NotVict)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("$n gives you $p.", &mob, _, &player, To::Vict)).IN_SEQUENCE(seq);

        CHECK(!summoner.check_catalyst(&player, &mob, &catalyst));
    }

    SECTION("with valid catalyst") {
        player.level = 12;
        set_enum_bit(catalyst.extra_flags, ObjectExtraFlag::SummonCorpse);
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
        ObjectIndex obj_idx{.type{ObjectType::Weapon}};
        std::vector<std::unique_ptr<Object>> obj_list;
        auto weapon = make_test_obj(&object_room, &obj_idx);
        obj_list.push_back(std::move(weapon));
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(!found);
    }

    SECTION("ignore corpse owned by another player") {
        ObjectIndex obj_idx{.short_descr{"corpse of Sinbad"}, .type{ObjectType::Pccorpse}};
        std::vector<std::unique_ptr<Object>> obj_list;
        auto corpse = make_test_obj(&object_room, &obj_idx);
        obj_list.push_back(std::move(corpse));
        player.in_room = &player_room;
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(!found);
    }

    SECTION("ignore player's corpse in same room as summoner") {
        ObjectIndex obj_idx{.short_descr{"corpse of Sinbad"}, .type{ObjectType::Pccorpse}};
        std::vector<std::unique_ptr<Object>> obj_list;
        auto corpse = make_test_obj(&player_room, &obj_idx);
        obj_list.push_back(std::move(corpse));
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(!found);
    }

    SECTION("found player's corpse") {
        ObjectIndex obj_idx{.short_descr{"corpse of Test"}, .type{ObjectType::Pccorpse}};
        std::vector<std::unique_ptr<Object>> obj_list;
        auto corpse = make_test_obj(&object_room, &obj_idx);
        auto corpse_ptr = corpse.get();
        obj_list.push_back(std::move(corpse));
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list);

        auto found = summoner.get_pc_corpse_world(&player, tests_corpse_desc);

        CHECK(*found == corpse_ptr);
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
    ObjectIndex obj_index{.level{12}};
    Object catalyst{&obj_index};
    set_enum_bit(catalyst.extra_flags, ObjectExtraFlag::SummonCorpse);
    const auto weaken_sn{68};

    SECTION("successful summmon") {
        ObjectIndex obj_idx{.short_descr{"corpse of Test"}, .type{ObjectType::Pccorpse}};
        std::vector<std::unique_ptr<Object>> obj_list;
        auto corpse = make_test_obj(&object_room, &obj_idx);
        auto corpse_ptr = corpse.get();
        obj_list.push_back(std::move(corpse));
        trompeloeil::sequence seq;
        REQUIRE_CALL(mock, act("$n clutches $p between $s bony fingers and begins to whisper."sv, &mob, &catalyst,
                               nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("The runes on the summoning stone begin to glow more brightly!"sv, &mob, &catalyst,
                               nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, object_list()).LR_RETURN(obj_list).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_from_room(corpse_ptr)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, obj_to_room(corpse_ptr, &player_room)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("|BThere is a flash of light and a corpse materialises on the ground before you!|w"sv,
                               &mob, nullptr, nullptr, To::Room))
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, weaken_sn()).RETURN(weaken_sn).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, affect_to_char(&player, _))
            .WITH(_2.location == AffectLocation::Str && _2.type == weaken_sn)
            .IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, act("$n is knocked off $s feet!"sv, &player, nullptr, nullptr, To::Room)).IN_SEQUENCE(seq);
        REQUIRE_CALL(mock, extract_obj(&catalyst)).IN_SEQUENCE(seq);

        summoner.SummonCorpse(&player, &mob, &catalyst);

        CHECK(player.position == Position::Type::Resting);
    }

    SECTION("failed summmon as corpse cannot be found") {
        std::vector<std::unique_ptr<Object>> obj_list;
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

        summoner.SummonCorpse(&player, &mob, &catalyst);
    }
}
}
