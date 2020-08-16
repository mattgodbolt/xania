#pragma once

#include <ctime>

struct time_info_data {
    int hour{};
    int day{};
    int month{};
    int year{};

    time_info_data() = default; // TODO: remove when we get rid of the global
    explicit time_info_data(time_t now);

    void advance();
};

extern time_info_data time_info;
