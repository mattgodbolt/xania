#pragma once

#include "Misc.hpp"

#include <gsl/span>

#include <vector>

class TelnetLineParser {
public:
    struct Handler {
        virtual ~Handler() = default;
        virtual void send_bytes(gsl::span<const byte> data) = 0;
        virtual void on_line(std::string_view line) = 0;
        virtual void on_terminal_size(int width, int height) = 0;
        virtual void on_terminal_type(std::string_view type, bool ansi_supported) = 0;
    };

private:
    Handler &handler_;
    std::vector<byte> buffer_;
    bool got_term_ = false;
    bool echoing_ = true;
    int width_ = 80;
    int height_ = 24;
    bool ansi_{false};

    void send_com(byte a, byte b);
    void send_opt(byte a);

public:
    explicit TelnetLineParser(Handler &handler) : handler_(handler) {}

    [[nodiscard]] size_t buffered_data_size() const { return buffer_.size(); }
    void add_data(gsl::span<const byte> data);
    void set_echo(bool echo);

    [[nodiscard]] bool supports_ansi() const { return ansi_; }
    void send_telopts();
};