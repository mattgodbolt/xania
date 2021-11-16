/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2021 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "ScrollTargetValidator.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "Target.hpp"

#include <fmt/core.h>

ScrollTargetValidator::ScrollTargetValidator(const struct skill_type *skill_table)
    : skill_table_(skill_table),
      incompatible_targets_{
          // clang-format off
          {Target::ObjectInventory, Target::CharOffensive},
          {Target::ObjectInventory, Target::CharSelf},
          {Target::ObjectInventory, Target::CharOther},
          {Target::ObjectInventory, Target::CharDefensive},
          {Target::ObjectInventory, Target::CharObject},
          {Target::CharObject, Target::CharOffensive},
          {Target::CharObject, Target::CharSelf},
          {Target::CharObject, Target::CharOther},
          {Target::CharObject, Target::CharDefensive},
          // clang-format on
      } {}

ObjectIndexValidator::Result ScrollTargetValidator::validate(const ObjectIndex &obj_index) const {
    if (obj_index.type != ObjectType::Scroll) {
        return Success;
    }
    // Scrolls store spell slots at index 1, 2 & 3.
    const auto opt_target1 = get_target(obj_index, 1);
    const auto opt_target2 = get_target(obj_index, 2);
    const auto opt_target3 = get_target(obj_index, 3);
    if (are_incompatible(opt_target1, opt_target2)) {
        return ObjectIndexValidator::Result{
            fmt::format("Incompatible target type for slot1 and slot2 in #{}", obj_index.vnum)};
    }
    if (are_incompatible(opt_target1, opt_target3)) {
        return ObjectIndexValidator::Result{
            fmt::format("Incompatible target type for slot1 and slot3 in #{}", obj_index.vnum)};
    }
    if (are_incompatible(opt_target2, opt_target3)) {
        return ObjectIndexValidator::Result{
            fmt::format("Incompatible target type for slot2 and slot3 in #{}", obj_index.vnum)};
    }
    return Success;
}

bool ScrollTargetValidator::are_incompatible(const std::optional<Target> &opt_target_a,
                                             const std::optional<Target> &opt_target_b) const {
    const auto are_mapped = [this](const auto key, const auto value) {
        auto range = incompatible_targets_.equal_range(key);
        for (auto it = range.first; it != range.second; it++) {
            if (value == it->second)
                return true;
        }
        return false;
    };

    if (!opt_target_a || !opt_target_b) {
        return false;
    }
    const auto target_a = *opt_target_a;
    const auto target_b = *opt_target_b;
    return are_mapped(target_a, target_b) || are_mapped(target_b, target_a);
}

std::optional<Target> ScrollTargetValidator::get_target(const ObjectIndex &obj_index, const int index) const {
    if (obj_index.value[index] >= 0) {
        return skill_table_[obj_index.value[index]].target;
    }
    return std::nullopt;
}