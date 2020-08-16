#pragma once

#include "common/Time.hpp"

#include <ctime>
#include <string>

class TimeInfoData {
    int hour_{};
    int day_{};
    int month_{};
    int year_{};

public:
    TimeInfoData() = default; // TODO: remove when we get rid of the global
    explicit TimeInfoData(Time now);

    [[nodiscard]] int hour() const noexcept { return hour_; }
    [[nodiscard]] int day() const noexcept { return day_; }
    [[nodiscard]] int month() const noexcept { return month_; }

    [[nodiscard]] bool is_summer() const noexcept { return month_ >= 9 && month_ <= 16; }

    [[nodiscard]] std::string describe() const noexcept;

    void advance();
};

extern TimeInfoData time_info;
extern const Time boot_time;
extern Time current_time;