#include "WeatherData.hpp"
#include "merc.h"

#include <fmt/format.h>

using namespace fmt::literals;

WeatherData weather_info;

int WeatherData::pressure_direction(const TimeInfoData &tid) const {
    if (tid.month() >= 9 && tid.month() <= 16)
        return mmhg_ > 985 ? -2 : 2;
    else
        return mmhg_ > 1015 ? -2 : 2;
}

std::string WeatherData::update(const TimeInfoData &tid) {
    std::string buf;
    switch (time_info.hour()) {
    case 5:
        sunlight_ = Sun::Light;
        buf = "The day has begun.\n\r";
        break;

    case 6:
        sunlight_ = Sun::Rise;
        buf = "The sun rises in the east.\n\r";
        break;

    case 19:
        sunlight_ = Sun::Light;
        buf = "The sun slowly disappears in the west.\n\r";
        break;

    case 20:
        sunlight_ = Sun::Dark;
        buf = "The night has begun.\n\r";
        break;
    }

    change_ += pressure_direction(tid) * dice(1, 4) + dice(2, 6) - dice(2, 6);
    change_ = std::min(std::max(change_, -12), 12);

    mmhg_ += change_;
    mmhg_ = std::min(std::max(mmhg_, 960), 1040);

    switch (sky_) {
    default:
        bug("Weather_update: bad sky %d.", static_cast<int>(sky_));
        sky_ = Sky::Cloudless;
        break;

    case Sky::Cloudless:
        if (mmhg_ < 990 || (mmhg_ < 1010 && number_bits(2) == 0)) {
            sky_ = Sky::Cloudy;
            return buf + "The sky is getting cloudy.\n\r";
        }
        break;

    case Sky::Cloudy:
        if (mmhg_ < 970 || (mmhg_ < 990 && number_bits(2) == 0)) {
            sky_ = Sky::Raining;
            return buf + "It starts to rain.\n\r";
        }

        if (mmhg_ > 1030 && number_bits(2) == 0) {
            sky_ = Sky::Cloudless;
            return buf + "The clouds disappear.\n\r";
        }
        break;

    case Sky::Raining:
        if (mmhg_ < 970 && number_bits(2) == 0) {
            sky_ = Sky::Lightning;
            return buf + "Lightning flashes in the sky.\n\r";
        }

        if (mmhg_ > 1030 || (mmhg_ > 1010 && number_bits(2) == 0)) {
            sky_ = Sky::Cloudy;
            return buf + "The rain stopped.\n\r";
        }
        break;

    case Sky::Lightning:
        if (mmhg_ > 1010 || (mmhg_ > 990 && number_bits(2) == 0)) {
            sky_ = Sky::Raining;
            return buf + "The lightning has stopped.\n\r";
        }
        break;
    }
    return buf;
}

WeatherData::WeatherData(const TimeInfoData &tid) {
    if (tid.hour() < 5)
        sunlight_ = Sun::Dark;
    else if (tid.hour() < 6)
        sunlight_ = Sun::Rise;
    else if (tid.hour() < 19)
        sunlight_ = Sun::Light;
    else if (tid.hour() < 20)
        sunlight_ = Sun::Set;
    else
        sunlight_ = Sun::Dark;

    change_ = 0;
    mmhg_ = 960;
    if (tid.month() >= 7 && tid.month() <= 12)
        mmhg_ += number_range(1, 50);
    else
        mmhg_ += number_range(1, 80);

    if (mmhg_ <= 980)
        sky_ = Sky::Lightning;
    else if (mmhg_ <= 1000)
        sky_ = Sky::Raining;
    else if (mmhg_ <= 1020)
        sky_ = Sky::Cloudy;
    else
        sky_ = Sky::Cloudless;
}

namespace {
std::string_view to_string(WeatherData::Sky sky) {
    switch (sky) {
    case WeatherData::Sky::Cloudless: return "cloudless";
    case WeatherData::Sky::Cloudy: return "cloudy";
    case WeatherData::Sky::Raining: return "rainy";
    case WeatherData::Sky::Lightning: return "lit by flashes of lightning";
    }
    return "not as expected";
}

}

std::string WeatherData::describe() const noexcept {
    return "The sky is {} and {}."_format(to_string(sky_),
                                          change_ > 0 ? "a warm southerly breeze blows" : "a cold northern gust blows");
}
