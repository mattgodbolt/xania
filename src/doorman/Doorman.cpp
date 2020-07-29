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
    remove_dead_channels();
}

void Doorman::remove_dead_channels() {
    // As channels can get deleted while we're iterating over the master list of channels, we protect ourselves from the
    // "delete this" problem by deferring their actual removal until we know that no calls to the Channel are on the
    // call stack.
    for (auto id : channels_to_remove_) {
        log_out("Removed channel %d from master channel list", id);
        channels_.erase(id);
    }
    channels_to_remove_.clear();
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
            if (auto pair_it =
                    std::find_if(begin(channels_), end(channels_),
                                 [&](const auto &id_and_chan) { return id_and_chan.second.has_lookup_pid(waiter); });
                pair_it != end(channels_)) {
                pair_it->second.on_lookup_died(status);
            }
        }
    } while (waiter);
}

void Doorman::socket_poll() {
    fd_set input_fds, exception_fds;
    FD_ZERO(&input_fds);
    FD_ZERO(&exception_fds);
    FD_SET(listenSock_.number(), &input_fds);
    int max_fd = listenSock_.number();
    if (mud_.fd_ok()) {
        FD_SET(mud_.fd().number(), &input_fds);
        FD_SET(mud_.fd().number(), &exception_fds);
        max_fd = std::max(max_fd, mud_.fd().number());
    }
    for_each_channel([&](Channel &channel) { max_fd = std::max(channel.set_fds(input_fds, exception_fds), max_fd); });

    timeval timeOut = {1, 0}; // Wakes up once a second to do housekeeping
    int nFDs = select(max_fd + 1, &input_fds, nullptr, &exception_fds, &timeOut);
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
    for (int i = 0; i <= max_fd; ++i) {
        /* Kick out the freaky folks */
        if (FD_ISSET(i, &exception_fds)) {
            auto *channel = find_channel_by_fd(i);
            if (!channel) {
                log_out("Erk! Unable to find channel for FD %d on exception!", i);
                continue;
            }
            channel->close();
        } else if (FD_ISSET(i, &input_fds)) { // Pending incoming data
            /* Is it the listening connection? */
            if (i == listenSock_.number()) {
                accept_new_connection();
            } else if (mud_.fd().is_open() && i == mud_.fd().number()) {
                mud_.process_mud_message();
            } else {
                /* It's a message from a user or ident - dispatch it */
                if (auto it =
                        std::find_if(begin(channels_), end(channels_),
                                     [&](const auto &id_and_chan) { return id_and_chan.second.is_hostname_fd(i); });
                    it != end(channels_)) {
                    it->second.on_host_info();
                } else {
                    auto *channel = find_channel_by_fd(i);
                    if (!channel) {
                        log_out("Erk! Unable to find channel for FD %d!", i);
                        continue;
                    }
                    channel->on_data_available();
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

        if (channels_.size() == MaxChannels) {
            newFd.write("Xania is out of channels!\n\rTry again soon\n\r"sv);
            log_out("Rejected connection - out of channels");
            return;
        }

        auto id = next_channel_id_++; // TODO after 2 billion we're in a bad place™ (UB for a
        auto insert_rec = channels_.try_emplace(id, *this, mud_, id, std::move(newFd), incoming);
        if (!insert_rec.second) {
            // This should be impossible! If this happens we'll silently close the connection.
            // TODO rethink this; either wrap and retry, but probably need a "ban list" of recently-recycyled IDs.
            // A lot of sophistication for essentially a "after 2 billion connection attempts" problem.
            log_out("Reused channel ID %d!", id);
        }
    } catch (const std::runtime_error &re) {
        log_out("Unable to accept new connection: %s", re.what());
    }
}

Channel *Doorman::find_channel_by_fd(int fd) {
    if (fd == 0) {
        log_out("Panic! fd==0 in find_channel_by_fd!  Bailing out with nullptr");
        return nullptr;
    }
    // TODO not this. Either we go full epoll() or we have a map for this.
    if (auto it = std::find_if(begin(channels_), end(channels_),
                               [&](const auto &id_and_chan) { return id_and_chan.second.has_fd(fd); });
        it != end(channels_))
        return &it->second;
    return nullptr;
}

Channel *Doorman::find_channel_by_id(int32_t channel_id) {
    if (auto it = channels_.find(channel_id); it != channels_.end())
        return &it->second;
    return nullptr;
}

void Doorman::broadcast(std::string_view message) {
    for_each_channel([&](Channel &chan) {
        try {
            chan.send_to_client(message);
        } catch (const std::runtime_error &re) {
            log_out("Error while broadcasting to %d: %s", chan.id(), re.what());
            chan.close();
        }
    });
}
