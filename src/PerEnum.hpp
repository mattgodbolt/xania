#pragma once

#include <array>
#include <unistd.h>

template <typename Enum, typename T, size_t MaxOrdinal>
class PerEnum {
    std::array<T, MaxOrdinal> ts_{};

public:
    constexpr PerEnum() = default;
    template <typename... Args>
    constexpr PerEnum(Args &&... args) : ts_{std::forward<Args>(args)...} {}
    constexpr T &operator[](Enum d) { return ts_[static_cast<int>(d)]; }
    constexpr const T &operator[](Enum d) const { return ts_[static_cast<int>(d)]; }
};
