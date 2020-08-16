#include "Weather.hpp"
#include "merc.h"

weather_data weather_info;

int weather_data::pressure_direction(const time_info_data &tid) const {
    if (tid.month >= 9 && tid.month <= 16)
        return mmhg > 985 ? -2 : 2;
    else
        return mmhg > 1015 ? -2 : 2;
}

std::string weather_data::update(const time_info_data &tid) {
    change += pressure_direction(tid) * dice(1, 4) + dice(2, 6) - dice(2, 6);
    change = std::min(std::max(change, -12), 12);

    mmhg += change;
    mmhg = std::min(std::max(mmhg, 960), 1040);

    switch (sky) {
    default:
        bug("Weather_update: bad sky %d.", sky);
        sky = SKY_CLOUDLESS;
        break;

    case SKY_CLOUDLESS:
        if (mmhg < 990 || (mmhg < 1010 && number_bits(2) == 0)) {
            sky = SKY_CLOUDY;
            return "The sky is getting cloudy.\n\r";
        }
        break;

    case SKY_CLOUDY:
        if (mmhg < 970 || (mmhg < 990 && number_bits(2) == 0)) {
            sky = SKY_RAINING;
            return "It starts to rain.\n\r";
        }

        if (mmhg > 1030 && number_bits(2) == 0) {
            sky = SKY_CLOUDLESS;
            return "The clouds disappear.\n\r";
        }
        break;

    case SKY_RAINING:
        if (mmhg < 970 && number_bits(2) == 0) {
            sky = SKY_LIGHTNING;
            return "Lightning flashes in the sky.\n\r";
        }

        if (mmhg > 1030 || (mmhg > 1010 && number_bits(2) == 0)) {
            sky = SKY_CLOUDY;
            return "The rain stopped.\n\r";
        }
        break;

    case SKY_LIGHTNING:
        if (mmhg > 1010 || (mmhg > 990 && number_bits(2) == 0)) {
            sky = SKY_RAINING;
            return "The lightning has stopped.\n\r";
        }
        break;
    }
    return "";
}

weather_data::weather_data(const time_info_data &tid) {
    if (tid.hour < 5)
        sunlight = SUN_DARK;
    else if (tid.hour < 6)
        sunlight = SUN_RISE;
    else if (tid.hour < 19)
        sunlight = SUN_LIGHT;
    else if (tid.hour < 20)
        sunlight = SUN_SET;
    else
        sunlight = SUN_DARK;

    change = 0;
    mmhg = 960;
    if (tid.month >= 7 && tid.month <= 12)
        mmhg += number_range(1, 50);
    else
        mmhg += number_range(1, 80);

    if (mmhg <= 980)
        sky = SKY_LIGHTNING;
    else if (mmhg <= 1000)
        sky = SKY_RAINING;
    else if (mmhg <= 1020)
        sky = SKY_CLOUDY;
    else
        sky = SKY_CLOUDLESS;
}
