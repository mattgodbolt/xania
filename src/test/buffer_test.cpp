#include "buffer.h"

#include <string_view>
#include <catch2/catch.hpp>

namespace {

// This could become the beginnings of a C++ wrapper over a C implementation...
// And then eventually a reimplementation in C++ (which maybe a C shim if needed).
class Buffer {
    struct Releaser {
        void operator()(BUFFER *buffer) const { buffer_destroy(buffer); }
    };
    std::unique_ptr<BUFFER, Releaser> buf_;

public:
    Buffer() : buf_(buffer_create()) {}
    [[nodiscard]] std::string_view sv() const { return std::string_view(buffer_string(buf_.get())); }

    int allocated() const { return buf_->size; }

    // NB all these "line" don't actually add a line. They add more text...
    void addline(const char *text) { buffer_addline(buf_.get(), text); }
    template <typename... Args>
    void addline_fmt(const char *text, Args &&... args) {
        buffer_addline_fmt(buf_.get(), text, std::forward<Args>(args)...);
    }
    void removeline() { buffer_removeline(buf_.get()); }
    void shrink() { buffer_shrink(buf_.get()); }
    void send(CHAR_DATA *ch) { buffer_send(buf_.release(), ch); }
};

}

TEST_CASE("Buffer tests") {
    SECTION("should be able to create and destroy a buffer") { Buffer _; }
    Buffer b;
    SECTION("Should start out empty") { CHECK(b.sv().empty()); }
    SECTION("Should append strings") {
        b.addline("hello");
        CHECK(b.sv() == "hello");
        b.addline("goodbye\n\r");
        CHECK(b.sv() == "hellogoodbye\n\r");
    }
    SECTION("Should remove lines") {
        SECTION("handling the empty case") {
            b.removeline();
            CHECK(b.sv().empty());
        }
        SECTION("handling an unfinished line") {
            b.addline("this is not a whole line");
            b.removeline();
            CHECK(b.sv().empty());
        }
        SECTION("handling a single line") {
            b.addline("This is a whole line\n\r");
            b.removeline();
            CHECK(b.sv().empty());
        }

        SECTION("handling multiple lines") {
            b.addline("This is a whole line\n\rAnd this is the second line\n\r");
            b.removeline();
            CHECK(b.sv() == "This is a whole line\n\r");
            b.removeline();
            CHECK(b.sv().empty());
        }
        SECTION("should handle a single newline character poorly") {
            // This is locking in existing dreadful behaviour.
            b.addline("This is a whole line\nAnd this is the second line\n");
b.removeline();
            CHECK(b.sv() == "This is a whole line\nA"); // yes, really
        }
    }
    SECTION("Should handle printf-style formats") {
        b.addline_fmt("What an %s way to print the number %d\n\r", "anachronistic", 12);
        CHECK(b.sv() == "What an anachronistic way to print the number 12\n\r");
    }
    SECTION("Should handle really big documents and expand") {
        constexpr auto NumLines = 10'000u;
        auto orig_allocated = b.allocated();
        for (auto i = 0u; i < NumLines; ++i) {
            b.addline_fmt("Line %d\n\r", i);
        }
        CHECK(b.sv().find("Line 1") != std::string_view::npos);
        CHECK(b.sv().find("Line 100") != std::string_view::npos);
        CHECK(b.sv().find("Line 9999") != std::string_view::npos);
        CHECK(b.allocated() > orig_allocated);
        SECTION("should shrink down after") {
            auto large_alloc = b.allocated();
            b.removeline();
            b.shrink();
            CHECK(b.allocated() < large_alloc);
        }
    }
}