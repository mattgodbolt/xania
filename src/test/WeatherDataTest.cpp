#include "WeatherData.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("weather data") {
    KnuthRng fake_rng(0xdeadbeef);
    SECTION("Notices day") {
        CHECK(!WeatherData(fake_rng, TimeInfoData(5, 0, 0, 0)).is_dark());
        CHECK(!WeatherData(fake_rng, TimeInfoData(9, 0, 0, 0)).is_dark());
        CHECK(!WeatherData(fake_rng, TimeInfoData(18, 0, 0, 0)).is_dark());
    }
    SECTION("Notices night") {
        CHECK(WeatherData(fake_rng, TimeInfoData(1, 0, 0, 0)).is_dark());
        CHECK(WeatherData(fake_rng, TimeInfoData(4, 0, 0, 0)).is_dark());
        CHECK(WeatherData(fake_rng, TimeInfoData(19, 0, 0, 0)).is_dark());
    }
    SECTION("Should have no description if nothing at all changed") {
        auto weather = WeatherData(fake_rng, TimeInfoData(5, 0, 0, 0));
        CHECK(weather.describe_change(weather).empty());
    }
    SECTION("Should have no description if nothing change in the position of the sun") {
        auto before = WeatherData(fake_rng, TimeInfoData(13, 0, 0, 0));
        auto after = WeatherData(fake_rng, TimeInfoData(14, 0, 0, 0));
        CHECK(after.describe_change(before).empty());
    }
    SECTION("Describes day break") {
        auto before = WeatherData(fake_rng, TimeInfoData(4, 0, 0, 0));
        auto after = WeatherData(fake_rng, TimeInfoData(5, 0, 0, 0));
        CHECK(after.describe_change(before) == "The day has begun.\n\r");
    }
    SECTION("Describes sunrise") {
        auto before = WeatherData(fake_rng, TimeInfoData(5, 0, 0, 0));
        auto after = WeatherData(fake_rng, TimeInfoData(6, 0, 0, 0));
        CHECK(after.describe_change(before) == "The sun rises in the east.\n\r");
    }
    SECTION("Describes sunset") {
        auto before = WeatherData(fake_rng, TimeInfoData(18, 0, 0, 0));
        auto after = WeatherData(fake_rng, TimeInfoData(19, 0, 0, 0));
        CHECK(after.describe_change(before) == "The sun slowly disappears in the west.\n\r");
    }
    SECTION("Describes day end") {
        auto before = WeatherData(fake_rng, TimeInfoData(19, 0, 0, 0));
        auto after = WeatherData(fake_rng, TimeInfoData(20, 0, 0, 0));
        CHECK(after.describe_change(before) == "The night has begun.\n\r");
    }
    SECTION("Describes weather chagnges") {
        using Sky = WeatherData::Sky;
        auto wd = [](Sky sky) { return WeatherData(sky, WeatherData::Sun::Light); };
        SECTION("clearing up") {
            CHECK(wd(Sky::Cloudless).describe_change(wd(Sky::Cloudy)) == "The clouds disappear.\n\r");
        }
        SECTION("becoming cloudy") {
            CHECK(wd(Sky::Cloudy).describe_change(wd(Sky::Cloudless)) == "The sky is getting cloudy.\n\r");
        }
        SECTION("becoming rainy") {
            CHECK(wd(Sky::Raining).describe_change(wd(Sky::Cloudy)) == "It starts to rain.\n\r");
        }
        SECTION("stopping rain") {
            CHECK(wd(Sky::Cloudy).describe_change(wd(Sky::Raining)) == "The rain stopped.\n\r");
        }
        SECTION("storm coming in") {
            CHECK(wd(Sky::Lightning).describe_change(wd(Sky::Raining)) == "Lightning flashes in the sky.\n\r");
        }
        SECTION("storm breaking") {
            CHECK(wd(Sky::Raining).describe_change(wd(Sky::Lightning)) == "The lightning has stopped.\n\r");
        }
        SECTION("no change") { CHECK(wd(Sky::Raining).describe_change(wd(Sky::Raining)).empty()); }
    }
}
