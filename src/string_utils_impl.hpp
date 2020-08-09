#pragma once

#include <string_view>

namespace impl {

struct LineSplitter {
    std::string_view sv;

    class Iter {
        std::string_view line_;
        std::string_view rest_;
        bool at_end_{false};

    public:
        Iter() : line_(), rest_(), at_end_(true) {}
        explicit Iter(std::string_view text, bool first);
        std::string_view operator*() const noexcept { return line_; }
        Iter operator++() noexcept;
        bool operator==(const Iter &rhs) const noexcept {
            return line_ == rhs.line_ && rest_ == rhs.rest_ && at_end_ == rhs.at_end_;
        }
        bool operator!=(const Iter &rhs) const noexcept { return !(rhs == *this); }
    };
};

inline LineSplitter::Iter begin(const LineSplitter &ls) { return LineSplitter::Iter(ls.sv, true); }
inline LineSplitter::Iter end(const LineSplitter &) { return LineSplitter::Iter(); }

}