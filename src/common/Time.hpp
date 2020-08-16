#pragma once

#include <chrono>
#include <date/date.h>
#include <fmt/ostream.h>

using Clock = std::chrono::system_clock;
using Time = Clock::time_point;
using Seconds = std::chrono::seconds;
using date::operator<<;

inline auto secs_only(Time time) { return date::floor<std::chrono::seconds>(time); }