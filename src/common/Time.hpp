#pragma once

#include <chrono>
#include <date/date.h>
#include <fmt/ostream.h>

using Clock = std::chrono::system_clock;
using Time = Clock::time_point;
using date::operator<<;