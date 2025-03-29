#pragma once

#include <chrono>
#include <date/date.h>
#include <fmt/chrono.h>
#include <fmt/ostream.h>

using Clock = std::chrono::system_clock;
using Time = Clock::time_point;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using date::operator<<;

inline auto formatted_time(const Time time) { return fmt::format("{:%Y-%m-%d %H:%M:%S}Z", fmt::gmtime(time)); }
