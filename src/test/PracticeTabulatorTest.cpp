/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "PracticeTabulator.hpp"
#include "Char.hpp"
#include "DescriptorList.hpp"
#include "lookup.h"

#include <catch2/catch_test_macros.hpp>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};

}

TEST_CASE("practice tabulator") {
    Char vic{mock_mud};
    Descriptor vic_desc{0, mock_mud};
    vic.desc = &vic_desc;
    vic_desc.character(&vic);
    vic.pcdata = std::make_unique<PcData>();
    vic.level = 5;
    vic.practice = 2;

    Char bob{mock_mud};
    Descriptor bob_desc{1, mock_mud};
    bob.desc = &bob_desc;
    bob_desc.character(&bob);
    bob.pcdata = std::make_unique<PcData>();
    bob.level = 5;
    bob.practice = 1;

    SECTION("tabulate one learned skill") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        PracticeTabulator::tabulate(&vic, &bob);

        CHECK(vic_desc.buffered_output() == "\n\raxe                100%\n\rThey have 1 practice sessions left.\n\r");
    }
    SECTION("tabulate three learned skills") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        bob.pcdata->learned[skill_lookup("sword")] = 50;
        bob.pcdata->learned[skill_lookup("polearm")] = 75;
        PracticeTabulator::tabulate(&vic, &bob);

        CHECK(vic_desc.buffered_output()
              == "\n\raxe                100% polearm             75% sword               50%\n\rThey have 1 practice "
                 "sessions left.\n\r");
    }
    SECTION("tabulate one learned skill and unavailable skill not listed") {
        bob.pcdata->learned[skill_lookup("axe")] = 100;
        bob.pcdata->learned[skill_lookup("bash")] = 50; // level 60 skill for bob's class
        PracticeTabulator::tabulate(&vic, &bob);

        CHECK(vic_desc.buffered_output() == "\n\raxe                100%\n\rThey have 1 practice sessions left.\n\r");
    }
    SECTION("tabulate three learned skills for self") {
        vic.pcdata->learned[skill_lookup("axe")] = 100;
        vic.pcdata->learned[skill_lookup("sword")] = 50;
        vic.pcdata->learned[skill_lookup("polearm")] = 75;
        PracticeTabulator::tabulate(&vic, &vic);

        CHECK(vic_desc.buffered_output()
              == "\n\raxe                100% polearm             75% sword               50%\n\rYou have 2 practice "
                 "sessions left.\n\r");
    }
}
