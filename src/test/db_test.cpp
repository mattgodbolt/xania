#include "MemFile.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "mob_prog.hpp"

#include <catch2/catch.hpp>

extern bool mprog_file_read(std::string_view file_name, FILE *prog_file, MobIndexData *mobIndex);

TEST_CASE("string loading functions") {
    SECTION("fread_stdstring") {
        SECTION("should read a simple case") {
            test::MemFile test_doc("I am a string~");
            CHECK(fread_stdstring(test_doc.file()) == "I am a string");
        }
        SECTION("should skip leading whitespace") {
            test::MemFile test_doc(" \n\r\tI am also a string~");
            CHECK(fread_stdstring(test_doc.file()) == "I am also a string");
        }
        SECTION("should keep trailing whitespace") {
            test::MemFile test_doc("Something with trailing whitespace   ~");
            CHECK(fread_stdstring(test_doc.file()) == "Something with trailing whitespace   ");
        }
        SECTION("multiple strings") {
            test::MemFile test_doc(R"(First string~
Second string~
A multi-line
string~)");
            CHECK(fread_stdstring(test_doc.file()) == "First string");
            CHECK(fread_stdstring(test_doc.file()) == "Second string");
            CHECK(fread_stdstring(test_doc.file()) == "A multi-line\n\rstring");
            CHECK(fread_stdstring(test_doc.file()).empty());
        }
    }

    SECTION("fread_stdstring_eol") {
        SECTION("should read a simple case") {
            test::MemFile test_doc("I am a string\n\r");
            CHECK(fread_stdstring_eol(test_doc.file()) == "I am a string");
        }
        SECTION("should not treat ~ specially") {
            test::MemFile test_doc("I love tildes~\n\r");
            CHECK(fread_stdstring_eol(test_doc.file()) == "I love tildes~");
        }
        SECTION("should skip leading whitespace") {
            test::MemFile test_doc("   something\n\r");
            CHECK(fread_stdstring_eol(test_doc.file()) == "something");
        }
        SECTION("should keep trailing whitespace") {
            test::MemFile test_doc("something   \n\r");
            CHECK(fread_stdstring_eol(test_doc.file()) == "something   ");
        }
        SECTION("multiple strings") {
            test::MemFile test_doc(R"(First string
Second string
Last string
)");
            CHECK(fread_stdstring_eol(test_doc.file()) == "First string");
            CHECK(fread_stdstring_eol(test_doc.file()) == "Second string");
            CHECK(fread_stdstring_eol(test_doc.file()) == "Last string");
            CHECK(fread_stdstring_eol(test_doc.file()).empty());
        }
    }
}
TEST_CASE("mob prog file reading") {
    test::MemFile orc(R"mob(
#13000
orc~
the orc shaman~
~
~
orc~
0 0 0 0
0 0 1d6+1 1d6+1 1d1+1 blast
0 0 0 0
0 0 0 0
stand stand male 200
0 0 medium 0

#0
)mob");
    auto opt_mob = MobIndexData::from_file(orc.file());
    CHECK(opt_mob);
    auto mob = *opt_mob;
    CHECK(mob.progtypes == 0);
    SECTION("well formed with one prog") {
        test::MemFile test_prog(">rand_prog 50~\n\rif rand(50)\n\r\tdance\n\relse\n\rn\tsing\n\rendif~\n\r|\n\r");
        auto result = mprog_file_read("test.prg", test_prog.file(), &mob);

        CHECK(result);
        CHECK(mob.mobprogs.size() == 1);
        const auto &prog = mob.mobprogs.at(0);
        CHECK(prog.type == MobProgTypeFlag::Random);
        CHECK(check_enum_bit(mob.progtypes, MobProgTypeFlag::Random));
    }
    SECTION("well formed with two progs") {
        test::MemFile test_prog(">rand_prog 50~\n\rif rand(50)\n\r\tdance\n\relse\n\rn\tsing\n\rendif~\n\r>greet_prog "
                                "50~\n\rwave~\n\r|\n\r");
        auto result = mprog_file_read("test.prg", test_prog.file(), &mob);

        CHECK(result);
        CHECK(mob.mobprogs.size() == 2);
        const auto &first = mob.mobprogs.at(0);
        CHECK(first.type == MobProgTypeFlag::Random);
        const auto &second = mob.mobprogs.at(1);
        CHECK(second.type == MobProgTypeFlag::Greet);
        CHECK(check_enum_bit(mob.progtypes, MobProgTypeFlag::Random));
        CHECK(check_enum_bit(mob.progtypes, MobProgTypeFlag::Greet));
    }
    SECTION("malformed empty") {
        test::MemFile test_prog("");
        auto result = mprog_file_read("test.prg", test_prog.file(), &mob);

        CHECK(!result);
        CHECK(mob.mobprogs.empty());
    }
    SECTION("malformed type error") {
        test::MemFile test_prog(">foo_prog 50~\n\rwave\n\r~\n\r|\n\r");
        auto result = mprog_file_read("test.prg", test_prog.file(), &mob);

        CHECK(!result);
        CHECK(mob.mobprogs.empty());
    }
    SECTION("malformed missing terminator") {
        test::MemFile test_prog(">foo_prog 50~\n\rwave\n\r~\n\r");
        auto result = mprog_file_read("test.prg", test_prog.file(), &mob);

        CHECK(!result);
        CHECK(mob.mobprogs.empty());
    }
}
TEST_CASE("fread_spnumber") {
    SECTION("one word spell with ~") {
        test::MemFile name("armor~");
        const auto spell = fread_spnumber(name.file());

        CHECK(spell == 1);
    }
    SECTION("two word spell") {
        test::MemFile name("giant strength~");
        const auto spell = fread_spnumber(name.file());

        CHECK(spell == 39);
    }
    SECTION("raw slot number") {
        test::MemFile name("1");
        const auto spell = fread_spnumber(name.file());

        CHECK(spell == 1);
    }
    SECTION("one word no terminator") {
        test::MemFile name("armor ");
        const auto spell = try_fread_spnumber(name.file());

        CHECK(!spell);
    }
    SECTION("single quotes disallowed") {
        test::MemFile name("'armor'");
        const auto spell = try_fread_spnumber(name.file());

        CHECK(!spell);
    }
    SECTION("double quotes disallowed") {
        test::MemFile name("'armor'");
        const auto spell = try_fread_spnumber(name.file());

        CHECK(!spell);
    }
    SECTION("named spell not found") {
        test::MemFile name("discombobulate~");
        const auto spell = try_fread_spnumber(name.file());

        CHECK(!spell);
    }
}
