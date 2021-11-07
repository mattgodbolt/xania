/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

// Sits on a port, and relays connections through a named pipe to the MUD itself, thus acting as a link between the MUD
// and the player independent of the MUD state. If the MUD crashes, or is rebooted, it should be able to maintain
// connections, and reconnect automatically.
//
// Terminal issues are resolved here now, which means ANSI-compliant terminals should automatically enable colour, and
// word-wrapping is done by doorman instead of the MUD, so if you have a wide display you can take advantage of it.

#include "Doorman.hpp"
#include "Logger.hpp"
#include "common/Configuration.hpp"
#include "version.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <lyra/lyra.hpp>

#include <csignal>

int Main(Logger &log, int argc, char *argv[]) {
    bool help = false;
    bool debug = false;
    auto cli = lyra::cli() | lyra::help(help).description("Connection \"bouncer\" for the mud.")("show this help")
               | lyra::opt(debug)["-d"]["--debug"]("enable debugging");

    auto result = cli.parse({argc, argv});
    if (!result) {
        fmt::print("Error in command line: {}\n", result.errorMessage());
        return 1;
    } else if (help) {
        fmt::print("{}", cli);
        return 0;
    }

    if (debug) {
        log.info("Debugging logging enabled");
        set_log_level(spdlog::level::debug);
    }

    log.info("Doorman version {} starting up", get_build_version());

    // Prevent crashing on SIGPIPE if the MUD goes down, or if a connection goes funny.
    signal(SIGPIPE, SIG_IGN);

    Doorman doorman(Configuration::singleton().port());

    // Loop forever.
    for (;;) {
        doorman.poll();
    }
}

int main(int argc, char *argv[]) {
    auto log = logger_for("main");
    try {
        return Main(log, argc, argv);
    } catch (const std::runtime_error &re) {
        log.error("{}", re.what());
        return 1;
    }
}
