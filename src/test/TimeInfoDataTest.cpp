#include "TimeInfoData.hpp"

#include <catch2/catch.hpp>

TEST_CASE("time info data") {
    SECTION("should construct from real time") {
        TimeInfoData tid(Time(std::chrono::seconds(1597630549)));
        CHECK(tid.day() == 0);
        CHECK(tid.hour() == 21);
        CHECK(tid.month() == 4);
        CHECK(tid.year() == 2211);
    }
    SECTION("should describe appropriately") {
        CHECK(TimeInfoData(1, 20, 5, 1000).describe()
              == "It is 1 o'clock am, Day of the Sun, 21st the Month of the Spring.");
    }
    SECTION("should advance") {
        SECTION("hours non-rolling") {
            TimeInfoData tid(1, 20, 5, 1000);
            tid.advance();
            CHECK(tid.hour() == 2);
            CHECK(tid.day() == 20);
            CHECK(tid.month() == 5);
            CHECK(tid.year() == 1000);
        }
        SECTION("hours rolling") {
            TimeInfoData tid(23, 20, 5, 1000);
            tid.advance();
            CHECK(tid.hour() == 0);
            CHECK(tid.day() == 21);
            CHECK(tid.month() == 5);
            CHECK(tid.year() == 1000);
        }
        SECTION("days") {
            TimeInfoData tid(23, 34, 5, 1000);
            tid.advance();
            CHECK(tid.hour() == 0);
            CHECK(tid.day() == 0);
            CHECK(tid.month() == 6);
            CHECK(tid.year() == 1000);
        }
        SECTION("years") {
            TimeInfoData tid(23, 34, 16, 1000);
            tid.advance();
            CHECK(tid.hour() == 0);
            CHECK(tid.day() == 0);
            CHECK(tid.month() == 0);
            CHECK(tid.year() == 1001);
        }
    }
    SECTION("seasons") {
        CHECK(!TimeInfoData(2, 2, 2, 1000).is_summer());
        CHECK(TimeInfoData(2, 2, 12, 1000).is_summer());
    }
}
