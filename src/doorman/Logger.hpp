#pragma once

#include <string>

#include <spdlog/logger.h>

using Logger = spdlog::logger;

void set_log_level(spdlog::level::level_enum level);
Logger logger_for(std::string name);