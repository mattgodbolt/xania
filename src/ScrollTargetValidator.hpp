/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2021 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once

#include "ObjectIndexValidator.hpp"
#include "SkillTables.hpp"

#include <map>
#include <optional>

class ObjectIndex;
enum class Target;
// Scrolls can be assigned up to three spells. This will detect whether the
// Target types of those spells are nonsensical from a targeting perspective
// e.g. a scroll of "enchant weapon" shouldn't be paired with "acid blast".
class ScrollTargetValidator : public ObjectIndexValidator {
public:
    explicit ScrollTargetValidator(const skill_type *skill_table);
    [[nodiscard]] ObjectIndexValidator::Result validate(const ObjectIndex &obj_index) const override;

private:
    [[nodiscard]] bool are_incompatible(const std::optional<Target> &opt_target_a,
                                        const std::optional<Target> &opt_target_b) const;
    [[nodiscard]] std::optional<Target> get_target(const ObjectIndex &obj_index, const int index) const;

    const skill_type *skill_table_;

    const std::multimap<const Target, const Target> incompatible_targets_;
};
