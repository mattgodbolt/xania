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
#include "Xania.hpp"

#include <csignal>
#include <fmt/format.h>
#include <getopt.h>
#include <sys/resource.h>

void usage() { fmt::print(stderr, "Usage: doorman [-h | --help] [-d | --debug] [-p | --port port] [port]\n"); }

int Main(int argc, char *argv[]) {
    int debug = 0;
    int port = 9000;
    option options[] = {
        {"port", 1, nullptr, 'p'}, {"debug", 0, &debug, 1}, {"help", 0, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

    /*
     * Parse any arguments
     */
    port = 9000;
    for (;;) {
        int optType;
        int index = 0;

        optType = getopt_long(argc, argv, "dp:h", options, &index);
        if (optType == -1)
            break;
        switch (optType) {
        case 'p':
            port = atoi(optarg);
            if (port <= 0) {
                fprintf(stderr, "Invalid port '%s'\n\r", optarg);
                usage();
                exit(1);
            }
            break;
        case 'd': debug = 1; break;
        case 'h':
            usage();
            exit(0);
            break;
        }
    }

    if (optind == (argc - 1)) {
        port = atoi(argv[optind]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port '%s'\n\r", argv[optind]);
            usage();
            exit(1);
        }
    } else if (optind < (argc - 1)) {
        usage();
        exit(1);
    }

    /*
     * Turn on core dumping under debug
     */
    if (debug) {
        rlimit coreLimit = {0, 16 * 1024 * 1024};
        int ret;
        log_out("Debugging enabled - core limit set to 16Mb");
        ret = setrlimit(RLIMIT_CORE, &coreLimit);
        if (ret < 0) {
            log_out("Unable to set limit - %d ('%s')", errno, strerror(errno));
        }
    }

    /*
     * Prevent crashing on SIGPIPE if the MUD goes down, or if a connection
     * goes funny
     */
    signal(SIGPIPE, SIG_IGN);

    Doorman doorman(port);

    /*
     * Loop forever!
     */
    for (;;) {
        doorman.poll();
    }
}

int main(int argc, char *argv[]) {
    try {
        return Main(argc, argv);
    } catch (const std::runtime_error &re) {
        log_out("Uncaught exception: %s", re.what());
        return 1;
    }
}
