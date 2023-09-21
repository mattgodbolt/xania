#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "SkillNumbers.hpp"

#include <catch2/catch_test_macros.hpp>

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
        CHECK(af.type == 0);
        CHECK(af.level == 0);
        CHECK(af.duration == 0);
        CHECK(af.location == AffectLocation::None);
        CHECK(af.bitvector == 0);
    }
    SECTION("should apply and unapply to a character") {
        Char ch{};
        ch.pcdata = std::make_unique<PcData>();
        AFFECT_DATA af;
        SECTION("affect bits") {
            ch.affected_by = to_int(AffectFlag::DarkVision);
            af.bitvector = to_int(AffectFlag::Charm);
            af.apply(ch);
            CHECK(check_enum_bit(ch.affected_by, AffectFlag::Charm));
            CHECK(check_enum_bit(ch.affected_by, AffectFlag::DarkVision));
            af.unapply(ch);
            CHECK(ch.affected_by == to_int(AffectFlag::DarkVision));
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
        CHECK(!AFFECT_DATA{gsn_blindness}.is_skill());
        CHECK(AFFECT_DATA{gsn_sneak}.is_skill());
        CHECK(AFFECT_DATA{gsn_ride}.is_skill());
    }

    SECTION("should describe effects") {
        auto spell_with_location = AFFECT_DATA{-1, 33, 22, AffectLocation::Wis, 1, to_int(AffectFlag::Haste)};
        SECTION("for items") {
            CHECK(spell_with_location.describe_item_effect(false) == "wisdom by 1");
            CHECK(spell_with_location.describe_item_effect(true) == "wisdom by 1 with bits |Chaste|w, level 33");
        }
        SECTION("for characters") {
            auto spell_no_location = AFFECT_DATA{-1, 33, 22, AffectLocation::None, 1, to_int(AffectFlag::Haste)};
            SECTION("spells") {
                CHECK(spell_with_location.describe_char_effect(false) == " modifies wisdom by 1 for 22 hours");
                CHECK(spell_with_location.describe_char_effect(true)
                      == " modifies wisdom by 1 for 22 hours with bits |Chaste|w, level 33");
                // The case where a spell effect does not affect a specific attribute, in which
                // case the 'affects' command shows a concise message rather than "modifies none by 0".
                CHECK(spell_no_location.describe_char_effect(false) == " for 22 hours");
                CHECK(spell_no_location.describe_char_effect(true)
                      == " modifies none by 1 for 22 hours with bits |Chaste|w, level 33");
            }
            SECTION("skills") {
                auto skill = AFFECT_DATA{gsn_sneak, 80, 0, AffectLocation::None, 0, to_int(AffectFlag::Sneak)};
                // Similar to above, concise message for effects like sneak and ride.
                CHECK(skill.describe_char_effect(false).empty());
                CHECK(skill.describe_char_effect(true) == " modifies none by 0 with bits |Csneak|w, level 80");
            }
        }
    }

    gsn_blindness = pre_b;
    gsn_sneak = pre_s;
    gsn_ride = pre_r;
}
