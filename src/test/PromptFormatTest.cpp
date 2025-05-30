#include "PromptFormat.hpp"
#include "Act.hpp"

#include "Char.hpp"
#include "MockMud.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

test::MockMud mock_mud{};

}

TEST_CASE("prompt formatting") {
    Char ch{mock_mud};
    ch.wait = 3;
    ch.hit = 10;
    ch.max_hit = 11;
    ch.move = 13;
    ch.max_move = 14;
    ch.mana = 15;
    ch.max_mana = 16;
    SECTION("default prompt") { CHECK(format_prompt(ch, "") == "|p<10/11hp 15/16m 13mv 3cd> |w"); }
    SECTION("arbitrary text") {
        CHECK(format_prompt(ch, "moose") == "|pmoose|w");
        CHECK(format_prompt(ch, " with spaces ") == "|p with spaces |w");
    }
    SECTION("escaped percents") { CHECK(format_prompt(ch, "100%% awesome") == "|p100% awesome|w"); }
    SECTION("power bars") {
        SECTION("full") {
            ch.hit = 10;
            ch.max_hit = ch.hit;
            // With ten gradations, we expect 3 pipes of red, 3 of yellow, then four of green. Pipes are doubled up
            // to escape them.
            CHECK(format_prompt(ch, "%B> ")
                  // red
                  == "|p|r"
                     // 3 pipes
                     "||||||"
                     // yellow
                     "|y"
                     // 3 pipes
                     "||||||"
                     // green
                     "|g"
                     // 4 pipes
                     "||||||||"
                     // rest
                     "|p> |w");
        }
        SECTION("25%") {
            ch.hit = 444;
            ch.max_hit = ch.hit * 4;
            // 25% should mean 2.5 pipes. half a pipe or less is a dot
            //                                      112234567890
            CHECK(format_prompt(ch, "%B> ") == "|p|r||||.       |p> |w");
        }
        SECTION("29%") {
            ch.hit = 29;
            ch.max_hit = 100;
            // 29% should mean 2.9 pipes. Less than a whole pipe is a :
            //                                      112234567890
            CHECK(format_prompt(ch, "%B> ") == "|p|r||||:       |p> |w");
        }
        SECTION("21%") {
            ch.hit = 21;
            ch.max_hit = 100;
            // 29% should mean 2.1 pipes. Less than a quarter pipe is empty
            //                                      112234567890
            CHECK(format_prompt(ch, "%B> ") == "|p|r||||        |p> |w");
        }
        SECTION("99.99%") {
            ch.hit = 9999;
            ch.max_hit = 10000;
            CHECK(format_prompt(ch, "%B> ")
                  == "|p|r"
                     "||||||"
                     "|y"
                     "||||||"
                     "|g"
                     "||||||:"
                     "|p> |w");
        }
    }
}
