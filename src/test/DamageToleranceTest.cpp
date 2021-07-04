/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageTolerance.hpp"
#include "BitsDamageTolerance.hpp"
#include "Char.hpp"
#include "DamageClass.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch.hpp>
#include <tuple>

TEST_CASE("check damage tolerance") {
    using std::make_tuple;
    Char ch{};
    int dam_type;
    int tolerance_bit;
    DamageTolerance expected;
    SECTION("weapon immunity includes all melee types") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, int, DamageTolerance>({
            make_tuple(DAM_NONE, 0 /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, 0, DamageTolerance::None),
            make_tuple(DAM_BASH, DMG_TOL_WEAPON, DamageTolerance::Immune),
            make_tuple(DAM_PIERCE, DMG_TOL_WEAPON, DamageTolerance::Immune),
            make_tuple(DAM_SLASH, DMG_TOL_WEAPON, DamageTolerance::Immune),
            make_tuple(DAM_BASH, DMG_TOL_FIRE, DamageTolerance::None),
        }));
        SECTION("expected immunity") {
            set_bit(ch.imm_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
        SECTION("immunity takes priority over resistance and vulnerability") {
            set_bit(ch.imm_flags, tolerance_bit);
            set_bit(ch.res_flags, tolerance_bit);
            set_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("weapon resistance includes all melee types") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, int, DamageTolerance>({
            make_tuple(DAM_NONE, 0 /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, 0, DamageTolerance::None),
            make_tuple(DAM_BASH, DMG_TOL_WEAPON, DamageTolerance::Resistant),
            make_tuple(DAM_PIERCE, DMG_TOL_WEAPON, DamageTolerance::Resistant),
            make_tuple(DAM_SLASH, DMG_TOL_WEAPON, DamageTolerance::Resistant),
            make_tuple(DAM_BASH, DMG_TOL_FIRE, DamageTolerance::None),
        }));
        SECTION("expected resistance") {
            set_bit(ch.res_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
        SECTION("resistance takes priority over vulnerability") {
            set_bit(ch.res_flags, tolerance_bit);
            set_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("weapon vulnerability includes all melee types") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, int, DamageTolerance>({
            make_tuple(DAM_NONE, 0 /*ignored */, DamageTolerance::None),
            make_tuple(DAM_BASH, 0, DamageTolerance::None),
            make_tuple(DAM_BASH, DMG_TOL_WEAPON, DamageTolerance::Vulnerable),
            make_tuple(DAM_PIERCE, DMG_TOL_WEAPON, DamageTolerance::Vulnerable),
            make_tuple(DAM_SLASH, DMG_TOL_WEAPON, DamageTolerance::Vulnerable),
            make_tuple(DAM_BASH, DMG_TOL_FIRE, DamageTolerance::None),
        }));
        SECTION("expected vulnerability") {
            set_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("the more generalized cases") {
        std::tie(dam_type, tolerance_bit, expected) = GENERATE(table<int, int, DamageTolerance>({
            make_tuple(DAM_BASH, DMG_TOL_BASH, DamageTolerance::Immune),
            make_tuple(DAM_PIERCE, DMG_TOL_PIERCE, DamageTolerance::Immune),
            make_tuple(DAM_SLASH, DMG_TOL_SLASH, DamageTolerance::Immune),
            make_tuple(DAM_FIRE, DMG_TOL_FIRE, DamageTolerance::Immune),
            make_tuple(DAM_COLD, DMG_TOL_COLD, DamageTolerance::Immune),
            make_tuple(DAM_LIGHTNING, DMG_TOL_LIGHTNING, DamageTolerance::Immune),
            make_tuple(DAM_ACID, DMG_TOL_ACID, DamageTolerance::Immune),
            make_tuple(DAM_POISON, DMG_TOL_POISON, DamageTolerance::Immune),
            make_tuple(DAM_NEGATIVE, DMG_TOL_NEGATIVE, DamageTolerance::Immune),
            make_tuple(DAM_HOLY, DMG_TOL_HOLY, DamageTolerance::Immune),
            make_tuple(DAM_ENERGY, DMG_TOL_ENERGY, DamageTolerance::Immune),
            make_tuple(DAM_MENTAL, DMG_TOL_MENTAL, DamageTolerance::Immune),
            make_tuple(DAM_DISEASE, DMG_TOL_DISEASE, DamageTolerance::Immune),
            make_tuple(DAM_DROWNING, DMG_TOL_DROWNING, DamageTolerance::Immune),
            make_tuple(DAM_LIGHT, DMG_TOL_LIGHT, DamageTolerance::Immune),
            make_tuple(DAM_OTHER, DMG_TOL_MAGIC, DamageTolerance::Immune),
            make_tuple(DAM_HARM, DMG_TOL_MAGIC, DamageTolerance::Immune),
        }));
        SECTION("expected immunity") {
            set_bit(ch.imm_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
        SECTION("immunity takes priority over resistance and vulnerability") {
            set_bit(ch.imm_flags, tolerance_bit);
            set_bit(ch.res_flags, tolerance_bit);
            set_bit(ch.vuln_flags, tolerance_bit);

            const auto result = check_damage_tolerance(&ch, dam_type);

            CHECK(result == expected);
        }
    }
    SECTION("cherrypicked resistance and vulnerability cases") {
        SECTION("resistant to acid") {
            set_bit(ch.res_flags, DMG_TOL_ACID);

            const auto result = check_damage_tolerance(&ch, DAM_ACID);

            CHECK(result == DamageTolerance::Resistant);
        }
        SECTION("vulnerable to acid") {
            set_bit(ch.vuln_flags, DMG_TOL_ACID);

            const auto result = check_damage_tolerance(&ch, DAM_ACID);

            CHECK(result == DamageTolerance::Vulnerable);
        }
    }
}