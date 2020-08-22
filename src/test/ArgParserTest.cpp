#include "ArgParser.hpp"

#include "catch2/catch.hpp"

#include <algorithm>
#include <vector>

using namespace std::literals;

TEST_CASE("Argument parsing") {
    SECTION("should treat empty strings as such") {
        ArgParser ap(""sv);
        CHECK(ap.empty());
        SECTION("should pop empty string") { CHECK(ap.pop_argument().empty()); }
        SECTION("should pop empty number arg") {
            auto arg = ap.pop_number_argument();
            CHECK(arg.argument.empty());
            CHECK(arg.number == 0);
        }
    }
    SECTION("should parse arguments") {
        ArgParser ap("this is a test"sv);
        CHECK(ap.pop_argument() == "this");
        CHECK(ap.pop_argument() == "is");
        CHECK(ap.pop_argument() == "a");
        CHECK(ap.pop_argument() == "test");
        CHECK(ap.empty());
    }
    SECTION("should remove extraneous whitespace") {
        ArgParser ap("   this    is  a  test     "sv);
        CHECK(ap.pop_argument() == "this");
        CHECK(ap.pop_argument() == "is");
        CHECK(ap.pop_argument() == "a");
        CHECK(ap.pop_argument() == "test");
        CHECK(ap.empty());
    }
    SECTION("should parse quotes") {
        ArgParser ap(R"('once' "upon" "a time" 'there was a' "scary 'witch'")"sv);
        CHECK(ap.pop_argument() == "once");
        CHECK(ap.pop_argument() == "upon");
        CHECK(ap.pop_argument() == "a time");
        CHECK(ap.pop_argument() == "there was a");
        CHECK(ap.pop_argument() == "scary 'witch'");
        CHECK(ap.empty());
    }
    SECTION("should handle apostrophes") {
        ArgParser ap(R"(It's lucky we handle these, Yie'Stei)"sv);
        CHECK(ap.pop_argument() == "It's");
        CHECK(ap.pop_argument() == "lucky");
        CHECK(ap.pop_argument() == "we");
        CHECK(ap.pop_argument() == "handle");
        CHECK(ap.pop_argument() == "these,");
        CHECK(ap.pop_argument() == "Yie'Stei");
        CHECK(ap.empty());
    }
    SECTION("should handle unmatched quotes") {
        ArgParser ap("'oh no"sv);
        CHECK(ap.pop_argument() == "oh no");
        CHECK(ap.empty());
    }
    SECTION("should be iterable") {
        ArgParser ap(R"(this is a "test of" iteration)"sv);
        std::vector<std::string_view> parts;
        std::copy(begin(ap), end(ap), std::back_inserter(parts));
        CHECK(parts == std::vector<std::string_view>{"this"sv, "is"sv, "a"sv, "test of"sv, "iteration"sv});
    }
    SECTION("should pop number args") {
        ArgParser ap("12.monkeys"sv);
        auto arg = ap.pop_number_argument();
        CHECK(arg.number == 12);
        CHECK(arg.argument == "monkeys");
    }
}