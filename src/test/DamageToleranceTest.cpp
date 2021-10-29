/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageTolerance.hpp"
#include "Char.hpp"
#include "DamageClass.hpp"
#include "ToleranceFlag.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch.hpp>
#include <tuple>

TEST_CASE("check damage tolerance") {
    using std::make_tuple;
    Char ch{};
    int dam_type;
    ToleranceFlag tolerance_bit;
    DamageTolerance expected;
    SECTION("weapon immunity includes all melee types") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, ToleranceFlag, DamageTolerance>({
            make_tuple(DAM_NONE, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, ToleranceFlag::None /* ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, ToleranceFlag::Weapon, DamageTolerance::Immune),
            make_tuple(DAM_PIERCE, ToleranceFlag::Weapon, DamageTolerance::Immune),
            make_tuple(DAM_SLASH, ToleranceFlag::Weapon, DamageTolerance::Immune),
            make_tuple(DAM_BASH, ToleranceFlag::Fire, DamageTolerance::None),
        }));
        SECTION("expected immunity") {
            set_enum_bit(ch.imm_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
        SECTION("immunity takes priority over resistance and vulnerability") {
            set_enum_bit(ch.imm_flags, tolerance_bit);
            set_enum_bit(ch.res_flags, tolerance_bit);
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("weapon resistance includes all melee types") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, ToleranceFlag, DamageTolerance>({
            make_tuple(DAM_NONE, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, ToleranceFlag::Weapon, DamageTolerance::Resistant),
            make_tuple(DAM_PIERCE, ToleranceFlag::Weapon, DamageTolerance::Resistant),
            make_tuple(DAM_SLASH, ToleranceFlag::Weapon, DamageTolerance::Resistant),
            make_tuple(DAM_BASH, ToleranceFlag::Fire, DamageTolerance::None),
        }));
        SECTION("expected resistance") {
            set_enum_bit(ch.res_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
        SECTION("resistance takes priority over vulnerability") {
            set_enum_bit(ch.res_flags, tolerance_bit);
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("weapon vulnerability includes all melee types") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, ToleranceFlag, DamageTolerance>({
            make_tuple(DAM_NONE, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, ToleranceFlag::Weapon, DamageTolerance::Vulnerable),
            make_tuple(DAM_PIERCE, ToleranceFlag::Weapon, DamageTolerance::Vulnerable),
            make_tuple(DAM_SLASH, ToleranceFlag::Weapon, DamageTolerance::Vulnerable),
            make_tuple(DAM_BASH, ToleranceFlag::Fire, DamageTolerance::None),
        }));
        SECTION("expected vulnerability") {
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("the more generalized cases") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, ToleranceFlag, DamageTolerance>({
            make_tuple(DAM_BASH, ToleranceFlag::Bash, DamageTolerance::Immune),
            make_tuple(DAM_PIERCE, ToleranceFlag::Pierce, DamageTolerance::Immune),
            make_tuple(DAM_SLASH, ToleranceFlag::Slash, DamageTolerance::Immune),
            make_tuple(DAM_FIRE, ToleranceFlag::Fire, DamageTolerance::Immune),
            make_tuple(DAM_COLD, ToleranceFlag::Cold, DamageTolerance::Immune),
            make_tuple(DAM_LIGHTNING, ToleranceFlag::Lightning, DamageTolerance::Immune),
            make_tuple(DAM_ACID, ToleranceFlag::Acid, DamageTolerance::Immune),
            make_tuple(DAM_POISON, ToleranceFlag::Poison, DamageTolerance::Immune),
            make_tuple(DAM_NEGATIVE, ToleranceFlag::Negative, DamageTolerance::Immune),
            make_tuple(DAM_HOLY, ToleranceFlag::Holy, DamageTolerance::Immune),
            make_tuple(DAM_ENERGY, ToleranceFlag::Energy, DamageTolerance::Immune),
            make_tuple(DAM_MENTAL, ToleranceFlag::Mental, DamageTolerance::Immune),
            make_tuple(DAM_DISEASE, ToleranceFlag::Disease, DamageTolerance::Immune),
            make_tuple(DAM_DROWNING, ToleranceFlag::Drowning, DamageTolerance::Immune),
            make_tuple(DAM_LIGHT, ToleranceFlag::Light, DamageTolerance::Immune),
            make_tuple(DAM_OTHER, ToleranceFlag::Magic, DamageTolerance::Immune),
            make_tuple(DAM_HARM, ToleranceFlag::Magic, DamageTolerance::Immune),
        }));
        SECTION("expected immunity") {
            set_enum_bit(ch.imm_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
        SECTION("immunity takes priority over resistance and vulnerability") {
            set_enum_bit(ch.imm_flags, tolerance_bit);
            set_enum_bit(ch.res_flags, tolerance_bit);
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("cherrypicked resistance and vulnerability cases") {
        SECTION("resistant to acid") {
            set_enum_bit(ch.res_flags, ToleranceFlag::Acid);

            const auto result = check_damage_tolerance(&ch, DAM_ACID);

            CHECK(result == DamageTolerance::Resistant);
        }
        SECTION("vulnerable to acid") {
            set_enum_bit(ch.vuln_flags, ToleranceFlag::Acid);

            const auto result = check_damage_tolerance(&ch, DAM_ACID);

            CHECK(result == DamageTolerance::Vulnerable);
        }
    }
}