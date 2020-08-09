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

    SECTION("should split lines") {
        using vs = std::vector<std::string>;
        SECTION("empty should be a single empty line") { CHECK(split_lines<vs>("") == vs{""}); }
        SECTION("handles a single unterminated line") { CHECK(split_lines<vs>("a string") == vs{"a string"}); }
        SECTION("handles a line then unterminated line") { CHECK(split_lines<vs>("one\ntwo") == vs{"one", "two"}); }
        SECTION("splits lines with LF") { CHECK(split_lines<vs>("first\nsecond\n") == vs{"first", "second"}); }
        SECTION("splits lines with CR") { CHECK(split_lines<vs>("first\rsecond\r") == vs{"first", "second"}); }
        SECTION("splits lines with LFCR") { CHECK(split_lines<vs>("first\n\rsecond\n\r") == vs{"first", "second"}); }
        SECTION("splits lines with CRLF") { CHECK(split_lines<vs>("first\r\nsecond\r\n") == vs{"first", "second"}); }
        SECTION("splits blank lines with LF") {
            CHECK(split_lines<vs>("first\n\nthird\n") == vs{"first", "", "third"});
        }
        SECTION("splits blank lines with CR") {
            CHECK(split_lines<vs>("first\r\rthird\r") == vs{"first", "", "third"});
        }
        SECTION("splits blank lines with LFCR") {
            CHECK(split_lines<vs>("first\n\r\n\rthird\n\r") == vs{"first", "", "third"});
        }
        SECTION("splits blank lines with CRLF") {
            CHECK(split_lines<vs>("first\r\n\r\nthird\r\n") == vs{"first", "", "third"});
        }
    }

    SECTION("lower_case") {
        SECTION("already lower") { CHECK(lower_case("already lowercase") == "already lowercase"); }
        SECTION("all upper") { CHECK(lower_case("I AM SHOUTY") == "i am shouty"); }
        SECTION("mixture") { CHECK(lower_case("I am A LITTLE shouty") == "i am a little shouty"); }
        SECTION("non-letters") { CHECK(lower_case("G0RD0N'5 AL111111V3!!!!!!") == "g0rd0n'5 al111111v3!!!!!!"); }
    }

    SECTION("has_prefix") {
        SECTION("prefix") { CHECK(has_prefix("boolean", "bool")); }
        SECTION("full match") { CHECK(has_prefix("integer", "integer")); }
        SECTION("suffix") { CHECK(!has_prefix("float", "at")); }
        SECTION("gibberish") { CHECK(!has_prefix("boson", "fermion")); }
        SECTION("needle larger than haystack") { CHECK(!has_prefix("bool", "boolean")); }
    }

    SECTION("colourisation") {
        SECTION("should handle normal text") {
            CHECK(colourise_mud_string(false, "A normal piece of text") == "A normal piece of text");
            CHECK(colourise_mud_string(true, "A normal piece of text") == "A normal piece of text");
        }
        SECTION("should handle empty text") {
            CHECK(colourise_mud_string(false, "") == "");
            CHECK(colourise_mud_string(true, "") == "");
        }
        SECTION("should handle control codes") {
            CHECK(colourise_mud_string(false, "|Rred|Ggreen|Bblue|rred|ggreen|bblue") == "redgreenblueredgreenblue");
            CHECK(colourise_mud_string(true, "|Rred|Ggreen|Bblue|rred|ggreen|bblue")
                  == "\033[1;31mred\033[1;32mgreen\033[1;34mblue"
                     "\033[0;31mred\033[0;32mgreen\033[0;34mblue");
        }
        SECTION("should handle trailing pipes") {
            CHECK(colourise_mud_string(false, "oh no|") == "oh no");
            CHECK(colourise_mud_string(true, "oh no|") == "oh no");
        }
        SECTION("should escape pipes") {
            CHECK(colourise_mud_string(false, "||") == "|");
            CHECK(colourise_mud_string(true, "||") == "|");
        }
        SECTION("Should ignore unknowns") {
            CHECK(colourise_mud_string(false, "|z") == "z");
            CHECK(colourise_mud_string(true, "|z") == "z");
        }
    }
}
