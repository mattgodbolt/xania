#pragma once

#include "TimeInfoData.hpp"

#include <string>

class WeatherData {
public:
    enum class Sun { Dark, Rise, Light, Set };
    enum class Sky { Cloudless, Cloudy, Raining, Lightning };

private:
    int mmhg_{960};
    int change_{};
    Sky sky_{Sky::Cloudless};
    Sun sunlight_{Sun::Rise};

public:
    WeatherData() = default; // TODO remove once we get rid of the global.
    explicit WeatherData(const TimeInfoData &tid);
    WeatherData(Sky sky, Sun sunlight) : sky_(sky), sunlight_(sunlight) {}

    void update(const TimeInfoData &tid);

    void control(int delta) { change_ += delta; }

    [[nodiscard]] bool is_raining() const { return sky_ == Sky::Raining || sky_ == Sky::Lightning; }
    [[nodiscard]] bool is_dark() const { return sunlight_ == Sun::Set || sunlight_ == Sun::Dark; }

    [[nodiscard]] std::string describe() const noexcept;
    [[nodiscard]] std::string describe_change(const WeatherData &before) const noexcept;

private:
    [[nodiscard]] int pressure_direction(const TimeInfoData &tid) const;
};

extern WeatherData weather_info;
