#pragma once

#include "Channel.hpp"
#include "Fd.hpp"
#include "Xania.hpp"

#include <unordered_map>
#include <unordered_set>

#include <gsl/span>

class Doorman {
    int port_;
    Xania mud_;
    Fd listenSock_;
    static constexpr auto MaxChannels = 64;
    int32_t next_channel_id_{};
    std::unordered_map<int32_t, Channel> channels_;
    std::unordered_set<int32_t> channels_to_remove_;

    [[nodiscard]] Channel *find_channel_by_fd(int fd);
    void accept_new_connection();
    void socket_poll();
    void check_for_dead_lookups();

public:
    explicit Doorman(int port);
    void poll();

    [[nodiscard]] Channel *find_channel_by_id(int32_t channel_id);
    template <typename Func>
    void for_each_channel(Func func) {
        for (auto &[_, chan] : channels_)
            func(chan);
    }
    void broadcast(std::string_view message);

    [[nodiscard]] int port() const { return port_; }

    // Should only be called by the channel.
    bool schedule_remove(const Channel &channel) { return channels_to_remove_.insert(channel.id()).second; }
    void remove_dead_channels();
};
