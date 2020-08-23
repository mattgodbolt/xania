#include "ArgParser.hpp"

#include "catch2/catch.hpp"

#include <algorithm>
#include <vector>

using namespace std::literals;

TEST_CASE("Argument parsing") {
    SECTION("should treat empty strings as such") {
        ArgParser ap(""sv);
        CHECK(ap.empty());
        SECTION("should shift empty string") { CHECK(ap.shift().empty()); }
        SECTION("should shift empty number arg") {
            auto arg = ap.shift_number();
            CHECK(arg.argument.empty());
            CHECK(arg.number == 0);
        }
    }
    SECTION("should parse arguments") {
        ArgParser ap("this is a test"sv);
        CHECK(ap.shift() == "this");
        CHECK(ap.shift() == "is");
        CHECK(ap.shift() == "a");
        CHECK(ap.shift() == "test");
        CHECK(ap.empty());
    }
    SECTION("should remove extraneous whitespace") {
        ArgParser ap("   this    is  a  test     "sv);
        CHECK(ap.shift() == "this");
        CHECK(ap.shift() == "is");
        CHECK(ap.shift() == "a");
        CHECK(ap.shift() == "test");
        CHECK(ap.empty());
    }
    SECTION("should parse quotes") {
        ArgParser ap(R"('once' "upon" "a time" 'there was a' "scary 'witch'")"sv);
        CHECK(ap.shift() == "once");
        CHECK(ap.shift() == "upon");
        CHECK(ap.shift() == "a time");
        CHECK(ap.shift() == "there was a");
        CHECK(ap.shift() == "scary 'witch'");
        CHECK(ap.empty());
    }
    SECTION("should handle apostrophes") {
        ArgParser ap(R"(It's lucky we handle these, Yie'Stei)"sv);
        CHECK(ap.shift() == "It's");
        CHECK(ap.shift() == "lucky");
        CHECK(ap.shift() == "we");
        CHECK(ap.shift() == "handle");
        CHECK(ap.shift() == "these,");
        CHECK(ap.shift() == "Yie'Stei");
        CHECK(ap.empty());
    }
    SECTION("should handle unmatched quotes") {
        ArgParser ap("'oh no"sv);
        CHECK(ap.shift() == "oh no");
        CHECK(ap.empty());
    }
    SECTION("should be iterable") {
        ArgParser ap(R"(this is a "test of" iteration)"sv);
        std::vector<std::string_view> parts;
        std::copy(begin(ap), end(ap), std::back_inserter(parts));
        CHECK(parts == std::vector<std::string_view>{"this"sv, "is"sv, "a"sv, "test of"sv, "iteration"sv});
    }
    SECTION("should shift number args") {
        ArgParser ap("12.monkeys"sv);
        auto arg = ap.shift_number();
        CHECK(arg.number == 12);
        CHECK(arg.argument == "monkeys");
    }
}