/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  skills.c:  the skills system                                         */
/*                                                                       */
/*************************************************************************/

#include "skills.hpp"
#include "BitsCharAct.hpp"
#include "Char.hpp"
#include "CharGeneration.hpp"
#include "Columner.hpp"
#include "Logging.hpp"
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
Char *find_trainer(Room *room) {
    for (auto *trainer : room->people)
        if (trainer->is_npc() && check_bit(trainer->act, ACT_GAIN))
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

// "gain points" -- convert 2 trains into 1 fewer creatoin points.
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
        col3.add(fmt::format("{:<14}{:<10}", "Group", "Points"));
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
        col3.add(fmt::format("{:<14}{:<10}", "Skill", "Points"));
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
    ch->send_line(fmt::format("Creation points: {}", ch->pcdata->points));
    ch->send_line(fmt::format("Experience per level: {}", exp_per_level(ch, ch->generation->points_chosen)));
}

}

/* used to get new skills */
void do_gain(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    if (ch->is_npc())
        return;
    auto *trainer = find_trainer(ch->in_room);
    if (!trainer || !can_see(ch, trainer)) {
        ch->send_line("You can't do that here.");
        return;
    }
    one_argument(argument, arg);
    if (arg[0] == '\0') {
        trainer->say("Gain what? Pssst! Try help gain.");
    } else if (matches_start(arg, "list")) {
        gain_list(ch);
    } else if (matches_start(arg, "convert")) {
        gain_convert(ch, trainer);
    } else if (matches_start(arg, "points")) {
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
    list_group_costs(
        ch, [&ch](const auto gn) { return !ch->generation->group_chosen[gn] && !ch->pcdata->group_known[gn]; },
        [&ch](const auto sn) { return !ch->generation->skill_chosen[sn] && ch->pcdata->learned[sn] == 0; });
}

// Shows skills, groups and costs in creation points if already learned.
void list_learned_group_costs(Char *ch) {
    list_group_costs(
        ch, [&ch](const auto gn) { return ch->generation->group_chosen[gn]; },
        [&ch](const auto sn) { return ch->generation->skill_chosen[sn]; });
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

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    int gn, sn, i;

    if (argument[0] == '\0')
        return false;

    argument = one_argument(argument, arg);

    if (!str_prefix(arg, "help")) {
        if (argument[0] == '\0') {
            do_help(ch, "group advice");
            do_help(ch, "group help");
            return true;
        }

        do_help(ch, argument);
        return true;
    }

    if (!str_prefix(arg, "add")) {
        if (argument[0] == '\0') {
            ch->send_line("You must provide a skill name.");
            return true;
        }

        gn = group_lookup(argument);
        if (gn != -1) {
            if (ch->generation->group_chosen[gn] || ch->pcdata->group_known[gn]) {
                ch->send_line("You already know that group!");
                return true;
            }

            if (get_group_trains(ch, gn) < 1) {
                ch->send_line("That group is not available.");
                return true;
            }

            ch->send_line(fmt::format("{} group added", group_table[gn].name));
            ch->generation->group_chosen[gn] = true;
            ch->generation->points_chosen += get_group_trains(ch, gn);
            gn_add(ch, gn);
            if (ch->pcdata->points < 200)
                ch->pcdata->points += get_group_trains(ch, gn);
            return true;
        }

        sn = skill_lookup(argument);
        if (sn != -1) {
            if (ch->generation->skill_chosen[sn] || ch->pcdata->learned[sn] > 0) {
                ch->send_line("You already know that skill!");
                return true;
            }

            if (get_skill_level(ch, sn) < 0
                || skill_table[sn].spell_fun != spell_null
                /*
                 * added Faramir 7/8/96 to stop people gaining level 60
                 * skills at during generation, with no cp cost either!
                 */
                || (get_skill_level(ch, sn) >= 60)) {
                ch->send_line("That skill is not available.");
                return true;
            }
            ch->send_line(fmt::format("{} skill added", skill_table[sn].name));
            ch->generation->skill_chosen[sn] = true;
            ch->generation->points_chosen += get_skill_trains(ch, sn);
            ch->pcdata->learned[sn] = 1;
            if (ch->pcdata->points < 200)
                ch->pcdata->points += get_skill_trains(ch, sn);
            return true;
        }

        ch->send_line("No skills or groups by that name...");
        return true;
    }

    if (!strcmp(arg, "drop")) {
        if (argument[0] == '\0') {
            ch->send_line("You must provide a skill to drop.");
            return true;
        }

        gn = group_lookup(argument);
        if (gn != -1 && ch->generation->group_chosen[gn]) {
            ch->send_line("Group dropped.");
            ch->generation->group_chosen[gn] = false;
            ch->generation->points_chosen -= get_group_trains(ch, gn);
            gn_remove(ch, gn);
            for (i = 0; i < MAX_GROUP; i++) {
                if (ch->generation->group_chosen[gn])
                    gn_add(ch, gn);
            }
            ch->pcdata->points -= get_group_trains(ch, gn);
            return true;
        }

        sn = skill_lookup(argument);
        if (sn != -1 && ch->generation->skill_chosen[sn]) {
            ch->send_line("Skill dropped.");
            ch->generation->skill_chosen[sn] = false;
            ch->generation->points_chosen -= get_skill_trains(ch, sn);
            ch->pcdata->learned[sn] = 0; // NOT ch.get_skill()
            ch->pcdata->points -= get_skill_trains(ch, sn);
            return true;
        }

        ch->send_line("You haven't bought any such skill or group.");
        return true;
    }

    if (!str_prefix(arg, "list")) {
        list_available_group_costs(ch);
        return true;
    }

    if (!str_prefix(arg, "learned")) {
        list_learned_group_costs(ch);
        return true;
    }

    if (!str_prefix(arg, "info")) {
        do_groups(ch, argument);
        return true;
    }

    return false;
}

/* shows all groups, or the sub-members of a group */
void do_groups(Char *ch, const char *argument) {
    if (ch->is_npc())
        return;
    Columner col3(*ch, 3);

    if (argument[0] == '\0') { /* show all groups */
        for (auto gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            if (ch->pcdata->group_known[gn]) {
                col3.add(group_table[gn].name);
            }
        }
        ch->send_line(fmt::format("Creation points: {}", ch->pcdata->points));
        return;
    }

    if (!str_cmp(argument, "all")) { /* show all groups */
        for (auto gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            col3.add(group_table[gn].name);
        }
        return;
    }

    /* show the sub-members of a group */
    const auto gn = group_lookup(argument);
    if (gn == -1) {
        ch->send_line("No group of that name exist.");
        ch->send_line("Type 'groups all' or 'info all' for a full listing.");
        return;
    }

    for (auto sn = 0; sn < MAX_IN_GROUP; sn++) {
        if (group_table[gn].spells[sn] == nullptr)
            break;
        col3.add(group_table[gn].spells[sn]);
    }
}

/* checks for skill improvement */
void check_improve(Char *ch, int sn, bool success, int multiplier) {
    int chance, how_good;
    if (ch->is_npc())
        return;

    if (ch->level < get_skill_level(ch, sn) || ch->pcdata->learned[sn] == 0 || ch->pcdata->learned[sn] == 100)
        return; /* skill is not known */

    how_good = get_skill_difficulty(ch, sn);
    /* check to see if the character has a chance to learn */
    chance = 10 * int_app[get_curr_stat(ch, Stat::Int)].learn;
    chance /= (multiplier * how_good * 4);
    chance += ch->level;

    if (number_range(1, 1000) > chance)
        return;

    /* now that the character has a CHANCE to learn, see if they really have */

    if (success) {
        chance = std::clamp(100 - ch->pcdata->learned[sn], 5, 95);
        if (number_percent() < chance) {
            ch->pcdata->learned[sn]++;
            ch->send_to(fmt::format("|WYou have become better at |C{}|W! ({})|w\n\r", skill_table[sn].name,
                                    ch->pcdata->learned[sn]));
            gain_exp(ch, 2 * how_good);
        }
    }

    else {
        chance = std::clamp(ch->get_skill(sn) / 2, 5, 30);
        if (number_percent() < chance) {
            ch->pcdata->learned[sn] += number_range(1, 3);
            ch->pcdata->learned[sn] = std::min(ch->pcdata->learned[sn], 100_s);
            ch->send_to(fmt::format("|WYou learn from your mistakes, and your |C{}|W skill improves. ({})|w\n\r",
                                    skill_table[sn].name, ch->pcdata->learned[sn]));
            gain_exp(ch, 2 * how_good);
        }
    }
}

/* returns a group index number given the name */
int group_lookup(const char *name) {
    int gn;

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;
        if (tolower(name[0]) == tolower(group_table[gn].name[0]) && !str_prefix(name, group_table[gn].name))
            return gn;
    }

    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add(Char *ch, int gn) {
    int i;

    ch->pcdata->group_known[gn] = true;
    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (group_table[gn].spells[i] == nullptr)
            break;
        group_add(ch, group_table[gn].spells[i], false);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove(Char *ch, int gn) {
    int i;

    ch->pcdata->group_known[gn] = false;

    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (group_table[gn].spells[i] == nullptr)
            break;
        group_remove(ch, group_table[gn].spells[i]);
    }
}

/* use for processing a skill or group for addition  */
void group_add(Char *ch, const char *name, bool deduct) {
    int sn, gn;

    if (ch->is_npc()) /* NPCs do not have skills */
        return;

    sn = skill_lookup(name);

    if (sn != -1) {
        if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
        {
            ch->pcdata->learned[sn] = 1;
            if (deduct)
                ch->pcdata->points += get_skill_trains(ch, sn);
        }
        return;
    }

    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1) {
        if (ch->pcdata->group_known[gn] == false) {
            ch->pcdata->group_known[gn] = true;
            if (deduct)
                ch->pcdata->points += get_group_trains(ch, gn);
        }
        gn_add(ch, gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(Char *ch, const char *name) {
    int sn, gn;

    sn = skill_lookup(name);

    if (sn != -1) {
        ch->pcdata->learned[sn] = 0;
        return;
    }

    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1 && ch->pcdata->group_known[gn] == true) {
        ch->pcdata->group_known[gn] = false;
        gn_remove(ch, gn); /* be sure to call gn_add on all remaining groups */
    }
}

// Returns the level that the Char may learn a skill.
// Long ago, the level 60 requirement for SKILL_ATTAINABLE was originally part of the mud's move from
// being a 60 level mud to 100 levels and with that, we introduced 'cross training'
// where  a class can learn skills normally only available to other classes.
int get_skill_level(const Char *ch, int gsn) {
    int level = 0, bonus;

    if (ch = ch->player(); !ch)
        return 1;

    /* First we work out which level they'd get it at because of their class */

    if (skill_table[gsn].rating[ch->class_num] > SKILL_UNATTAINABLE) {
        /*  They *can* get it at level xxxx */
        level = skill_table[gsn].skill_level[ch->class_num];
    }
    if (skill_table[gsn].rating[ch->class_num] == SKILL_ATTAINABLE) {
        /*  They get it at level sixty */
        level = 60;
    }
    if (skill_table[gsn].rating[ch->class_num] == SKILL_ASSASSIN) {
        /*  It's an assassin thing */
        if (ch->pcdata->group_known[group_lookup("assassin")]) {
            /*  they have the group so they get the skill at level 30 */
            level = 30;
        } else /*  They got the skill on its own */
            level = 60;
    }

    /* Check stuff because of race */
    for (bonus = 0; bonus < 5; bonus++) {
        if (pc_race_table[ch->race].skills[bonus] != nullptr) {
            /* they have a bonus skill or group */
            if (skill_lookup(pc_race_table[ch->race].skills[bonus]) == gsn) {
                /* ie we have a race-specific skill!! */
                if (level > 30)
                    level = 30; /*obviously*/
            }
        }
    }
    return level;
}

int get_skill_difficulty(Char *ch, int gsn) {
    int level, hard, bonus;

    if ((level = get_skill_level(ch, gsn)) == 0) /* ie you can't gain it ever! */
        return 0;

    if (level > ch->level)
        return 0; /* as you're not high enough level */

    hard = skill_table[gsn].rating[ch->class_num];
    switch (hard) {
    case SKILL_UNATTAINABLE: return 0; /* this should never happen as get_skill_level does this */
    case SKILL_ATTAINABLE:
        hard = 8; /* skills at level 60 */
        break;
    case SKILL_ASSASSIN:
        hard = 5; /* Assassin group stuff */
        break;
    }
    /* Check for race skills */
    for (bonus = 0; bonus < 5; bonus++) {
        if (pc_race_table[ch->race].skills[bonus] != nullptr) {
            /* they have a bonus skill or group */
            if (skill_lookup(pc_race_table[ch->race].skills[bonus]) == gsn) {
                /* ie we have a race-specific skill!! */
                if (hard > 2)
                    --hard; /* Make it a bit easier */
            }
        }
    }
    return hard;
}

int get_skill_trains(Char *ch, int gsn) {
    int number;
    if ((number = get_skill_level(ch, gsn)) >= 60 && (ch->level < 60))
        /* can't show it */
        return 0;

    if (skill_table[gsn].spell_fun != spell_null)
        return 0;

    switch (skill_table[gsn].rating[ch->class_num]) {
    case SKILL_UNATTAINABLE: return 0; /* shouldn't happen */
    case SKILL_ATTAINABLE: return 10; /* $-) */
    case SKILL_ASSASSIN: return 10;
    }
    return (skill_table[gsn].rating[ch->class_num]);
}

int get_group_trains(Char *ch, int gsn) {
    int number;
    if ((number = get_group_level(ch, gsn)) >= 60 && (ch->level < 60))
        /* can't show it */
        return 0;

    switch (group_table[gsn].rating[ch->class_num]) {
    case 0: return 0;
    case -1: return 20; /* $-) */
    case -2:
    case -3: return 0;
    }
    return (group_table[gsn].rating[ch->class_num]);
}

int get_group_level(Char *ch, int gsn) {
    switch (group_table[gsn].rating[ch->class_num]) {
    case 0: return 0;
    case -1: return 60;
    case -2:
    case -3: return 0;
    default: return (group_table[gsn].rating[ch->class_num]);
    }
}
