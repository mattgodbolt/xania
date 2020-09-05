#pragma once

#include <cstdio>
#include <memory>
#include <string_view>

namespace test {

class MemFile {
    struct Closer {
        void operator()(FILE *f) { ::fclose(f); }
    };
    char *ptr_;
    size_t size_;
    std::unique_ptr<FILE, Closer> file_;

public:
    explicit MemFile(std::string_view contents) : file_(open_memstream(&ptr_, &size_)) {
        ::fwrite(contents.data(), 1, contents.size(), file_.get());
        ::fseek(file_.get(), 0, SEEK_SET);
    }
    MemFile(const MemFile &) = delete;
    MemFile &operator=(const MemFile &) = delete;
    // MemFile can't be moved (even though the file_ could) as the memstream has references to
    // values inside the MemFile (size_ and _ptr).
    MemFile(MemFile &&) = delete;
    MemFile &operator=(MemFile &&) = delete;

    FILE *file() const noexcept { return file_.get(); }
};

}