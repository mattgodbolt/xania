#pragma once

#include "Fd.hpp"
#include "doorman_protocol.h"

#include <chrono>
#include <string_view>

class Channel;
class Doorman;

class Xania {
    Doorman &doorman_;
    Fd fd_;
    enum class State { Disconnected, ConnectedAwaitingInit, Connected };
    State state_{State::Disconnected};
    std::chrono::system_clock::time_point last_connect_attempt_;

    void try_connect();

public:
    explicit Xania(Doorman &doorman) : doorman_(doorman) {}
    [[nodiscard]] bool fd_ok() const { return state_ != State::Disconnected; }
    [[nodiscard]] bool connected() const { return state_ == State::Connected; }
    [[nodiscard]] const Fd &fd() const { return fd_; }

    void poll();

    void send_to_mud(const Packet &p, const void *payload = nullptr);
    void close();

    void process_mud_message();
    void send_close_msg(const Channel &channel);
    void on_client_message(const Channel &channel, std::string_view message) const;
};
