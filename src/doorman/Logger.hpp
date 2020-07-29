#pragma once

#include <string>

#include <spdlog/logger.h>

using Logger = spdlog::logger;

Logger logger_for(std::string name);