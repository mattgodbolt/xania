/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#include "MemFile.hpp"
#include "MobIndexData.hpp"
#include "common/BitOps.hpp"
#include "mob_prog.hpp"

#include <catch2/catch.hpp>

TEST_CASE("IfExpr parse_if") {
    using namespace MProg::impl;
    SECTION("empty") {
        auto expr = "";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("whitespace only") {
        auto expr = "   ";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("no open paren") {
        auto expr = "func arg";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("no function name") {
        auto expr = "(arg)";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("no close paren") {
        auto expr = "func(";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("no function arg") {
        auto expr = "func()";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("function with arg") {
        auto expr = "func(arg)";
        auto expected = IfExpr{"func", "arg", "", ""};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
    }
    SECTION("function with arg ignore trailing space") {
        auto expr = "func(arg) ";
        auto expected = IfExpr{"func", "arg", "", ""};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
    }
    SECTION("function with arg ignore extra space") {
        auto expr = "func ( arg )";
        auto expected = IfExpr{"func", "arg", "", ""};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
    }
    SECTION("missing operand") {
        auto expr = "func(arg) >";

        auto result = IfExpr::parse_if(expr);
        CHECK(!result);
    }
    SECTION("function with arg, op and operand") {
        auto expr = "func(arg) > 1";
        auto expected = IfExpr{"func", "arg", ">", "1"};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
    }
    SECTION("function with arg, op and operand ignore extra space") {
        auto expr = "func(arg)  >  1 ";
        auto expected = IfExpr{"func", "arg", ">", "1"};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
    }
}

TEST_CASE("mob prog file reading") {
    using namespace MProg;
    test::MemFile orc(R"mob(
#13000
orc~
the orc shaman~
~
~
orc~
0 0 0 0
0 0 1d6+1 1d6+1 1d1+1 blast
0 0 0 0
0 0 0 0
stand stand male 200
0 0 medium 0

#0
)mob");
    auto opt_mob = MobIndexData::from_file(orc.file());
    CHECK(opt_mob);
    auto mob = *opt_mob;
    CHECK(mob.progtypes == 0);
    SECTION("well formed with one prog") {
        test::MemFile test_prog(">rand_prog 50~\n\rif rand(50)\n\r\tdance\n\relse\n\rn\tsing\n\rendif~\n\r|\n\r");
        auto result = read_program("test.prg", test_prog.file(), &mob);

        CHECK(result);
        CHECK(mob.mobprogs.size() == 1);
        const auto &prog = mob.mobprogs.at(0);
        CHECK(prog.type == MobProgTypeFlag::Random);
        CHECK(check_enum_bit(mob.progtypes, MobProgTypeFlag::Random));
    }
    SECTION("well formed with two progs") {
        test::MemFile test_prog(">rand_prog 50~\n\rif rand(50)\n\r\tdance\n\relse\n\rn\tsing\n\rendif~\n\r>greet_prog "
                                "50~\n\rwave~\n\r|\n\r");
        auto result = read_program("test.prg", test_prog.file(), &mob);

        CHECK(result);
        CHECK(mob.mobprogs.size() == 2);
        const auto &first = mob.mobprogs.at(0);
        CHECK(first.type == MobProgTypeFlag::Random);
        const auto &second = mob.mobprogs.at(1);
        CHECK(second.type == MobProgTypeFlag::Greet);
        CHECK(check_enum_bit(mob.progtypes, MobProgTypeFlag::Random));
        CHECK(check_enum_bit(mob.progtypes, MobProgTypeFlag::Greet));
    }
    SECTION("malformed empty") {
        test::MemFile test_prog("");
        auto result = read_program("test.prg", test_prog.file(), &mob);

        CHECK(!result);
        CHECK(mob.mobprogs.empty());
    }
    SECTION("malformed type error") {
        test::MemFile test_prog(">foo_prog 50~\n\rwave\n\r~\n\r|\n\r");
        auto result = read_program("test.prg", test_prog.file(), &mob);

        CHECK(!result);
        CHECK(mob.mobprogs.empty());
    }
    SECTION("malformed missing terminator") {
        test::MemFile test_prog(">foo_prog 50~\n\rwave\n\r~\n\r");
        auto result = read_program("test.prg", test_prog.file(), &mob);

        CHECK(!result);
        CHECK(mob.mobprogs.empty());
    }
}
