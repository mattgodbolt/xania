/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "SkillsTabulator.hpp"
#include "Char.hpp"
#include "Columner.hpp"
#include "SkillTables.hpp"
#include "magic.h"
#include "skills.hpp"

SkillsTabulator SkillsTabulator::skills(Char *ch, const Char *victim) {
    return SkillsTabulator(ch, victim, Mode::SkillsOnly);
}

SkillsTabulator SkillsTabulator::skills(Char *ch) { return SkillsTabulator(ch, ch, Mode::SkillsOnly); }

SkillsTabulator SkillsTabulator::spells(Char *ch, const Char *victim) {
    return SkillsTabulator(ch, victim, Mode::SpellsOnly);
}

SkillsTabulator SkillsTabulator::spells(Char *ch) { return SkillsTabulator(ch, ch, Mode::SpellsOnly); }

SkillsTabulator::SkillsTabulator(Char *ch, const Char *victim, const Mode mode)
    : ch_(ch), victim_(victim), mode_(mode) {
    init_skill_map();
}

bool SkillsTabulator::tabulate() const {
    if (level_to_skill_nums_.empty()) {
        return false;
    }
    Columner col2(*ch_, 2);
    for (const auto &it : level_to_skill_nums_) {
        col2.flush();
        bool first_at_level = true;
        for (const auto skill_num : it.second) {
            std::string description;
            if (std::exchange(first_at_level, false)) {
                description = fmt::format("L{:>2}: ", it.first);
            } else {
                description = "     ";
            }
            description += describe_skill(skill_num, it.first);
            col2.add(description);
        }
    }
    col2.flush();
    return true;
}

void SkillsTabulator::init_skill_map() {
    for (auto sn = 0; sn < MAX_SKILL; sn++) {
        const int skill_level = get_skill_level(victim_, sn);
        if (skill_level < LEVEL_HERO && ((mode_ == Mode::SkillsOnly) ^ (skill_table[sn].spell_fun != spell_null))
            && victim_->pcdata->learned[sn] > 0) {
            auto &skill_nums = vector_for_key(skill_level);
            skill_nums.push_back(sn);
        }
    }
}

std::vector<int> &SkillsTabulator::vector_for_key(const int key) {
    auto lower_bound = level_to_skill_nums_.lower_bound(key);
    const auto comp = level_to_skill_nums_.key_comp();
    if (lower_bound != level_to_skill_nums_.end() && !comp(key, lower_bound->first)) {
        return lower_bound->second;
    } else {
        std::vector<int> new_vec;
        return level_to_skill_nums_.insert(lower_bound, {key, new_vec})->second;
    }
}

std::string SkillsTabulator::describe_skill(const int skill_num, const int level) const {

    if (victim_->level < level) {
        return fmt::format("{:<18}  n/a     ", skill_table[skill_num].name);
    } else if (mode_ == Mode::SkillsOnly) {
        return fmt::format("{:<18} {:>3}%     ", skill_table[skill_num].name, victim_->pcdata->learned[skill_num]);
    } else {
        const auto mana_cost = mana_for_spell(victim_, skill_num);
        return fmt::format("{:<18} {:>3}m {:>3}%", skill_table[skill_num].name, mana_cost,
                           victim_->pcdata->learned[skill_num]);
    }
}