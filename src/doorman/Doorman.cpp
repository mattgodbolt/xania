#include "Doorman.hpp"
#include "Channel.hpp"
#include "Xania.hpp"
#include "common/doorman_protocol.h"

#include <algorithm>
#include <string_view>
#include <vector>

using namespace std::literals;

Doorman::Doorman(unsigned int port) : log_(logger_for("Doorman")), port_(port), mud_(*this) {
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
    if (nFDs <= 0)
        return;

    if (mud_.fd_ok()) {
        // Mud handling.
        if (FD_ISSET(mud_.fd().number(), &exception_fds)) {
            log_.warn("Got exception from MUD file descriptor");
            mud_.close();
        } else if (FD_ISSET(mud_.fd().number(), &input_fds))
            mud_.process_mud_message();
    }

    if (FD_ISSET(listenSock_.number(), &input_fds))
        accept_new_connection();

    for_each_channel([&](Channel &channel) { channel.check_fds(input_fds, exception_fds); });
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

        auto id = id_allocator_.reserve();
        auto insert_rec =
            channels_.emplace(id->id(), std::make_unique<Channel>(*this, mud_, id, std::move(newFd), incoming));
        if (!insert_rec.second) {
            // This should be impossible! If this happens we'll silently close the connection.
            log_.error("Reused channel ID {}!", id->id());
        }
    } catch (const std::runtime_error &re) {
        log_.warn("Unable to accept new connection: {}", re.what());
    }
}

Channel *Doorman::find_channel_by_id(int32_t channel_id) {
    if (auto it = channels_.find(channel_id); it != channels_.end())
        return it->second.get();
    return nullptr;
}

void Doorman::broadcast(std::string_view message) {
    for_each_channel([&](Channel &chan) {
        try {
            chan.send_to_client(message);
        } catch (const std::runtime_error &re) {
            log_.debug("Error while broadcasting to {}: {}", chan.reservation()->id(), re.what());
            chan.close();
        }
    });
}
