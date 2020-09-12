#include "Help.hpp"

#include "CatchFormatters.hpp"
#include "MemFile.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

TEST_CASE("Help") {
    SECTION("should parse an end of help") {
        test::MemFile empty("0 $~");
        CHECK(Help::load(empty.file(), nullptr) == std::nullopt);
    }
    SECTION("should parse an end of help even with junk after it") {
        test::MemFile empty("0 $this isn't strictly on spec but the end code supported it~");
        CHECK(Help::load(empty.file(), nullptr) == std::nullopt);
    }
    SECTION("should parse a normal help entry") {
        test::MemFile monkeys(R"(
12 twelve monkeys~
This is a test of the
help parser.
~
)");
        CHECK(Help::load(monkeys.file(), nullptr)
              == Help(nullptr, 12, "twelve monkeys", "This is a test of the\n\rhelp parser.\n\r"));
    }
    SECTION("should match appropriately") {
        SECTION("all level help") {
            auto help = Help(nullptr, 0, "motd", "Message of the day");
            CHECK(help.matches(0, "motd"));
            CHECK(help.matches(0, "mot"));
            CHECK(!help.matches(0, "immotd"));
        }
        SECTION("imm only") {
            auto help = Help(nullptr, 91, "imotd", "Imm stuff");
            CHECK(!help.matches(0, "motd"));
            CHECK(!help.matches(0, "mot"));
            CHECK(!help.matches(0, "imotd"));
            CHECK(!help.matches(0, "imo"));
            CHECK(!help.matches(90, "imotd"));
            CHECK(!help.matches(90, "imo"));
            CHECK(help.matches(91, "imotd"));
            CHECK(help.matches(91, "imo"));
        }
    }
    SECTION("help lists") {
        HelpList all_help;
        all_help.add(Help(nullptr, 91, "imotd", "Imm stuff"));
        all_help.add(Help(nullptr, 0, "motd", "Message of the day"));
        all_help.add(Help(nullptr, 0, "xania", "badger"));
        CHECK(all_help.count() == 3);
        SECTION("should find motd") {
            auto *motd = all_help.lookup(0, "motd");
            REQUIRE(motd);
            CHECK(motd->text() == "Message of the day");
        }
        SECTION("should find xania") {
            auto *xania = all_help.lookup(0, "xania");
            REQUIRE(xania);
            CHECK(xania->text() == "badger");
        }
        SECTION("should not find imm for noobs") {
            auto *imotd = all_help.lookup(1, "imotd");
            CHECK(!imotd);
        }
        SECTION("should find xania") {
            auto *imotd = all_help.lookup(100, "imotd");
            REQUIRE(imotd);
            CHECK(imotd->text() == "Imm stuff");
        }
    }
}