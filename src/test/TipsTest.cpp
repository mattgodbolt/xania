/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "Tips.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

TEST_CASE("Tips") {
    Tips tips{};
    SECTION("all tips loaded") {
        std::string tip_file = fmt::format("{}{}", TEST_DATA_DIR, "/area/tipfile.txt");
        const auto count = tips.load(tip_file);
        CHECK(count == 35);
    }
    SECTION("no tips loaded from wrong file") {
        std::string tip_file = fmt::format("{}{}", TEST_DATA_DIR, "/area/missing.txt");
        const auto count = tips.load(tip_file);
        CHECK(count == -1);
    }
}
