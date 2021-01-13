#include "AffectList.hpp"

#include "Logging.hpp"

#include <range/v3/algorithm/find.hpp>
#include <range/v3/iterator/operations.hpp>

AFFECT_DATA &AffectList::add(const AFFECT_DATA &aff) { return list_.add_front(aff); }

AFFECT_DATA &AffectList::add_at_end(const AFFECT_DATA &aff) { return list_.add_back(aff); }

void AffectList::remove(const AFFECT_DATA &aff) {
    if (!list_.remove(aff))
        bug("Affect_remove: cannot find paf.");
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

void AffectList::clear() { list_.clear(); }
