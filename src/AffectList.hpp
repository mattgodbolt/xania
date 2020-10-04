#pragma once

#include "AFFECT_DATA.hpp"
#include "GenericList.hpp"

class AffectList {
    GenericList<AFFECT_DATA> list_;

public:
    AffectList() = default;
    AffectList(const AffectList &) = delete;
    AffectList &operator=(const AffectList &) = delete;
    AffectList(AffectList &&lhs) noexcept = default;
    AffectList &operator=(AffectList &&) = delete;

    AFFECT_DATA &add(const AFFECT_DATA &aff);
    AFFECT_DATA &add_at_end(const AFFECT_DATA &aff);
    void remove(const AFFECT_DATA &aff);

    void clear();
    [[nodiscard]] bool empty() const noexcept { return list_.empty(); }

    [[nodiscard]] AFFECT_DATA *find_by_skill(int skill_number);
    [[nodiscard]] const AFFECT_DATA *find_by_skill(int skill_number) const;

    [[nodiscard]] size_t size() const noexcept { return list_.size(); }
    [[nodiscard]] AFFECT_DATA &front() { return *list_.begin(); }
    [[nodiscard]] const AFFECT_DATA &front() const { return *list_.begin(); }

    [[nodiscard]] auto begin() { return list_.begin(); }
    [[nodiscard]] auto end() { return list_.end(); }
    [[nodiscard]] auto begin() const { return list_.begin(); }
    [[nodiscard]] auto end() const { return list_.end(); }
};

inline auto begin(AffectList &al) { return al.begin(); }
inline auto end(AffectList &al) { return al.end(); }
inline auto begin(const AffectList &al) { return al.begin(); }
inline auto end(const AffectList &al) { return al.end(); }