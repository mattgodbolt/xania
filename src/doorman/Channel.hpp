#pragma once

#include "IdAllocator.hpp"
#include "Logger.hpp"
#include "TelnetProtocol.hpp"
#include "common/Byte.hpp"
#include "common/Fd.hpp"

#include <cstdint>
#include <gsl/span>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>

class Doorman;
class Xania;

class Channel : private TelnetProtocol::Handler {
    mutable Logger log_;

    Doorman &doorman_;
    Xania &mud_;
    IdAllocator::Reservation id_;
    Fd fd_;
    std::string hostname_;
    TelnetProtocol telnet_{*this};
    uint16_t port_;
    uint32_t netaddr_;
    bool sent_reconnect_message_ = false;
    bool connected_ = false; // We've sent a connect packet to the mud
    std::string auth_char_name_; // name of authorized character

    void send_to_client(const void *data, size_t length) const {
        if (fd_.is_open())
            fd_.write(data, length);
    }
    void send_info_packet() const;
    void on_data(gsl::span<const byte> incoming_data);
    void send_bytes(gsl::span<const byte> data) override;
    void on_line(std::string_view line) override;
    void on_terminal_size(unsigned int width, unsigned int height) override;
    void on_terminal_type(std::string_view type, bool ansi_supported) override;
    void on_data_available();

    std::string lookup(const sockaddr_in &address) const;

public:
    explicit Channel(Doorman &doorman, Xania &xania, IdAllocator::Reservation id, Fd fd, const sockaddr_in &address);

    Channel(const Channel &) = delete;
    Channel(Channel &&) = delete;
    Channel &operator=(const Channel &) = delete;
    Channel &operator=(Channel &&) = delete;

    [[nodiscard]] const IdAllocator::Reservation &reservation() const { return id_; }
    void send_connect_packet();
    void close();

    void on_auth(std::string_view name);
    [[nodiscard]] const std::string &authed_name() const { return auth_char_name_; }
    void on_reconnect_attempt();
    [[nodiscard]] bool is_connected() const noexcept { return connected_; }
    void mark_disconnected() noexcept { connected_ = false; }
    void send_to_client(gsl::span<const char> span) const { send_to_client(span.data(), span.size_bytes()); }
    void send_to_client(gsl::span<const byte> span) const { send_to_client(span.data(), span.size_bytes()); }

    [[nodiscard]] bool is_closed() const { return !fd_.is_open(); }

    void set_echo(bool echo);

    [[nodiscard]] int set_fds(fd_set &input_fds, fd_set &exception_fds) noexcept;
    void check_fds(const fd_set &input_fds, const fd_set &exception_fds);
};
