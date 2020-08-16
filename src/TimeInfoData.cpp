#include "TimeInfoData.hpp"
#include "merc.h"

TimeInfoData time_info;

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

TimeInfoData::TimeInfoData(time_t now) {
    auto lhour = (now - 650336715) / (PULSE_TICK / PULSE_PER_SECOND);
    hour_ = lhour % 24;
    auto lday = lhour / 24;
    day_ = lday % 35;
    auto lmonth = lday / 35;
    month_ = lmonth % 17;
    year_ = lmonth / 17;
}
