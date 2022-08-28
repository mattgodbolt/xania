#include "CommandSet.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_version_macros.hpp>

#include <sstream>
#include <string_view>

namespace {

template <typename T>
std::string enumerate_str(const CommandSet<T> &cs, int min_level, int max_level) {
    std::ostringstream result;
    cs.enumerate(cs.level_restrict(min_level, max_level, [&result](const std::string &name, const T value, int level) {
        result << name << "=" << value << "@" << level << " ";
    }));
    return result.str();
}

} // namespace

TEST_CASE("CommandSet tests") {
    SECTION("empty") {
        CommandSet<int> cs;
        CHECK(!cs.get("nothing", 1).has_value());
        CHECK(enumerate_str(cs, 0, 100).empty());
    }
    SECTION("level limits") {
        CommandSet<std::string> cs;
        cs.add("kick", "foot", 0);
        cs.add("punch", "fist", 5);
        cs.add("disembowel", "spade", 4);
        cs.add("smit", "spell it out", 92);
        cs.add("smite", "godlike power", 92);
        cs.add("smitten", "*blush*", 1);

        SECTION("low level enumerate") {
            CHECK(enumerate_str(cs, 0, 5) == "disembowel=spade@4 kick=foot@0 punch=fist@5 smitten=*blush*@1 ");
        }
        SECTION("mid level enumerate") { CHECK(enumerate_str(cs, 3, 5) == "disembowel=spade@4 punch=fist@5 "); }
        SECTION("high level enumerate") {
            CHECK(enumerate_str(cs, 5, 100) == "punch=fist@5 smit=spell it out@92 smite=godlike power@92 ");
        }
        SECTION("all level enumerate") {
            CHECK(enumerate_str(cs, 0, 100)
                  == "disembowel=spade@4 kick=foot@0 punch=fist@5 smit=spell it out@92 smite=godlike power@92 "
                     "smitten=*blush*@1 ");
        }
        CHECK(cs.get("kick", 0).value() == "foot");

        CHECK(!cs.get("punch", 4).has_value());
        CHECK(cs.get("punch", 5).value() == "fist");

        CHECK(cs.get("smit", 80).value() == "*blush*");
        CHECK(cs.get("smit", 92).value() == "spell it out");
        CHECK(cs.get("smite", 92).value() == "godlike power");
        CHECK(!cs.get("smite", 91).has_value());
    }
    SECTION("case insensitive") {
        CommandSet<std::string> cs;
        cs.add("SHOUT", "RARARAAAA", 0);
        cs.add("whisper", "pssst", 0);
        cs.add("Talk", "Blah blah.", 0);

        CHECK(cs.get("SHOUT", 0) == "RARARAAAA");
        CHECK(cs.get("shout", 0) == "RARARAAAA");
        CHECK(cs.get("sHoUT", 0) == "RARARAAAA");

        CHECK(cs.get("whisper", 0) == "pssst");
        CHECK(cs.get("WHISPER", 0) == "pssst");

        CHECK(cs.get("talk", 0) == "Blah blah.");
        CHECK(cs.get("tALK", 0) == "Blah blah.");
        CHECK(cs.get("TA", 0) == "Blah blah.");

        SECTION("insensitive enum") {
            CHECK(enumerate_str(cs, 0, 100) == "shout=RARARAAAA@0 talk=Blah blah.@0 whisper=pssst@0 ");
        }
    }
    SECTION("notes") {
        CommandSet<std::string> cs;
        cs.add("send", "post", 0);
        cs.add("show", "demonstrate", 0);
        cs.add("subject", "citizen", 0);

        CHECK(cs.get("se", 0) == "post");
        CHECK(cs.get("sh", 0) == "demonstrate");
        CHECK(cs.get("su", 0) == "citizen");
        CHECK(cs.get("s", 0) == "post");
    }
    // If several commands match the requested prefix, it's vital that the first one added
    // is the one we pick. If we just pick the first alphabetically, we end up with weird
    // annoying bugs, like "l" matching first "list", whereas everyone expects "look".
    SECTION("look first") {
        CommandSet<std::string> cs;
        cs.add("look", "with your eyes", 0);
        cs.add("list", "with a notepad", 0);
        CHECK(cs.get("l", 0) == "with your eyes");
    }
}
