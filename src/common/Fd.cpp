#include "Fd.hpp"

#include <fmt/format.h>

void Fd::write(const void *data, size_t length) const {
    if (!is_open())
        throw std::runtime_error("Write called on invalid file descriptor");
    auto num = ::write(fd_, data, length);
    if (num < 0)
        throw fmt::system_error(errno, "Unable to write to {}", fd_);
    if (static_cast<size_t>(num) != length)
        throw std::runtime_error(fmt::format("Truncated write to file descriptor {} ({}/{})", fd_, num, length));
}

void Fd::writev(gsl::span<const iovec> io_vecs) const {
    if (!is_open())
        throw std::runtime_error("Write called on invalid file descriptor");
    auto num = ::writev(fd_, io_vecs.data(), io_vecs.size());
    if (num < 0)
        throw fmt::system_error(errno, "Unable to write to {}", fd_);

    size_t length = 0;
    for (auto &v : io_vecs)
        length += v.iov_len;

    if (static_cast<size_t>(num) != length)
        throw std::runtime_error(fmt::format("Truncated write to file descriptor {} ({}/{})", fd_, num, length));
}

Fd Fd::accept(sockaddr *address, socklen_t *socklen) const {
    auto accepted_fd = ::accept(fd_, address, socklen);
    if (accepted_fd < 0)
        throw fmt::system_error(errno, "Unable to accept from file descriptor {}", fd_);
    return Fd(accepted_fd);
}

Fd Fd::socket(int domain, int type, int protocol) {
    int socket_fd = ::socket(domain, type, protocol);
    if (socket_fd < 0)
        throw fmt::system_error(
            errno, "Unable to create a socket (domain {}, type {}, protocol {})", domain, type, protocol);
    return Fd(socket_fd);
}

const Fd &Fd::setsockopt(int level, int optname, const void *optval, socklen_t optlen) const {
    if (!is_open())
        throw std::runtime_error("setsockopt called on invalid file descriptor");

    if (::setsockopt(fd_, level, optname, optval, optlen) < 0)
        throw fmt::system_error(errno, "Unable to set socket option {}:{}", level, optname);
    return *this;
}

const Fd &Fd::bind(const sockaddr *address, socklen_t socklen) const {
    // TODO: ideally would include the address in the error message but it's non-trivial to do this, and potentially
    //  could cause cascading errors.
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
    // TODO: ideally would include the address in the error message but it's non-trivial to do this, and potentially
    //  could cause cascading errors.
    if (::connect(fd_, address, socklen) < 0)
        throw fmt::system_error(errno, "Unable to connect");
    return *this;
}

void Fd::read_all(void *data, size_t length) const {
    auto num_read = ::read(fd_, data, length);
    if (num_read < 0)
        throw fmt::system_error(errno, "Unable to read from {}", fd_);
    if (static_cast<size_t>(num_read) != length)
        throw std::runtime_error(fmt::format("Truncated read from file descriptor {} ({}/{})", fd_, num_read, length));
}

size_t Fd::try_read_some(void *data, size_t length) const {
    ssize_t num_bytes = ::read(fd_, data, length);
    if (num_bytes < 0)
        throw fmt::system_error(errno, "Unable to read from {}", fd_);
    return static_cast<size_t>(num_bytes);
}
