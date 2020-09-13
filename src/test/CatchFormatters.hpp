#pragma once

#include <catch2/catch.hpp>
#include <fmt/format.h>

template <typename T>
requires fmt::has_formatter<T, fmt::format_context>::value struct Catch::StringMaker<T> {
    static std::string convert(const T &value) { return fmt::to_string(value); }
};
