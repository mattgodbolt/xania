#pragma once

#include "CharExtraFlag.hpp"

#include <string>
#include <string_view>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/printf.h>

void bug(std::string_view message);
template <typename... Args>
void bug(fmt::string_view format, Args &&... args) {
    bug(fmt::vformat(format, fmt::make_format_args(args...)));
}
// TODO: somehow combine log_new and log_string and all that. Maybe spdlog?
void log_new(std::string_view str, const CharExtraFlag loglevel, int level);
inline void log_string(std::string_view str) { log_new(str, CharExtraFlag::WiznetDebug, 0); }
template <typename... Args>
void log_string(fmt::string_view str, Args &&... args) {
    log_new(fmt::vformat(str, fmt::make_format_args(args...)), CharExtraFlag::WiznetDebug, 0);
}

class BugAreaFileContext {
public:
    BugAreaFileContext(std::string_view name, FILE *file);
    ~BugAreaFileContext();

    BugAreaFileContext(const BugAreaFileContext &) = delete;
    BugAreaFileContext &operator=(const BugAreaFileContext &) = delete;
    BugAreaFileContext(BugAreaFileContext &&) = delete;
    BugAreaFileContext &operator=(BugAreaFileContext &&) = delete;
};