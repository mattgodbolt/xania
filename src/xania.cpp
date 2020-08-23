/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  xania.c: a variety of Xania-specific modifications and new utilities */
/*                                                                       */
/*************************************************************************/

#include "DescriptorList.hpp"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "magic.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace fmt::literals;

/*
 * KLUDGEMONGER III, Revenge of Kludgie, the Malicious Code Murderer...
 */

/* tip wizard */

bool ignore_tips;
TIP_TYPE *tip_top; /* top of and current list item */
TIP_TYPE *tip_current;

/* report_object, takes an object_index_data obj and a param boot and returns the 'worth' of an
   object in points.  If boot is non-zero it will also 'BUG' these, along with any other things
   that could be wrong with the object */

void objectbug(const char *str, OBJ_INDEX_DATA *obj) {
    log_string("obj> {} (#{}): {}"_format(obj->short_descr, obj->vnum, str));
}

void mobbug(const char *str, MOB_INDEX_DATA *mob) {
    log_string("mob> {} (#{}): {}"_format(mob->short_descr, mob->vnum, str));
}

int report_object(OBJ_DATA *object, int boot) {
    int worth = 0, hit = 0, dam = 0;
    int averagedam, allowedaverage;
    AFFECT_DATA *paf;
    OBJ_INDEX_DATA *obj = object->pIndexData;

    for (paf = obj->affected; paf; paf = paf->next) {
        switch (paf->location) {

        default: break;

        case APPLY_STR:
        case APPLY_DEX:
        case APPLY_INT:
        case APPLY_WIS:
        case APPLY_CON:
            if (paf->modifier > 0)
                worth += paf->modifier;
            break;

        case APPLY_HITROLL: hit += paf->modifier; break;
        case APPLY_DAMROLL: dam += paf->modifier; break;

        case APPLY_SAVING_PARA:
        case APPLY_SAVING_ROD:
        case APPLY_SAVING_PETRI:
        case APPLY_SAVING_BREATH:
        case APPLY_SAVING_SPELL:
        case APPLY_AC:
            if (paf->modifier < 0)
                worth -= paf->modifier;
            break;

        case APPLY_MANA:
        case APPLY_HIT:
        case APPLY_MOVE:
            if (paf->modifier > 0)
                worth += (paf->modifier + 6) / 7;
            break;
        }
    }
    /* Weapons are allowed 1 hit and 1 dam for each point */

    if (obj->item_type == ITEM_WEAPON) {
        worth += (hit + dam) / 2;
    } else {
        worth += (hit + dam);
    }

    /* Object specific routines */

    switch (obj->item_type) {

    case ITEM_WEAPON:
        /* Calculate the damage allowed and actual */
        allowedaverage = (object->level / 2) + 4;
        if (IS_SET(obj->value[4], WEAPON_TWO_HANDS) && IS_SET(obj->wear_flags, ITEM_TWO_HANDS))
            allowedaverage += UMAX(1, (allowedaverage) / 20);
        averagedam = (obj->value[1] * obj->value[2] + obj->value[1]) / 2;
        if ((averagedam > allowedaverage) && boot) {
            objectbug("average damage too high", obj);
        }
        /* Add to worth for each weapon type */
        if (IS_SET(obj->value[4], WEAPON_FLAMING))
            worth++;
        if (IS_SET(obj->value[4], WEAPON_FROST))
            worth++;
        if (IS_SET(obj->value[4], WEAPON_VAMPIRIC))
            worth++;
        if (IS_SET(obj->value[4], WEAPON_SHARP))
            worth++;
        if (IS_SET(obj->value[4], WEAPON_VORPAL))
            worth++;
        break;

    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_SCROLL:
    case ITEM_BOMB:
    case ITEM_STAFF:
        if ((obj->value[4] > (object->level + (UMAX(5, obj->level / 10)))) && boot)
            objectbug("level of spell too high", obj);
        break;

    default: break;
    }

    if (boot && (worth > ((obj->level / 10) + 1))) {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "points too high: has %d points (max should be %d)", worth, ((obj->level / 10) + 1));
        objectbug(buf, obj);
    }
    return worth;
}

/* report_mobile - for checking those not-hard-enough mobs */

void report_mobile(MOB_INDEX_DATA *mob) {

    if ((mob->damage[DICE_BONUS] + mob->hitroll
         + ((mob->damage[DICE_NUMBER] * mob->damage[DICE_TYPE] + mob->damage[DICE_NUMBER]) / 2))
        < (mob->level * 3 / 2))
        mobbug("can't do enough damage", mob);

    if ((mob->hit[DICE_NUMBER] + mob->hit[DICE_BONUS]) < (mob->level * 30))
        mobbug("has too few health points", mob);
}

/* check_xania - check all of Xania and report those things that aren't what they should be */

void check_xania() {
    OBJ_DATA *object;
    CHAR_DATA *mobile;

    bug("obj> **********************************************************************");
    bug("obj> **               Beginning sweep of all object in Xania             **");
    bug("obj> **********************************************************************");

    for (object = object_list; object; object = object->next) {
        report_object(object, 1);
    }

    bug("obj> **********************************************************************");
    bug("obj> **                       Object sweep completed                     **");
    bug("obj> **********************************************************************");

    bug("mob> **********************************************************************");
    bug("mob> **                       Beginning mobile sweep                     **");
    bug("mob> **********************************************************************");

    for (mobile = char_list; mobile; mobile = mobile->next) {
        report_mobile(mobile->pIndexData);
    }

    bug("mob> **********************************************************************");
    bug("mob> **                       Mobile sweep completed                     **");
    bug("mob> **********************************************************************");
}

void do_immworth(CHAR_DATA *ch, const char *argument) {
    OBJ_DATA *obj;
    int worth, shouldbe;
    char buf[MAX_STRING_LENGTH];

    if ((obj = get_obj_world(ch, argument)) == nullptr) {
        send_to_char("Nothing like that in Xania.\n\r", ch);
        return;
    }

    worth = report_object(obj, 0);
    shouldbe = ((obj->level / 10) + 1);
    if (worth == shouldbe) {
        snprintf(buf, sizeof(buf), "Object '%s' has %d point(s) - exactly right.\n\r", obj->pIndexData->short_descr,
                 worth);
        send_to_char(buf, ch);
        return;
    }
    if (worth > shouldbe) {
        snprintf(buf, sizeof(buf), "Object '%s' has %d point(s), %d points |Rtoo high|w.\n\r",
                 obj->pIndexData->short_descr, worth, (worth - shouldbe));
    } else {
        snprintf(buf, sizeof(buf), "Object '%s' has %d point(s), within the %d point maximum.\n\r",
                 obj->pIndexData->short_descr, worth, shouldbe);
    }
    send_to_char(buf, ch);
}

/* do_prefix added 19-05-97 PCFN */
void do_prefix(CHAR_DATA *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;

    auto prefix = smash_tilde(argument);
    if (prefix.length() > (MAX_STRING_LENGTH - 1))
        prefix.resize(MAX_STRING_LENGTH - 1);

    if (prefix.empty()) {
        if (ch->pcdata->prefix.empty()) {
            ch->send_to("No prefix to remove.\n\r");
        } else {
            ch->send_to("Prefix removed.\n\r");
            ch->pcdata->prefix.clear();
        }
    } else {
        ch->pcdata->prefix = prefix;
        ch->send_to("Prefix set to \"{}\"\n\r"_format(ch->pcdata->prefix));
    }
}

/* do_timezone added PCFN 24-05-97 */
void do_timezone(CHAR_DATA *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;

    if (argument[0] == '\0') {
        if (ch->pcdata->minoffset == 0 && ch->pcdata->houroffset == 0)
            ch->send_to("British time is already being used\n\r");
        else {
            ch->send_to("British time will be used\n\r");
            ch->pcdata->minoffset = 0;
            ch->pcdata->houroffset = 0;
        }
    } else {
        sscanf(argument, "%hd:%hd", &ch->pcdata->houroffset, &ch->pcdata->minoffset);
        ch->send_to(
            "Time will now be displayed %d:%02d from GMT\n\r"_format(ch->pcdata->houroffset, ch->pcdata->minoffset));
    }
}

/********************************************************************************************/

/*****************************************
 ******************************************************************/
int get_skill_level(const CHAR_DATA *ch, int gsn) {
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

/*****************************************/
int get_skill_difficulty(CHAR_DATA *ch, int gsn) {
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

/*****************************************/
int get_skill_trains(CHAR_DATA *ch, int gsn) {
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

// New skill learned function - use instead of looking it up directly in the pcdata!!
// Similar to get_skill
int get_skill_learned(CHAR_DATA *ch, int skill_number) {
    // Mobs calling this get either 100 or 90
    if (ch->is_npc()) {
        if (is_affected(ch, gsn_insanity))
            return 90;
        else
            return 100;
    } else {
        if (is_affected(ch, gsn_insanity))
            return (9 * ch->pcdata->learned[skill_number]) / 10;
        else
            return ch->pcdata->learned[skill_number];
    }
}

/*****************************************
 ******************************************************************/
int get_group_trains(CHAR_DATA *ch, int gsn) {
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

int get_group_level(CHAR_DATA *ch, int gsn) {
    switch (group_table[gsn].rating[ch->class_num]) {
    case 0: return 0;
    case -1: return 60;
    case -2:
    case -3: return 0;
    default: return (group_table[gsn].rating[ch->class_num]);
    }
}

/********************************************************************************************/

/*****************************************
 ******************************************************************/
int is_made_of(OBJ_DATA *obj, const char *material);

int check_material_vulnerability(CHAR_DATA *ch, OBJ_DATA *object) {

    if (IS_SET(ch->vuln_flags, VULN_WOOD)) {
        if (is_made_of(object, "wood"))
            return 1;
    }

    if (IS_SET(ch->vuln_flags, VULN_SILVER)) {
        if (is_made_of(object, "silver"))
            return 1;
    }

    if (IS_SET(ch->vuln_flags, VULN_IRON)) {
        if (is_made_of(object, "iron"))
            return 1;
    }
    return 0;
}

int is_made_of(OBJ_DATA *obj, const char *material) {
    if (!str_cmp(material_table[obj->pIndexData->material].material_name, material))
        return 1;
    return 0;
}

/********************************************************************************************/

/*****************************************
 ******************************************************************/

void spell_reincarnate(int sn, int level, CHAR_DATA *ch, void *vo) {
    (void)sn;
    (void)level;
    (void)vo;
    /* is of type TAR_IGNORE so ignore *vo */

    OBJ_DATA *obj;
    int num_of_corpses = 0, corpse, chance;

    if (ch->is_npc())
        return;

    /* scan the room looking for an appropriate objects...count them */
    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        if ((obj->pIndexData->item_type == ITEM_CORPSE_NPC) || (obj->pIndexData->item_type == ITEM_CORPSE_PC))
            num_of_corpses++;
    }

    /* Did we find *any* corpses? */
    if (num_of_corpses == 0) {
        send_to_char("There are no dead in this room to reincarnate!\n\r", ch);
        return;
    }

    /* choose a corpse at random & find it's corresponding OBJ_DATA */
    corpse = number_range(1, num_of_corpses);
    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        if ((obj->pIndexData->item_type == ITEM_CORPSE_NPC) || (obj->pIndexData->item_type == ITEM_CORPSE_PC)) {
            if (--corpse == 0)
                break;
        }
    }

    if (obj == nullptr) { /* Oh dear */
        bug("spell_reincarnate: Couldn't find corpse!");
        return;
    }

    /* Tell people what happened */
    act("$n summons mighty powers and reanimates $s.", ch, nullptr, obj, To::Room);
    act("You summon mighty powers and reanimate $s.", ch, nullptr, obj, To::Char);

    /* Can we re-animate this corpse? Include check for a non-empty PC corpse */

    chance = URANGE(1, (50 + ((ch->level - obj->pIndexData->level) * 3)), 99);

    if ((number_percent() > chance) || /* if random failed */
        ((obj->pIndexData->item_type == ITEM_CORPSE_PC) && (obj->contains != nullptr)))
    /* if non-empty PC corpse */ {
        act("$s stands, then falls over again - lifeless.", ch, nullptr, obj, To::Room);
        act("$s stands, then falls over again - lifeless.", ch, nullptr, obj, To::Char);
        return;
    } else {
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *animated;

        act("$s stands up.", ch, nullptr, obj, To::Room);
        act("$s stands up.", ch, nullptr, obj, To::Char);
        animated = create_mobile(get_mob_index(MOB_VNUM_ZOMBIE));
        if (animated == nullptr) {
            bug("spell_reincarnate: Couldn't find corpse vnum!");
            return;
        }

        /* Put the zombie in the room */
        char_to_room(animated, ch->in_room);
        /* Give the zombie the kit that was in the corpse */
        animated->carrying = obj->contains;
        obj->contains = nullptr;
        /* Give the zombie its correct name and stuff */
        snprintf(buf, sizeof(buf), animated->description, obj->description);
        free_string(animated->long_descr);
        animated->long_descr = str_dup(buf);
        snprintf(buf, sizeof(buf), animated->name, obj->name);
        free_string(animated->name);
        animated->name = str_dup(buf);

        /* Say byebye to the corpse */
        extract_obj(obj);

        /* Set up the zombie correctly */
        animated->master = ch;
        animated->leader = ch;
        do_follow(animated, ch->name);
    }
}

void do_smit(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    send_to_char("If you wish to smite someone, then SPELL it out!\n\r", ch);
}

void do_smite(CHAR_DATA *ch, const char *argument) {
    /* Power of the Immortals! By Faramir
                                 Don't use this too much, it hurts :) */

    const char *smitestring;
    char smitebuf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if (argument[0] == '\0') {
        send_to_char("Upon whom do you wish to unleash your power?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    } /* Not (visibly) present in room! */

    /* In case a mort thinks he can get away
                             with it, shah right! MMFOOMB
                             Should be dealt with in interp.c already*/

    if (ch->is_npc()) {
        send_to_char("You must take your true form before unleashing your power.\n\r", ch);
        return;
    } /* done whilst switched? No way Jose */

    if (victim->get_trust() > ch->get_trust()) {

        send_to_char("You failed.\n\rUmmm...beware of retaliation!\n\r", ch);
        act("$n attempted to smite $N!", ch, nullptr, victim, To::NotVict);
        act("$n attempted to smite you!", ch, nullptr, victim, To::Vict);
        return;
    }
    /* Immortals cannot smite those with a
                             greater trust than themselves */

    smitestring = "__NeutralSmite";

    if (ch->alignment >= 300) {
        smitestring = "__GoodSmite";
    } /* Good Gods */
    if (ch->alignment <= -300) {
        smitestring = "__BadSmite";
    } /* Establish what message will actually
           be sent when the smite takes place.
           Evil Gods have their own evil method of
           incurring their wrath, and so neutral and
           good Gods =)  smitemessage default of 0
           for neutral. */

    /* ok, now the appropriate smite
                                           string has been determined we must
                                           send it to the victim, and tell
                                           the smiter and others in the room. */

    do_help(victim, smitestring);
    act("|WThe wrath of the Gods has fallen upon you!\n\rYou are blown helplessly from your feet and are stunned!|w",
        ch, nullptr, victim, To::Vict);

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == nullptr) {
        act("|R$n has been cast down by the power of $N!|w", victim, nullptr, ch, To::NotVict);
    } else {
        act("|R$n has been cast down by the power of $N!\n\rTheir weapon is sent flying!|w", victim, nullptr, ch,
            To::NotVict);
    }

    /* tells others that the victim has
                                            been disarmed, but not the victim :) */

    snprintf(smitebuf, sizeof(smitebuf), "You |W>>> |YSMITE|W <<<|w %s with all of your Godly powers!\n\r",
             (victim == ch) ? "yourself" : victim->name);
    send_to_char(smitebuf, ch);

    victim->hit /= 2; /* easiest way of halving hp? */
    if (victim->hit < 1)
        victim->hit = 1; /* Cap the damage */

    victim->position = POS_RESTING;
    /* Knock them into resting
                                            and disarm them regardless of whether
                                            they have talon or a noremove weapon */

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == nullptr) {
        return;
    } /* can't be disarmed if no weapon */
    else {
        obj_from_char(obj);
        obj_to_room(obj, victim->in_room);
        if (victim->is_npc() && victim->wait == 0 && can_see_obj(victim, obj))
            get_obj(victim, obj, nullptr);
    } /* disarms them, and NPCs will collect
              their weapon if they can see it.
              Ta-daa, smite compleeeeeet. Ouch. */
}

/* web related functions */

bool web_see(CHAR_DATA *ch) {

    if (IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL) || IS_SET(ch->act, PLR_AFK)
        || IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED(ch, AFF_SNEAK) || IS_AFFECTED(ch, AFF_HIDE))
        return false;
    else
        return true;
}

void web_who() {

    FILE *fp;
    int count = 0;

    if ((fp = fopen(WEB_WHO_FILE, "w")) == nullptr) {
        bug("web_who: couldn't open %s for writing!", WEB_WHO_FILE);
        return;
    }
    fprintf(fp, "<HTML><HEAD><TITLE TEXT=\"#FFFFFF\">Who's playing Xania right now?</TITLE>"
                "</HEAD><BODY BGCOLOR=\"#000000\" TEXT=\"#FFAA44\">"
                "<CENTER><H2>Who's playing Xania right now?</H2><P></CENTER>"
                "<TABLE BORDER=0>"

    );
    fprintf(fp, "<b><TR><TD>Level</TD> <TD>Race</TD> <TD>Class</TD>  <TD>Who</TD></TR></b>\n");

    for (auto &d : descriptors().playing()) {
        auto wch = d.person();
        if (web_see(wch)) {
            fprintf(fp, "<TR><TD>%d</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD>\n", wch->level,
                    wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ",
                    class_table[wch->class_num].name, wch->name, wch->pcdata->title.c_str());
            count++;
        }
    }
    fprintf(fp,
            "</TABLE>"
            "<P>Players found: %d <ADDRESS>Generated by Xania MUD. xania.org</ADDRESS></BODY></HTML>\n",
            count);

    fclose(fp);
}

void load_tipfile() {

    FILE *fp = nullptr;
    int tipcount = 0;
    TIP_TYPE *ptt = nullptr;

    tip_top = tip_current = nullptr; /* initialise globals */

    if ((fp = fopen(TIPWIZARD_FILE, "r")) == nullptr) {
        bug("Couldn't open tip file \'%s\' for reading", TIPWIZARD_FILE);
        ignore_tips = true;
        return;
    }
    for (;;) {
        char c;
        ptt = nullptr;

        while (isspace(c = getc(fp)))
            ;
        ungetc(c, fp);
        if (feof(fp)) {
            fclose(fp);
            log_string("Loaded {} tips"_format(tipcount));
            if (tipcount == 0)
                ignore_tips = true; /* don't bother polling the tip loop*/
            tip_current = tip_top;
            return;
        }
        ptt = (TIP_TYPE *)malloc(sizeof(TIP_TYPE));
        ptt->next = nullptr;
        ptt->tip = fread_string(fp);

        if (tip_top == nullptr) {
            tip_top = ptt;
            tip_current = tip_top;
        } else
            tip_current->next = ptt;
        tip_current = ptt;
        tipcount++;
    }
    /* now set the current tip to the top of the list ready for use*/
}

void tip_players() {
    /* check the tip wizard list first ... */

    if (tip_current == nullptr)
        tip_current = tip_top; /* send us back to top of list */

    if (tip_top == nullptr) { /* we didn't load a tip file so ignore */
        ignore_tips = true;
        return;
    }
    if (tip_current->tip == nullptr) {
        tip_current = tip_current->next;
        return;
    }
    if (strlen(tip_current->tip) == 0) {
        tip_current = tip_current->next;
        return;
    }
    auto tip = "|WTip: {}|w\n\r"_format(tip_current->tip);
    for (auto &d : descriptors().playing()) {
        CHAR_DATA *ch = d.person();
        if (is_set_extra(ch, EXTRA_TIP_WIZARD))
            send_to_char(tip, ch);
    }
    tip_current = tip_current->next;
}

void do_tipwizard(CHAR_DATA *ch, const char *arg) {

    if (arg[0] == '\0') {
        if (is_set_extra(ch, EXTRA_TIP_WIZARD)) {
            remove_extra(ch, EXTRA_TIP_WIZARD);
            send_to_char("Tipwizard deactivated.\n\r", ch);
        } else {
            set_extra(ch, EXTRA_TIP_WIZARD);
            send_to_char("Tipwizard activated!\n\r", ch);
        }
        return;
    }
    if (!strcmp(arg, "on")) {
        set_extra(ch, EXTRA_TIP_WIZARD);
        send_to_char("Tipwizard activated!\n\r", ch);
        return;
    }
    if (!strcmp(arg, "off")) {
        remove_extra(ch, EXTRA_TIP_WIZARD);
        send_to_char("Tipwizard deactivated.\n\r", ch);
        return;
    }
    send_to_char("Syntax: tipwizard {on/off}\n\r", ch);
}
