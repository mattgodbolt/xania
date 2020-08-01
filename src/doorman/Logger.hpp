#pragma once

#include <string>

#include <spdlog/logger.h>

using Logger = spdlog::logger;

// Sets the default root log level, and for any logger subsequently created by logger_for.
// Probably only worthwhile calling before any Loggers are created (e.g. in main()).
void set_log_level(spdlog::level::level_enum level);
Logger logger_for(std::string name);
