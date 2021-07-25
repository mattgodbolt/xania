/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <catch2/catch.hpp>

TEST_CASE("serialize flags") {
    SECTION("no flags set") {
        const auto result = serialize_flags(0);

        CHECK(result == "0");
    }
    SECTION("A or B") {
        const auto result = serialize_flags(A | B);

        CHECK(result == "AB");
    }
    SECTION("Z or a or f") {
        const auto result = serialize_flags(Z | aa | ff);

        CHECK(result == "Zaf");
    }
    SECTION("all flags") {
        auto all_bits = 0;
        for (auto i = 0; i < 32; i++) {
            set_bit(all_bits, 1 << i);
        }
        const auto result = serialize_flags(all_bits);

        CHECK(result == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef");
    }
}

TEST_CASE("get flag bit by name") {
    std::array<Flag, 3> flags = {{{A, 0, "first"}, {B, 0, "second"}, {C, 100, "third"}}};
    SECTION("first") {
        auto result = get_flag_bit_by_name(flags, "first", 1);

        CHECK(result == A);
    }
    SECTION("second") {
        auto result = get_flag_bit_by_name(flags, "second", 1);

        CHECK(result == B);
    }
    SECTION("third not found due to trust") {
        auto result = get_flag_bit_by_name(flags, "third", 99);

        CHECK(!result);
    }
}

TEST_CASE("flag set") {
    Char admin{};
    Descriptor admin_desc(0);
    admin.desc = &admin_desc;
    admin_desc.character(&admin);
    admin.pcdata = std::make_unique<PcData>();
    admin.level = 99;
    std::array<Flag, 3> flags = {{{A, 0, "first"}, {B, 0, "second"}, {C, 100, "third"}}};
    unsigned long current_val = 0;
    SECTION("first toggled on") {
        auto args = ArgParser("first");
        auto result = flag_set(flags, args, current_val, &admin);

        CHECK(check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\rfirst\n\r");
    }
    SECTION("first toggled") {
        set_bit(current_val, A);
        auto args = ArgParser("first");
        auto result = flag_set(flags, args, current_val, &admin);

        CHECK(!check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\r\n\r");
    }
    SECTION("first set on") {
        auto args = ArgParser("+first");
        auto result = flag_set(flags, args, current_val, &admin);

        CHECK(check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\rfirst\n\r");
    }
    SECTION("first set off") {
        set_bit(current_val, A);
        auto args = ArgParser("-first");
        auto result = flag_set(flags, args, current_val, &admin);

        CHECK(!check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\r\n\r");
    }
    SECTION("unrecognized flag name") {
        auto args = ArgParser("zero");
        auto result = flag_set(flags, args, current_val, &admin);

        CHECK(result == 0);
        CHECK(admin_desc.buffered_output() == "\n\r\n\rAvailable flags:\n\rfirst second\n\r");
    }
    SECTION("insufficient trust for flag") {
        auto args = ArgParser("third");
        auto result = flag_set(flags, args, current_val, &admin);

        CHECK(result == 0);
        CHECK(admin_desc.buffered_output() == "\n\r\n\rAvailable flags:\n\rfirst second\n\r");
    }
}
