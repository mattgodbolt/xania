/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "AttackType.hpp"
#include "Constants.hpp"
#include "merc.h"

bool is_attack_skill(const skill_type *opt_skill, const sh_int skill_num) {
    if (skill_num < 0 || skill_num >= MAX_SKILL) {
        return false;
    }
    return opt_skill && &skill_table[skill_num] == opt_skill;
}

bool is_attack_skill(const AttackType atk_type, const sh_int skill_num) {
    if (skill_num < 0 || skill_num >= MAX_SKILL) {
        return false;
    } else if (const auto attack_skill = std::get_if<const skill_type *>(&atk_type)) {
        return is_attack_skill(*attack_skill, skill_num);
    } else {
        return false;
    }
}
