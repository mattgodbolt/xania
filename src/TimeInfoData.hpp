#pragma once

#include "common/Time.hpp"

#include <string>

class TimeInfoData {
    unsigned int hour_{};
    unsigned int day_{};
    unsigned int month_{};
    unsigned int year_{};

public:
    explicit TimeInfoData(Time now);
    explicit TimeInfoData(int hour, int day, int month, int year);

    static constexpr auto HoursPerDay = 24u;
    static constexpr auto DaysPerWeek = 7u;
    static constexpr auto DaysPerMonth = 35u;
    static constexpr auto MonthsPerYear = 17u;

    [[nodiscard]] unsigned int hour() const noexcept { return hour_; }
    [[nodiscard]] unsigned int day() const noexcept { return day_; }
    [[nodiscard]] unsigned int month() const noexcept { return month_; }
    [[nodiscard]] unsigned int year() const noexcept { return year_; }

    [[nodiscard]] bool is_summer() const noexcept { return month_ >= 9 && month_ <= 16; }
    [[nodiscard]] bool is_autumnal() const noexcept { return month_ >= 7 && month_ <= 12; }

    [[nodiscard]] std::string describe() const noexcept;

    // This is advanced by the main loop every PULSE_TICK.
    void advance();
};
