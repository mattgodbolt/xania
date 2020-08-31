#include "AffectList.hpp"
#include "merc.h"

#include <range/v3/algorithm/find.hpp>
#include <range/v3/iterator/operations.hpp>

AFFECT_DATA &AffectList::add(const AFFECT_DATA &aff) {
    data_.insert(data_.begin(), aff);
    return data_.front();
}

void AffectList::remove(const AFFECT_DATA &aff) {
    auto index = &aff - data_.data();
    if (index < 0 || static_cast<size_t>(index) > data_.size()) {
        bug("Affect_remove: cannot find paf.");
        return;
    }
    data_.erase(std::next(begin(), index));
    for (auto *next_ptr : next_ptrs_) {
        if (*next_ptr > static_cast<size_t>(index))
            --*next_ptr;
    }
}

AFFECT_DATA *AffectList::find_by_skill(int skill_number) {
    if (auto it = ranges::find(*this, skill_number, [](auto &af) { return af.type; }); it != end())
        return &*it;
    return nullptr;
}

const AFFECT_DATA *AffectList::find_by_skill(int skill_number) const {
    if (auto it = ranges::find(*this, skill_number, [](auto &af) { return af.type; }); it != end())
        return &*it;
    return nullptr;
}

size_t AffectList::size() const noexcept { return data_.size(); }

void AffectList::clear() { data_.clear(); }
