#pragma once

#include <list>
#include <optional>
#include <utility>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>

template <typename ElemT, typename GenListT, typename It>
class Iter;

// Issues:
// * we need to be able to remove elements from a list while iterating over it.
// * very often the element being remove is the "current" element.
// * rarely it's another element in the list.
// TODO:
// * more tests
// * support owning pointers? maybe?

template <typename T, typename OptType = std::optional<T>>
class GenericList {
public:
    struct Elem {
        OptType item;
        mutable size_t count{};
        [[nodiscard]] bool removed() const { return !item; }
    };
    T &add_back(T item) { return *list_.emplace_back(Elem{std::move(item), 0}).item; }
    T &add_front(T item) { return *list_.emplace_front(Elem{std::move(item), 0}).item; }
    bool remove(const T &item) {
        if (auto it = ranges::find_if(list_, [&](const Elem &elem) { return elem.item == item; }); it != list_.end()) {
            if (it->count == 0)
                list_.erase(it);
            else
                it->item = OptType{};
            return true;
        }
        return false;
    }
    void clear() {
        for (auto it = list_.begin(); it != list_.end();) {
            if (it->count == 0)
                it = list_.erase(it);
            else {
                it->item = OptType{};
                ++it;
            }
        }
    }

    using MutIter = Iter<T, GenericList<T>, typename std::list<Elem>::iterator>;
    using ConstIter = Iter<const T, const GenericList<T>, typename std::list<Elem>::const_iterator>;
    [[nodiscard]] MutIter begin() {
        return MutIter(this, get_first_non_tombstone());
    }
    [[nodiscard]] MutIter end() { return MutIter(nullptr, list_.end()); }
    [[nodiscard]] ConstIter begin() const {
        return ConstIter(this, get_first_non_tombstone());
    }
    [[nodiscard]] ConstIter end() const { return ConstIter(nullptr, list_.end()); }

    [[nodiscard]] bool empty() const {
        // We're trivially empty if the list is empty, or if our begin() (accounting for tombstones) is equal to end.
        return list_.empty() || begin() == end();
    }
    [[nodiscard]] size_t size() const { return std::distance(begin(), end()); }
    [[nodiscard]] size_t debug_count_all_nodes() const { return list_.size(); }

    template <typename... Args>
    static GenericList of(Args &&... args) {
        GenericList result;
        (result.add_back(args), ...);
        return result;
    }

private:
    auto get_first_non_tombstone() const {
        auto first = list_.begin();
        while (first != list_.end() && first->removed()) {
            // Skip any removed items; erasing any that are safe to do so.
            if (first->count == 0)
                first = list_.erase(first);
            else
                ++first;
        }
        return first;
    }
    mutable std::list<Elem> list_;
    friend MutIter;
    friend ConstIter;
};

template <typename ElemT, typename GenListT, typename It>
class Iter {
    GenListT *list_{};
    It current_{};

    void advance() {
        --current_->count;
        do {
            // If we need to remove this node, and we're the only reference to this node,
            // we can erase it now.
            if (current_->removed() && current_->count == 0)
                current_ = list_->list_.erase(current_);
            else
                ++current_;
        } while (*this != list_->end() && current_->removed());
        if (valid())
            ++current_->count;
    }

public:
    Iter() = default;
    explicit Iter(GenListT *list, It it) : list_(list), current_(it) {
        if (valid())
            current_->count++;
    }
    Iter(const Iter &other) : list_(other.list_), current_(other.current_) {
        if (valid())
            current_->count++;
    }
    Iter &operator=(const Iter &other) {
        if (this == &other)
            return *this;
        if (valid())
            current_->count--;
        list_ = other.list_;
        current_ = other.current_;
        if (valid())
            current_->count++;
        return *this;
    }
    Iter(Iter &&other) noexcept : list_(std::exchange(other.list_, nullptr)), current_(other.current_) {}
    Iter &operator=(Iter &&other) noexcept {
        if (this == &other)
            return *this;
        if (valid())
            current_->count--;
        list_ = std::exchange(other.list_, nullptr);
        current_ = other.current_;
        return *this;
    }
    ~Iter() {
        if (valid()) {
            // If we're the last reference to a removed element we can erase it.
            if (--current_->count == 0 && current_->removed())
                list_->list_.erase(current_);
        }
    }

    ElemT &operator*() const noexcept { return *current_->item; }
    ElemT *operator->() const noexcept { return &*current_->item; }

    Iter &operator++() noexcept {
        advance();
        return *this;
    }
    Iter operator++(int) noexcept {
        auto prev = *this;
        advance();
        return prev;
    }
    bool operator==(const Iter &rhs) const noexcept { return current_ == rhs.current_; }
    bool operator!=(const Iter &rhs) const noexcept { return !(rhs == *this); }

    [[nodiscard]] bool valid() const { return list_ && current_ != list_->list_.end(); }

    using iterator_category = std::forward_iterator_tag;
    using value_type = ElemT;
    using difference_type = ptrdiff_t;
    using pointer = ElemT *;
    using reference = ElemT &;
};

namespace std {
template <typename ElemT, typename GenListT, typename I>
struct iterator_traits<Iter<ElemT, GenListT, I>> {
    using iterator_category = typename Iter<ElemT, GenListT, I>::iterator_category;
    using value_type = typename Iter<ElemT, GenListT, I>::value_type;
    using difference_type = typename Iter<ElemT, GenListT, I>::difference_type;
    using pointer = typename Iter<ElemT, GenListT, I>::pointer;
    using reference = typename Iter<ElemT, GenListT, I>::reference;
};
}

template <typename T>
auto begin(GenericList<T> &al) {
    return al.begin();
}
template <typename... Args>
auto end(GenericList<Args...> &al) {
    return al.end();
}
template <typename T>
auto begin(const GenericList<T> &al) {
    return al.begin();
}
template <typename... Args>
auto end(const GenericList<Args...> &al) {
    return al.end();
}
