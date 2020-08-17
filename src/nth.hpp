#pragma once

#include <fmt/format.h>
#include <type_traits>

namespace impl {
template <typename T>
struct Nth {
    T value;
};
std::string_view suffix_for(size_t value);
}

template <typename T>
struct fmt::formatter<impl::Nth<T>> : formatter<T> {
    template <typename FormatContext>
    auto format(const impl::Nth<T> &number, FormatContext &ctx) {
        formatter<T>::format(number.value, ctx);
        size_t abs_value;
        if constexpr (std::is_signed_v<T>) {
            abs_value = static_cast<size_t>(std::abs(number.value));
        } else {
            abs_value = static_cast<size_t>(number.value);
        }
        return format_to(ctx.out(), "{}", impl::suffix_for(abs_value));
    }
};

// Wrap a value-type integral value so that it outputs as nth. e.g. "3rd", "45th", "2nd".
template <typename T>
auto nth(T t) {
    static_assert(std::is_arithmetic_v<T>);
    return impl::Nth<T>{t};
}
