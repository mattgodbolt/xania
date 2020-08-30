#include "AffectList.hpp"
#include "merc.h"

#include <range/v3/algorithm/find.hpp>

void AffectList::add(AFFECT_DATA *aff) {
    aff->next = first_;
    first_ = aff;
}

void AffectList::remove(AFFECT_DATA *aff) {
    if (aff == first_) {
        first_ = aff->next;
    } else {
        AFFECT_DATA *prev;

        for (prev = first_; prev != nullptr; prev = prev->next) {
            if (prev->next == aff) {
                prev->next = aff->next;
                break;
            }
        }

        if (prev == nullptr) {
            bug("Affect_remove: cannot find paf.");
            // TODO may leak
            return;
        }
    }
    delete aff;
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
