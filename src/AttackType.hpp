/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Types.hpp"

#include <variant>

struct attack_type;
struct skill_type;

// When an attack is made via multi_hit(), one_hit() or directly to damage()
// it may be a regular physical blow (attack_type) or a particular skill.
using AttackType = std::variant<const attack_type *, const skill_type *>;

bool is_attack_skill(const AttackType atk_type, const sh_int skill_num);
bool is_attack_skill(const skill_type *opt_skill, const sh_int skill_num);
