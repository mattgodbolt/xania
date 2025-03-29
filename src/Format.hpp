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
    auto format(const InitialCap &str, FormatContext &ctx) const {
        if (str.sv.empty())
            return fmt::format_to(ctx.out(), "");
        return fmt::format_to(ctx.out(), "{}{}", static_cast<char>(std::toupper(str.sv.front())), str.sv.substr(1));
    }
};

// Generic formatter for enums
template <typename EnumType>
requires std::is_enum_v<EnumType>
struct fmt::formatter<EnumType> : fmt::formatter<std::underlying_type_t<EnumType>> {
    auto format(const EnumType &enumValue, format_context &ctx) const {
        return fmt::formatter<std::underlying_type_t<EnumType>>::format(
            static_cast<std::underlying_type_t<EnumType>>(enumValue), ctx);
    }
};
