/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "Database.hpp"
#include "KeywordResponses.hpp"

#include <catch2/catch.hpp>

using namespace chat;

static constexpr std::string_view expected_default_response = "I don't really know much about that, say more.";

TEST_CASE("find response simple") {
    KeywordResponses key1{"(key1)"};
    key1.add_response(9, "key1 response");

    KeywordResponses key2{"(key2)"};
    key2.add_response(9, "key2 response");
    std::vector<KeywordResponses> db1_responses = {key1, key2};
    Database db1{db1_responses, nullptr};
    int overflow = 10;

    SECTION("first keyword found") {
        CHECK(db1.find_response("player", "key1 trigger!", "npc", overflow) == "key1 response");
    }
    SECTION("first keyword found within") {
        CHECK(db1.find_response("player", "trigger key1!", "npc", overflow) == "key1 response");
    }
    SECTION("second keyword found") { CHECK(db1.find_response("player", "key2", "npc", overflow) == "key2 response"); }
    SECTION("second keyword found case insensitive") {
        CHECK(db1.find_response("player", "KEY2", "npc", overflow) == "key2 response");
    }
}

TEST_CASE("find response from linked database") {
    KeywordResponses key1{"(key1)"};
    key1.add_response(9, "key1 response");

    KeywordResponses key2{"(key2)"};
    key2.add_response(9, "key2 response");

    std::vector<KeywordResponses> responses = {key1, key2};
    Database db2{responses, nullptr};

    std::vector<KeywordResponses> db1_responses = {};
    // db1 links forward to db2
    Database db1{db1_responses, &db2};
    int overflow = 10;

    SECTION("first keyword found and that walking db links decrements overflow") {
        CHECK(db1.find_response("player", "key1 trigger!", "npc", overflow) == "key1 response");
        CHECK(overflow == 9);
    }
    SECTION("first keyword found within") {
        CHECK(db1.find_response("player", "trigger key1!", "npc", overflow) == "key1 response");
    }
    SECTION("second keyword found within") {
        CHECK(db1.find_response("player", "key2 trigger!", "npc", overflow) == "key2 response");
    }
    SECTION("second keyword found case insensitive") {
        CHECK(db1.find_response("player", "KEY2 trigger!", "npc", overflow) == "key2 response");
    }
}

TEST_CASE("find response orred keywords") {
    KeywordResponses key1{"(key1|key2)"};
    key1.add_response(9, "key1|key2 response");

    std::vector<KeywordResponses> db1_responses = {key1};
    Database db1{db1_responses, nullptr};
    int overflow = 10;

    SECTION("first keyword found") {
        CHECK(db1.find_response("player", "key1", "npc", overflow) == "key1|key2 response");
    }
    SECTION("first keyword found within") {
        CHECK(db1.find_response("player", "trigger key1", "npc", overflow) == "key1|key2 response");
    }
    SECTION("second keyword found") {
        CHECK(db1.find_response("player", "key2", "npc", overflow) == "key1|key2 response");
    }
    SECTION("second keyword found case insensitive") {
        CHECK(db1.find_response("player", "KEY2", "npc", overflow) == "key1|key2 response");
    }
}

TEST_CASE("find response anded keywords") {
    KeywordResponses key1{"(key1&key2)"};
    key1.add_response(9, "key1&key2 response");

    KeywordResponses key345{"(key3&key4&key5)"};
    key345.add_response(9, "key3&key4&key5 response");

    std::vector<KeywordResponses> db1_responses = {key1, key345};
    Database db1{db1_responses, nullptr};
    int overflow = 10;

    SECTION("both keywords found") {
        CHECK(db1.find_response("player", "key1 and key2", "npc", overflow) == "key1&key2 response");
    }
    SECTION("one of two keywords not found") {
        CHECK(db1.find_response("player", "key1 and key 2", "npc", overflow) == expected_default_response);
    }
    SECTION("three keywords found") {
        CHECK(db1.find_response("player", "key3 with key4 and key5", "npc", overflow) == "key3&key4&key5 response");
    }
    SECTION("one of three keywords not found") {
        CHECK(db1.find_response("player", "key3 with key4 and not key 5", "npc", overflow)
              == expected_default_response);
    }
}

TEST_CASE("find response and not keywords") {
    KeywordResponses key1{"(key1~key2)"};
    key1.add_response(9, "key1~key2 response");

    std::vector<KeywordResponses> db1_responses = {key1};
    Database db1{db1_responses, nullptr};
    int overflow = 10;

    SECTION("xor failure") {
        CHECK(db1.find_response("player", "key1 key2", "npc", overflow) == expected_default_response);
    }
    SECTION("first keyword found not second") {
        CHECK(db1.find_response("player", "key1 foo", "npc", overflow) == "key1~key2 response");
    }
}

TEST_CASE("variable substitutions") {
    // Some keywords that trigger responses containing variables that will get substituted.
    KeywordResponses key1{"(playername)"};
    key1.add_response(9, "$t");
    KeywordResponses key2{"(npcname)"};
    key2.add_response(9, "$n");
    KeywordResponses key3{"(remainder)"};
    key3.add_response(9, "$r");
    KeywordResponses key4{"(dollar)"};
    key4.add_response(9, "$$");
    KeywordResponses key5{"(helpversion)"};
    key5.add_response(9, "$V");
    KeywordResponses key6{"(compiletime)"};
    key6.add_response(9, "$C");
    KeywordResponses key7{"(author)"};
    key7.add_response(9, "$A");

    std::vector<KeywordResponses> db1_responses = {key1, key2, key3, key4, key5, key6, key7};
    Database db1{db1_responses, nullptr};
    int overflow = 10;

    SECTION("echo player name") { CHECK(db1.find_response("player", "playername", "npc", overflow) == "player"); }
    SECTION("echo npc name") { CHECK(db1.find_response("player", "npcname", "npc", overflow) == "npc"); }
    SECTION("echo remainder") {
        CHECK(db1.find_response("player", "remainder tell them", "npc", overflow) == "tell them");
    }
    SECTION("echo dollar") { CHECK(db1.find_response("player", "dollar", "npc", overflow) == "$"); }
    SECTION("echo help version") {
        CHECK(db1.find_response("player", "helpversion", "npc", overflow)
              == "The version number can be seen using 'help version'.");
    }
    SECTION("echo compiletime") {
        CHECK(db1.find_response("player", "compiletime", "npc", overflow) == __DATE__ " " __TIME__);
    }
    SECTION("echo author") {
        CHECK(db1.find_response("player", "author", "npc", overflow)
              == "Originally by Christopher Busch  Copyright (c)1993. Rewritten by the Xania team.");
    }
}
