/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "VampiricDrain.hpp"
#include "Alignment.hpp"
#include "Char.hpp"
#include "ToleranceFlag.hpp"
#include "common/BitOps.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <tuple>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};

}

TEST_CASE("vampiric drain amount") {
    using std::make_tuple;
    SECTION("5% of the damage dealt, rounded") {
        int dealt, expected;
        std::tie(dealt, expected) = GENERATE(table<int, int>({
            make_tuple(-5, 0),
            make_tuple(0, 0),
            make_tuple(9, 0),
            make_tuple(10, 1),
            make_tuple(14, 1), // the mean hit of the lowest-level vampiric weapon at level 20
            make_tuple(50, 3),
            make_tuple(99, 5), // drained 0 before: (99 / 100) * 10
            make_tuple(100, 5),
            make_tuple(132, 7),
        }));
        CHECK(VampiricDrain::amount(dealt, DamageTolerance::None) == expected);
    }
    SECTION("negative energy tolerance, as damage() applies tolerances") {
        CHECK(VampiricDrain::amount(100, DamageTolerance::Immune) == 0);
        CHECK(VampiricDrain::amount(100, DamageTolerance::Resistant) == 4);
        CHECK(VampiricDrain::amount(100, DamageTolerance::Vulnerable) == 7);
    }
}

TEST_CASE("vampiric drain") {
    Char wielder{mock_mud};
    Char victim{mock_mud};
    wielder.max_hit = 500;
    wielder.hit = 300;
    wielder.alignment = Alignment::Neutral;
    victim.max_hit = 1000;
    victim.hit = 900;

    SECTION("moves the drained hit points from the victim to the wielder") {
        CHECK(VampiricDrain::drain(wielder, victim, 100) == 5);
        CHECK(victim.hit == 895);
        CHECK(wielder.hit == 305);
    }
    SECTION("costs a point of alignment") {
        VampiricDrain::drain(wielder, victim, 100);
        CHECK(wielder.alignment == Alignment::Neutral - 1);
    }
    SECTION("alignment stops at satanic") {
        wielder.alignment = Alignment::Satanic;
        VampiricDrain::drain(wielder, victim, 100);
        CHECK(wielder.alignment == Alignment::Satanic);
    }
    SECTION("heals no further than max hit points") {
        wielder.hit = 498;
        CHECK(VampiricDrain::drain(wielder, victim, 100) == 5);
        CHECK(wielder.hit == 500);
        CHECK(victim.hit == 895);
    }
    SECTION("doesn't pull a wielder already over max back down") {
        wielder.hit = 520;
        VampiricDrain::drain(wielder, victim, 100);
        CHECK(wielder.hit == 520);
    }
    SECTION("drains nothing, and costs nothing, from a victim immune to negative energy") {
        set_enum_bit(victim.imm_flags, ToleranceFlag::Negative);
        CHECK(VampiricDrain::drain(wielder, victim, 100) == 0);
        CHECK(victim.hit == 900);
        CHECK(wielder.hit == 300);
        CHECK(wielder.alignment == Alignment::Neutral);
    }
    SECTION("drains a third less from a victim resistant to negative energy") {
        set_enum_bit(victim.res_flags, ToleranceFlag::Negative);
        CHECK(VampiricDrain::drain(wielder, victim, 100) == 4);
        CHECK(victim.hit == 896);
    }
}
