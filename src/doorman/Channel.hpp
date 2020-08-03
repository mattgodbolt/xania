#pragma once

#include "Byte.hpp"
#include "Fd.hpp"
#include "IdAllocator.hpp"
#include "Logger.hpp"
#include "TelnetProtocol.hpp"

#include <cstdint>
#include <gsl/span>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>

class Doorman;
class Xania;

class ChannelBase {
public:
    virtual ~ChannelBase() = default;
    [[nodiscard]] virtual const IdAllocator::Reservation &reservation() const = 0;
    virtual void send_to_client(std::string_view message) = 0;
    virtual void set_echo(bool echo) = 0;
    virtual void close() = 0;
    virtual void on_auth(std::string_view name) = 0;
    virtual void mark_disconnected() noexcept = 0;
    virtual void on_reconnect_attempt() = 0;
    virtual void send_connect_packet() = 0;
};

class Channel : private TelnetProtocol::Handler, public ChannelBase {
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

    [[nodiscard]] const IdAllocator::Reservation &reservation() const override { return id_; }
    void send_connect_packet() override;
    void close() override;

    void on_auth(std::string_view name) override;
    void on_reconnect_attempt() override;
    void mark_disconnected() noexcept override { connected_ = false; }
    void send_to_client(std::string_view message) override { send_to_client(message.data(), message.size()); }

    void set_echo(bool echo) override;

    [[nodiscard]] int set_fds(fd_set &input_fds, fd_set &exception_fds) noexcept;
    void check_fds(const fd_set &input_fds, const fd_set &exception_fds);
};
