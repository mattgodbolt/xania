/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Attacks.hpp"
#include "DamageType.hpp"
#include "SkillTables.hpp"
#include "string_utils.hpp"

#include <range/v3/algorithm/find_if.hpp>

namespace Attacks {

const auto MaxAttackTypes = 42u;

/* attack table  -- not very organized :( */

constexpr std::array<attack_type, MaxAttackTypes> attack_table = {
    {{"none", "hit", DamageType::None}, /*  0 */
     {"slice", "slice", DamageType::Slash},
     {"stab", "stab", DamageType::Pierce},
     {"slash", "slash", DamageType::Slash},
     {"whip", "whip", DamageType::Slash},
     {"claw", "claw", DamageType::Slash}, /*  5 */
     {"blast", "blast", DamageType::Bash},
     {"pound", "pound", DamageType::Bash},
     {"crush", "crush", DamageType::Bash},
     {"grep", "grep", DamageType::Slash},
     {"bite", "bite", DamageType::Pierce}, /* 10 */
     {"pierce", "pierce", DamageType::Pierce},
     {"suction", "suction", DamageType::Bash},
     {"beating", "beating", DamageType::Bash},
     {"digestion", "|Gdigestion|w", DamageType::Acid},
     {"charge", "charge", DamageType::Bash}, /* 15 */
     {"slap", "slap", DamageType::Bash},
     {"punch", "punch", DamageType::Bash},
     {"wrath", "wrath", DamageType::Energy},
     {"magic", "magic", DamageType::Energy},
     {"divine", "|Wdivine power|w", DamageType::Holy}, /* 20 */
     {"cleave", "cleave", DamageType::Slash},
     {"scratch", "scratch", DamageType::Pierce},
     {"peck", "peck", DamageType::Pierce},
     {"peckb", "peck", DamageType::Bash},
     {"chop", "chop", DamageType::Slash}, /* 25 */
     {"sting", "sting", DamageType::Pierce},
     {"smash", "smash", DamageType::Bash},
     {"shbite", "|Cshocking bite|w", DamageType::Lightning},
     {"flbite", "|Rflaming bite|w", DamageType::Fire},
     {"frbite", "|Wfreezing bite|w", DamageType::Cold}, /* 30 */
     {"acbite", "|Yacidic bite|w", DamageType::Acid},
     {"chomp", "chomp", DamageType::Pierce},
     {"drain", "life drain", DamageType::Negative},
     {"thrust", "thrust", DamageType::Pierce},
     {"slime", "slime", DamageType::Acid},
     {"shock", "shock", DamageType::Lightning},
     {"thwack", "thwack", DamageType::Bash},
     {"flame", "flame", DamageType::Fire},
     {"chill", "chill", DamageType::Cold},
     // Newly added to allow levels to load in without warning (TM)
     {"maul", "maul", DamageType::Slash},
     {"hit", "hit", DamageType::Bash}}};

int index_of(std::string_view name) {
    const auto name_matches = [&name](const auto &attack) { return matches_start(name, attack.name); };
    const auto it = ranges::find_if(attack_table, name_matches);
    if (it != attack_table.end()) {
        return it - attack_table.begin();
    } else {
        return -1;
    }
}

const attack_type *at(const size_t index) {
    if (index >= MaxAttackTypes) {
        bug("Invalid attack table index: {}", index);
        return nullptr;
    } else {
        return &attack_table[index];
    }
}

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

}
