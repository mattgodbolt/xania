/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "Eliza.hpp"
#include "Database.hpp"
#include "KeywordResponses.hpp"
#include <unistd.h>

#include <catch2/catch.hpp>

using namespace chat;

TEST_CASE("production database sanity") {
    std::string_view chatdata_file = TEST_RESOURCE_DIR "/chat.data";
    Eliza eliza;

    SECTION("validate production chat database loads and responds to a basic lookup") {
        CHECK(eliza.load_databases(chatdata_file) == true);
        CHECK(eliza.handle_player_message("player", "what is your name?", "Heimdall") == "My name is \"Heimdall\"");
        CHECK(eliza.handle_player_message("player", "you remind me of a big yellow submarine", "Heimdall")
              == "In what way am I like a big yellow submarine?");
    }
}
