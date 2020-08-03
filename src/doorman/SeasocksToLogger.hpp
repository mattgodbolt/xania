#pragma once

#include "Logger.hpp"
#include "seasocks/Logger.h"

struct SeasocksToLogger : seasocks::Logger {
    ::Logger logger = logger_for("web");

    void log(Level level, const char *message) override {
        switch (level) {
        case Level::Access:
        case Level::Debug: logger.debug(message); break;
        case Level::Info: logger.info(message); break;
        case Level::Warning: logger.warn(message); break;
        case Level::Error:
        case Level::Severe: logger.error(message); break;
        }
    }
};
