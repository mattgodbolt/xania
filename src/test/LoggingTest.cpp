/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Logging.hpp"
#include "MemFile.hpp"

#include <catch2/catch.hpp>

TEST_CASE("bug") {
    SECTION("bug doesn't infinite loop if there are no newlines") {
        test::MemFile invalid_area_file(R"(has-no-newline)");
        BugAreaFileContext context("invalid.are", invalid_area_file.file());

        bug("Some error message");
    }
}
