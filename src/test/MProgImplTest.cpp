/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#include "MProgImpl.hpp"

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
    SECTION("function with arg, op and number operand") {
        auto expr = "func(arg) > 1";
        auto expected = IfExpr{"func", "arg", ">", 1};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
        CHECK(std::holds_alternative<const int>(result->operand));
        CHECK(std::get<const int>(result->operand) == 1);
    }
    SECTION("function with arg, op and number operand ignore extra space") {
        auto expr = "func(arg)  >  1 ";
        auto expected = IfExpr{"func", "arg", ">", 1};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
        CHECK(std::holds_alternative<const int>(result->operand));
        CHECK(std::get<const int>(result->operand) == 1);
    }
    SECTION("function with arg, op and string operand ignore extra space") {
        auto expr = "func(arg)  ==  text ";
        auto expected = IfExpr{"func", "arg", "==", "text"};

        auto result = IfExpr::parse_if(expr);
        CHECK(*result == expected);
        CHECK(std::holds_alternative<std::string_view>(result->operand));
        CHECK(std::get<std::string_view>(result->operand) == "text");
    }
}
