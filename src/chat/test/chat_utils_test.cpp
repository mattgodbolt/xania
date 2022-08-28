/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "chat/chat_utils.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace chat;

TEST_CASE("strpos") {
    SECTION("db_keyword found within input_str") {
        CHECK(strpos("a", "a") == 1);
        CHECK(strpos("A", "a") == 1);
        CHECK(strpos("ab", "ab") == 2);
        CHECK(strpos("abc", "abc") == 3);
        CHECK(strpos("subs", "ubs") == 4);
        CHECK(strpos("spaced words ok", "words") == 12);
    }
    SECTION("db_keyword not found within input_str") {
        CHECK(strpos("a", "") == 0);
        CHECK(strpos("a", "b") == 0);
        CHECK(strpos("ab", "bb") == 0);
        CHECK(strpos("abc", "abb") == 0);
        CHECK(strpos("long", "longer") == 0);
    }
    SECTION("db_keyword matched at start of input_str") {
        CHECK(strpos("a", "^a") == 1);
        CHECK(strpos("A", "^a") == 1);
    }
    SECTION("db_keyword not matched at start of input_str") {
        CHECK(strpos("ba", "^a") == 0);
        CHECK(strpos("BA", "^a") == 0);
    }
    SECTION("db_keyword equals input_str") {
        CHECK(strpos("a", "=a") == 1);
        CHECK(strpos("A", "=a") == 1);
        CHECK(strpos("ab", "=ab") == 2);
    }
    SECTION("db_keyword not equals input_str") {
        CHECK(strpos("b", "=a") == 0);
        CHECK(strpos("aa", "=ab") == 0);
    }
}

TEST_CASE("swap pronouns and possessives") {
    SECTION("no matches") { CHECK(swap_pronouns_and_possessives("foo bar") == "foo bar"); }
    SECTION("single matches") {
        CHECK(swap_pronouns_and_possessives("i") == "you");
        CHECK(swap_pronouns_and_possessives("you bar") == "I bar");
        CHECK(swap_pronouns_and_possessives("mine is") == "yours is");
        CHECK(swap_pronouns_and_possessives("adjust myself") == "adjust yourself");
    }
    SECTION("multiple matches") {
        CHECK(swap_pronouns_and_possessives("you mine") == "I yours");
        // This demonstrates how unintelligent the grammar rules are...for now.
        CHECK(swap_pronouns_and_possessives("you are mine") == "I are yours");
    }
}
