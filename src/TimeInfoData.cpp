#include "TimeInfoData.hpp"
#include "merc.h"

time_info_data time_info;

void time_info_data::advance() {
    if (++hour < 24)
        return;
    hour = 0;

    if (++day < 35)
        return;
    day = 0;

    if (++month < 16)
        return;
    month = 0;
    year++;
}

time_info_data::time_info_data(time_t now) {
    auto lhour = (now - 650336715) / (PULSE_TICK / PULSE_PER_SECOND);
    hour = lhour % 24;
    auto lday = lhour / 24;
    day = lday % 35;
    auto lmonth = lday / 35;
    month = lmonth % 17;
    year = lmonth / 17;
}
