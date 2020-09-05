#include "MemFile.hpp"
#include "merc.h"

#include <catch2/catch.hpp>

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
