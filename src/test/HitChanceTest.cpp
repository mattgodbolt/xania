/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2022 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "HitChance.hpp"
#include "AffectFlag.hpp"
#include "CharActFlag.hpp"
#include "Room.hpp"
#include "SkillNumbers.hpp"
#include "handler.hpp"
#include "lookup.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

TEST_CASE("hit chance") {
    using std::make_tuple;
    const skill_type *no_skill{nullptr};
    const auto bash_damage{DamageType::Bash};
    const auto max_weapon_skill{100};
    Room room{};
    Char attacker{};
    Char victim{};
    attacker.pcdata = std::make_unique<PcData>();
    const auto mage = class_lookup("mage");
    attacker.class_num = mage;
    victim.class_num = mage;
    const auto human = race_lookup("human");
    attacker.race = human;
    victim.race = human;
    attacker.in_room = &room;
    victim.in_room = &room;
    attacker.position = Position::Type::Fighting;
    victim.position = Position::Type::Fighting;
    attacker.level = 1;
    victim.level = 1;

    SECTION("pc vs pc") {
        REQUIRE(victim.curr_stat(Stat::Dex) == 13);
        REQUIRE(attacker.curr_stat(Stat::Str) == 13);
        SECTION("attacker: L1 default armour; victim L1 default armour, defensive dex bonuses") {
            sh_int dex_bonus;
            int expected_chance;
            std::tie(dex_bonus, expected_chance) = GENERATE(table<sh_int, int>({
                make_tuple(0, 95),
                make_tuple(1, 95),
                make_tuple(2, 18),
                make_tuple(3, 12),
                make_tuple(4, 10),
                make_tuple(5, 10),
                make_tuple(6, 10),
            }));
            SECTION("chance scales inversely with dexterity") {
                victim.mod_stat[Stat::Dex] = dex_bonus;
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }
        SECTION("attacker: L1 default armour; victim L1 default armour, prone skips dex bonus") {
            sh_int dex_bonus;
            int expected_chance;
            victim.position = Position::Type::Sleeping;
            std::tie(dex_bonus, expected_chance) = GENERATE(table<sh_int, int>({
                make_tuple(0, 95),
                make_tuple(6, 95),
            }));
            SECTION("no dex bonus when prone") {
                victim.mod_stat[Stat::Dex] = dex_bonus;
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }

        SECTION("attacker: L1 default armour; victim L1 bonus armour") {
            sh_int armour_bonus; // lower == better
            int expected_chance;
            std::tie(armour_bonus, expected_chance) = GENERATE(table<sh_int, int>({
                make_tuple(1, 95),   make_tuple(0, 95),   make_tuple(-1, 95),  make_tuple(-2, 95),  make_tuple(-3, 66),
                make_tuple(-4, 50),  make_tuple(-5, 40),  make_tuple(-6, 33),  make_tuple(-7, 28),  make_tuple(-8, 25),
                make_tuple(-9, 22),  make_tuple(-10, 20), make_tuple(-11, 18), make_tuple(-12, 16), make_tuple(-13, 15),
                make_tuple(-14, 14), make_tuple(-15, 13), make_tuple(-16, 12), make_tuple(-17, 11), make_tuple(-18, 11),
                make_tuple(-19, 10), make_tuple(-30, 10),
            }));
            SECTION("chance scales inversely with armour bonus") {
                victim.armor[magic_enum::enum_integer<ArmourClass>(ArmourClass::Bash)] = armour_bonus;
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }

        SECTION("attacker: L1 default armour; victim L1, sleeping position") {
            victim.position = Position::Type::Sleeping;
            sh_int armour_bonus; // lower == better
            int expected_chance;
            std::tie(armour_bonus, expected_chance) = GENERATE(table<sh_int, int>({
                make_tuple(1, 95),
                make_tuple(0, 95),
                make_tuple(-1, 95),
                make_tuple(-2, 95),
                make_tuple(-3, 95),
                make_tuple(-4, 66),
                make_tuple(-5, 50),
                make_tuple(-6, 50),
                make_tuple(-7, 40),
                make_tuple(-8, 33),
                make_tuple(-9, 28),
                make_tuple(-10, 25),
                make_tuple(-11, 25),
                make_tuple(-12, 22),
            }));
            SECTION("armour bonus has reduced effect when sleeping") {
                victim.armor[magic_enum::enum_integer<ArmourClass>(ArmourClass::Bash)] = armour_bonus;
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }
        SECTION("attacker: L1 default armour; victim L1, resting position") {
            victim.position = Position::Type::Resting;
            sh_int armour_bonus; // lower == better
            int expected_chance;
            std::tie(armour_bonus, expected_chance) = GENERATE(table<sh_int, int>({
                make_tuple(1, 95),
                make_tuple(0, 95),
                make_tuple(-1, 95),
                make_tuple(-2, 95),
                make_tuple(-3, 95),
                make_tuple(-4, 66),
                make_tuple(-5, 50),
                make_tuple(-6, 40),
                make_tuple(-7, 33),
                make_tuple(-8, 28),
                make_tuple(-9, 25),
                make_tuple(-10, 22),
                make_tuple(-11, 22),
                make_tuple(-12, 20),
            }));
            SECTION("armour bonus has reduced effect when resting") {
                victim.armor[magic_enum::enum_integer<ArmourClass>(ArmourClass::Bash)] = armour_bonus;
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }
        SECTION("attacker: L32 default armour varied weapon skill; victim L32 reasonable armour") {
            int weapon_skill;
            int expected_chance;
            attacker.level = 32;
            victim.level = 32;
            // victim armour is mostly based on the player's worn equipment. Fudge their armour to be
            // reasonable for a L32 player so that with max weapon skill, the attacker only has 55% chance to hit
            victim.armor[magic_enum::enum_integer<ArmourClass>(ArmourClass::Bash)] = -85;
            std::tie(weapon_skill, expected_chance) = GENERATE(table<int, int>({
                make_tuple(100, 55),
                make_tuple(95, 51),
                make_tuple(90, 49),
                make_tuple(80, 43),
                make_tuple(70, 37),
                make_tuple(40, 21),
                make_tuple(37, 20),
                make_tuple(34, 17),
                make_tuple(30, 16),
                make_tuple(25, 12),
                make_tuple(22, 11),
                make_tuple(20, 10),
                make_tuple(1, 10),
                make_tuple(0, 10),
            }));
            SECTION("chance scales with weapon skill") {
                const auto chance = HitChance(attacker, victim, weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }

        SECTION("attacker: L1 default armour, str increases hitroll; victim L1 default armour") {
            sh_int str_bonus;
            int expected_chance;
            // Boost the victim dex so that their chance of avoiding means we can test a lower chance range.
            victim.mod_stat[Stat::Dex] = 3;
            std::tie(str_bonus, expected_chance) = GENERATE(table<sh_int, int>({
                make_tuple(0, 12),
                make_tuple(1, 12),
                make_tuple(2, 18),
                make_tuple(3, 18),
                make_tuple(4, 25),
                make_tuple(5, 25),
                make_tuple(6, 31),
                make_tuple(7, 31),
                make_tuple(8, 37),
                make_tuple(9, 37),
                make_tuple(10, 37),
                make_tuple(11, 37),
                make_tuple(12, 37),
            }));
            SECTION("chance scales with hitroll via strength") {
                attacker.mod_stat[Stat::Str] = str_bonus;
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == expected_chance);
            }
        }
        SECTION("attacker: L32 default armour; victim L32 reasonable armour") {
            attacker.level = 32;
            victim.level = 32;
            // victim armour is mostly based on the player's worn equipment. Fudge their armour to be
            // reasonable for a L32 player so that with max weapon skill, the attacker only has 55% chance to hit
            victim.armor[magic_enum::enum_integer<ArmourClass>(ArmourClass::Bash)] = -85;
            SECTION("chance if can see victim") {
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == 55);
            }
            SECTION("chance if can't see victim") {
                set_enum_bit(attacker.affected_by, AffectFlag::Blind);
                const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

                REQUIRE(chance == 43);
            }
            SECTION("chance scales with backstab skill") {
                SECTION("50% backstab") {
                    attacker.pcdata->learned[skill_lookup("backstab")] = 50;
                    const auto chance =
                        HitChance(attacker, victim, max_weapon_skill, &skill_table[gsn_backstab], bash_damage)
                            .calculate();

                    REQUIRE(chance == 74);
                }
                SECTION("100% backstab") {
                    attacker.pcdata->learned[skill_lookup("backstab")] = 100;
                    const auto chance =
                        HitChance(attacker, victim, max_weapon_skill, &skill_table[gsn_backstab], bash_damage)
                            .calculate();

                    REQUIRE(chance == 94);
                }
            }
        }
    }
    SECTION("npc vs pc") {
        attacker.level = 32;
        victim.level = 32;
        // victim armour is mostly based on the player's worn equipment. Fudge their armour to be
        // reasonable for a L32 player so that with max weapon skill, the attacker only has 55% chance to hit
        victim.armor[magic_enum::enum_integer<ArmourClass>(ArmourClass::Bash)] = -85;
        // Enable attacker as an NPC but there's no need to link it with a MobIndexData as the hit calculation code
        // doesn't depend on that.
        set_enum_bit(attacker.act, CharActFlag::Npc);
        REQUIRE(victim.curr_stat(Stat::Dex) == 13);
        REQUIRE(attacker.curr_stat(Stat::Str) == 13);
        SECTION("npc has no combat act flag") {
            const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

            REQUIRE(chance == 82);
        }
        SECTION("npc has CharActFlag::Warrior") {
            set_enum_bit(attacker.act, CharActFlag::Warrior);
            const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

            REQUIRE(chance == 89);
        }
        SECTION("npc has CharActFlag::Thief") {
            set_enum_bit(attacker.act, CharActFlag::Thief);
            const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

            REQUIRE(chance == 82);
        }
        SECTION("npc has CharActFlag::Cleric") {
            set_enum_bit(attacker.act, CharActFlag::Cleric);
            const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

            REQUIRE(chance == 77);
        }
        SECTION("npc has CharActFlag::Mage") {
            set_enum_bit(attacker.act, CharActFlag::Mage);
            const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

            REQUIRE(chance == 74);
        }
        SECTION("npc has multiple char act flags and warrior takes precedence") {
            set_enum_bit(attacker.act, CharActFlag::Warrior);
            set_enum_bit(attacker.act, CharActFlag::Thief);
            set_enum_bit(attacker.act, CharActFlag::Cleric);
            set_enum_bit(attacker.act, CharActFlag::Mage);
            const auto chance = HitChance(attacker, victim, max_weapon_skill, no_skill, bash_damage).calculate();

            REQUIRE(chance == 89);
        }
    }
}
