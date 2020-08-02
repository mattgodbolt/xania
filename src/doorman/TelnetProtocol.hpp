#pragma once

#include "Byte.hpp"

#include <gsl/span>

#include <vector>

class TelnetProtocol {
public:
    struct Handler {
        virtual ~Handler() = default;
        virtual void send_bytes(gsl::span<const byte> data) = 0;
        virtual void on_line(std::string_view line) = 0;
        virtual void on_terminal_size(unsigned int width, unsigned int height) = 0;
        virtual void on_terminal_type(std::string_view type, bool ansi_supported) = 0;
    };

private:
    Handler &handler_;
    std::vector<byte> buffer_;
    bool got_term_ = false;
    bool echoing_ = true;
    unsigned int width_ = 80;
    unsigned int height_ = 24;
    bool ansi_ = false;
    char previous_separator_ = 0;

    void send_com(byte a, byte b);
    void send_opt(byte a);

    // Returns the number of bytes consumed, or zero to indicate no complete subcommand found.
    size_t on_subcommand(gsl::span<const byte> command_sequence);
    // Returns the number of bytes consumed, or zero to indicate no complete command found.
    size_t on_command(gsl::span<const byte> command_sequence);
    gsl::span<const byte> interpret_iacs();
    // Calls the appropriate handler for any complete line found in data. Returns the unparsed remainder.
    std::string_view handle_whole_lines(std::string_view data);

public:
    explicit TelnetProtocol(Handler &handler) : handler_(handler) {}

    [[nodiscard]] size_t buffered_data_size() const { return buffer_.size(); }
    void add_data(gsl::span<const byte> data);
    void set_echo(bool echo);

    [[nodiscard]] bool supports_ansi() const { return ansi_; }
    void send_telopts();
};