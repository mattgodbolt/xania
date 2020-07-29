#include "Logger.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace {
std::shared_ptr<spdlog::sinks::sink> console_sink;
}

Logger logger_for(std::string name) {
    if (!console_sink)
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    return spdlog::logger(std::move(name), console_sink);
}