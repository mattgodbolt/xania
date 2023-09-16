/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Char.hpp"
#include "interp.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("apply prefix") {
    Char bob{};

    SECTION("npc prefix") {
        SECTION("command unmodified") {
            const auto cmd = apply_prefix(&bob, "north");

            CHECK(cmd == "north");
        }
    }
    SECTION("player prefix") {
        Descriptor bob_desc(1);
        bob.desc = &bob_desc;
        bob_desc.character(&bob);
        bob.pcdata = std::make_unique<PcData>();

        SECTION("prefix command") {
            const auto cmd = apply_prefix(&bob, "prefix");

            CHECK(cmd == "prefix");
        }
        SECTION("normal command, char has no prefix") {
            const auto cmd = apply_prefix(&bob, "north");

            CHECK(cmd == "north");
        }
        SECTION("normal command, char has prefix") {
            bob.pcdata->prefix = "hug ";
            const auto cmd = apply_prefix(&bob, "north");

            CHECK(cmd == "hug north");
            CHECK(bob.pcdata->prefix == "hug ");
        }
        SECTION("one backslash, no command, char has no prefix") {
            const auto cmd = apply_prefix(&bob, "\\");

            CHECK(cmd.empty());
        }
        SECTION("one backslash, command, char has no prefix") {
            const auto cmd = apply_prefix(&bob, "\\north");

            CHECK(cmd == "north");
        }
        SECTION("two backslashes, no command, char has no prefix") {
            const auto cmd = apply_prefix(&bob, "\\\\");

            CHECK(cmd.empty());
            CHECK(bob_desc.buffered_output() == "\n\r(no prefix to remove)\n\r");
        }
        SECTION("two backslashes, command, char has a prefix") {
            bob.pcdata->prefix = "hug ";
            const auto cmd = apply_prefix(&bob, "\\\\north");

            CHECK(cmd == "north");
            CHECK(bob_desc.buffered_output() == "\n\r(prefix removed)\n\r");
            CHECK(bob.pcdata->prefix == "");
        }
    }
}
