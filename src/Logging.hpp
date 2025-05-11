#pragma once

#include "CharExtraFlag.hpp"

#include <string_view>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/printf.h>

struct DescriptorList;

class Logger {
public:
    Logger(DescriptorList &descriptors);
    Logger(Logger &&) = delete;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger &operator=(Logger &&) = delete;
    void bug(std::string_view message) const;
    template <typename... Args>
    void bug(fmt::string_view format, Args &&...args) const {
        bug(fmt::format(fmt::runtime(format), args...));
    }
    // TODO: somehow combine log_new and log_string and all that. Maybe spdlog?
    void log_new(std::string_view str, const CharExtraFlag loglevel, int level) const;
    void log_string(std::string_view str) const { log_new(str, CharExtraFlag::WiznetDebug, 0); }
    template <typename... Args>
    void log_string(fmt::string_view str, Args &&...args) const {
        log_new(fmt::format(fmt::runtime(str), args...), CharExtraFlag::WiznetDebug, 0);
    }

private:
    DescriptorList &descriptors_;
};

class BugAreaFileContext {
public:
    BugAreaFileContext(std::string_view name, FILE *file);
    ~BugAreaFileContext();

    BugAreaFileContext(const BugAreaFileContext &) = delete;
    BugAreaFileContext &operator=(const BugAreaFileContext &) = delete;
    BugAreaFileContext(BugAreaFileContext &&) = delete;
    BugAreaFileContext &operator=(BugAreaFileContext &&) = delete;
};