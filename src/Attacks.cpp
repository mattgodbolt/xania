/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Attacks.hpp"
#include "DamageClass.hpp"
#include "SkillTables.hpp"

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

/* attack table  -- not very organized :( */

const struct attack_type attack_table[] = {{"none", "hit", -1}, /*  0 */
                                           {"slice", "slice", DAM_SLASH},
                                           {"stab", "stab", DAM_PIERCE},
                                           {"slash", "slash", DAM_SLASH},
                                           {"whip", "whip", DAM_SLASH},
                                           {"claw", "claw", DAM_SLASH}, /*  5 */
                                           {"blast", "blast", DAM_BASH},
                                           {"pound", "pound", DAM_BASH},
                                           {"crush", "crush", DAM_BASH},
                                           {"grep", "grep", DAM_SLASH},
                                           {"bite", "bite", DAM_PIERCE}, /* 10 */
                                           {"pierce", "pierce", DAM_PIERCE},
                                           {"suction", "suction", DAM_BASH},
                                           {"beating", "beating", DAM_BASH},
                                           {"digestion", "|Gdigestion|w", DAM_ACID},
                                           {"charge", "charge", DAM_BASH}, /* 15 */
                                           {"slap", "slap", DAM_BASH},
                                           {"punch", "punch", DAM_BASH},
                                           {"wrath", "wrath", DAM_ENERGY},
                                           {"magic", "magic", DAM_ENERGY},
                                           {"divine", "|Wdivine power|w", DAM_HOLY}, /* 20 */
                                           {"cleave", "cleave", DAM_SLASH},
                                           {"scratch", "scratch", DAM_PIERCE},
                                           {"peck", "peck", DAM_PIERCE},
                                           {"peckb", "peck", DAM_BASH},
                                           {"chop", "chop", DAM_SLASH}, /* 25 */
                                           {"sting", "sting", DAM_PIERCE},
                                           {"smash", "smash", DAM_BASH},
                                           {"shbite", "|Cshocking bite|w", DAM_LIGHTNING},
                                           {"flbite", "|Rflaming bite|w", DAM_FIRE},
                                           {"frbite", "|Wfreezing bite|w", DAM_COLD}, /* 30 */
                                           {"acbite", "|Yacidic bite|w", DAM_ACID},
                                           {"chomp", "chomp", DAM_PIERCE},
                                           {"drain", "life drain", DAM_NEGATIVE},
                                           {"thrust", "thrust", DAM_PIERCE},
                                           {"slime", "slime", DAM_ACID},
                                           {"shock", "shock", DAM_LIGHTNING},
                                           {"thwack", "thwack", DAM_BASH},
                                           {"flame", "flame", DAM_FIRE},
                                           {"chill", "chill", DAM_COLD},
                                           // Newly added to allow levels to load in without warning (TM)
                                           {"maul", "maul", DAM_SLASH},
                                           {"hit", "hit", DAM_BASH},
                                           {nullptr, nullptr, 0}};
