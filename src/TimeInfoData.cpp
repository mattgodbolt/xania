#include "TimeInfoData.hpp"
#include "merc.h"
#include "nth.hpp"

#include <fmt/format.h>

#include <array>
#include <string_view>

using namespace fmt::literals;

// Hateful globals.
TimeInfoData time_info;
const Time boot_time = std::chrono::system_clock::now();
Time current_time = std::chrono::system_clock::now();

namespace {
constexpr std::array<std::string_view, TimeInfoData::DaysPerWeek> day_name{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom", "the Great Gods", "the Sun"};

constexpr std::array<std::string_view, TimeInfoData::MonthsPerYear> month_name{"Winter",
                                                                               "the Winter Wolf",
                                                                               "the Frost Giant",
                                                                               "the Old Forces",
                                                                               "the Grand Struggle",
                                                                               "the Spring",
                                                                               "Nature",
                                                                               "Futility",
                                                                               "the Dragon",
                                                                               "the Sun",
                                                                               "the Heat",
                                                                               "the Battle",
                                                                               "the Dark Shades",
                                                                               "the Shadows",
                                                                               "the Long Shadows",
                                                                               "the Ancient Darkness",
                                                                               "the Great Evil"};

}

TimeInfoData::TimeInfoData(int hour, int day, int month, int year)
    : hour_(hour), day_(day), month_(month), year_(year) {}

void TimeInfoData::advance() {
    if (++hour_ < HoursPerDay)
        return;
    hour_ = 0;

    if (++day_ < DaysPerMonth)
        return;
    day_ = 0;

    if (++month_ < MonthsPerYear)
        return;
    month_ = 0;
    year_++;
}

TimeInfoData::TimeInfoData(Time now) {
    using namespace date;
    using namespace std::chrono;
    // The epoch here, August 11, 1990 1:05:15 AM, is derived from the 650336715 in the original source.
    static constexpr auto DikuEpoch = sys_days(1990_y / August / 11_d) + 1h + 5min + 15s;
    const auto hours_since_epoch =
        static_cast<int>(duration_cast<seconds>(now - DikuEpoch).count() / (PULSE_TICK / PULSE_PER_SECOND));
    hour_ = hours_since_epoch % HoursPerDay;
    const auto days_since_epoch = hours_since_epoch / HoursPerDay;
    day_ = days_since_epoch % 35;
    const auto months_since_epoch = days_since_epoch / DaysPerMonth;
    month_ = months_since_epoch % MonthsPerYear;
    year_ = months_since_epoch / MonthsPerYear;
}

std::string TimeInfoData::describe() const noexcept {
    return "It is {} o'clock {}, Day of {}, {} the Month of {}."_format(
        (hour_ % 12 == 0) ? 12 : hour_ % 12, hour_ >= 12 ? "pm" : "am", day_name[day_ % day_name.size()], nth(day_ + 1),
        month_name[month_]);
}

// TODO move elsewhere once we break char_data into a class
Seconds CHAR_DATA::total_played() const { return std::chrono::duration_cast<Seconds>(current_time - logon + played); }
