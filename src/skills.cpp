/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  skills.c:  the skills system                                         */
/*                                                                       */
/*************************************************************************/

#include "comm.hpp"
#include "interp.h"
#include "magic.h"
#include "merc.h"

#include <cstdio>
#include <cstring>

/* used to get new skills */
void do_gain(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *trainer;
    int gn = 0, sn = 0, i = 0;

    if (IS_NPC(ch))
        return;

    /* find a trainer */
    for (trainer = ch->in_room->people; trainer != nullptr; trainer = trainer->next_in_room)
        if (IS_NPC(trainer) && IS_SET(trainer->act, ACT_GAIN))
            break;

    if (trainer == nullptr || !can_see(ch, trainer)) {
        send_to_char("You can't do that here.\n\r", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        do_say(trainer, "Pardon me?");
        return;
    }

    if (!str_prefix(arg, "list")) {
        int col;

        col = 0;

        bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cost", "group", "cost",
                     "group", "cost");
        send_to_char(buf, ch);

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;

            if (!ch->pcdata->group_known[gn] && ((i = get_group_trains(ch, gn)) != 0)) {
                bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", group_table[gn].name, i);
                send_to_char(buf, ch);
                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\n\r", ch);

        send_to_char("\n\r", ch);

        col = 0;

        bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cost", "skill", "cost",
                     "skill", "cost");
        send_to_char(buf, ch);

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == nullptr)
                break;

            if (!ch->pcdata->learned[sn] // NOT get_skill_learned
                && ((i = get_skill_trains(ch, sn)) != 0)) {
                bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", skill_table[sn].name, i);
                send_to_char(buf, ch);
                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\n\r", ch);
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

        if (ch->pcdata->learned[sn]) // NOT get_skill_learned
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

/* R Spells and skills show the players spells (or skills) */

void do_spells(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char spell_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO];
    int sn, lev, mana;
    bool found = false;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    /* initilize data */
    for (lev = 0; lev < LEVEL_HERO; lev++) {
        spell_columns[lev] = 0;
        spell_list[lev][0] = '\0';
    }

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (get_skill_level(ch, sn) < LEVEL_HERO && skill_table[sn].spell_fun != spell_null
            && ch->pcdata->learned[sn] > 0) // NOT get_skill_learned
        {
            found = true;
            lev = get_skill_level(ch, sn);
            if (ch->level < lev)
                bug_snprintf(buf, sizeof(buf), "%-18s   n/a      ", skill_table[sn].name);
            else {
                mana = UMAX(skill_table[sn].min_mana, 100 / (2 + ch->level - lev));
                bug_snprintf(buf, sizeof(buf), "%-18s  %3d mana  ", skill_table[sn].name, mana);
            }

            if (spell_list[lev][0] == '\0')
                bug_snprintf(spell_list[lev], sizeof(spell_list[lev]), "\n\rLevel %2d: %s", lev, buf);
            else /* append */
            {
                if (++spell_columns[lev] % 2 == 0)
                    strcat(spell_list[lev], "\n\r          ");
                strcat(spell_list[lev], buf);
            }
        }
    }

    /* return results */

    if (!found) {
        send_to_char("You know no spells.\n\r", ch);
        return;
    }

    for (lev = 0; lev < LEVEL_HERO; lev++)
        if (spell_list[lev][0] != '\0')
            send_to_char(spell_list[lev], ch);
    send_to_char("\n\r", ch);
}

void do_skills(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char skill_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO];
    int sn, lev;
    bool found = false;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    /* initilize data */
    for (lev = 0; lev < LEVEL_HERO; lev++) {
        skill_columns[lev] = 0;
        skill_list[lev][0] = '\0';
    }

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (get_skill_level(ch, sn) < LEVEL_HERO && skill_table[sn].spell_fun == spell_null
            && ch->pcdata->learned[sn] > 0) // NOT get_skill_learned
        {
            found = true;
            lev = get_skill_level(ch, sn);
            if (ch->level < lev)
                bug_snprintf(buf, sizeof(buf), "%-18s  n/a      ", skill_table[sn].name);
            else
                bug_snprintf(buf, sizeof(buf), "%-18s %3d%%      ", skill_table[sn].name, ch->pcdata->learned[sn]);

            if (skill_list[lev][0] == '\0')
                bug_snprintf(skill_list[lev], sizeof(skill_list[lev]), "\n\rLevel %2d: %s", lev, buf);
            else /* append */
            {
                if (++skill_columns[lev] % 2 == 0)
                    strcat(skill_list[lev], "\n\r          ");
                strcat(skill_list[lev], buf);
            }
        }
    }

    /* return results */

    if (!found) {
        send_to_char("You know no skills.\n\r", ch);
        return;
    }

    for (lev = 0; lev < LEVEL_HERO; lev++)
        if (skill_list[lev][0] != '\0')
            send_to_char(skill_list[lev], ch);
    send_to_char("\n\r", ch);
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(CHAR_DATA *ch) {
    char buf[100];
    int gn, sn, col;

    if (IS_NPC(ch))
        return;

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cp", "group", "cp", "group", "cp");
    send_to_char(buf, ch);

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;

        if (!ch->gen_data->group_chosen[gn] && !ch->pcdata->group_known[gn] && (get_group_trains(ch, gn) > 0)) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", group_table[gn].name, get_group_trains(ch, gn));
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cp", "skill", "cp", "skill", "cp");
    send_to_char(buf, ch);

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (!ch->gen_data->skill_chosen[sn] && ch->pcdata->learned[sn] == 0 // NOT get_skill_learned
            && skill_table[sn].spell_fun == spell_null && get_skill_trains(ch, sn) > 0) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", skill_table[sn].name, get_skill_trains(ch, sn));
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
    send_to_char(buf, ch);
    bug_snprintf(buf, sizeof(buf), "Experience per level: %u\n\r", exp_per_level(ch, ch->gen_data->points_chosen));
    send_to_char(buf, ch);
}

void list_group_chosen(CHAR_DATA *ch) {
    char buf[100];
    int gn, sn, col;

    if (IS_NPC(ch))
        return;

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s", "group", "cp", "group", "cp", "group", "cp\n\r");
    send_to_char(buf, ch);

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;

        if (ch->gen_data->group_chosen[gn] && get_group_trains(ch, gn) > 0) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", group_table[gn].name, get_group_trains(ch, gn));
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    bug_snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s", "skill", "cp", "skill", "cp", "skill", "cp\n\r");
    send_to_char(buf, ch);

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (ch->gen_data->skill_chosen[sn] && get_skill_level(ch, sn) > 0) {
            bug_snprintf(buf, sizeof(buf), "%-18s %-5d ", skill_table[sn].name, get_skill_trains(ch, sn));
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->gen_data->points_chosen);
    send_to_char(buf, ch);
    bug_snprintf(buf, sizeof(buf), "Experience per level: %u\n\r", exp_per_level(ch, ch->gen_data->points_chosen));
    send_to_char(buf, ch);
}

unsigned int exp_per_level(const CHAR_DATA *ch, int points) {
    unsigned int expl, inc;
    unsigned int expl2;

    if (IS_NPC(ch))
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
bool parse_gen_groups(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int gn, sn, i;

    if (argument[0] == '\0')
        return false;

    argument = one_argument(argument, arg);

    if (!str_prefix(arg, "help")) {
        if (argument[0] == '\0') {
            do_help(ch, "group help");
            return true;
        }

        do_help(ch, argument);
        return true;
    }

    if (!str_prefix(arg, "add")) {
        if (argument[0] == '\0') {
            send_to_char("You must provide a skill name.\n\r", ch);
            return true;
        }

        gn = group_lookup(argument);
        if (gn != -1) {
            if (ch->gen_data->group_chosen[gn] || ch->pcdata->group_known[gn]) {
                send_to_char("You already know that group!\n\r", ch);
                return true;
            }

            if (get_group_trains(ch, gn) < 1) {
                send_to_char("That group is not available.\n\r", ch);
                return true;
            }

            bug_snprintf(buf, sizeof(buf), "%s group added\n\r", group_table[gn].name);
            send_to_char(buf, ch);
            ch->gen_data->group_chosen[gn] = true;
            ch->gen_data->points_chosen += get_group_trains(ch, gn);
            gn_add(ch, gn);
            if (ch->pcdata->points < 200)
                ch->pcdata->points += get_group_trains(ch, gn);
            return true;
        }

        sn = skill_lookup(argument);
        if (sn != -1) {
            if (ch->gen_data->skill_chosen[sn] || ch->pcdata->learned[sn] > 0) {
                send_to_char("You already know that skill!\n\r", ch);
                return true;
            }

            if (get_skill_level(ch, sn) < 0
                || skill_table[sn].spell_fun != spell_null
                /*
                 * added Faramir 7/8/96 to stop people gaining level 60
                 * skills at during generation, with no cp cost either!
                 */
                || (get_skill_level(ch, sn) >= 60)) {
                send_to_char("That skill is not available.\n\r", ch);
                return true;
            }
            bug_snprintf(buf, sizeof(buf), "%s skill added\n\r", skill_table[sn].name);
            send_to_char(buf, ch);
            ch->gen_data->skill_chosen[sn] = true;
            ch->gen_data->points_chosen += get_skill_trains(ch, sn);
            ch->pcdata->learned[sn] = 1;
            if (ch->pcdata->points < 200)
                ch->pcdata->points += get_skill_trains(ch, sn);
            return true;
        }

        send_to_char("No skills or groups by that name...\n\r", ch);
        return true;
    }

    if (!strcmp(arg, "drop")) {
        if (argument[0] == '\0') {
            send_to_char("You must provide a skill to drop.\n\r", ch);
            return true;
        }

        gn = group_lookup(argument);
        if (gn != -1 && ch->gen_data->group_chosen[gn]) {
            send_to_char("Group dropped.\n\r", ch);
            ch->gen_data->group_chosen[gn] = false;
            ch->gen_data->points_chosen -= get_group_trains(ch, gn);
            gn_remove(ch, gn);
            for (i = 0; i < MAX_GROUP; i++) {
                if (ch->gen_data->group_chosen[gn])
                    gn_add(ch, gn);
            }
            ch->pcdata->points -= get_group_trains(ch, gn);
            return true;
        }

        sn = skill_lookup(argument);
        if (sn != -1 && ch->gen_data->skill_chosen[sn]) {
            send_to_char("Skill dropped.\n\r", ch);
            ch->gen_data->skill_chosen[sn] = false;
            ch->gen_data->points_chosen -= get_skill_trains(ch, sn);
            ch->pcdata->learned[sn] = 0; // NOT get_skill_learned
            ch->pcdata->points -= get_skill_trains(ch, sn);
            return true;
        }

        send_to_char("You haven't bought any such skill or group.\n\r", ch);
        return true;
    }

    if (!str_prefix(arg, "premise")) {
        do_help(ch, "premise");
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
void do_groups(CHAR_DATA *ch, const char *argument) {
    char buf[100];
    int gn, sn, col;

    if (IS_NPC(ch))
        return;

    col = 0;

    if (argument[0] == '\0') { /* show all groups */

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            if (ch->pcdata->group_known[gn]) {
                bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].name);
                send_to_char(buf, ch);
                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\n\r", ch);
        bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
        send_to_char(buf, ch);
        return;
    }

    if (!str_cmp(argument, "all")) /* show all groups */
    {
        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name == nullptr)
                break;
            bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].name);
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
        if (col % 3 != 0)
            send_to_char("\n\r", ch);
        return;
    }

    /* show the sub-members of a group */
    gn = group_lookup(argument);
    if (gn == -1) {
        send_to_char("No group of that name exist.\n\r", ch);
        send_to_char("Type 'groups all' or 'info all' for a full listing.\n\r", ch);
        return;
    }

    for (sn = 0; sn < MAX_IN_GROUP; sn++) {
        if (group_table[gn].spells[sn] == nullptr)
            break;
        bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].spells[sn]);
        send_to_char(buf, ch);
        if (++col % 3 == 0)
            send_to_char("\n\r", ch);
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
}

/* checks for skill improvement */
void check_improve(CHAR_DATA *ch, int sn, bool success, int multiplier) {
    int chance, how_good;
    char buf[100];

    if (IS_NPC(ch))
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
        chance = URANGE(5, 100 - ch->pcdata->learned[sn], 95);
        if (number_percent() < chance) {
            bug_snprintf(buf, sizeof(buf), "|WYou have become better at |C%s|W!|w\n\r", skill_table[sn].name);
            send_to_char(buf, ch);
            ch->pcdata->learned[sn]++;
            gain_exp(ch, 2 * how_good);
        }
    }

    else {
        chance = URANGE(5, get_skill_learned(ch, sn) / 2, 30);
        if (number_percent() < chance) {
            bug_snprintf(buf, sizeof(buf), "|WYou learn from your mistakes, and your |C%s|W skill improves.|w\n\r",
                         skill_table[sn].name);
            send_to_char(buf, ch);
            ch->pcdata->learned[sn] += number_range(1, 3);
            ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 100);
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
        if (LOWER(name[0]) == LOWER(group_table[gn].name[0]) && !str_prefix(name, group_table[gn].name))
            return gn;
    }

    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add(CHAR_DATA *ch, int gn) {
    int i;

    ch->pcdata->group_known[gn] = true;
    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (group_table[gn].spells[i] == nullptr)
            break;
        group_add(ch, group_table[gn].spells[i], false);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove(CHAR_DATA *ch, int gn) {
    int i;

    ch->pcdata->group_known[gn] = false;

    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (group_table[gn].spells[i] == nullptr)
            break;
        group_remove(ch, group_table[gn].spells[i]);
    }
}

/* use for processing a skill or group for addition  */
void group_add(CHAR_DATA *ch, const char *name, bool deduct) {
    int sn, gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
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

void group_remove(CHAR_DATA *ch, const char *name) {
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
