#pragma once

#include "AFFECT_DATA.hpp"
#include "GenericListIter.hpp"

#include <utility>

class AffectList {
    AFFECT_DATA *first_{};

public:
    AffectList() = default;
    ~AffectList() { clear(); }
    AffectList(const AffectList &) = delete;
    AffectList &operator=(const AffectList &) = delete;
    AffectList(AffectList &&lhs) noexcept : first_(std::exchange(lhs.first_, nullptr)) {}
    AffectList &operator=(AffectList &&) = delete;

    AFFECT_DATA &add(const AFFECT_DATA &aff);
    AFFECT_DATA &add_at_end(const AFFECT_DATA &aff);
    void remove(const AFFECT_DATA &aff);

    void clear();
    [[nodiscard]] bool empty() const noexcept { return !first_; }

    [[nodiscard]] AFFECT_DATA *find_by_skill(int skill_number);
    [[nodiscard]] const AFFECT_DATA *find_by_skill(int skill_number) const;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] AFFECT_DATA &front() { return *first_; }
    [[nodiscard]] const AFFECT_DATA &front() const { return *first_; }

    template <typename Func>
    void modification_safe_for_each(const Func &func) {
        AFFECT_DATA *next;
        for (auto *paf = first_; paf; paf = next) {
            next = paf->next;
            func(*paf);
        }
    }

    [[nodiscard]] auto begin() { return GenericListIter<AFFECT_DATA>(first_); }
    [[nodiscard]] auto end() { return GenericListIter<AFFECT_DATA>(); }
    [[nodiscard]] auto begin() const { return GenericListIter<const AFFECT_DATA>(first_); }
    [[nodiscard]] auto end() const { return GenericListIter<const AFFECT_DATA>(); }
};

inline auto begin(AffectList &al) { return al.begin(); }
inline auto end(AffectList &al) { return al.end(); }
inline auto begin(const AffectList &al) { return al.begin(); }
inline auto end(const AffectList &al) { return al.end(); }