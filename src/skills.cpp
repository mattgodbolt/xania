/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  skills.c:  the skills system                                         */
/*                                                                       */
/*************************************************************************/

#include "skills.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Columner.hpp"
#include "Learning.hpp"
#include "Logging.hpp"
#include "PcCustomization.hpp"
#include "Races.hpp"
#include "Room.hpp"
#include "SkillTables.hpp"
#include "SkillsTabulator.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "string_utils.hpp"
#include "update.hpp"

#include <map>

namespace {

// Char creation points won't grow beyond this cap, which seems to be pretty arbitrary but
// a character with this many creation points will require a huge amount of exp per level!
const auto CreationPointsCap = 200;
// Level at which a character can learn pretty much any skill not previously available.
const auto LevelCrossClassTraining = 60;
// A special case for certain skill groups (like "assassin") that can be obtained sooner.
const auto LevelSpecialClassTraining = 30;

Char *find_trainer(Room *room) {
    for (auto *trainer : room->people)
        if (trainer->is_npc() && check_enum_bit(trainer->act, CharActFlag::Gain))
            return trainer;
    return nullptr;
}

// "gain list" -- show the Char the list of skill groups and skills they can gain.
void gain_list(Char *ch) {
    Columner col3(*ch, 3);
    for (auto i = 0; i < 3; i++)
        col3.add(fmt::format("{:<18} {:<5}", "Group", "Cost"));

    for (auto gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;

        if (!ch->pcdata->group_known[gn]) {
            const auto group_trains = get_group_trains(ch, gn);
            if (group_trains != 0) {
                col3.add(fmt::format("{:<18} {:<5}", group_table[gn].name, group_trains));
            }
        }
    }
    ch->send_line("");
    for (auto i = 0; i < 3; i++)
        col3.add(fmt::format("{:<18} {:<5}", "Skill", "Cost"));

    for (auto sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (!ch->pcdata->learned[sn]) { // NOT ch.get_skill()
            const auto skill_trains = get_skill_trains(ch, sn);
            if (skill_trains != 0) {
                col3.add(fmt::format("{:<18} {:<5}", skill_table[sn].name, skill_trains));
            }
        }
    }
}

// "gain convert" -- convert 10 practice points into 1 train.
void gain_convert(Char *ch, const Char *trainer) {
    if (ch->practice < 10) {
        act("$N tells you 'You are not yet ready.'", ch, nullptr, trainer, To::Char);
        return;
    }

    act("$N helps you apply your practice to training.", ch, nullptr, trainer, To::Char);
    ch->practice -= 10;
    ch->train += 1;
}

// "gain points" -- convert 2 trains into 1 fewer creation points.
void gain_points(Char *ch, const Char *trainer) {
    if (ch->train < 2) {
        act("$N tells you 'You are not yet ready.'", ch, nullptr, trainer, To::Char);
        return;
    }
    if (ch->pcdata->points <= 40) {
        act("$N tells you 'Your ability to learn more quickly cannot be improved any further.'", ch, nullptr, trainer,
            To::Char);
        return;
    }
    act("$N trains you, and you feel more at ease with your skills.", ch, nullptr, trainer, To::Char);

    ch->train -= 2;
    ch->pcdata->points -= 1;
    ch->exp = exp_per_level(ch, ch->pcdata->points) * ch->level;
}

// "gain <skill group name>" -- learn a skill group for the price of the group in train points.
void gain_skill_group(Char *ch, const int group, const Char *trainer) {
    if (ch->pcdata->group_known[group]) {
        act("$N tells you 'You already know that group!'", ch, nullptr, trainer, To::Char);
        return;
    }
    const auto group_trains = get_group_trains(ch, group);
    if (group_trains == 0) {
        act("$N tells you 'That group is beyond your powers.'", ch, nullptr, trainer, To::Char);
        return;
    }
    if (ch->train < group_trains) {
        act("$N tells you 'You are not yet ready for that group.'", ch, nullptr, trainer, To::Char);
        return;
    }
    gn_add(ch, group);
    act("$N trains you in the art of $t.", ch, group_table[group].name, trainer, To::Char);
    ch->train -= group_trains;
}

// "gain <skill name>" -- learn a skill for the price of the skill in train points.
void gain_skill(Char *ch, const int skill, const Char *trainer) {
    if (skill_table[skill].spell_fun != spell_null) {
        act("$N tells you 'You must learn the full group.'", ch, nullptr, trainer, To::Char);
        return;
    }
    if (ch->pcdata->learned[skill]) { // NOT ch.get_skill()
        act("$N tells you 'You already know that skill!'", ch, nullptr, trainer, To::Char);
        return;
    }
    const auto skill_trains = get_skill_trains(ch, skill);
    if (skill_trains == 0) {
        act("$N tells you 'That skill is beyond your powers.'", ch, nullptr, trainer, To::Char);
        return;
    }
    if (ch->train < skill_trains) {
        act("$N tells you 'You are not yet ready for that skill.'", ch, nullptr, trainer, To::Char);
        return;
    }
    /* add the skill */
    ch->pcdata->learned[skill] = 1;
    act("$N trains you in the art of $t", ch, skill_table[skill].name, trainer, To::Char);
    ch->train -= skill_trains;
}

// Private routine for listing skill/group costs in creation points
// filtering on whether they're learned by the Char yet.
void list_group_costs(Char *ch, const auto group_filter, const auto skill_filter) {
    if (ch->is_npc())
        return;
    Columner col3(*ch, 3);
    for (auto i = 0; i < 3; i++) {
        col3.add(fmt::format("|c{:<14}|y{:<10}|w", "Group", "Points"));
    }
    for (auto gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;
        if (group_filter(gn) && (get_group_trains(ch, gn) > 0)) {
            col3.add("{:<18}{:>2}    ", group_table[gn].name, get_group_trains(ch, gn));
        }
    }
    col3.flush();
    ch->send_line("");
    for (auto i = 0; i < 3; i++) {
        col3.add(fmt::format("|g{:<14}|y{:<10}|w", "Skill", "Points"));
    }
    for (auto sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        if (skill_filter(sn) && skill_table[sn].spell_fun == spell_null && get_skill_trains(ch, sn) > 0) {
            col3.add("{:<18}{:>2}    ", skill_table[sn].name, get_skill_trains(ch, sn));
        }
    }
    col3.flush();
    ch->send_line("");
    ch->send_line(fmt::format("|WCreation points:|w {}", ch->pcdata->points));
    ch->send_line(
        fmt::format("|GExperience per level:|w {}", exp_per_level(ch, ch->pcdata->customization->points_chosen)));
}

}

/* used to get new skills */
void do_gain(Char *ch, std::string_view argument) {
    if (ch->is_npc())
        return;
    auto *trainer = find_trainer(ch->in_room);
    if (!trainer || !ch->can_see(*trainer)) {
        ch->send_line("You can't do that here.");
        return;
    }
    if (argument.empty()) {
        trainer->say("Gain what? Pssst! Try help gain.");
    } else if (matches_start(argument, "list")) {
        gain_list(ch);
    } else if (matches_start(argument, "convert")) {
        gain_convert(ch, trainer);
    } else if (matches_start(argument, "points")) {
        gain_points(ch, trainer);
    } else if (const auto group = group_lookup(argument); group > 0) {
        gain_skill_group(ch, group, trainer);
    } else if (const auto skill = skill_lookup(argument); skill > -1) {
        gain_skill(ch, skill, trainer);
    } else {
        trainer->say("I don't think I can help you gain that.");
    }
}

/* Display the player's spells, mana cost and skill level, organized by level. */

void do_spells(Char *ch) {
    if (ch->is_npc())
        return;
    const auto tabulator = SkillsTabulator::spells(ch);
    if (!tabulator.tabulate()) {
        ch->send_line("You know no spells.");
        return;
    }
}

void do_skills(Char *ch) {
    if (ch->is_npc())
        return;
    const auto tabulator = SkillsTabulator::skills(ch);
    if (!tabulator.tabulate()) {
        ch->send_line("You know no skills.");
        return;
    }
}

// Shows skills, groups and costs in creation points if not yet learned.
void list_available_group_costs(Char *ch) {
    ch->send_line("        |G[Skills and Skill Groups Available]|w");
    list_group_costs(
        ch,
        [&ch](const auto gn) { return !ch->pcdata->customization->group_chosen[gn] && !ch->pcdata->group_known[gn]; },
        [&ch](const auto sn) { return !ch->pcdata->customization->skill_chosen[sn] && ch->pcdata->learned[sn] == 0; });
}

// Shows skills, groups and costs in creation points if already learned.
void list_learned_group_costs(Char *ch) {
    ch->send_line("        |R[Skills and Skill Groups You've Learned]|w");
    list_group_costs(
        ch, [&ch](const auto gn) { return ch->pcdata->customization->group_chosen[gn]; },
        [&ch](const auto sn) { return ch->pcdata->customization->skill_chosen[sn]; });
}

unsigned int exp_per_level(const Char *ch, int points) {
    unsigned int expl, inc;
    unsigned int expl2;

    if (ch->is_npc())
        return 1000;

    expl = 1000;
    inc = 500;

    if (points < 40)
        return 1000 * pc_race_table[ch->race].class_mult[ch->class_num] / 100;

    /* processing */
    points -= 40;

    while (points > 9) {
        expl += inc;
        points -= 10;
        if (points > 9) {
            expl += inc;
            inc *= 2;
            points -= 10;
        }
    }

    expl += points * inc / 10;
    expl2 = expl * pc_race_table[ch->race].class_mult[ch->class_num] / 100;

    if (expl2 > 65500)
        return 65500;
    else
        return expl2;
}

void refund_group_members(Char *ch, const int group_num) { gn_remove(ch, group_num); }

/* this procedure handles the input parsing for the skill generator */
bool parse_customizations(Char *ch, ArgParser args) {
    if (args.empty())
        return false;

    auto command = args.shift();
    if (matches_start(command, "help")) {
        if (args.empty()) {
            do_help(ch, "group advice");
            do_help(ch, "group help");
            return true;
        }

        do_help(ch, args.shift());
        return true;
    }

    if (matches_start(command, "add")) {
        auto skill_group = args.remaining();
        if (skill_group.empty()) {
            ch->send_line("You must provide a skill name.");
            return true;
        }

        const auto group_num = group_lookup(skill_group);
        if (group_num != -1) {
            if (ch->pcdata->customization->group_chosen[group_num] || ch->pcdata->group_known[group_num]) {
                ch->send_line("You already know that group!");
                return true;
            }
            const auto group_trains = get_group_trains(ch, group_num);
            if (group_trains < 1) {
                ch->send_line("That group is not available.");
                return true;
            }

            ch->send_line(fmt::format("{} group added", group_table[group_num].name));
            ch->pcdata->customization->group_chosen[group_num] = true;
            ch->pcdata->customization->points_chosen += group_trains;
            // Prevent double charging for skills added directly that are now being
            // added indirectly via a group.
            refund_group_members(ch, group_num);
            gn_add(ch, group_num);
            if (ch->pcdata->points < CreationPointsCap)
                ch->pcdata->points += group_trains;
            return true;
        }

        const auto skill_num = skill_lookup(skill_group);
        if (skill_num != -1) {
            if (ch->pcdata->customization->skill_chosen[skill_num] || ch->pcdata->learned[skill_num] > 0) {
                ch->send_line("You already know that skill!");
                return true;
            }

            if (get_skill_level(ch, skill_num) < 0
                || skill_table[skill_num].spell_fun != spell_null
                /*
                 * added Faramir 7/8/96 to stop people gaining level 60
                 * skills at during generation, with no cp cost either!
                 */
                || (get_skill_level(ch, skill_num) >= LevelCrossClassTraining)) {
                ch->send_line("That skill is not available.");
                return true;
            }
            const auto skill_trains = get_skill_trains(ch, skill_num);
            ch->send_line(fmt::format("{} skill added", skill_table[skill_num].name));
            ch->pcdata->customization->skill_chosen[skill_num] = true;
            ch->pcdata->customization->points_chosen += skill_trains;
            ch->pcdata->learned[skill_num] = 1;
            if (ch->pcdata->points < CreationPointsCap)
                ch->pcdata->points += skill_trains;
            return true;
        }

        ch->send_line("No skills or groups by that name...");
        return true;
    }

    if (matches_start(command, "drop")) {
        auto skill_group = args.remaining();
        if (skill_group.empty()) {
            ch->send_line("You must provide a skill to drop.");
            return true;
        }

        const auto group_num = group_lookup(skill_group);
        if (group_num != -1 && ch->pcdata->customization->group_chosen[group_num]) {
            const auto group_trains = get_group_trains(ch, group_num);
            ch->send_line("Group dropped.");
            ch->pcdata->customization->group_chosen[group_num] = false;
            ch->pcdata->customization->points_chosen -= group_trains;
            gn_remove(ch, group_num);
            for (auto i = 0; i < MAX_GROUP; i++) {
                if (ch->pcdata->customization->group_chosen[group_num])
                    gn_add(ch, group_num);
            }
            ch->pcdata->points -= group_trains;
            return true;
        }

        const auto skill_num = skill_lookup(skill_group);
        if (skill_num != -1 && ch->pcdata->customization->skill_chosen[skill_num]) {
            const auto skill_trains = get_skill_trains(ch, skill_num);
            ch->send_line("Skill dropped.");
            ch->pcdata->customization->skill_chosen[skill_num] = false;
            ch->pcdata->customization->points_chosen -= skill_trains;
            ch->pcdata->learned[skill_num] = 0; // NOT ch.get_skill()
            ch->pcdata->points -= skill_trains;
            return true;
        }

        ch->send_line("You haven't bought any such skill or group.");
        return true;
    }

    if (matches_start(command, "list")) {
        list_available_group_costs(ch);
        return true;
    }

    if (matches_start(command, "learned")) {
        list_learned_group_costs(ch);
        return true;
    }

    if (matches_start(command, "info")) {
        do_groups(ch, args);
        return true;
    }

    return false;
}

/* shows all groups, or the sub-members of a group */
void do_groups(Char *ch, ArgParser argument) {
    if (ch->is_npc())
        return;
    Columner col3(*ch, 3);
    auto group_name = argument.shift();
    if (group_name.empty()) { /* show all groups */
        for (auto gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            if (ch->pcdata->group_known[gn]) {
                col3.add(group_table[gn].name);
            }
        }
        ch->send_line(fmt::format("|WCreation points:|w {}", ch->pcdata->points));
        return;
    }

    if (matches(group_name, "all")) { /* show all groups */
        for (auto gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            col3.add(group_table[gn].name);
        }
        return;
    }

    /* show the sub-members of a group */
    const auto group_num = group_lookup(group_name);
    if (group_num == -1) {
        ch->send_line("No group of that name exist.");
        ch->send_line("Type 'groups all' or 'info all' for a full listing.");
        return;
    }

    for (auto sn = 0; sn < MAX_IN_GROUP; sn++) {
        if (group_table[group_num].spells[sn] == nullptr)
            break;
        col3.add(group_table[group_num].spells[sn]);
    }
}

/* checks for skill improvement */
void check_improve(Char *ch, const int skill_num, const bool success, const int multiplier) {
    const auto learning = Learning(ch, skill_num, success, multiplier, Rng::global_rng());
    if (learning.has_learned()) {
        ch->apply_skill_points(skill_num, learning.gainable_points());
        ch->send_line(learning.learning_message());
        gain_exp(ch, learning.experience_reward());
    }
}

/* returns a group index number given the name */
int group_lookup(std::string_view name) {
    int gn;

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;
        if (matches_start(name, group_table[gn].name))
            return gn;
    }

    return -1;
}

// recursively adds a group given its number
void gn_add(Char *ch, const int gn) {
    int i;

    ch->pcdata->group_known[gn] = true;
    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (group_table[gn].spells[i] == nullptr)
            break;
        group_add(ch, group_table[gn].spells[i], false);
    }
}

// recusively removes a group given its number
void gn_remove(Char *ch, const int gn) {
    ch->pcdata->group_known[gn] = false;
    for (auto i = 0; i < MAX_IN_GROUP; i++) {
        if (group_table[gn].spells[i] == nullptr)
            break;
        group_remove(ch, group_table[gn].spells[i]);
    }
}

// adds a skill or skill group
void group_add(Char *ch, const char *name, const bool deduct) {
    if (ch->is_npc()) /* NPCs do not have skills */
        return;

    if (const auto sn = skill_lookup(name); sn != -1) {
        if (!ch->pcdata->learned[sn]) { // not known
            ch->pcdata->learned[sn] = 1;
            if (deduct)
                ch->pcdata->points += get_skill_trains(ch, sn);
        }
        return;
    }

    if (const auto gn = group_lookup(name); gn != -1) {
        if (!ch->pcdata->group_known[gn]) {
            ch->pcdata->group_known[gn] = true;
            if (deduct)
                ch->pcdata->points += get_group_trains(ch, gn);
        }
        gn_add(ch, gn); // make sure all skills in the group are known
    }
}

// Unlearns a skill or skill group during char creation.
// It will recusively unlearn all of the group's member skills and refund
// their creation points.
void group_remove(Char *ch, const char *name) {
    if (const auto sn = skill_lookup(name); sn != -1) {
        if (ch->pcdata->learned[sn]) {
            ch->pcdata->learned[sn] = 0;
            ch->pcdata->customization->skill_chosen[sn] = false;
            ch->pcdata->points = std::clamp(ch->pcdata->points - get_skill_trains(ch, sn), 0, CreationPointsCap);
        }
        return;
    }
    if (const auto gn = group_lookup(name); gn != -1) {
        if (ch->pcdata->group_known[gn]) {
            ch->pcdata->group_known[gn] = false;
            gn_remove(ch, gn); /* be sure to call gn_add on all remaining groups */
        }
    }
}

// Returns the level that the Char may learn a skill.
// Long ago, the level 60 requirement for SkillRatingAttainable was originally part of the mud's move from
// being a 60 level mud to 100 levels and with that, we introduced 'cross training'
// where  a class can learn skills normally only available to other classes.
int get_skill_level(const Char *ch, const int gsn) {
    int level = 0, bonus;

    if (ch = ch->player(); !ch)
        return 1;

    /* First we work out which level they'd get it at because of their class */

    if (skill_table[gsn].rating[ch->class_num] > SkillRatingUnattainable) {
        /*  They *can* get it at level xxxx */
        level = skill_table[gsn].skill_level[ch->class_num];
    }
    if (skill_table[gsn].rating[ch->class_num] == SkillRatingAttainable) {
        level = LevelCrossClassTraining;
    }
    if (skill_table[gsn].rating[ch->class_num] == SkillRatingSpecial) {
        /*  Hack: It's an assassin thing */
        if (ch->pcdata->group_known[group_lookup("assassin")]) {
            /*  they have the group so they get the skill at LevelSpecialClassTraining */
            level = LevelSpecialClassTraining;
        } else /*  They got the skill on its own */
            level = LevelCrossClassTraining;
    }

    /* Check stuff because of race */
    for (bonus = 0; bonus < 5; bonus++) {
        if (pc_race_table[ch->race].skills[bonus] != nullptr) {
            /* they have a bonus skill or group */
            if (skill_lookup(pc_race_table[ch->race].skills[bonus]) == gsn) {
                /* ie we have a race-specific skill!! */
                if (level > LevelSpecialClassTraining)
                    level = LevelSpecialClassTraining;
            }
        }
    }
    return level;
}

int get_skill_difficulty(Char *ch, const int gsn) {
    int level;
    if ((level = get_skill_level(ch, gsn)) == 0) /* ie you can't gain it ever! */
        return 0;
    if (level > ch->level)
        return 0; /* as you're not high enough level */
    int difficulty = skill_table[gsn].rating[ch->class_num];
    switch (difficulty) {
    case SkillRatingUnattainable: return 0; /* this should never happen as get_skill_level does this */
    case SkillRatingAttainable:
        difficulty = 8; /* skills at LevelCrossClassTraining */
        break;
    case SkillRatingSpecial:
        difficulty = 5; /* Assassin group stuff */
        break;
    }
    /* Check for race skills */
    for (auto bonus = 0u; bonus < MAX_PC_RACE_BONUS_SKILLS; bonus++) {
        if (pc_race_table[ch->race].skills[bonus] != nullptr) {
            /* they have a bonus skill or group */
            if (skill_lookup(pc_race_table[ch->race].skills[bonus]) == gsn) {
                /* ie we have a race-specific skill!! */
                if (difficulty > 2)
                    --difficulty; /* Make it a bit easier */
            }
        }
    }
    return difficulty;
}

int get_skill_trains(Char *ch, const int gsn) {
    if (get_skill_level(ch, gsn) >= LevelCrossClassTraining && ch->level < LevelCrossClassTraining)
        return 0;
    if (skill_table[gsn].spell_fun != spell_null)
        return 0;

    switch (skill_table[gsn].rating[ch->class_num]) {
    case SkillRatingUnattainable: return 0; // shouldn't happen
    case SkillRatingAttainable: return 10;
    case SkillRatingSpecial: return 10;
    }
    return (skill_table[gsn].rating[ch->class_num]);
}

// Returns the cost in "creation points" of a skill group for a character.
// Typically groups will differ in cost for different classes.
// Returns 0 if the group can't be learned yet.
//
// "Creation points" are largely synonymous with "training points" (how many trains it
// costs the player to learn in-game later on from a guildmaster). Having said that, players
// can also spend trains on reducing their creation points so that it's quicker to level, but
// the exchange rate from trains to CP reduction is pretty poor, see gain_points().
int get_group_trains(Char *ch, const int gsn) {
    if (get_group_level(ch, gsn) >= LevelCrossClassTraining && ch->level < LevelCrossClassTraining)
        return 0;
    const auto rating = group_table[gsn].rating[ch->class_num];
    switch (rating) {
    case 0: return 0;
    case -1: return 20; // An expensive group for the class to learn.
    case -2:
    case -3: return 0;
    }
    return rating;
}

// Returns the character level required to learn a skill group.
int get_group_level(Char *ch, const int gsn) {
    const auto rating = group_table[gsn].rating[ch->class_num];
    switch (rating) {
    case 0: return 0;
    case -1: return LevelCrossClassTraining;
    case -2:
    case -3: return 0;
    default: return rating;
    }
}
