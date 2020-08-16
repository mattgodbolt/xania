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

void TimeInfoData::advance() {
    if (++hour_ < 24)
        return;
    hour_ = 0;

    if (++day_ < 35)
        return;
    day_ = 0;

    if (++month_ < 16)
        return;
    month_ = 0;
    year_++;
}

TimeInfoData::TimeInfoData(Time now) {
    auto lhour = static_cast<int>((Clock::to_time_t(now) - 650336715) / (PULSE_TICK / PULSE_PER_SECOND));
    hour_ = lhour % 24;
    auto lday = lhour / 24;
    day_ = lday % 35;
    auto lmonth = lday / 35;
    month_ = lmonth % 17;
    year_ = lmonth / 17;
}

std::string TimeInfoData::describe() const noexcept {
    std::array<std::string_view, 7> day_name{"the Moon", "the Bull",       "Deception", "Thunder",
                                             "Freedom",  "the Great Gods", "the Sun"};

    std::array<std::string_view, 17> month_name{"Winter",
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

    return "It is {} o'clock {}, Day of {}, {} the Month of {}."_format(
        (hour_ % 12 == 0) ? 12 : hour_ % 12, hour_ >= 12 ? "pm" : "am", day_name[day_ % day_name.size()], nth(day_ + 1),
        month_name[month_]);
}

// TODO move elsewhere once we break char_data into a class
Seconds CHAR_DATA::total_played() const { return std::chrono::duration_cast<Seconds>(current_time - logon + played); }
