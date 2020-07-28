#pragma once

#include "Channel.hpp"
#include "Fd.hpp"
#include "Xania.hpp"

#include <array>
#include <gsl/span>

class Doorman {
    int port_;
    Xania mud_;
    Fd listenSock_;
    static constexpr auto MaxChannels = 64;
    std::array<Channel, MaxChannels> channels_;

    [[nodiscard]] int32_t find_channel_id(int fd) const;
    [[nodiscard]] Channel *find_channel(int fd);
    void on_incoming_data(int fd, gsl::span<const byte> data);
    void abort_connection(int fd);
    void accept_new_connection();
    void socket_poll();
    void check_for_dead_lookups();

public:
    explicit Doorman(int port);
    void poll();

    [[nodiscard]] Channel *find_channel_by_id(int32_t channel_id);
    template <typename Func>
    void for_each_channel(Func func) {
        for (auto &chan : channels_)
            func(chan);
    }
    void broadcast(std::string_view message);

    [[nodiscard]] int port() const { return port_; }
};

