/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "SkillsTabulator.hpp"
#include "Char.hpp"
#include "lookup.h"

#include <catch2/catch.hpp>

TEST_CASE("skills tabulator") {
    Char vic{};
    Descriptor vic_desc(0);
    vic.desc = &vic_desc;
    vic_desc.character(&vic);
    vic.pcdata = std::make_unique<PcData>();

    Char bob{};
    Descriptor bob_desc(1);
    bob.desc = &bob_desc;
    bob_desc.character(&bob);
    bob.pcdata = std::make_unique<PcData>();
    bob.level = 59; // so Bash isn't available yet.

    SECTION("tabulate one learned skill") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        const auto tabulator = SkillsTabulator::skills(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(vic_desc.buffered_output() == "\n\rL 1: axe                100%     \n\r");
    }
    SECTION("tabulate two learned skills and one not available yet") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        bob.pcdata->learned[skill_lookup("sword")] = 100;
        bob.pcdata->learned[skill_lookup("bash")] = 75;
        const auto tabulator = SkillsTabulator::skills(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(vic_desc.buffered_output()
              == "\n\rL 1: axe                100%           sword              100%     \n\rL60: bash                "
                 "n/a     \n\r");
    }
    SECTION("tabulate three learned skills") {
        bob.level = 60;
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        bob.pcdata->learned[skill_lookup("sword")] = 100;
        bob.pcdata->learned[skill_lookup("bash")] = 75;
        const auto tabulator = SkillsTabulator::skills(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(vic_desc.buffered_output()
              == "\n\rL 1: axe                100%           sword              100%     \n\rL60: bash                "
                 "75%     \n\r");
    }
    SECTION("tabulate one learned skill and spells skipped") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        bob.pcdata->learned[skill_lookup("acid blast")] = 100;
        const auto tabulator = SkillsTabulator::skills(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(vic_desc.buffered_output() == "\n\rL 1: axe                100%     \n\r");
    }
    SECTION("tabulate one learned skill to self") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        const auto tabulator = SkillsTabulator::skills(&bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(bob_desc.buffered_output() == "\n\rL 1: axe                100%     \n\r");
    }
    SECTION("no skills") {
        const auto tabulator = SkillsTabulator::skills(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(!result);
        CHECK(vic_desc.buffered_output() == "");
    }
    SECTION("tabulate one learned spell") {
        bob.pcdata->learned[skill_lookup("chill touch")] = 100;
        const auto tabulator = SkillsTabulator::spells(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(vic_desc.buffered_output() == "\n\rL 4: chill touch         15m 100%\n\r");
    }
    SECTION("tabulate one learned spell and skills skipped") {
        bob.pcdata->learned[skill_lookup("chill touch")] = 100;
        bob.pcdata->learned[skill_lookup("sword")] = 100;
        const auto tabulator = SkillsTabulator::spells(&vic, &bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(vic_desc.buffered_output() == "\n\rL 4: chill touch         15m 100%\n\r");
    }
    SECTION("tabulate one learned spell to self") {
        bob.pcdata->learned[skill_lookup("chill touch")] = 100;
        const auto tabulator = SkillsTabulator::spells(&bob);
        const auto result = tabulator.tabulate();

        CHECK(result);
        CHECK(bob_desc.buffered_output() == "\n\rL 4: chill touch         15m 100%\n\r");
    }
}