#pragma once

#include "ExtraFlags.hpp"

#include <string_view>

#include <fmt/core.h>

// TODO: replace bug() with fmt formats
void bug(const char *str, ...) __attribute__((format(printf, 1, 2)));
// TODO: somehow combine log_new and log_string and all that. Maybe spdlog?
void log_new(std::string_view str, int loglevel, int level);
inline void log_string(std::string_view str) { log_new(str, EXTRA_WIZNET_DEBUG, 0); }
template <typename... Args>
void log_string(std::string_view str, Args &&... args) {
    log_new(fmt::format(str, std::forward<Args>(args)...), EXTRA_WIZNET_DEBUG, 0);
}