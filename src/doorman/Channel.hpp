#pragma once

#include "Byte.hpp"
#include "Fd.hpp"
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

class Channel : private TelnetProtocol::Handler {
    mutable Logger log_;

    Doorman &doorman_;
    Xania &mud_;
    int32_t id_;
    Fd fd_;
    std::string hostname_;
    TelnetProtocol telnet_{*this};
    uint16_t port_;
    uint32_t netaddr_;
    bool sent_reconnect_message_ = false;
    bool connected_ = false; // We've sent a connect packet to the mud
    pid_t host_lookup_pid_{}; // PID of look-up process
    Fd host_lookup_fd_; // Receive end of a pipe to the look-up process
    std::string auth_char_name_; // name of authorized character

    void async_lookup_process(sockaddr_in address, Fd reply_fd);

    void send_to_client(const void *data, size_t length) const {
        if (fd_.is_open())
            fd_.write(data, length);
    }
    void send_info_packet() const;
    void close_silently(); // used from the lookup thread.
    void on_data(gsl::span<const byte> incoming_data);
    void send_bytes(gsl::span<const byte> data) override;
    void on_line(std::string_view line) override;
    void on_terminal_size(int width, int height) override;
    void on_terminal_type(std::string_view type, bool ansi_supported) override;

public:
    explicit Channel(Doorman &doorman, Xania &xania, int32_t id, Fd fd, const sockaddr_in &address);

    Channel(const Channel &) = delete;
    Channel(Channel &&) = delete;
    Channel &operator=(const Channel &) = delete;
    Channel &operator=(Channel &&) = delete;

    [[nodiscard]] int32_t id() const { return id_; }
    void send_connect_packet();
    void on_host_info();
    void close();

    void on_auth(std::string_view name);
    [[nodiscard]] const std::string &authed_name() const { return auth_char_name_; }
    void on_reconnect_attempt();
    void mark_disconnected() noexcept { connected_ = false; }
    void send_to_client(gsl::span<const char> span) const { send_to_client(span.data(), span.size_bytes()); }
    void send_to_client(gsl::span<const byte> span) const { send_to_client(span.data(), span.size_bytes()); }

    // Intention here is to prevent direct access to the fd.
    [[nodiscard]] bool has_fd(int fd) const { return fd == fd_.number(); }
    [[nodiscard]] bool is_closed() const { return !fd_.is_open(); }

    void set_echo(bool echo);

    [[nodiscard]] int set_fds(fd_set &input_fds, fd_set &exception_fds) noexcept;

    [[nodiscard]] bool is_hostname_fd(int fd) const {
        return fd_.is_open() && host_lookup_fd_.is_open() && host_lookup_fd_.number() == fd;
    }

    [[nodiscard]] bool has_lookup_pid(pid_t pid) const { return fd_.is_open() && host_lookup_pid_ == pid; }
    void on_lookup_died(int status);
    void on_data_available();
};
