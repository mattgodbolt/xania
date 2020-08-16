#pragma once

#include "TimeInfoData.hpp"
#include <string>

#define SUN_DARK 0
#define SUN_RISE 1
#define SUN_LIGHT 2
#define SUN_SET 3

#define SKY_CLOUDLESS 0
#define SKY_CLOUDY 1
#define SKY_RAINING 2
#define SKY_LIGHTNING 3

struct weather_data {
    int mmhg{960};
    int change{};
    int sky{SKY_CLOUDLESS};
    int sunlight{SUN_RISE};

    weather_data() = default; // TODO remove once we get rid of the global.
    explicit weather_data(const time_info_data &tid);

    std::string update(const time_info_data &tid);

    [[nodiscard]] bool is_raining() const { return sky >= SKY_RAINING; }
    [[nodiscard]] bool is_dark() const { return sunlight == SUN_SET || sunlight == SUN_DARK; }

private:
    int pressure_direction(const time_info_data &tid) const;
};

extern weather_data weather_info;
