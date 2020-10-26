/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  xania.c: a variety of Xania-specific modifications and new utilities */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "DescriptorList.hpp"
#include "MobIndexData.hpp"
#include "comm.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/numeric/accumulate.hpp>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

void objectbug(std::string_view str, OBJ_INDEX_DATA *obj) {
    log_string("obj> {} (#{}): {}", obj->short_descr, obj->vnum, str);
}

void mobbug(std::string_view str, MobIndexData *mob) {
    log_string("mob> {} (#{}): {}", mob->short_descr, mob->vnum, str);
}

int report_object(OBJ_DATA *object, int boot) {
    int averagedam, allowedaverage;
    OBJ_INDEX_DATA *obj = object->pIndexData;

    auto value =
        ranges::accumulate(obj->affected | ranges::views::transform(&AFFECT_DATA::worth), AFFECT_DATA::Value{});

    /* Weapons are allowed 1 hit and 1 dam for each point */

    auto worth =
        value.worth + obj->item_type == ITEM_WEAPON ? (value.hit + value.damage) / 2 : value.hit + value.damage;

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
        objectbug(fmt::format("points too high: has {} points (max should be {})", worth, ((obj->level / 10) + 1)),
                  obj);
    }
    return worth;
}

/* report_mobile - for checking those not-hard-enough mobs */

void report_mobile(MobIndexData *mob) {

    if ((mob->damage.bonus() + mob->hitroll + ((mob->damage.number() * mob->damage.type()) + mob->damage.number() / 2))
        < (mob->level * 3 / 2))
        mobbug("can't do enough damage", mob);

    if ((mob->hit.number() + mob->hit.bonus()) < (mob->level * 30))
        mobbug("has too few health points", mob);
}

/* check_xania - check all of Xania and report those things that aren't what they should be */

void check_xania() {
    bug("obj> **********************************************************************");
    bug("obj> **               Beginning sweep of all object in Xania             **");
    bug("obj> **********************************************************************");

    for (auto *object : object_list) {
        report_object(object, 1);
    }

    bug("obj> **********************************************************************");
    bug("obj> **                       Object sweep completed                     **");
    bug("obj> **********************************************************************");

    bug("mob> **********************************************************************");
    bug("mob> **                       Beginning mobile sweep                     **");
    bug("mob> **********************************************************************");

    for (auto *mobile : char_list) {
        report_mobile(mobile->pIndexData);
    }

    bug("mob> **********************************************************************");
    bug("mob> **                       Mobile sweep completed                     **");
    bug("mob> **********************************************************************");
}

void do_immworth(Char *ch, const char *argument) {
    OBJ_DATA *obj;
    int worth, shouldbe;

    if ((obj = get_obj_world(ch, argument)) == nullptr) {
        ch->send_line("Nothing like that in Xania.");
        return;
    }

    worth = report_object(obj, 0);
    shouldbe = ((obj->level / 10) + 1);
    if (worth == shouldbe) {
        ch->send_line("Object '{}' has {} point(s) - exactly right.", obj->pIndexData->short_descr, worth);
    } else if (worth > shouldbe) {
        ch->send_line("Object '{}' has {} point(s), {} points |Rtoo high|w.", obj->pIndexData->short_descr, worth,
                      worth - shouldbe);
    } else {
        ch->send_line("Object '{}' has {} point(s), within the {} point maximum.", obj->pIndexData->short_descr, worth,
                      shouldbe);
    }
}

/* do_prefix added 19-05-97 PCFN */
void do_prefix(Char *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;

    auto prefix = smash_tilde(argument);
    if (prefix.length() > (MAX_STRING_LENGTH - 1))
        prefix.resize(MAX_STRING_LENGTH - 1);

    if (prefix.empty()) {
        if (ch->pcdata->prefix.empty()) {
            ch->send_line("No prefix to remove.");
        } else {
            ch->send_line("Prefix removed.");
            ch->pcdata->prefix.clear();
        }
    } else {
        ch->pcdata->prefix = prefix;
        ch->send_line("Prefix set to \"{}\"", ch->pcdata->prefix);
    }
}

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

/*****************************************/
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

/*****************************************/
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

// New skill learned function - use instead of looking it up directly in the pcdata!!
// Similar to get_skill
int get_skill_learned(Char *ch, int skill_number) {
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

/********************************************************************************************/

/*****************************************
 ******************************************************************/
int is_made_of(OBJ_DATA *obj, const char *material);

int check_material_vulnerability(Char *ch, OBJ_DATA *object) {

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

void spell_reincarnate(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    (void)vo;
    /* is of type TAR_IGNORE so ignore *vo */

    int num_of_corpses = 0, corpse, chance;

    if (ch->is_npc())
        return;

    /* scan the room looking for an appropriate objects...count them */
    for (auto *obj : ch->in_room->contents) {
        if ((obj->pIndexData->item_type == ITEM_CORPSE_NPC) || (obj->pIndexData->item_type == ITEM_CORPSE_PC))
            num_of_corpses++;
    }

    /* Did we find *any* corpses? */
    if (num_of_corpses == 0) {
        ch->send_line("There are no dead in this room to reincarnate!");
        return;
    }

    /* choose a corpse at random & find its corresponding OBJ_DATA */
    corpse = number_range(1, num_of_corpses);
    OBJ_DATA *obj{};
    for (auto *c : ch->in_room->contents) {
        if ((c->pIndexData->item_type == ITEM_CORPSE_NPC) || (c->pIndexData->item_type == ITEM_CORPSE_PC)) {
            if (--corpse == 0) {
                obj = c;
                break;
            }
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
        ((obj->pIndexData->item_type == ITEM_CORPSE_PC) && !obj->contains.empty()))
    /* if non-empty PC corpse */ {
        act("$s stands, then falls over again - lifeless.", ch, nullptr, obj, To::Room);
        act("$s stands, then falls over again - lifeless.", ch, nullptr, obj, To::Char);
        return;
    } else {
        Char *animated;

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
        animated->carrying = std::move(obj->contains);
        obj->contains.clear();
        /* Give the zombie its correct name and stuff */
        animated->long_descr = fmt::printf(animated->description, obj->description);
        animated->name = fmt::printf(animated->name, obj->name);

        /* Say byebye to the corpse */
        extract_obj(obj);

        /* Set up the zombie correctly */
        animated->master = ch;
        animated->leader = ch;
        do_follow(animated, ArgParser(ch->name));
    }
}

void do_smit(Char *ch) { ch->send_line("If you wish to smite someone, then SPELL it out!"); }

void do_smite(Char *ch, const char *argument) {
    /* Power of the Immortals! By Faramir
                                 Don't use this too much, it hurts :) */

    const char *smitestring;
    Char *victim;
    OBJ_DATA *obj;

    if (argument[0] == '\0') {
        ch->send_line("Upon whom do you wish to unleash your power?");
        return;
    }

    if ((victim = get_char_room(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    } /* Not (visibly) present in room! */

    /* In case a mort thinks he can get away
                             with it, shah right! MMFOOMB
                             Should be dealt with in interp.c already*/

    if (ch->is_npc()) {
        ch->send_line("You must take your true form before unleashing your power.");
        return;
    } /* done whilst switched? No way Jose */

    if (victim->get_trust() > ch->get_trust()) {

        ch->send_line("You failed.\n\rUmmm...beware of retaliation!");
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

    ch->send_line("You |W>>> |YSMITE|W <<<|w {} with all of your Godly powers!",
                  (victim == ch) ? "yourself" : victim->name);

    victim->hit /= 2; /* easiest way of halving hp? */
    if (victim->hit < 1)
        victim->hit = 1; /* Cap the damage */

    victim->position = POS_RESTING;
    /* Knock them into resting and disarm them regardless of whether they have talon or a noremove weapon */

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

bool web_see(Char *ch) {

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
        bug("web_who: couldn't open {} for writing!", WEB_WHO_FILE);
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
                    class_table[wch->class_num].name, wch->name.c_str(), wch->pcdata->title.c_str());
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
        bug("Couldn't open tip file \'{}\' for reading", TIPWIZARD_FILE);
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
            log_string("Loaded {} tips", tipcount);
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
    auto tip = fmt::format("|WTip: {}|w\n\r", tip_current->tip);
    for (auto &d : descriptors().playing()) {
        Char *ch = d.person();
        if (is_set_extra(ch, EXTRA_TIP_WIZARD))
            ch->send_to(tip);
    }
    tip_current = tip_current->next;
}

void do_tipwizard(Char *ch, const char *arg) {

    if (arg[0] == '\0') {
        if (is_set_extra(ch, EXTRA_TIP_WIZARD)) {
            remove_extra(ch, EXTRA_TIP_WIZARD);
            ch->send_line("Tipwizard deactivated.");
        } else {
            set_extra(ch, EXTRA_TIP_WIZARD);
            ch->send_line("Tipwizard activated!");
        }
        return;
    }
    if (!strcmp(arg, "on")) {
        set_extra(ch, EXTRA_TIP_WIZARD);
        ch->send_line("Tipwizard activated!");
        return;
    }
    if (!strcmp(arg, "off")) {
        remove_extra(ch, EXTRA_TIP_WIZARD);
        ch->send_line("Tipwizard deactivated.");
        return;
    }
    ch->send_line("Syntax: tipwizard {on/off}");
}
