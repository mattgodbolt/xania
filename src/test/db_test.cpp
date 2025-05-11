#include "DescriptorList.hpp"
#include "MemFile.hpp"
#include "MockMud.hpp"
#include "common/BitOps.hpp"
#include "db.h"

#include <catch2/catch_test_macros.hpp>

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

}
TEST_CASE("string loading functions") {
    SECTION("fread_string") {
        SECTION("should read a simple case") {
            test::MemFile test_doc("I am a string~");
            CHECK(fread_string(test_doc.file()) == "I am a string");
        }
        SECTION("should skip leading whitespace") {
            test::MemFile test_doc(" \n\r\tI am also a string~");
            CHECK(fread_string(test_doc.file()) == "I am also a string");
        }
        SECTION("should keep trailing whitespace") {
            test::MemFile test_doc("Something with trailing whitespace   ~");
            CHECK(fread_string(test_doc.file()) == "Something with trailing whitespace   ");
        }
        SECTION("multiple strings") {
            test::MemFile test_doc(R"(First string~
Second string~
A multi-line
string~)");
            CHECK(fread_string(test_doc.file()) == "First string");
            CHECK(fread_string(test_doc.file()) == "Second string");
            CHECK(fread_string(test_doc.file()) == "A multi-line\n\rstring");
            CHECK(fread_string(test_doc.file()).empty());
        }
    }

    SECTION("fread_string_eol") {
        SECTION("should read a simple case") {
            test::MemFile test_doc("I am a string\n\r");
            CHECK(fread_string_eol(test_doc.file()) == "I am a string");
        }
        SECTION("should not treat ~ specially") {
            test::MemFile test_doc("I love tildes~\n\r");
            CHECK(fread_string_eol(test_doc.file()) == "I love tildes~");
        }
        SECTION("should skip leading whitespace") {
            test::MemFile test_doc("   something\n\r");
            CHECK(fread_string_eol(test_doc.file()) == "something");
        }
        SECTION("should keep trailing whitespace") {
            test::MemFile test_doc("something   \n\r");
            CHECK(fread_string_eol(test_doc.file()) == "something   ");
        }
        SECTION("multiple strings") {
            test::MemFile test_doc(R"(First string
Second string
Last string
)");
            CHECK(fread_string_eol(test_doc.file()) == "First string");
            CHECK(fread_string_eol(test_doc.file()) == "Second string");
            CHECK(fread_string_eol(test_doc.file()) == "Last string");
            CHECK(fread_string_eol(test_doc.file()).empty());
        }
    }
}
TEST_CASE("fread_spnumber") {
    ALLOW_CALL(mock_mud, descriptors()).LR_RETURN(descriptors);
    SECTION("one word spell with ~") {
        test::MemFile name("armor~");
        const auto spell = fread_spnumber(name.file(), logger);

        CHECK(spell == 1);
    }
    SECTION("two word spell") {
        test::MemFile name("giant strength~");
        const auto spell = fread_spnumber(name.file(), logger);

        CHECK(spell == 39);
    }
    SECTION("raw slot number") {
        test::MemFile name("1");
        const auto spell = fread_spnumber(name.file(), logger);

        CHECK(spell == 1);
    }
    SECTION("one word no terminator") {
        test::MemFile name("armor ");
        const auto spell = try_fread_spnumber(name.file(), logger);

        CHECK(!spell);
    }
    SECTION("single quotes disallowed") {
        test::MemFile name("'armor'");
        const auto spell = try_fread_spnumber(name.file(), logger);

        CHECK(!spell);
    }
    SECTION("double quotes disallowed") {
        test::MemFile name("'armor'");
        const auto spell = try_fread_spnumber(name.file(), logger);

        CHECK(!spell);
    }
    SECTION("named spell not found") {
        test::MemFile name("discombobulate~");
        const auto spell = try_fread_spnumber(name.file(), logger);

        CHECK(!spell);
    }
}
TEST_CASE("fread_word") {
    SECTION("one char space") {
        test::MemFile input("a ");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char tab") {
        test::MemFile input("a\t");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char nl") {
        test::MemFile input("a\n");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char return") {
        test::MemFile input("a\r");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char vtab") {
        test::MemFile input("a\v");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char formfeed") {
        test::MemFile input("a\f");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char single quoted") {
        test::MemFile input("'a'");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("one char double quoted") {
        test::MemFile input("\"a\"");
        const auto word = fread_word(input.file());

        CHECK(word == "a");
    }
    SECTION("three words no quotes") {
        test::MemFile input("power of greyskull");
        const auto word = fread_word(input.file());

        CHECK(word == "power");
    }
    SECTION("three words with quotes") {
        test::MemFile input("'power of greyskull'");
        const auto word = fread_word(input.file());

        CHECK(word == "power of greyskull");
    }
    SECTION("three words with quotes and trailing") {
        test::MemFile input("'power of greyskull' schnarf");
        const auto word = fread_word(input.file());

        CHECK(word == "power of greyskull");
    }
    SECTION("one char no space terminator") {
        test::MemFile input("a");
        const auto word = try_fread_word(input.file());

        CHECK(!word);
    }
    SECTION("one char no single quote terminator") {
        test::MemFile input("'a");
        const auto word = try_fread_word(input.file());

        CHECK(!word);
    }
    SECTION("one char no double quote terminator") {
        test::MemFile input("\"a");
        const auto word = try_fread_word(input.file());

        CHECK(!word);
    }
    SECTION("empty input") {
        test::MemFile input("");
        const auto word = try_fread_word(input.file());

        CHECK(!word);
    }
}
