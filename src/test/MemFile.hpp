#pragma once

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace test {

class MemFile {
    char *ptr_{};
    size_t size_{};
    FILE *file_;

public:
    MemFile() : file_(open_memstream(&ptr_, &size_)) {}
    explicit MemFile(std::string_view contents) : MemFile() {
        ::fwrite(contents.data(), 1, contents.size(), file_);
        rewind();
    }
    ~MemFile() {
        ::fclose(file_);
        ::free(ptr_);
    }
    MemFile(const MemFile &) = delete;
    MemFile &operator=(const MemFile &) = delete;
    // MemFile can't be moved (even though the file_ could) as the memstream has references to
    // values inside the MemFile (size_ and _ptr).
    MemFile(MemFile &&) = delete;
    MemFile &operator=(MemFile &&) = delete;

    [[nodiscard]] FILE *file() const noexcept { return file_; }
    void rewind() { ::fseek(file_, 0, SEEK_SET); }
    // Provides a way to release the original in-memory file and start a fresh one.
    // Useful in scenarios where the SUT is both reading then rewriting
    // the same logical file.
    void truncate() {
        ::fclose(file_);
        ::free(ptr_);
        file_ = open_memstream(&ptr_, &size_);
    }

    [[nodiscard]] std::string_view as_string_view() const noexcept {
        ::fflush(file_);
        if (!ptr_)
            return "";
        // Not passing size_ to the overloaded constructor, because in some
        // cases, fflush() causes size_ to be zero (especially after two flushes),
        // and open_memstream() maintains a null at the end of the buffer anyway.
        return {ptr_};
    }
};

}