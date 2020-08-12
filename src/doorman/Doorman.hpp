#pragma once

#include "Channel.hpp"
#include "IdAllocator.hpp"
#include "Logger.hpp"
#include "Xania.hpp"
#include "common/Fd.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class Doorman {
    Logger log_;
    int port_;
    Xania mud_;
    Fd listenSock_;
    static constexpr auto MaxChannels = 64;
    IdAllocator id_allocator_;
    std::unordered_map<uint32_t, std::unique_ptr<Channel>> channels_;
    std::unordered_set<uint32_t> channels_to_remove_;

    void accept_new_connection();
    void socket_poll();
    void remove_dead_channels();

public:
    explicit Doorman(int port);
    void poll();

    [[nodiscard]] Channel *find_channel_by_id(int32_t channel_id);

    template <typename Func>
    void for_each_channel(Func func) {
        for (auto &id_and_chan : channels_)
            func(*id_and_chan.second);
    }
    void broadcast(std::string_view message);

    [[nodiscard]] int port() const { return port_; }

    // Should only be called by the channel.
    bool schedule_remove(const Channel &channel) {
        return channels_to_remove_.insert(channel.reservation()->id()).second;
    }
};
