//
// Created by mgodbolt on 8/7/20.
//

#include "WrappedFd.hpp"

WrappedFd::WrappedFd(FILE *fd) : fd_(fd) {}

WrappedFd::~WrappedFd() { close(); }

WrappedFd &WrappedFd::operator=(WrappedFd &&other) noexcept {
    close();
    fd_ = std::exchange(other.fd_, nullptr);
    return *this;
}

WrappedFd::operator bool() const noexcept { return fd_ != nullptr; }

WrappedFd::operator FILE *() const noexcept { return fd_; }

void WrappedFd::close() {
    if (fd_)
        fclose(fd_);
    fd_ = nullptr;
}

WrappedFd WrappedFd::open(const std::string &filename) { return WrappedFd(fopen(filename.c_str(), "r")); }
WrappedFd WrappedFd::open_write(const std::string &filename) { return WrappedFd(fopen(filename.c_str(), "w")); }
WrappedFd WrappedFd::open_append(const std::string &filename) { return WrappedFd(fopen(filename.c_str(), "a+")); }
