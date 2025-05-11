#include "WeatherData.hpp"
#include "Logging.hpp"

#include <fmt/format.h>

WeatherData weather_info; // TODO fix

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

std::string_view change_for(WeatherData::Sun sunlight) {
    switch (sunlight) {
    case WeatherData::Sun::Light: return "The day has begun.";
    case WeatherData::Sun::Rise: return "The sun rises in the east.";
    case WeatherData::Sun::Set: return "The sun slowly disappears in the west.";
    case WeatherData::Sun::Dark: return "The night has begun.";
    }
    return "";
}

WeatherData::Sun sun_from_time(const TimeInfoData &tid) {
    if (tid.hour() >= 5) {
        if (tid.hour() == 5)
            return WeatherData::Sun::Light;
        if (tid.hour() < 19)
            return WeatherData::Sun::Rise;
        if (tid.hour() < 20)
            return WeatherData::Sun::Set;
    }
    return WeatherData::Sun::Dark;
}

}

int WeatherData::pressure_direction(const TimeInfoData &tid) const {
    if (tid.is_summer())
        return mmhg_ > 985 ? -2 : 2;
    else
        return mmhg_ > 1015 ? -2 : 2;
}

void WeatherData::update(Rng &rng, const TimeInfoData &tid, const Logger &logger) {
    sunlight_ = sun_from_time(tid);

    auto change_change = pressure_direction(tid) * rng.dice(1, 4) + rng.dice(2, 6) - rng.dice(2, 6);
    change_ = std::min(std::max(change_ + change_change, -12), 12);
    mmhg_ = std::min(std::max(mmhg_ + change_, 960), 1040);

    switch (sky_) {
    default:
        logger.bug("Weather_update: bad sky {}.", static_cast<int>(sky_));
        sky_ = Sky::Cloudless;
        break;

    case Sky::Cloudless:
        if (mmhg_ < 990 || (mmhg_ < 1010 && rng.number_bits(2) == 0))
            sky_ = Sky::Cloudy;
        break;

    case Sky::Cloudy:
        if (mmhg_ < 970 || (mmhg_ < 990 && rng.number_bits(2) == 0))
            sky_ = Sky::Raining;

        if (mmhg_ > 1030 && rng.number_bits(2) == 0)
            sky_ = Sky::Cloudless;
        break;

    case Sky::Raining:
        if (mmhg_ < 970 && rng.number_bits(2) == 0)
            sky_ = Sky::Lightning;

        if (mmhg_ > 1030 || (mmhg_ > 1010 && rng.number_bits(2) == 0))
            sky_ = Sky::Cloudy;
        break;

    case Sky::Lightning:
        if (mmhg_ > 1010 || (mmhg_ > 990 && rng.number_bits(2) == 0))
            sky_ = Sky::Raining;
        break;
    }
}

WeatherData::WeatherData(Rng &rng, const TimeInfoData &tid) : sunlight_(sun_from_time(tid)) {
    change_ = 0;
    mmhg_ = 960 + rng.number_range(1, tid.is_autumnal() ? 50 : 80);

    if (mmhg_ <= 980)
        sky_ = Sky::Lightning;
    else if (mmhg_ <= 1000)
        sky_ = Sky::Raining;
    else if (mmhg_ <= 1020)
        sky_ = Sky::Cloudy;
    else
        sky_ = Sky::Cloudless;
}

std::string WeatherData::describe() const noexcept {
    return fmt::format("The sky is {} and {}.", to_string(sky_),
                       change_ > 0 ? "a warm southerly breeze blows" : "a cold northern gust blows");
}

std::string WeatherData::describe_change(const WeatherData &before) const noexcept {
    std::string change = before.sunlight_ != sunlight_ ? std::string(change_for(sunlight_)) + "\n\r" : "";
    switch (sky_) {
    case Sky::Cloudy:
        if (before.sky_ == Sky::Cloudless)
            change += "The sky is getting cloudy.\n\r";
        else if (before.sky_ == Sky::Raining)
            change += "The rain stopped.\n\r";
        break;
    case Sky::Cloudless:
        if (before.sky_ != Sky::Cloudless)
            change += "The clouds disappear.\n\r";
        break;
    case Sky::Raining:
        if (before.sky_ == Sky::Lightning) {
            change += "The lightning has stopped.\n\r";
        } else if (before.sky_ != Sky::Raining) {
            change += "It starts to rain.\n\r";
        }
        break;
    case Sky::Lightning:
        if (before.sky_ != Sky::Lightning)
            change += "Lightning flashes in the sky.\n\r";
    }

    return change;
}
