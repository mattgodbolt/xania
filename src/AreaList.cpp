#include "AreaList.hpp"

#include "AreaData.hpp"

#include <algorithm>
#include <range/v3/algorithm/sort.hpp>

AreaList::AreaList() = default;
AreaList::~AreaList() = default;

AreaList &AreaList::singleton() {
    static AreaList singleton;
    return singleton;
}

// Sort the areas so that:
// - the lowest level ones come first
// - those with same minimum level,  those with narrowest level range come first
// Note: areas applicable to ALL levels always have a
// min_level of 0, so are always at the front.
void AreaList::sort() {
    ranges::sort(areas_, [](const auto &a, const auto &b) {
        return a->min_level() < b->min_level()
               || (a->min_level() == b->min_level() && a->level_difference() < b->level_difference());
    });
}
