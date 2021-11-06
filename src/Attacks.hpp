/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "DamageType.hpp"
#include "Types.hpp"
#include <variant>

struct skill_type;

struct attack_type {
    const char *name; /* message in the area file */
    const char *verb; /* message in the mud */
    const DamageType damage_type;
};

extern const struct attack_type attack_table[];

// When an attack is made via multi_hit(), one_hit() or directly to damage()
// it may be a regular physical blow (attack_type) or a particular skill.
using AttackType = std::variant<const attack_type *, const skill_type *>;

bool is_attack_skill(const AttackType atk_type, const sh_int skill_num);
bool is_attack_skill(const skill_type *opt_skill, const sh_int skill_num);
