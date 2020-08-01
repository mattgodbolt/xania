#include "Logger.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace {
std::shared_ptr<spdlog::sinks::sink> console_sink;
}

Logger logger_for(std::string name) {
    if (!console_sink)
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = spdlog::logger(std::move(name), console_sink);
    logger.set_level(spdlog::default_logger()->level());
    return logger;
}

void set_log_level(spdlog::level::level_enum level) { spdlog::set_level(level); }
