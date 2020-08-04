#include "interp.h"

#include <catch2/catch.hpp>

#include <string_view>
using namespace std::literals;

TEST_CASE("Interpreter tests") {
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
}
