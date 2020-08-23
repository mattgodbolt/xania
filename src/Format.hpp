#pragma once

#include <string_view>

#include "fmt/format.h"

// Wrap a string_view so that it formats with an initial upper case letter.
struct InitialCap {
    std::string_view sv;
};

template <>
struct fmt::formatter<InitialCap> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const InitialCap &str, FormatContext &ctx) {
        if (str.sv.empty())
            return format_to(ctx.out(), "");
        return format_to(ctx.out(), "{}{}", std::toupper(str.sv.front()), str.sv.substr(1));
    }
};
