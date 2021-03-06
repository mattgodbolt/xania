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

#include <cstdio>
#include <cstring>
#include <map>

namespace {
Char *find_trainer(Room *room) {
    for (auto *trainer : room->people)
        if (trainer->is_npc() && check_bit(trainer->act, ACT_GAIN))
            return trainer;
    return nullptr;
}
}

/* used to get new skills */
void do_gain(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int gn = 0, sn = 0, i = 0;

    if (ch->is_npc())
        return;

    auto *trainer = find_trainer(ch->in_room);
    if (!trainer || !can_see(ch, trainer)) {
        ch->send_line("You can't do that here.");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        trainer->say("Pardon me?");
        return;
    }

    if (!str_prefix(arg, "list")) {
        int col;

        col = 0;

        bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cost", "group", "cost",
                     "group", "cost");
        ch->send_to(buf);

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;

            if (!ch->pcdata->group_known[gn] && ((i = get_group_trains(ch, gn)) != 0)) {
                bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", group_table[gn].name, i);
                ch->send_to(buf);
                if (++col % 3 == 0)
                    ch->send_line("");
            }
        }
        if (col % 3 != 0)
            ch->send_line("");

        ch->send_line("");

        col = 0;

        bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cost", "skill", "cost",
                     "skill", "cost");
        ch->send_to(buf);

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == nullptr)
                break;

            if (!ch->pcdata->learned[sn] // NOT ch.get_skill()
                && ((i = get_skill_trains(ch, sn)) != 0)) {
                bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", skill_table[sn].name, i);
                ch->send_to(buf);
                if (++col % 3 == 0)
                    ch->send_line("");
            }
        }
        if (col % 3 != 0)
            ch->send_line("");
        return;
    }

    if (!str_prefix(arg, "convert")) {
        if (ch->practice < 10) {
            act("$N tells you 'You are not yet ready.'", ch, nullptr, trainer, To::Char);
            return;
        }

        act("$N helps you apply your practice to training.", ch, nullptr, trainer, To::Char);
        ch->practice -= 10;
        ch->train += 1;
        return;
    }

    if (!str_prefix(arg, "points")) {
        if (ch->train < 2) {
            act("$N tells you 'You are not yet ready.'", ch, nullptr, trainer, To::Char);
            return;
        }

        if (ch->pcdata->points <= 40) {
            act("$N tells you 'There would be no point in that.'", ch, nullptr, trainer, To::Char);
            return;
        }

        act("$N trains you, and you feel more at ease with your skills.", ch, nullptr, trainer, To::Char);

        ch->train -= 2;
        ch->pcdata->points -= 1;
        ch->exp = exp_per_level(ch, ch->pcdata->points) * ch->level;
        return;
    }

    /* else add a group/skill */

    gn = group_lookup(argument);
    if (gn > 0) {
        if (ch->pcdata->group_known[gn]) {
            act("$N tells you 'You already know that group!'", ch, nullptr, trainer, To::Char);
            return;
        }

        if ((i = get_group_trains(ch, gn)) == 0) {
            act("$N tells you 'That group is beyond your powers.'", ch, nullptr, trainer, To::Char);
            return;
        }

        if (ch->train < i) {
            act("$N tells you 'You are not yet ready for that group.'", ch, nullptr, trainer, To::Char);
            return;
        }

        /* add the group */
        gn_add(ch, gn);
        act("$N trains you in the art of $t.", ch, group_table[gn].name, trainer, To::Char);
        ch->train -= i;
        return;
    }

    sn = skill_lookup(argument);
    if (sn > -1) {
        if (skill_table[sn].spell_fun != spell_null) {
            act("$N tells you 'You must learn the full group.'", ch, nullptr, trainer, To::Char);
            return;
        }

        if (ch->pcdata->learned[sn]) // NOT ch.get_skill()
        {
            act("$N tells you 'You already know that skill!'", ch, nullptr, trainer, To::Char);
            return;
        }

        if ((i = get_skill_trains(ch, sn)) == 0) {
            act("$N tells you 'That skill is beyond your powers.'", ch, nullptr, trainer, To::Char);
            return;
        }

        if (ch->train < i) {
            act("$N tells you 'You are not yet ready for that skill.'", ch, nullptr, trainer, To::Char);
            return;
        }

        /* add the skill */
        ch->pcdata->learned[sn] = 1;
        act("$N trains you in the art of $t", ch, skill_table[sn].name, trainer, To::Char);
        ch->train -= i;
        return;
    }

    act("$N tells you 'I do not understand...'", ch, nullptr, trainer, To::Char);
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

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(Char *ch) {
    char buf[100];
    int gn, sn, col;

    if (ch->is_npc())
        return;

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cp", "group", "cp", "group", "cp");
    ch->send_to(buf);

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;

        if (!ch->generation->group_chosen[gn] && !ch->pcdata->group_known[gn] && (get_group_trains(ch, gn) > 0)) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", group_table[gn].name, get_group_trains(ch, gn));
            ch->send_to(buf);
            if (++col % 3 == 0)
                ch->send_line("");
        }
    }
    if (col % 3 != 0)
        ch->send_line("");
    ch->send_line("");

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cp", "skill", "cp", "skill", "cp");
    ch->send_to(buf);

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (!ch->generation->skill_chosen[sn] && ch->pcdata->learned[sn] == 0 // NOT ch.get_skill()
            && skill_table[sn].spell_fun == spell_null && get_skill_trains(ch, sn) > 0) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", skill_table[sn].name, get_skill_trains(ch, sn));
            ch->send_to(buf);
            if (++col % 3 == 0)
                ch->send_line("");
        }
    }
    if (col % 3 != 0)
        ch->send_line("");
    ch->send_line("");

    bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
    ch->send_to(buf);
    bug_snprintf(buf, sizeof(buf), "Experience per level: %u\n\r", exp_per_level(ch, ch->generation->points_chosen));
    ch->send_to(buf);
}

void list_group_chosen(Char *ch) {
    char buf[100];
    int gn, sn, col;

    if (ch->is_npc())
        return;

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s", "group", "cp", "group", "cp", "group", "cp\n\r");
    ch->send_to(buf);

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;

        if (ch->generation->group_chosen[gn] && get_group_trains(ch, gn) > 0) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", group_table[gn].name, get_group_trains(ch, gn));
            ch->send_to(buf);
            if (++col % 3 == 0)
                ch->send_line("");
        }
    }
    if (col % 3 != 0)
        ch->send_line("");
    ch->send_line("");

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s", "skill", "cp", "skill", "cp", "skill", "cp\n\r");
    ch->send_to(buf);

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (ch->generation->skill_chosen[sn] && get_skill_level(ch, sn) > 0) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", skill_table[sn].name, get_skill_trains(ch, sn));
            ch->send_to(buf);
            if (++col % 3 == 0)
                ch->send_line("");
        }
    }
    if (col % 3 != 0)
        ch->send_line("");
    ch->send_line("");

    bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->generation->points_chosen);
    ch->send_to(buf);
    bug_snprintf(buf, sizeof(buf), "Experience per level: %u\n\r", exp_per_level(ch, ch->generation->points_chosen));
    ch->send_to(buf);
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
    char buf[100];
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

            bug_snprintf(buf, sizeof(buf), "%s group added\n\r", group_table[gn].name);
            ch->send_to(buf);
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
            bug_snprintf(buf, sizeof(buf), "%s skill added\n\r", skill_table[sn].name);
            ch->send_to(buf);
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
        list_group_costs(ch);
        return true;
    }

    if (!str_prefix(arg, "learned")) {
        list_group_chosen(ch);
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
    char buf[100];
    int gn, sn, col;

    if (ch->is_npc())
        return;

    col = 0;

    if (argument[0] == '\0') { /* show all groups */

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            if (ch->pcdata->group_known[gn]) {
                bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].name);
                ch->send_to(buf);
                if (++col % 3 == 0)
                    ch->send_line("");
            }
        }
        if (col % 3 != 0)
            ch->send_line("");
        bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
        ch->send_to(buf);
        return;
    }

    if (!str_cmp(argument, "all")) /* show all groups */
    {
        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].name);
            ch->send_to(buf);
            if (++col % 3 == 0)
                ch->send_line("");
        }
        if (col % 3 != 0)
            ch->send_line("");
        return;
    }

    /* show the sub-members of a group */
    gn = group_lookup(argument);
    if (gn == -1) {
        ch->send_line("No group of that name exist.");
        ch->send_line("Type 'groups all' or 'info all' for a full listing.");
        return;
    }

    for (sn = 0; sn < MAX_IN_GROUP; sn++) {
        if (group_table[gn].spells[sn] == nullptr)
            break;
        bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].spells[sn]);
        ch->send_to(buf);
        if (++col % 3 == 0)
            ch->send_line("");
    }
    if (col % 3 != 0)
        ch->send_line("");
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
