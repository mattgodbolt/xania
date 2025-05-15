#include "common/Configuration.hpp"
#include "db.h"
#include "pfu.hpp"
#include "update.hpp"

#include "MudImpl.hpp"
#include <fmt/format.h>
#include <lyra/lyra.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// pfu - player file upgrade utility for bulk upgrading all player files
// as part of a release. Especially where the release contains code changes
// that will affect character stats or anything else that ought to be
// persisted in the pfiles.
// This is intended to be used with the prepare-pfiles.sh wrapper script.
//
// There are some limitations right now:
// - There's only a single "loader" for pfiles, the default one used by the mud.
//   But this may need to be refactored to support a separate loader for legacy formats.
// - Likewise there's only a single "saver", the one used by the mud.
//   In due course this can probably be refactored to introduce
//   a new saver that writes the pfiles out in a different format e.g. yaml.
// - It could be enhanced to have cmd line arg to support processing a specific file.

template <>
struct fmt::formatter<lyra::cli> : ostream_formatter {};

int main(int argc, const char **argv) {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    auto logger = spdlog::logger("pfu", sink);
    bool help{};
    bool verbose{};
    auto cli = lyra::cli()
               | lyra::help(help).description("Perform upgrades on a copy of production or test player files")
               | lyra::opt(verbose)["-V"]("verbose logging");

    auto result = cli.parse({argc, argv});
    if (!result) {
        fmt::print("Error in command line: {}\n", result.message());
        exit(1);
    } else if (help) {
        fmt::print("{}", cli);
        exit(0);
    }
    if (verbose) {
        logger.set_level(spdlog::level::debug);
    }
    const auto mud = std::make_unique<MudImpl>();
    const auto player_dir = mud->config().player_dir();
    const auto names = pfu::collect_player_names(player_dir, logger);
    if (names.empty()) {
        logger.critical("No players found!");
        exit(1);
    }
    boot_db(*mud.get());
    pfu::upgrade_players(*mud.get(), names, logger);
    collect_all_garbage();
}
