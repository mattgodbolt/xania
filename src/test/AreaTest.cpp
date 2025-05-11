/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Area.hpp"

#include "DescriptorList.hpp"
#include "MemFile.hpp"
#include <catch2/catch_test_macros.hpp>

#include "MockMud.hpp"

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

}
TEST_CASE("area loading") {
    ALLOW_CALL(mock_mud, logger()).LR_RETURN(logger);
    SECTION("happy path") {
        test::MemFile fp(R"(ignored~
Short name~
{ 1 50} TheMoog Some kind of area~
6200 6399
)");
        auto area = Area::parse(1, fp.file(), "bob", mock_mud);
        CHECK(area.filename() == "bob");
        CHECK(area.num() == 1);
        CHECK(area.short_name() == "Short name");
        CHECK(area.description() == "{ 1 50} TheMoog Some kind of area");
        CHECK(area.min_level() == 1);
        CHECK(area.max_level() == 50);
        CHECK(area.level_difference() == 49);
        CHECK(area.lowest_vnum() == 6200);
        CHECK(area.highest_vnum() == 6399);
        CHECK(!area.all_levels());
    }
    SECTION("parses without level range") {
        test::MemFile fp(R"(ignored~
Short name~
{     } TheMoog Some kind of area~
6200 6399
)");
        auto area = Area::parse(1, fp.file(), "bob", mock_mud);
        CHECK(area.all_levels());
        CHECK(area.min_level() == 0);
        CHECK(area.max_level() == 100);
    }
    SECTION("Occupancy and update") {
        test::MemFile fp(R"(ignored~
Short name~
{ 1 50} TheMoog Some kind of area~
6200 6399
)");
        auto area = Area::parse(1, fp.file(), "bob", mock_mud);
        SECTION("should starts out unoccupied") { CHECK(!area.occupied()); }
        SECTION("should need update straight away") {
            CHECK(!area.empty_since_last_reset());
            SECTION("and should reset straight away") {
                area.update(); // TODO: ideally fake out "get_room" so no weird global side effects
                CHECK(area.empty_since_last_reset());
            }
        }
        SECTION("notes new players") {
            area.player_entered();
            CHECK(!area.empty_since_last_reset());
            CHECK(area.occupied());
            SECTION("player leaves") {
                area.player_left();
                CHECK(!area.empty_since_last_reset());
                CHECK(!area.occupied());
            }
        }
    }
}
