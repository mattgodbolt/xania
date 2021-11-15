/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "DamageType.hpp"
#include "Types.hpp"
#include <array>
#include <string_view>
#include <variant>

struct skill_type;

struct attack_type {
    std::string_view name; /* message in the area file */
    std::string_view verb; /* message in the mud */
    const DamageType damage_type;
};

// When an attack is made via multi_hit(), one_hit() or directly to damage()
// it may be a regular physical blow (attack_type) or a particular skill.
using AttackType = std::variant<const attack_type *, const skill_type *>;

// Utilities concerned with the static attack type tables and attack skills.
namespace Attacks {

// Looks up an attack_type using a case insensitive partial name match
// and returns its index. Returns -1 if not found.
[[nodiscard]] int index_of(std::string_view name);

// Lookups up an attack_type pointer by its index. An attack_type index
// can be stored in locations like an Object's values array.
// Returns nullptr if the index is invalid.
[[nodiscard]] const attack_type *at(const size_t index);

[[nodiscard]] bool is_attack_skill(const AttackType atk_type, const sh_int skill_num);

[[nodiscard]] bool is_attack_skill(const skill_type *opt_skill, const sh_int skill_num);

}