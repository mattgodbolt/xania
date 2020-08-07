#include "string_utils.hpp"

#include "merc.h" // TODO remove once we test just the pair<> version
#include <catch2/catch.hpp>

#include <string_view>
using namespace std::literals;

TEST_CASE("string_util tests") {
    SECTION("should determine numbers") {
        CHECK(is_number("0"));
        CHECK(is_number("123"));
        CHECK(is_number("-123"));
        CHECK(is_number("+123"));
        CHECK(is_number("0"));
        CHECK(!is_number("a"));
        CHECK(!is_number("1-1"));
        CHECK(!is_number("-+123"));
        CHECK(!is_number("++123"));
        CHECK(!is_number("--123"));
        CHECK(!is_number(""));
    }

    SECTION("should split numbers and arguments") {
        // TODO check pair version
        char buf[MAX_STRING_LENGTH];
        SECTION("with just a name") {
            char str[] = "object";
            CHECK(number_argument(str, buf) == 1);
            CHECK(buf == "object"sv);
        }
        SECTION("with a name and a number") {
            SECTION("1") {
                char str[] = "1.first";
                CHECK(number_argument(str, buf) == 1);
                CHECK(buf == "first"sv);
            }
            SECTION("negative") {
                char str[] = "-1.wtf";
                CHECK(number_argument(str, buf) == -1);
                CHECK(buf == "wtf"sv);
            }
            SECTION("22") {
                char str[] = "22.twentysecond";
                CHECK(number_argument(str, buf) == 22);
                CHECK(buf == "twentysecond"sv);
                SECTION("and the original string is left untouched") { CHECK(str == "22.twentysecond"sv); }
            }
        }
        SECTION("garbage non-numbers") {
            char str[] = "spider.pig";
            CHECK(number_argument(str, buf) == 0);
            CHECK(buf == "pig"sv);
        }
    }

    SECTION("skips whitespace") {
        SECTION("should not skip non-whitespace") {
            CHECK(skip_whitespace("") == "");
            CHECK(skip_whitespace("monkey") == "monkey");
            CHECK(skip_whitespace("monkey  ") == "monkey  ");
        }
        SECTION("should skip actual whitespace") {
            CHECK(skip_whitespace("   badger") == "badger");
            CHECK(skip_whitespace("   badger  ") == "badger  ");
            CHECK(skip_whitespace("   bad ger") == "bad ger");
            CHECK(skip_whitespace(" \t\nbadger") == "badger");
        }
        SECTION("should work with entirely whitespace") {
            CHECK(skip_whitespace("   ") == "");
            CHECK(skip_whitespace(" \n\n\r") == "");
            CHECK(skip_whitespace("\t\t") == "");
        }
    }

    SECTION("smashes tildes") {
        CHECK(smash_tilde("moose") == "moose");
        CHECK(smash_tilde("m~~se") == "m--se");
    }

    SECTION("removes last lines") {
        SECTION("should preserve empty line") { CHECK(remove_last_line("") == ""); }
        SECTION("should trim tiny non-terminated") {
            CHECK(remove_last_line("a") == "");
            CHECK(remove_last_line("ab") == "");
            CHECK(remove_last_line("abc") == "");
        }
        SECTION("should trim tiny terrminated") {
            CHECK(remove_last_line("\n\r") == "");
            CHECK(remove_last_line("\n\r\n\r") == "\n\r");
            CHECK(remove_last_line("a\n\r") == "");
        }
        SECTION("should trim long non-terminated") { CHECK(remove_last_line("This is a longer line") == ""); }
        SECTION("should trim single line") { CHECK(remove_last_line("This is a single line\n\r") == ""); }
        SECTION("should trim last complete line") {
            CHECK(remove_last_line("This is a line\n\rThis is the second line\n\rAnd the last\n\r")
                  == "This is a line\n\rThis is the second line\n\r");
        }
        SECTION("should trim partial last line") {
            CHECK(remove_last_line("This is a line\n\rThis is the second line\n\rAnd I didn't finish the newline")
                  == "This is a line\n\rThis is the second line\n\r");
        }
    }

    SECTION("sanitizes text input") {
        SECTION("handles simple case") { CHECK(sanitise_input("I am a fish") == "I am a fish"); }
        SECTION("preserves leading and trailing whitespace") {
            CHECK(sanitise_input("  I am a fish  ") == "  I am a fish  ");
        }
        SECTION("strips trailing CRLF") { CHECK(sanitise_input("Some string   \r\n") == "Some string   "); }
        SECTION("handles empty strings") { CHECK(sanitise_input("") == ""); }
        SECTION("handles empty strings that are just non-printing") { CHECK(sanitise_input("\n\r\t") == ""); }
        SECTION("removes non-printing") { CHECK(sanitise_input("arg\tl\u00ffe") == "argle"); }
        SECTION("removes backspaces") { CHECK(sanitise_input("TheMa\boog sucks\b\b\b\b\b\b") == "TheMoog"); }
        SECTION("removes backspaces after non-printing") { CHECK(sanitise_input("Oops\xff\b!") == "Oops!"); }
    }
}
