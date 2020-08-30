#pragma once

#include <iterator>

template <typename T>
struct NextFielder {
    T *operator()(T *current) const noexcept { return current->next; }
};

template <typename T, typename Nexter = NextFielder<T>>
class GenericListIter {
    T *current_{};
    Nexter nexter_;

public:
    GenericListIter() = default;
    explicit GenericListIter(T *current) : current_(current) {}
    T &operator*() const noexcept { return *current_; }
    GenericListIter &operator++() noexcept {
        current_ = nexter_(current_);
        return *this;
    }
    GenericListIter operator++(int) noexcept {
        auto prev = *this;
        current_ = nexter_(current_);
        return prev;
    }
    bool operator==(const GenericListIter &rhs) const noexcept { return current_ == rhs.current_; }
    bool operator!=(const GenericListIter &rhs) const noexcept { return !(rhs == *this); }

    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T *;
    using reference = T &;
};

namespace std {
template <typename T, typename Nexter>
struct iterator_traits<GenericListIter<T, Nexter>> {
    using iterator_category = typename GenericListIter<T, Nexter>::iterator_category;
    using value_type = typename GenericListIter<T, Nexter>::value_type;
    using difference_type = typename GenericListIter<T, Nexter>::difference_type;
    using pointer = typename GenericListIter<T, Nexter>::pointer;
    using reference = typename GenericListIter<T, Nexter>::reference;
};
}
