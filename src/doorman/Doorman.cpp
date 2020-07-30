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

Doorman::Doorman(int port) : log_(logger_for("Doorman")), port_(port), mud_(*this) {
    log_.info("Attempting to bind to port {}", port);
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

    log_.info("Doorman is ready to rock on port {}", port);
}

void Doorman::poll() {
    mud_.poll();
    socket_poll();
    remove_dead_channels();
}

void Doorman::remove_dead_channels() {
    // As channels can get deleted while we're iterating over the master list of channels, we protect ourselves from the
    // "delete this" problem by deferring their actual removal until we know that no calls to the Channel are on the
    // call stack.
    for (auto id : channels_to_remove_) {
        log_.debug("Removed channel {} from master channel list", id);
        channels_.erase(id);
    }
    channels_to_remove_.clear();
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
        log_.error("Unable to select()!");
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
                log_.error("Erk! Unable to find channel for FD {} on exception!", i);
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
                auto *channel = find_channel_by_fd(i);
                if (!channel) {
                    log_.error("Erk! Unable to find channel for FD {}!", i);
                    continue;
                }
                channel->on_data_available();
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
            log_.warn("Rejected connection - out of channels");
            return;
        }

        auto id = next_channel_id_++; // TODO after 2 billion we're in a bad place™ (UB for a
        auto insert_rec = channels_.try_emplace(id, *this, mud_, id, std::move(newFd), incoming);
        if (!insert_rec.second) {
            // This should be impossible! If this happens we'll silently close the connection.
            // TODO rethink this; either wrap and retry, but probably need a "ban list" of recently-recycyled IDs.
            // A lot of sophistication for essentially a "after 2 billion connection attempts" problem.
            log_.error("Reused channel ID {}!", id);
        }
    } catch (const std::runtime_error &re) {
        log_.warn("Unable to accept new connection: {}", re.what());
    }
}

Channel *Doorman::find_channel_by_fd(int fd) {
    if (fd == 0) {
        log_.error("Panic! fd==0 in find_channel_by_fd!  Bailing out with nullptr");
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
            log_.debug("Error while broadcasting to {}: {}", chan.id(), re.what());
            chan.close();
        }
    });
}
