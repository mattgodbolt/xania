#pragma once

#include "CharExtraFlag.hpp"

#include <string>
#include <string_view>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/printf.h>

void bug(std::string_view message);
template <typename... Args>
void bug(std::string_view format, Args &&... args) {
    bug(fmt::format(format, std::forward<Args>(args)...));
}
// TODO: somehow combine log_new and log_string and all that. Maybe spdlog?
void log_new(std::string_view str, const CharExtraFlag loglevel, int level);
inline void log_string(std::string_view str) { log_new(str, CharExtraFlag::WiznetDebug, 0); }
template <typename... Args>
void log_string(std::string_view str, Args &&... args) {
    log_new(fmt::format(str, std::forward<Args>(args)...), CharExtraFlag::WiznetDebug, 0);
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