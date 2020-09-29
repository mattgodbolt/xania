#pragma once

#include <list>
#include <optional>
#include <utility>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>

template <typename T, typename It>
class Iter;

// Issues:
// * we need to be able to remove elements from a list while iterating over it.
// * very often the element being remove is the "current" element.
// * rarely it's another element in the list.
// TODO:
// * more tests
// * handle const-ness correctly
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

    using MutIter = Iter<T, typename std::list<Elem>::iterator>;
    using ConstIter = Iter<T, typename std::list<Elem>::const_iterator>;
    [[nodiscard]] MutIter begin() {
        auto first = list_.begin();
        while (first != list_.end() && first->removed()) {
            // Skip any removed items; erasing any that are safe to do so.
            if (first->count == 0)
                first = list_.erase(first);
            else
                ++first;
        }
        return MutIter(this, first);
    }
    [[nodiscard]] MutIter end() { return MutIter(nullptr, list_.end()); }
    [[nodiscard]] MutIter begin() const { return const_cast<GenericList<T> *>(this)->begin(); }
    [[nodiscard]] MutIter end() const { return const_cast<GenericList<T> *>(this)->end(); }
    //    [[nodiscard]] ConstIter begin() const {
    //        auto first = list_.begin();
    //        while (first != list_.end() && first->removed())
    //            ++first;
    //        return ConstIter(const_cast<GenericList<T>*>(this), first);
    //    }
    //    [[nodiscard]] ConstIter end() const { return ConstIter(nullptr, list_.end()); }

    [[nodiscard]] bool empty() const {
        return begin() == end(); // cannot use list.empty here
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
    std::list<Elem> list_;
    friend MutIter;
    friend ConstIter;
};

template <typename T, typename It>
class Iter {
    GenericList<T> *list_{};
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
    explicit Iter(GenericList<T> *list, It it) : list_(list), current_(it) {
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

    T &operator*() const noexcept { return *current_->item; }
    T *operator->() const noexcept { return &*current_->item; }

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
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T *;
    using reference = T &;
};

namespace std {
template <typename T, typename I>
struct iterator_traits<Iter<T, I>> {
    using iterator_category = typename Iter<T, I>::iterator_category;
    using value_type = typename Iter<T, I>::value_type;
    using difference_type = typename Iter<T, I>::difference_type;
    using pointer = typename Iter<T, I>::pointer;
    using reference = typename Iter<T, I>::reference;
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
