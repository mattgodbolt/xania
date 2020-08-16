#pragma once

#include <fmt/format.h>

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
        return format_to(ctx.out(), "{}", impl::suffix_for(static_cast<size_t>(std::abs(number.value))));
    }
};

// Wrap a value-type integral value so that it outputs as nth. e.g. "3rd", "45th", "2nd".
template <typename T>
auto nth(T t) {
    return impl::Nth<T>{t};
}
