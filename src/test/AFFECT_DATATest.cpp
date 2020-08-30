#include "AFFECT_DATA.hpp"

#include "Char.hpp"
#include "merc.h"

#include <catch2/catch.hpp>

TEST_CASE("AFFECT_DATA") {
    // Horrendous hacks here - gsn_* is only initialised globally once. TODO: remove this hack and make "skillness"
    // part of the affect?
    auto pre_b = gsn_blindness;
    auto pre_s = gsn_sneak;
    auto pre_r = gsn_ride;
    gsn_blindness = 1;
    gsn_sneak = 2;
    gsn_ride = 3;

    SECTION("should start with sane defaults") {
        AFFECT_DATA af;
        CHECK(af.next == nullptr);
        CHECK(af.type == 0);
        CHECK(af.level == 0);
        CHECK(af.duration == 0);
        CHECK(af.location == AffectLocation::None);
        CHECK(af.bitvector == 0);
    }
    SECTION("should apply and unapply to a character") {
        Char ch;
        AFFECT_DATA af;
        SECTION("affect bits") {
            ch.affected_by = AFF_DARK_VISION;
            af.bitvector = AFF_CHARM;
            af.apply(ch);
            CHECK(ch.affected_by == (AFF_CHARM | AFF_DARK_VISION));
            af.unapply(ch);
            CHECK(ch.affected_by == AFF_DARK_VISION);
        }
        auto test_stat = [&](AffectLocation location, auto get) {
            auto stat_before = get();
            af.location = location;
            af.modifier = 3;
            af.apply(ch);
            CHECK(get() == stat_before + 3);
            af.unapply(ch);
            CHECK(get() == stat_before);
        };
        SECTION("stats") {
            test_stat(AffectLocation::Wis, [&] { return ch.curr_stat(Stat::Wis); });
        }
        SECTION("saves") {
            test_stat(AffectLocation::SavingSpell, [&] { return ch.saving_throw; });
        }
        SECTION("hitroll") {
            test_stat(AffectLocation::Hitroll, [&] { return ch.hitroll; });
        }
        SECTION("damroll") {
            test_stat(AffectLocation::Damroll, [&] { return ch.damroll; });
        }
        SECTION("mana") {
            test_stat(AffectLocation::Mana, [&] { return ch.max_mana; });
        }
        SECTION("hit") {
            test_stat(AffectLocation::Hit, [&] { return ch.max_hit; });
        }
        SECTION("move") {
            test_stat(AffectLocation::Move, [&] { return ch.max_move; });
        }
        SECTION("sex") {
            test_stat(AffectLocation::Sex, [&] { return ch.sex; });
        }
        SECTION("armour class") {
            auto ac_before = ch.armor;
            af.location = AffectLocation::Ac;
            af.modifier = 3;
            af.apply(ch);
            auto ac_expected = ac_before;
            for (auto &ac : ac_expected)
                ac += 3;
            CHECK(ch.armor == ac_expected);
            af.unapply(ch);
            CHECK(ch.armor == ac_before);
        }
    }

    SECTION("should name things") {
        // Just a few random checks.
        CHECK(name(AffectLocation::Damroll) == "damage roll");
        CHECK(name(AffectLocation::None) == "none");
        CHECK(name(AffectLocation::Height) == "height");
    }

    SECTION("should identify skills") {
        CHECK(!AFFECT_DATA{nullptr, gsn_blindness}.is_skill());
        CHECK(AFFECT_DATA{nullptr, gsn_sneak}.is_skill());
        CHECK(AFFECT_DATA{nullptr, gsn_ride}.is_skill());
    }

    SECTION("should describe effects") {
        auto spell = AFFECT_DATA{nullptr, -1, 33, 22, AffectLocation::Wis, 1, AFF_HASTE};
        SECTION("for items") {
            CHECK(spell.describe_item_effect(false) == "wisdom by 1");
            CHECK(spell.describe_item_effect(true) == "wisdom by 1 with bits haste, level 33");
        }
        SECTION("for characters") {
            SECTION("spells") {
                CHECK(spell.describe_char_effect(false) == "wisdom by 1 for 22 hours");
                CHECK(spell.describe_char_effect(true) == "wisdom by 1 for 22 hours with bits haste, level 33");
            }
            SECTION("skills") {
                auto skill = AFFECT_DATA{nullptr, gsn_sneak, 80, 0, AffectLocation::None, 0, AFF_SNEAK};
                CHECK(skill.describe_char_effect(false) == "none by 0");
                CHECK(skill.describe_char_effect(true) == "none by 0 with bits sneak, level 80");
            }
        }
    }

    gsn_blindness = pre_b;
    gsn_sneak = pre_s;
    gsn_ride = pre_r;
}
