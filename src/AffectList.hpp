#pragma once

#include "AFFECT_DATA.hpp"

#include <vector>

class AffectList {
    std::vector<AFFECT_DATA> data_;

    // The next_ptrs tracks a list of all the "next index" variables on the call stack. If an element is removed that
    // is strictly less than any of the "next index" variables, then those variables need to be decremented to account
    // for the removed element.
    std::vector<size_t *> next_ptrs_;
    struct NextIndex {
        AffectList &list;
        size_t next{};
        explicit NextIndex(AffectList &list) : list(list) { list.next_ptrs_.emplace_back(&next); }
        ~NextIndex() { list.next_ptrs_.pop_back(); }
    };

public:
    AFFECT_DATA &add(const AFFECT_DATA &aff);
    void remove(const AFFECT_DATA &aff);

    void clear();
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

    [[nodiscard]] AFFECT_DATA *find_by_skill(int skill_number);
    [[nodiscard]] const AFFECT_DATA *find_by_skill(int skill_number) const;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] AFFECT_DATA &front() { return data_.front(); }
    [[nodiscard]] const AFFECT_DATA &front() const { return data_.front(); }

    template <typename Func>
    void modification_safe_for_each(const Func &func) {
        NextIndex idx{*this};
        for (size_t index = idx.next; index < data_.size(); index = idx.next) {
            idx.next++;
            func(data_[index]);
        }
    }

    [[nodiscard]] auto begin() { return data_.begin(); }
    [[nodiscard]] auto end() { return data_.end(); }
    [[nodiscard]] auto begin() const { return data_.begin(); }
    [[nodiscard]] auto end() const { return data_.end(); }
};

inline auto begin(AffectList &al) { return al.begin(); }
inline auto end(AffectList &al) { return al.end(); }
inline auto begin(const AffectList &al) { return al.begin(); }
inline auto end(const AffectList &al) { return al.end(); }