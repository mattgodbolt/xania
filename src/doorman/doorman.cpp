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

bool IncomingData(int fd, const byte *incoming_data, int nBytes);

/* Global variables */
Xania mud;
pid_t child;
int port = 9000; // TODO!
int listenSock;
int debug = 0;

/* printf() to an fd */
__attribute__((format(printf, 2, 3))) bool fdprintf(int fd, const char *format, ...) {
    char buffer[4096];
    int len;

    va_list args;
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    int numWritten = write(fd, buffer, len);
    return numWritten == len;
}

/* printf() to everyone */
__attribute__((format(printf, 1, 2))) void wall(const char *format, ...) {
    char buffer[4096];

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (auto &chan : channels)
        chan.send_to_client(buffer, len);
}

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

/* Create a new connection */
void NewConnection(int fd, sockaddr_in address) {
    auto channelIter = std::find_if(begin(channels), end(channels), [](const Channel &c) { return c.is_closed(); });
    if (channelIter == end(channels)) {
        fdprintf(fd, "Xania is out of channels!\n\rTry again soon\n\r");
        close(fd);
        log_out("Rejected connection - out of channels");
        return;
    }

    channelIter->newConnection(fd, address);
}

int32_t FindChannelId(int fd) {
    if (fd == 0) {
        log_out("Panic!  fd==0 in FindChannel!  Bailing out with -1");
        return -1;
    }
    if (auto it = std::find_if(begin(channels), end(channels), [&](const Channel &chan) { return chan.has_fd(fd); });
        it != end(channels))
        return static_cast<int32_t>(std::distance(begin(channels), it));
    return -1;
}

Channel *FindChannel(int fd) {
    if (auto channelId = FindChannelId(fd); channelId >= 0)
        return &channels[channelId];
    return nullptr;
}

bool IncomingData(int fd, const byte *incoming_data, int nBytes) {
    if (auto *channel = FindChannel(fd))
        return channel->incomingData(incoming_data, nBytes);
    log_out("Oh dear - I got data on fd %d, but no channel!", fd);
    return false;
}

/*
 * Aborts a connection - closes the socket
 * and tells the MUD that the channel has closed
 */
void AbortConnection(int fd) {
    auto channelPtr = FindChannel(fd);
    if (!channelPtr) {
        log_out("Erk - unable to find channel for fd %d", fd);
        return;
    }
    mud.send_close_msg(*channelPtr);
    channelPtr->closeConnection();
}

void MudHasCrashed(int sig) {
    (void)sig;
    mud.close();
}

/*
 * Deal with new connections from outside
 */
void ProcessNewConnection() {
    sockaddr_in incoming{};
    socklen_t len = sizeof(incoming);
    int newFD;

    newFD = accept(listenSock, (sockaddr *)&incoming, &len);
    if (newFD == -1) {
        log_out("Unable to accept new connection!");
        perror("accept");
        return;
    }
    NewConnection(newFD, incoming);
}

/*
 * The main loop
 */
void ExecuteServerLoop() {
    mud.poll();

    fd_set input_fds, exception_fds;
    FD_ZERO(&input_fds);
    FD_ZERO(&exception_fds);
    FD_SET(listenSock, &input_fds);
    int maxFd = listenSock;
    if (mud.fd_ok()) {
        FD_SET(mud.fd(), &input_fds);
        FD_SET(mud.fd(), &exception_fds);
        maxFd = std::max(maxFd, mud.fd());
    }
    for (auto &channel : channels)
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
    if (mud.connected() && FD_ISSET(mud.fd(), &exception_fds)) {
        MudHasCrashed(0);
        return; /* Prevent falling into packet handling */
    }
    for (int i = 0; i <= maxFd; ++i) {
        /* Kick out the freaky folks */
        if (FD_ISSET(i, &exception_fds)) {
            AbortConnection(i);
        } else if (FD_ISSET(i, &input_fds)) { // Pending incoming data
            /* Is it the listening connection? */
            if (i == listenSock) {
                ProcessNewConnection();
            } else if (i == mud.fd()) {
                mud.process_mud_message();
            } else {
                /* It's a message from a user or ident - dispatch it */
                if (auto it = std::find_if(begin(channels), end(channels),
                                           [&](const Channel &chan) { return chan.is_hostname_fd(i); });
                    it != end(channels)) {
                    it->incomingHostnameInfo();
                } else {
                    int nBytes;
                    byte buf[256];
                    do {
                        nBytes = read(i, buf, sizeof(buf));
                        if (nBytes <= 0) {
                            int32_t channelId = FindChannelId(i);
                            if (nBytes < 0) {
                                log_out("[%d] Received error %d (%s) on read - closing connection", channelId, errno,
                                        strerror(errno));
                            }
                            AbortConnection(i);
                        } else {
                            if (nBytes > 0)
                                IncomingData(i, buf, nBytes);
                        }
                        // See how many more bytes we can read
                        nBytes = 0; // XXX Need to look this ioctl up
                    } while (nBytes > 0);
                }
            }
        }
    }
}

void CheckForDeadLookups() {
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
            if (auto chanIt = std::find_if(begin(channels), end(channels),
                                           [&](const Channel &chan) { return chan.has_lookup_pid(waiter); });
                chanIt != end(channels)) {
                chanIt->on_lookup_died(status);
            }
        }
    } while (waiter);
}

// Our luvverly options go here :
option OurOptions[] = {
    {"port", 1, nullptr, 'p'}, {"debug", 0, &debug, 1}, {"help", 0, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

void usage() { fprintf(stderr, "Usage: doorman [-h | --help] [-d | --debug] [-p | --port port] [port]\n\r"); }

// Here we go:

int main(int argc, char *argv[]) {
    protoent *tcpProto = getprotobyname("tcp");
    int yup = 1;
    linger linger = {1, 2}; // linger seconds
    sockaddr_in sin{};

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
     * Initialise the channel array
     */
    for (int32_t i = 0; i < static_cast<int32_t>(channels.size()); ++i)
        channels[i].set_id(i);

    /*
     * Bit of 'Hello Mum'
     */
    log_out("Doorman version 0.2 starting up");
    log_out("Attempting to bind to port %d", port);

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
     * Create a tcp socket, bind and listen on it
     */
    if (tcpProto == nullptr) {
        log_out("Unable to get TCP number!");
        exit(1);
    }
    listenSock = socket(PF_INET, SOCK_STREAM, tcpProto->p_proto);
    if (listenSock < 0) {
        log_out("Unable to create a socket!");
        exit(1);
    }

    /*
     * Prevent crashing on SIGPIPE if the MUD goes down, or if a connection
     * goes funny
     */
    signal(SIGPIPE, SIG_IGN);

    /*
     * Set up a few options
     */
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup)) < 0) {
        log_out("Unable to listen!");
        exit(1);
    }
    if (setsockopt(listenSock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) < 0) {
        log_out("Unable to listen!");
        exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenSock, (sockaddr *)&sin, sizeof(sin)) < 0) {
        log_out("Unable to bind socket!");
        exit(1);
    }
    if (listen(listenSock, 4) < 0) {
        log_out("Unable to listen!");
        exit(1);
    }

    log_out("Doorman is ready to rock on port %d", port);

    /*
     * Loop forever!
     */
    for (;;) {
        // Do all the nitty-gritties
        ExecuteServerLoop();
        // Ensure we don't leave zombie processes hanging around
        CheckForDeadLookups();
    }
}
