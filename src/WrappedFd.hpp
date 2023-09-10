#pragma once

#include <string>
#include <utility>

class WrappedFd {
    FILE *fd_;

    void close();

public:
    explicit WrappedFd(FILE *fd);
    WrappedFd(const WrappedFd &) = delete;
    WrappedFd &operator=(const WrappedFd &) = delete;
    WrappedFd &operator=(WrappedFd &&other) noexcept;
    ~WrappedFd();
    explicit operator bool() const noexcept;
    explicit operator FILE *() const noexcept;

    static WrappedFd open(const std::string &filename);
    static WrappedFd open_write(const std::string &filename);
    static WrappedFd open_append(const std::string &filename);
};
