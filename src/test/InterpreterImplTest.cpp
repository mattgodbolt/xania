/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "InterpreterImpl.hpp"
#include "Char.hpp"
#include "DescriptorList.hpp"

#include <catch2/catch_test_macros.hpp>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};
Interpreter::Impl interp_impl{};

}

TEST_CASE("apply prefix") {
    Char bob{mock_mud};

    SECTION("npc prefix") {
        SECTION("command unmodified") {
            const auto cmd = interp_impl.apply_prefix(&bob, "north");

            CHECK(cmd == "north");
        }
    }
    SECTION("player prefix") {
        Descriptor bob_desc{1, mock_mud};
        bob.desc = &bob_desc;
        bob_desc.character(&bob);
        bob.pcdata = std::make_unique<PcData>();

        SECTION("prefix command") {
            const auto cmd = interp_impl.apply_prefix(&bob, "prefix");

            CHECK(cmd == "prefix");
        }
        SECTION("normal command, char has no prefix") {
            const auto cmd = interp_impl.apply_prefix(&bob, "north");

            CHECK(cmd == "north");
        }
        SECTION("normal command, char has prefix") {
            bob.pcdata->prefix = "hug ";
            const auto cmd = interp_impl.apply_prefix(&bob, "north");

            CHECK(cmd == "hug north");
            CHECK(bob.pcdata->prefix == "hug ");
        }
        SECTION("one backslash, no command, char has no prefix") {
            const auto cmd = interp_impl.apply_prefix(&bob, "\\");

            CHECK(cmd.empty());
        }
        SECTION("one backslash, command, char has no prefix") {
            const auto cmd = interp_impl.apply_prefix(&bob, "\\north");

            CHECK(cmd == "north");
        }
        SECTION("two backslashes, no command, char has no prefix") {
            const auto cmd = interp_impl.apply_prefix(&bob, "\\\\");

            CHECK(cmd.empty());
            CHECK(bob_desc.buffered_output() == "\n\r(no prefix to remove)\n\r");
        }
        SECTION("two backslashes, command, char has a prefix") {
            bob.pcdata->prefix = "hug ";
            const auto cmd = interp_impl.apply_prefix(&bob, "\\\\north");

            CHECK(cmd == "north");
            CHECK(bob_desc.buffered_output() == "\n\r(prefix removed)\n\r");
            CHECK(bob.pcdata->prefix == "");
        }
    }
}
