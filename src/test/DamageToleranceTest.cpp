/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageTolerance.hpp"
#include "Char.hpp"
#include "DamageType.hpp"
#include "ToleranceFlag.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

TEST_CASE("check damage tolerance") {
    using std::make_tuple;
    Char ch{};
    DamageType damage_type;
    ToleranceFlag tolerance_bit;
    DamageTolerance expected;
    SECTION("weapon immunity includes all melee types") {
        std::tie(damage_type, tolerance_bit, expected) = GENERATE(table<DamageType, ToleranceFlag, DamageTolerance>({
            make_tuple(DamageType::None, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DamageType::Bash, ToleranceFlag::None /* ignored */, DamageTolerance::None),
            make_tuple(DamageType::Bash, ToleranceFlag::Weapon, DamageTolerance::Immune),
            make_tuple(DamageType::Pierce, ToleranceFlag::Weapon, DamageTolerance::Immune),
            make_tuple(DamageType::Slash, ToleranceFlag::Weapon, DamageTolerance::Immune),
            make_tuple(DamageType::Bash, ToleranceFlag::Fire, DamageTolerance::None),
        }));
        SECTION("expected immunity") {
            set_enum_bit(ch.imm_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
        SECTION("immunity takes priority over resistance and vulnerability") {
            set_enum_bit(ch.imm_flags, tolerance_bit);
            set_enum_bit(ch.res_flags, tolerance_bit);
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
    }
    SECTION("weapon resistance includes all melee types") {
        std::tie(damage_type, tolerance_bit, expected) = GENERATE(table<DamageType, ToleranceFlag, DamageTolerance>({
            make_tuple(DamageType::None, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DamageType::Bash, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DamageType::Bash, ToleranceFlag::Weapon, DamageTolerance::Resistant),
            make_tuple(DamageType::Pierce, ToleranceFlag::Weapon, DamageTolerance::Resistant),
            make_tuple(DamageType::Slash, ToleranceFlag::Weapon, DamageTolerance::Resistant),
            make_tuple(DamageType::Bash, ToleranceFlag::Fire, DamageTolerance::None),
        }));
        SECTION("expected resistance") {
            set_enum_bit(ch.res_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
        SECTION("resistance takes priority over vulnerability") {
            set_enum_bit(ch.res_flags, tolerance_bit);
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
    }
    SECTION("weapon vulnerability includes all melee types") {
        std::tie(damage_type, tolerance_bit, expected) = GENERATE(table<DamageType, ToleranceFlag, DamageTolerance>({
            make_tuple(DamageType::None, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DamageType::Bash, ToleranceFlag::None /*ignored */, DamageTolerance::None),
            make_tuple(DamageType::Bash, ToleranceFlag::Weapon, DamageTolerance::Vulnerable),
            make_tuple(DamageType::Pierce, ToleranceFlag::Weapon, DamageTolerance::Vulnerable),
            make_tuple(DamageType::Slash, ToleranceFlag::Weapon, DamageTolerance::Vulnerable),
            make_tuple(DamageType::Bash, ToleranceFlag::Fire, DamageTolerance::None),
        }));
        SECTION("expected vulnerability") {
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
    }
    SECTION("the more generalized cases") {
        std::tie(damage_type, tolerance_bit, expected) = GENERATE(table<DamageType, ToleranceFlag, DamageTolerance>({
            make_tuple(DamageType::Bash, ToleranceFlag::Bash, DamageTolerance::Immune),
            make_tuple(DamageType::Pierce, ToleranceFlag::Pierce, DamageTolerance::Immune),
            make_tuple(DamageType::Slash, ToleranceFlag::Slash, DamageTolerance::Immune),
            make_tuple(DamageType::Fire, ToleranceFlag::Fire, DamageTolerance::Immune),
            make_tuple(DamageType::Cold, ToleranceFlag::Cold, DamageTolerance::Immune),
            make_tuple(DamageType::Lightning, ToleranceFlag::Lightning, DamageTolerance::Immune),
            make_tuple(DamageType::Acid, ToleranceFlag::Acid, DamageTolerance::Immune),
            make_tuple(DamageType::Poison, ToleranceFlag::Poison, DamageTolerance::Immune),
            make_tuple(DamageType::Negative, ToleranceFlag::Negative, DamageTolerance::Immune),
            make_tuple(DamageType::Holy, ToleranceFlag::Holy, DamageTolerance::Immune),
            make_tuple(DamageType::Energy, ToleranceFlag::Energy, DamageTolerance::Immune),
            make_tuple(DamageType::Mental, ToleranceFlag::Mental, DamageTolerance::Immune),
            make_tuple(DamageType::Disease, ToleranceFlag::Disease, DamageTolerance::Immune),
            make_tuple(DamageType::Drowning, ToleranceFlag::Drowning, DamageTolerance::Immune),
            make_tuple(DamageType::Light, ToleranceFlag::Light, DamageTolerance::Immune),
            make_tuple(DamageType::Other, ToleranceFlag::Magic, DamageTolerance::Immune),
            make_tuple(DamageType::Harm, ToleranceFlag::Magic, DamageTolerance::Immune),
        }));
        SECTION("expected immunity") {
            set_enum_bit(ch.imm_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
        SECTION("immunity takes priority over resistance and vulnerability") {
            set_enum_bit(ch.imm_flags, tolerance_bit);
            set_enum_bit(ch.res_flags, tolerance_bit);
            set_enum_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, damage_type);

            CHECK(result == expected);
        }
    }
    SECTION("cherrypicked resistance and vulnerability cases") {
        SECTION("resistant to acid") {
            set_enum_bit(ch.res_flags, ToleranceFlag::Acid);

            const auto result = check_damage_tolerance(&ch, DamageType::Acid);

            CHECK(result == DamageTolerance::Resistant);
        }
        SECTION("vulnerable to acid") {
            set_enum_bit(ch.vuln_flags, ToleranceFlag::Acid);

            const auto result = check_damage_tolerance(&ch, DamageType::Acid);

            CHECK(result == DamageTolerance::Vulnerable);
        }
    }
}
