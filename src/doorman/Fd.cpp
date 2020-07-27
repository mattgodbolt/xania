#include "Fd.hpp"

#include <fmt/format.h>

using namespace fmt::literals;

void Fd::write(const void *data, size_t length) const {
    if (!is_open())
        throw std::runtime_error("Write called on invalid file descriptor");
    auto num = ::write(fd_, data, length);
    if (num < 0)
        throw fmt::system_error(errno, "Unable to write to {}", fd_);
    if (static_cast<size_t>(num) != length)
        throw std::runtime_error("Truncated write to file descriptor {}"_format(fd_));
}

Fd Fd::accept(const Fd &listenSock, sockaddr *address, socklen_t *socklen) {
    auto accepted_fd = ::accept(listenSock.fd_, address, socklen);
    if (accepted_fd < 0)
        throw fmt::system_error(errno, "Unable to accept from file descriptor {}", listenSock.fd_);
    return Fd(accepted_fd);
}

Fd Fd::socket(int domain, int type, int protocol) {
    int socket_fd = ::socket(domain, type, protocol);
    if (socket_fd < 0)
        throw fmt::system_error(errno, "Unable to create a socket");
    return Fd(socket_fd);
}

const Fd &Fd::setsockopt(int level, int optname, const void *optval, socklen_t optlen) const {
    if (!is_open())
        throw std::runtime_error("Setsockopt called on invalid file descriptor");

    if (::setsockopt(fd_, level, optname, optval, optlen) < 0)
        throw fmt::system_error(errno, "Unable to set socket option {}:{}", level, optname);
    return *this;
}

const Fd &Fd::bind(const sockaddr *address, socklen_t socklen) const {
    if (::bind(fd_, address, socklen) < 0)
        throw fmt::system_error(errno, "Unable to bind to address");
    return *this;
}

const Fd &Fd::listen(int backlog) const {
    if (::listen(fd_, backlog) < 0)
        throw fmt::system_error(errno, "Unable to listen");
    return *this;
}

const Fd &Fd::connect(const sockaddr *address, socklen_t socklen) const {
    if (::connect(fd_, address, socklen) < 0)
        throw fmt::system_error(errno, "Unable to connect");
    return *this;
}

void Fd::read_all(void *data, size_t length) const {
    auto num_read = ::read(fd_, data, length);
    if (num_read < 0)
        throw fmt::system_error(errno, "Unable to read from {}", fd_);
    if (static_cast<size_t>(num_read) != length)
        throw std::runtime_error("Truncated read from file descriptor {}"_format(fd_));
}
