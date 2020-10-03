#pragma once

#include <list>
#include <optional>
#include <utility>

#include <range/v3/algorithm/find.hpp>

// A generic, modification-while-iterating-over safe list of objects.
// This makes it safe to write code like:
// for (auto *obj : things) { things.remove(obj); }
//
// Objects are stored in a doubly-linked list.
// Each doubly-linked node has a reference count: removing an item with a non-zero reference doesn't actually remove it
// from the list, instead storing a "tombstone" at that list location. Iteration skips tombstones (and cleans them up as
// it finds it can).

template <typename ElemT, typename GenListT, typename It>
class Iter;

template <typename T, typename OptType = std::optional<T>>
class GenericList {
public:
    struct Elem {
        OptType item;
        mutable size_t count{};
        [[nodiscard]] bool removed() const { return !item; }
    };
    using MutIter = Iter<T, GenericList<T>, typename std::list<Elem>::iterator>;
    using ConstIter = Iter<const T, const GenericList<T>, typename std::list<Elem>::const_iterator>;

    // Add to the back of the list.
    T &add_back(T item) { return *list_.emplace_back(Elem{std::move(item), 0}).item; }
    // Add to the front of the list.
    T &add_front(T item) { return *list_.emplace_front(Elem{std::move(item), 0}).item; }

    // Remove an item. Removes by comparison to T, so comparing Ts must be defined.
    // Returns true if item was successfully removed.
    bool remove(const T &item) {
        if (auto it = ranges::find(list_, item, &Elem::item); it != list_.end()) {
            remove_or_tombstone(it);
            return true;
        }
        return false;
    }

    // Clear the list entirely.
    void clear() {
        for (auto it = list_.begin(); it != list_.end();)
            it = remove_or_tombstone(it);
    }

    // Is the list empty?
    [[nodiscard]] bool empty() const {
        // We're trivially empty if the list is empty, or if our begin() (accounting for tombstones) is equal to end.
        return list_.empty() || begin() == end();
    }
    // How many elements are in the list? O(N).
    [[nodiscard]] size_t size() const { return std::distance(begin(), end()); }

    // Construct a list of the given items.
    template <typename... Args>
    static GenericList of(Args &&... args) {
        GenericList result;
        (result.add_back(args), ...);
        return result;
    }

    // Iteration support.s
    [[nodiscard]] MutIter begin() { return MutIter(this, get_first_non_tombstone()); }
    [[nodiscard]] MutIter end() { return MutIter(nullptr, list_.end()); }
    [[nodiscard]] ConstIter begin() const { return ConstIter(this, get_first_non_tombstone()); }
    [[nodiscard]] ConstIter end() const { return ConstIter(nullptr, list_.end()); }

    [[nodiscard]] size_t debug_count_all_nodes() const { return list_.size(); }

private:
    auto remove_or_tombstone(auto it) {
        if (it->count == 0)
            return list_.erase(it);
        it->item = OptType{};
        return ++it;
    }
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
        // We don't decref here, as we are going to walk the list, removing zero-count items, manually.
        --current_->count;
        do {
            // If we need to remove this node, and we're the only reference to this node, we can erase it now.
            if (current_->removed() && current_->count == 0)
                current_ = list_->list_.erase(current_);
            else
                ++current_;
        } while (*this != list_->end() && current_->removed());
        incref();
    }
    void incref() {
        if (valid())
            current_->count++;
    }
    void decref() {
        if (valid()) {
            // If we're the last reference to a removed element we can erase it.
            if (--current_->count == 0 && current_->removed())
                list_->list_.erase(current_);
        }
    }

public:
    Iter() = default;
    explicit Iter(GenListT *list, It it) noexcept : list_(list), current_(it) { incref(); }
    Iter(const Iter &other) noexcept : list_(other.list_), current_(other.current_) { incref(); }
    Iter &operator=(const Iter &other) noexcept {
        if (this == &other)
            return *this;
        decref();
        list_ = other.list_;
        current_ = other.current_;
        incref();
        return *this;
    }
    Iter(Iter &&other) noexcept : list_(std::exchange(other.list_, nullptr)), current_(other.current_) {}
    Iter &operator=(Iter &&other) noexcept {
        if (this == &other)
            return *this;
        decref();
        list_ = std::exchange(other.list_, nullptr);
        current_ = other.current_;
        return *this;
    }
    ~Iter() noexcept { decref(); }

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

    [[nodiscard]] bool valid() const noexcept { return list_ && current_ != list_->list_.end(); }

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
