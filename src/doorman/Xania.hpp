#pragma once

#include "doorman.h"

#include <chrono>
#include <string_view>

class Channel;

class Xania {
    int fd_ = 0;
    enum class State { Disconnected, ConnectedAwaitingInit, Connected };
    State state_;
    std::chrono::system_clock::time_point last_connect_attempt_;

    void try_connect();

public:
    [[nodiscard]] bool fd_ok() const { return state_ != State::Disconnected; }
    [[nodiscard]] bool connected() const { return state_ == State::Connected; }
    [[nodiscard]] int fd() const { return fd_; }

    void poll();

    void SendToMUD(const Packet &p, const void *payload = nullptr);
    void close();

    void process_mud_message();
    void send_close_msg(const Channel &channel);
    void on_client_message(const Channel &channel, std::string_view message) const ;
};
