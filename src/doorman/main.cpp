/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

/*
 * Sits on a port, and relays connections through a named pipe to the MUD itself, thus acting as a link between the MUD
 * and the player independent of the MUD state. If the MUD crashes, or is rebooted, it should be able to maintain
 * connections, and reconnect automatically
 *
 * Terminal issues are resolved here now, which means ANSI-compliant terminals should automatically enable colour, and
 * word-wrapping is done by doorman instead of the MUD, so if you have a wide display you can take advantage of it.
 */

#include "Doorman.hpp"
#include "Logger.hpp"
#include "common/Configuration.hpp"
#include "version.h"

#include <fmt/format.h>

#include <csignal>
#include <getopt.h>

void usage() { fmt::print(stderr, "Usage: doorman [-h | --help] [-d | --debug]\n"); }

int Main(Logger &log, int argc, char *argv[]) {
    int debug = 0;
    option options[] = {{"debug", 0, &debug, 1}, {"help", 0, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

    /*
     * Parse any arguments
     */
    for (;;) {
        int optType;
        int index = 0;

        optType = getopt_long(argc, argv, "dh", options, &index);
        if (optType == -1)
            break;
        switch (optType) {
        case 'd': debug = 1; break;
        case 'h':
            usage();
            exit(0);
            break;
        }
    }

    if (optind < (argc - 1)) {
        usage();
        exit(1);
    }

    if (debug) {
        log.info("Debugging logging enabled");
        set_log_level(spdlog::level::debug);
    }

    log.info("Doorman version {} starting up", BUILD_VERSION);

    /*
     * Prevent crashing on SIGPIPE if the MUD goes down, or if a connection
     * goes funny
     */
    signal(SIGPIPE, SIG_IGN);

    Doorman doorman(Configuration::singleton().port());

    /*
     * Loop forever!
     */
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
