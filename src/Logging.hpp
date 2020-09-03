#pragma once

#include "ExtraFlags.hpp"

#include <string>
#include <string_view>

#include <fmt/core.h>
#include <fmt/printf.h>

void bug(std::string_view message);
template <typename... Args>
void bug(std::string_view format, Args &&... args) {
    bug(fmt::format(format, std::forward<Args>(args)...));
}
// TODO: somehow combine log_new and log_string and all that. Maybe spdlog?
void log_new(std::string_view str, int loglevel, int level);
inline void log_string(std::string_view str) { log_new(str, EXTRA_WIZNET_DEBUG, 0); }
template <typename... Args>
void log_string(std::string_view str, Args &&... args) {
    log_new(fmt::format(str, std::forward<Args>(args)...), EXTRA_WIZNET_DEBUG, 0);
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