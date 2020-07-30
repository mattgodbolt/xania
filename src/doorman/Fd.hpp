#pragma once

#include "Byte.hpp"

#include <gsl/span>

#include <utility>

#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

class Fd {
    int fd_;

    void writev(gsl::span<const iovec> io_vecs) const;

    static void *iovec_cast(const void *thing) { return const_cast<void *>(thing); }

    template <typename T>
    static iovec iovec_for(const T &some_object) {
        static_assert(std::is_trivially_copyable_v<T>);
        return {iovec_cast(&some_object), sizeof(some_object)};
    }

    static iovec iovec_for(gsl::span<const byte> bytes) { return {iovec_cast(bytes.data()), bytes.size_bytes()}; }
    static iovec iovec_for(std::string_view string) { return {iovec_cast(string.data()), string.size()}; }

    template <typename... Args>
    static std::array<iovec, sizeof...(Args)> iovecs_for(Args &&... args) {
        return {iovec_for(args)...};
    }

public:
    Fd() : fd_(-1) {}
    explicit Fd(int fd) noexcept : fd_(fd) {}
    ~Fd() noexcept { close(); }
    Fd(const Fd &) = delete;
    Fd &operator=(const Fd &) = delete;
    Fd(Fd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    Fd &operator=(Fd &&other) noexcept {
        close();
        fd_ = std::exchange(other.fd_, -1);
        return *this;
    }
    [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
    [[nodiscard]] int number() const {
        if (!is_open())
            throw std::runtime_error("Attempt to get handle for invalid file descriptor");
        return fd_;
    }
    void close() {
        if (is_open())
            ::close(fd_);
        fd_ = -1;
    }
    void write(gsl::span<const char> span) const { write(span.data(), span.size_bytes()); }
    void write(gsl::span<const byte> span) const { write(span.data(), span.size_bytes()); }
    void write(const void *data, size_t length) const;
    template <typename T>
    void write(const T &t) const {
        write(&t, sizeof(t));
    }
    template <typename... Args>
    void write_many(Args &&... args) const {
        writev(iovecs_for(std::forward<Args>(args)...));
    }

    template <typename T>
    T read_all() const {
        T object;
        read_all(&object, sizeof(object));
        return object;
    }
    template <typename T>
    void read_all(T &object) const {
        read_all(&object, sizeof(object));
    }
    void read_all(void *data, size_t length) const;

    [[nodiscard]] size_t try_read_some(gsl::span<byte> span) const;

    template <typename T>
    const Fd &setsockopt(int level, int optname, const T &optval) const {
        return setsockopt(level, optname, &optval, sizeof(optval));
    }
    const Fd &setsockopt(int level, int optname, const void *optval, socklen_t optlen) const;

    template <typename T>
    const Fd &bind(const T &address) const {
        static_assert(sizeof(T) >= sizeof(sockaddr));
        return bind(reinterpret_cast<const sockaddr *>(&address), sizeof(T));
    }
    const Fd &bind(const sockaddr *address, socklen_t socklen) const;
    template <typename T>
    const Fd &connect(const T &address) const {
        static_assert(sizeof(T) >= sizeof(sockaddr));
        return connect(reinterpret_cast<const sockaddr *>(&address), sizeof(T));
    }
    const Fd &connect(const sockaddr *address, socklen_t socklen) const;
    const Fd &listen(int backlog) const;

    Fd accept(sockaddr *address, socklen_t *socklen) const;
    static Fd socket(int domain, int type, int protocol);
};