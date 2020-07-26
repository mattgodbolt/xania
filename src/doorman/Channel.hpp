#pragma once

#include "doorman.hpp"

#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>

class Channel {
    int32_t id_{}; // the channel id
    int fd_{}; /* File descriptor of the channel */
    std::string hostname_;
    std::vector<byte> buffer_;
    uint16_t port_{};
    uint32_t netaddr_{};
    int width_{}, height_{}; /* Width and height of terminal */
    bool ansi_{}, got_term_{};
    bool echoing_{};
    bool sent_reconnect_message_{};
    bool connected_{}; /* Socket is ready to connect to Xania */
    pid_t host_lookup_pid_{}; /* PID of look-up process */
    int host_lookup_fds_[2]{}; /* FD of look-up processes pipe (read on 0) */
    std::string auth_char_name_; /* name of authorized character */

public:
    [[nodiscard]] int32_t id() const { return id_; }
    void newConnection(int fd, sockaddr_in address);
    void closeConnection();
    void sendInfoPacket() const;
    void sendConnectPacket();
    bool incomingData(const byte *incoming_data, int nBytes);
    void incomingHostnameInfo();
    void sendTelopts() const;

    void on_auth(std::string_view name) { auth_char_name_ = name; }
    [[nodiscard]] const std::string &authed_name() const { return auth_char_name_; }
    void on_reconnect_attempt();
    void mark_disconnected() { connected_ = false; }
    bool send_to_client(const void *data, size_t length) const {
        if (!fd_)
            return false;
        auto written = ::write(fd_, data, length);
        // TODO close fd_? remove it? if errored?
        return written == static_cast<ssize_t>(length);
    }

    // Intention here is to prevent direct access to the fd.
    [[nodiscard]] bool has_fd(int fd) const { return fd == fd_; }
    [[nodiscard]] bool is_closed() const { return fd_ == 0; }

    void set_echo(bool echo);
    void close();

    [[nodiscard]] int set_fds(fd_set &input_fds, fd_set &exception_fds) {
        if (!fd_)
            return 0;
        FD_SET(fd_, &exception_fds);
        FD_SET(fd_, &input_fds);
        if (host_lookup_pid_) {
            FD_SET(host_lookup_fds_[0], &input_fds);
            return std::max(host_lookup_fds_[0], fd_);
        }
        return fd_;
    }

    [[nodiscard]] bool is_hostname_fd(int fd) const { return fd_ && host_lookup_pid_ && host_lookup_fds_[0] == fd; }

    [[nodiscard]] bool has_lookup_pid(pid_t pid) const { return fd_ && host_lookup_pid_ == pid; }
    void on_lookup_died(int status);
    void set_id(int32_t id);
};

// TODO this is dreadful
#include <array>
static constexpr auto MaxChannels = 64;
extern std::array<Channel, MaxChannels> channels;
