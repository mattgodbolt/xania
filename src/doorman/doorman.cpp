/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  doorman.c                                                            */
/*                                                                       */
/*************************************************************************/

/*
 * Doorman.c
 * Stand-alone 'doorman' for the MUD
 * Sits on port 9000, and relays connections through a named pipe
 * to the MUD itself, thus acting as a link between the MUD
 * and the player independant of the MUD state.
 * If the MUD crashes, or is rebooted, it should be able to maintain
 * connections, and reconnect automatically
 *
 * Terminal issues are resolved here now, which means ANSI-compliant terminals
 * should automatically enable colour, and word-wrapping is done by doorman
 * instead of the MUD, so if you have a wide display you can take advantage
 * of it.
 *
 */

#include "doorman.h"
#include "Channel.hpp"
#include "Xania.hpp"
#include "doorman.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std::literals;

/* log_out() replaces fprintf (stderr,...) */
__attribute__((format(printf, 1, 2))) void log_out(const char *format, ...) {
    char buffer[4096], *Time;
    time_t now;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    time(&now);
    Time = ctime(&now);
    Time[strlen(Time) - 1] = '\0'; // Kill the newline
    fprintf(stderr, "%d::%s %s\n", getpid(), Time, buffer);
}

unsigned long djb2_hash(std::string_view str) {
    unsigned long hash = 5381;
    for (auto c : str)
        hash = hash * 33 + c;
    return hash;
}

/**
 * Returns the hostname, masked for privacy and with a hashcode of the full hostname. This can be used by admins
 * to spot users coming from the same IP.
 */
std::string get_masked_hostname(std::string_view hostname) {
    return fmt::format("{}*** [#{}]", hostname.substr(0, 6), djb2_hash(hostname));
}

void usage() { fprintf(stderr, "Usage: doorman [-h | --help] [-d | --debug] [-p | --port port] [port]\n\r"); }

int Main(int argc, char *argv[]) {
    int debug = 0;
    int port = 9000;
    option OurOptions[] = {
        {"port", 1, nullptr, 'p'}, {"debug", 0, &debug, 1}, {"help", 0, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

    /*
     * Parse any arguments
     */
    port = 9000;
    for (;;) {
        int optType;
        int index = 0;

        optType = getopt_long(argc, argv, "dp:h", OurOptions, &index);
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

Doorman::Doorman(int port) : port_(port), mud_(*this) {
    log_out("Attempting to bind to port %d", port);
    for (int32_t i = 0; i < static_cast<int32_t>(channels_.size()); ++i)
        channels_[i].initialise(*this, mud_, i);

    protoent *tcpProto = getprotobyname("tcp");
    sockaddr_in sin{};
    if (tcpProto == nullptr) {
        log_out("Unable to get TCP number!");
        exit(1);
    }
    listenSock_ = Fd::socket(PF_INET, SOCK_STREAM, tcpProto->p_proto);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    listenSock_.setsockopt(SOL_SOCKET, SO_REUSEADDR, static_cast<int>(1))
        .setsockopt(SOL_SOCKET, SO_LINGER, linger{true, 2})
        .bind(sin)
        .listen(4);

    log_out("Doorman is ready to rock on port %d", port);
}

void Doorman::poll() {
    mud_.poll();
    socket_poll();
    check_for_dead_lookups();
}

void Doorman::check_for_dead_lookups() {
    pid_t waiter;
    int status;

    do {
        waiter = waitpid(-1, &status, WNOHANG);
        if (waiter == -1) {
            break; // Usually 'no child processes'
        }
        if (waiter) {
            // Clear the zombie status
            waitpid(waiter, nullptr, 0);
            log_out("Lookup process %d died", waiter);
            // Find the corresponding channel, if still present
            if (auto chanIt = std::find_if(begin(channels_), end(channels_),
                                           [&](const Channel &chan) { return chan.has_lookup_pid(waiter); });
                chanIt != end(channels_)) {
                chanIt->on_lookup_died(status);
            }
        }
    } while (waiter);
}

void Doorman::socket_poll() {
    fd_set input_fds, exception_fds;
    FD_ZERO(&input_fds);
    FD_ZERO(&exception_fds);
    FD_SET(listenSock_.number(), &input_fds);
    int maxFd = listenSock_.number();
    if (mud_.fd_ok()) {
        FD_SET(mud_.fd().number(), &input_fds);
        FD_SET(mud_.fd().number(), &exception_fds);
        maxFd = std::max(maxFd, mud_.fd().number());
    }
    for (auto &channel : channels_)
        maxFd = std::max(channel.set_fds(input_fds, exception_fds), maxFd);

    timeval timeOut = {1, 0}; // Wakes up once a second to do housekeeping
    int nFDs = select(maxFd + 1, &input_fds, nullptr, &exception_fds, &timeOut);
    if (nFDs == -1 && errno != EINTR) {
        log_out("Unable to select()!");
        perror("select");
        exit(1);
    }
    /* Anything happened in the last 10 seconds? */
    if (nFDs <= 0)
        return;
    /* Has the MUD crashed out? */
    if (mud_.connected() && FD_ISSET(mud_.fd().number(), &exception_fds)) {
        mud_.close();
        return; /* Prevent falling into packet handling */
    }
    for (int i = 0; i <= maxFd; ++i) {
        /* Kick out the freaky folks */
        if (FD_ISSET(i, &exception_fds)) {
            abort_connection(i);
        } else if (FD_ISSET(i, &input_fds)) { // Pending incoming data
            /* Is it the listening connection? */
            if (i == listenSock_.number()) {
                accept_new_connection();
            } else if (mud_.fd().is_open() && i == mud_.fd().number()) {
                mud_.process_mud_message();
            } else {
                /* It's a message from a user or ident - dispatch it */
                if (auto it = std::find_if(begin(channels_), end(channels_),
                                           [&](const Channel &chan) { return chan.is_hostname_fd(i); });
                    it != end(channels_)) {
                    it->incomingHostnameInfo();
                } else {
                    int nBytes;
                    byte buf[256];
                    do {
                        nBytes = read(i, buf, sizeof(buf));
                        if (nBytes <= 0) {
                            int32_t channelId = find_channel_id(i);
                            if (nBytes < 0) {
                                log_out("[%d] Received error %d (%s) on read - closing connection", channelId, errno,
                                        strerror(errno));
                            }
                            abort_connection(i);
                        } else {
                            if (nBytes > 0)
                                on_incoming_data(i, gsl::span<const byte>(buf, buf + nBytes));
                        }
                        // See how many more bytes we can read
                        nBytes = 0; // XXX Need to look this ioctl up
                    } while (nBytes > 0);
                }
            }
        }
    }
}
void Doorman::accept_new_connection() {
    sockaddr_in incoming{};
    socklen_t len = sizeof(incoming);
    try {
        auto newFd = listenSock_.accept(reinterpret_cast<sockaddr *>(&incoming), &len);

        auto channelIter =
            std::find_if(begin(channels_), end(channels_), [](const Channel &c) { return c.is_closed(); });
        if (channelIter == end(channels_)) {
            newFd.write("Xania is out of channels!\n\rTry again soon\n\r"sv);
            newFd.close();
            log_out("Rejected connection - out of channels");
            return;
        }

        channelIter->newConnection(std::move(newFd), incoming);
    } catch (const std::runtime_error &re) {
        log_out("Unable to accept new connection: %s", re.what());
    }
}
void Doorman::abort_connection(int fd) {
    auto channelPtr = find_channel(fd);
    if (!channelPtr) {
        log_out("Erk - unable to find channel for fd %d", fd);
        return;
    }
    // Eventually when the channel knows about the mud this we can "just" closeConnection here.
    mud_.send_close_msg(*channelPtr);
    channelPtr->closeConnection();
}

int32_t Doorman::find_channel_id(int fd) const {
    if (fd == 0) {
        log_out("Panic!  fd==0 in FindChannel!  Bailing out with -1");
        return -1;
    }
    if (auto it = std::find_if(begin(channels_), end(channels_), [&](const Channel &chan) { return chan.has_fd(fd); });
        it != end(channels_))
        return static_cast<int32_t>(std::distance(begin(channels_), it));
    return -1;
}

Channel *Doorman::find_channel(int fd) {
    if (auto channelId = find_channel_id(fd); channelId >= 0)
        return &channels_[channelId];
    return nullptr;
}

void Doorman::on_incoming_data(int fd, gsl::span<const byte> data) {
    if (auto *channel = find_channel(fd))
        channel->incomingData(data);
    else
        log_out("Oh dear - I got data on fd %d, but no channel!", fd);
}

Channel *Doorman::find_channel_by_id(int32_t channel_id) {
    if (channel_id >= 0 && channel_id < MaxChannels) {
        auto &channel = channels_[channel_id];
        if (!channel.is_closed())
            return &channel;
    }
    return nullptr;
}
void Doorman::broadcast(std::string_view message) {
    for (auto &chan : channels_) {
        try {
            chan.send_to_client(message);
        } catch (const std::runtime_error &re) {
            log_out("Error while broadcasting to %d: %s", chan.id(), re.what());
            chan.closeConnection();
        }
    }
}
