#include "AREA_DATA.hpp"
#include <algorithm>

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
    std::sort(areas_.begin(), areas_.end(), [](auto &a, auto &b) {
        return a->min_level < b->min_level
               || (a->min_level == b->min_level && a->level_difference < b->level_difference);
    });
}
