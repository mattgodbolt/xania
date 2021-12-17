/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2021 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "Learning.hpp"
#include "Char.hpp"
#include "SkillTables.hpp"
#include "Stats.hpp"
#include "handler.hpp"
#include "skills.hpp"

Learning::Learning(Char *ch, const int skill_num, const bool used_successfully, const int difficulty_multiplier,
                   Rng &rng)
    : ch_(ch), rng_(rng), skill_num_(skill_num), used_successfully_(used_successfully),
      has_learned_(init_has_learned(difficulty_multiplier)) {}

bool Learning::has_learned() const { return has_learned_; }

std::string Learning::learning_message() const {
    if (used_successfully_) {
        return fmt::format("|WYou have become better at |C{}|W! ({})|w\n\r", skill_table[skill_num_].name,
                           ch_->pcdata->learned[skill_num_]);
    } else {
        return fmt::format("|WYou learn from your mistakes, and your |C{}|W skill improves. ({})|w\n\r",
                           skill_table[skill_num_].name, ch_->pcdata->learned[skill_num_]);
    }
}

sh_int Learning::gainable_points() const {
    if (used_successfully_) {
        return 1;
    } else {
        return rng_.number_range(1, 3);
    }
}

ush_int Learning::experience_reward() const { return 2 * get_skill_difficulty(ch_, skill_num_); }

// Whether or not the Char has learned depends on a couple of factors along
// with two probability checks. The first is based on the difficulty
// of the skill. The second is based on the Char's proficiency
// and whether they used the skill successfully this time.
bool Learning::init_has_learned(const int difficulty_multiplier) {
    if (ch_->is_npc())
        return false;
    if (ch_->level < get_skill_level(ch_, skill_num_) || ch_->pcdata->learned[skill_num_] == 0
        || ch_->pcdata->learned[skill_num_] == 100)
        return false;
    const int skill_difficulty = get_skill_difficulty(ch_, skill_num_);
    auto difficulty_for_char = 10 * int_app[get_curr_stat(ch_, Stat::Int)].learn;
    difficulty_for_char /= std::max(1, difficulty_multiplier * skill_difficulty * 4);
    difficulty_for_char += ch_->level;
    if (rng_.number_range(1, 1000) > difficulty_for_char) {
        return rng_.number_percent() < chance_of_learning();
    } else {
        return false;
    }
}

int Learning::chance_of_learning() const {
    if (used_successfully_) {
        return std::clamp(100 - ch_->pcdata->learned[skill_num_], 5, 95);
    } else {
        return std::clamp(ch_->get_skill(skill_num_) / 2, 5, 30);
    }
}
