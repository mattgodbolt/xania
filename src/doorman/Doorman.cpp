#include "Doorman.hpp"
#include "Channel.hpp"
#include "Xania.hpp"
#include "doorman_protocol.h"

#include <algorithm>
#include <string_view>
#include <vector>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <wait.h>

using namespace std::literals;

Doorman::Doorman(int port) : port_(port), mud_(*this) {
    log_out("Attempting to bind to port %d", port);
    for (int32_t i = 0; i < static_cast<int32_t>(channels_.size()); ++i)
        channels_[i].initialise(*this, mud_, i);

    listenSock_ = Fd::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in sin{};
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
                    it->on_host_info();
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

        channelIter->new_connection(std::move(newFd), incoming);
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
    // Eventually when the channel knows about the mud this we can "just" close_connection here.
    mud_.send_close_msg(*channelPtr);
    channelPtr->close();
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
        channel->on_data(data);
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
            chan.close();
        }
    }
}
