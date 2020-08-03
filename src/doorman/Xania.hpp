#pragma once

#include "Fd.hpp"
#include "IdAllocator.hpp"
#include "Logger.hpp"
#include "doorman_protocol.h"

#include <chrono>
#include <string_view>
#include <unordered_map>

class ChannelBase;
class Doorman;

class Xania {
    Logger log_;

    Doorman &doorman_;
    Fd fd_;
    enum class State { Disconnected, ConnectedAwaitingInit, Connected };
    State state_{State::Disconnected};
    std::chrono::system_clock::time_point last_connect_attempt_;
    std::unordered_map<uint32_t, IdAllocator::Reservation> ids_known_to_mud_;

    void try_connect();

public:
    explicit Xania(Doorman &doorman);
    [[nodiscard]] bool fd_ok() const { return state_ != State::Disconnected; }
    [[nodiscard]] bool connected() const { return state_ == State::Connected; }
    [[nodiscard]] const Fd &fd() const { return fd_; }

    void poll();

    bool send_to_mud(const Packet &p, const void *payload = nullptr);
    void close();

    void process_mud_message();
    void send_connect(const ChannelBase &channel);
    void send_close_msg(const ChannelBase &channel);
    void on_client_message(const ChannelBase &channel, std::string_view message) const;
    void handle_init();
    void handle_channel_message(const Packet &p, const std::string &payload, ChannelBase &channel);
};
