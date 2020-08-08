#pragma once

#include <cstdio>
#include <string>
#include <utility>

class WrappedFd {
    FILE *fd_;

    void close() {
        if (fd_)
            fclose(fd_);
        fd_ = nullptr;
    }

public:
    explicit WrappedFd(FILE *fd) : fd_(fd) {}
    WrappedFd(const WrappedFd &) = delete;
    WrappedFd &operator=(const WrappedFd &) = delete;
    WrappedFd(WrappedFd &&other) noexcept : fd_(std::exchange(other.fd_, nullptr)) {}
    WrappedFd &operator=(WrappedFd &&other) noexcept {
        close();
        fd_ = std::exchange(other.fd_, nullptr);
        return *this;
    }
    ~WrappedFd() { close(); }
    operator bool() const noexcept { return fd_ != nullptr; }
    operator FILE *() const noexcept { return fd_; }

    static WrappedFd open_text(const std::string &filename) { return WrappedFd(fopen(filename.c_str(), "r")); }
};
