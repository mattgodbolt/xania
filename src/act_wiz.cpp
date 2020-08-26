/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_wiz.c: commands for immortals                                    */
/*                                                                       */
/*************************************************************************/

#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "TimeInfoData.hpp"
#include "buffer.h"
#include "challeng.h"
#include "comm.hpp"
#include "db.h"
#include "flags.h"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "merc.h"
#include "string_utils.hpp"
#include "tables.h"

#include <fmt/format.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <sys/types.h>

using namespace fmt::literals;

static const char ROOM_FLAGS[] = "dark * nomob indoors * * * * * private safe solitary petshop norecall 100imponly "
                                 "92godonly heroonly newbieonly law";

void do_mskills(CHAR_DATA *ch, const char *argument);
void do_maffects(CHAR_DATA *ch, const char *argument);
void do_mpracs(CHAR_DATA *ch, const char *argument);
void do_minfo(CHAR_DATA *ch, const char *argument);
void do_mspells(CHAR_DATA *ch, const char *argument);

/*
 * Local functions.
 */
ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, const char *arg);

/* Permits or denies a player from playing the Mud from a PERMIT banned site */
void do_permit(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    int flag = 1;
    if (ch->is_npc())
        return;
    if (argument[0] == '-') {
        argument++;
        flag = 0;
    }
    if (argument[0] == '+')
        argument++;
    victim = get_char_room(ch, argument);
    if (victim == nullptr || victim->is_npc()) {
        send_to_char("Permit whom?\n\r", ch);
        return;
    }
    if (flag) {
        set_extra(victim, EXTRA_PERMIT);
    } else {
        remove_extra(victim, EXTRA_PERMIT);
    }
    bug_snprintf(buf, sizeof(buf), "PERMIT flag %s for %s.\n\r", (flag) ? "set" : "removed", victim->name);
    send_to_char(buf, ch);
}

/* equips a character */
void do_outfit(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];

    if (ch->level > 5 || ch->is_npc()) {
        send_to_char("Find it yourself!\n\r", ch);
        return;
    }

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) == nullptr) {
        obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_BANNER));
        obj->cost = 0;
        obj_to_char(obj, ch);
        equip_char(ch, obj, WEAR_LIGHT);
    }

    if ((obj = get_eq_char(ch, WEAR_BODY)) == nullptr) {
        obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_VEST));
        obj->cost = 0;
        obj_to_char(obj, ch);
        equip_char(ch, obj, WEAR_BODY);
    }

    if ((obj = get_eq_char(ch, WEAR_SHIELD)) == nullptr) {
        obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_SHIELD));
        obj->cost = 0;
        obj_to_char(obj, ch);
        equip_char(ch, obj, WEAR_SHIELD);
    }

    if ((obj = get_eq_char(ch, WEAR_WIELD)) == nullptr) {
        obj = create_object(get_obj_index(class_table[ch->class_num].weapon));
        obj_to_char(obj, ch);
        equip_char(ch, obj, WEAR_WIELD);
    }

    bug_snprintf(buf, sizeof(buf), "You have been equipped by %s.\n\r", deity_name);
    send_to_char(buf, ch);
}

/* RT nochannels command, for those spammers */
void do_nochannels(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Nochannel whom?", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOCHANNELS)) {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char("The gods have restored your channel privileges.\n\r", victim);
        send_to_char("NOCHANNELS removed.\n\r", ch);
    } else {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char("The gods have revoked your channel privileges.\n\r", victim);
        send_to_char("NOCHANNELS set.\n\r", ch);
    }
}

void do_bamfin(CHAR_DATA *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfin = smash_tilde(argument);

    if (bamfin.empty()) {
        ch->send_to("Your poofin is {}\n\r"_format(ch->pcdata->bamfin));
        return;
    }

    if (strstr(bamfin.c_str(), ch->name) == nullptr) {
        ch->send_to("You must include your name.\n\r");
        return;
    }

    ch->pcdata->bamfin = bamfin;
    ch->send_to("Your poofin is now {}\n\r"_format(ch->pcdata->bamfin));
}

void do_bamfout(CHAR_DATA *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfout = smash_tilde(argument);

    if (bamfout.empty()) {
        ch->send_to("Your poofout is {}\n\r"_format(ch->pcdata->bamfout));
        return;
    }

    if (strstr(bamfout.c_str(), ch->name) == nullptr) {
        ch->send_to("You must include your name.\n\r");
        return;
    }

    ch->pcdata->bamfout = bamfout;
    ch->send_to("Your poofout is now {}\n\r"_format(ch->pcdata->bamfout));
}

void do_deny(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Deny whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    SET_BIT(victim->act, PLR_DENY);
    send_to_char("You are denied access!\n\r", victim);
    send_to_char("OK.\n\r", ch);
    save_char_obj(victim);
    do_quit(victim, "");
}

void do_disconnect(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Disconnect whom?\n\r", ch);
        return;
    }

    if (argument[0] == '+') {
        argument++;
        for (auto &d : descriptors().all()) {
            if (d.character() && !str_cmp(d.character()->name, argument)) {
                d.close();
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }
        send_to_char("Couldn't find a matching descriptor.\n\r", ch);
        return;
    } else {
        if ((victim = get_char_world(ch, arg)) == nullptr) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (victim->desc == nullptr) {
            act("$N doesn't have a descriptor.", ch, nullptr, victim, To::Char);
            return;
        }

        for (auto &d : descriptors().all()) {
            if (&d == victim->desc) {
                d.close();
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }

        bug("Do_disconnect: desc not found.");
        send_to_char("Descriptor not found!\n\r", ch);
        return;
    }
}

void do_pardon(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: pardon <character> <killer|thief>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "killer")) {
        if (IS_SET(victim->act, PLR_KILLER)) {
            REMOVE_BIT(victim->act, PLR_KILLER);
            send_to_char("Killer flag removed.\n\r", ch);
            send_to_char("You are no longer a KILLER.\n\r", victim);
        }
        return;
    }

    if (!str_cmp(arg2, "thief")) {
        if (IS_SET(victim->act, PLR_THIEF)) {
            REMOVE_BIT(victim->act, PLR_THIEF);
            send_to_char("Thief flag removed.\n\r", ch);
            send_to_char("You are no longer a THIEF.\n\r", victim);
        }
        return;
    }

    send_to_char("Syntax: pardon <character> <killer|thief>.\n\r", ch);
}

void do_echo(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_to("Global echo what?\n\r");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::to_character()) {
        victim.send_to("{}{}\n\r"_format(victim.get_trust() >= ch->get_trust() ? "global> " : "", argument));
    }
}

void do_recho(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_to("Local echo what?\n\r");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_room(*ch) | DescriptorFilter::to_character()) {
        victim.send_to("{}{}\n\r"_format(
            victim.get_trust() >= ch->get_trust() && ch->in_room->vnum != CHAL_VIEWING_GALLERY ? "local> " : "",
            argument));
    }
}

void do_zecho(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_to("Zone echo what?\n\r");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_area(*ch) | DescriptorFilter::to_character()) {
        victim.send_to("{}{}\n\r"_format(victim.get_trust() >= ch->get_trust() ? "zone> " : "", argument));
    }
}

void do_pecho(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0') {
        ch->send_to("Personal echo what?\n\r");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_to("Target not found.\n\r");
        return;
    }

    if (victim->get_trust() >= ch->get_trust() && ch->get_trust() != MAX_LEVEL)
        victim->send_to("personal> ");

    victim->send_to("{}\n\r"_format(argument));
    ch->send_to("personal> {}\n\r"_format(argument));
}

ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, const char *arg) {
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if (is_number(arg))
        return get_room_index(atoi(arg));

    if ((!str_cmp(arg, "here")))
        return ch->in_room;

    if ((victim = get_char_world(ch, arg)) != nullptr)
        return victim->in_room;

    if ((obj = get_obj_world(ch, arg)) != nullptr)
        return obj->in_room;

    return nullptr;
}

void do_transfer(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        send_to_char("Transfer whom (and where)?\n\r", ch);
        return;
    }

    if (!str_cmp(arg1, "all")) {
        for (auto &victim :
             descriptors().all_visible_to(*ch) | DescriptorFilter::except(*ch) | DescriptorFilter::to_character()) {
            if (victim.in_room != nullptr) {
                char buf[MAX_STRING_LENGTH];
                bug_snprintf(buf, sizeof(buf), "%s %s", victim.name, arg2);
                do_transfer(ch, buf);
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0') {
        location = ch->in_room;
    } else {
        if ((location = find_location(ch, arg2)) == nullptr) {
            send_to_char("No such location.\n\r", ch);
            return;
        }

        if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
            send_to_char("That room is private right now.\n\r", ch);
            return;
        }
    }

    CHAR_DATA *victim;
    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->in_room == nullptr) {
        send_to_char("They are in limbo.\n\r", ch);
        return;
    }

    if (victim->fighting != nullptr)
        stop_fighting(victim, true);
    if (victim->riding != nullptr)
        unride_char(victim, victim->riding);
    act("$n disappears in a mushroom cloud.", victim);
    char_from_room(victim);
    char_to_room(victim, location);
    act("$n arrives from a puff of smoke.", victim);
    if (ch != victim)
        act("$n has transferred you.", ch, nullptr, victim, To::Vict);
    do_look(victim, "auto");
    send_to_char("Ok.\n\r", ch);
}

void do_wizlock(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    extern bool wizlock;
    wizlock = !wizlock;

    if (wizlock)
        send_to_char("Game wizlocked.\n\r", ch);
    else
        send_to_char("Game un-wizlocked.\n\r", ch);
}

/* RT anti-newbie code */

void do_newlock(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    extern bool newlock;
    newlock = !newlock;

    if (newlock)
        send_to_char("New characters have been locked out.\n\r", ch);
    else
        send_to_char("Newlock removed.\n\r", ch);
}

void do_at(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    CHAR_DATA *wch;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        send_to_char("At where what?\n\r", ch);
        return;
    }

    if ((location = find_location(ch, arg)) == nullptr) {
        send_to_char("No such location.\n\r", ch);
        return;
    }
    if ((ch->in_room != nullptr) && location == ch->in_room) {
        send_to_char("But that's in here.......\n\r", ch);
        return;
    }
    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }
    original = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    interpret(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (wch = char_list; wch != nullptr; wch = wch->next) {
        if (wch == ch) {
            char_from_room(ch);
            char_to_room(ch, original);
            break;
        }
    }
}

void do_goto(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        send_to_char("Goto where?\n\r", ch);
        return;
    }

    ROOM_INDEX_DATA *location;
    if ((location = find_location(ch, argument)) == nullptr) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    if (ch->fighting != nullptr)
        stop_fighting(ch, true);

    // We don't user ch->player()'s bamfin/bamfout to avoid exposing which IMM is controlling a switched mob.
    for (auto *rch = ch->in_room->people; rch != nullptr; rch = rch->next_in_room) {
        if (rch->get_trust() >= ch->invis_level) {
            if (ch->pcdata != nullptr && !ch->pcdata->bamfout.empty())
                act("$t", ch, ch->pcdata->bamfout, rch, To::Vict);
            else
                act("$n leaves in a swirling mist.", ch, nullptr, rch, To::Vict);
        }
    }

    char_from_room(ch);
    char_to_room(ch, location);
    if (ch->pet) {
        char_from_room(ch->pet);
        char_to_room(ch->pet, location);
    }

    for (auto *rch = ch->in_room->people; rch != nullptr; rch = rch->next_in_room) {
        if (rch->get_trust() >= ch->invis_level) {
            if (ch->pcdata != nullptr && !ch->pcdata->bamfin.empty())
                act("$t", ch, ch->pcdata->bamfin, rch, To::Vict);
            else
                act("$n appears in a swirling mist.", ch, nullptr, rch, To::Vict);
        }
    }

    do_look(ch, "auto");
}

/* RT to replace the 3 stat commands */

void do_stat(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    const char *string;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *location;
    CHAR_DATA *victim;

    string = one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  stat <name>\n\r", ch);
        send_to_char("  stat obj <name>\n\r", ch);
        send_to_char("  stat mob <name>\n\r", ch);
        send_to_char("  stat room <number>\n\r", ch);
        send_to_char("  stat <skills/spells/info/pracs/affects> <name>\n\r", ch);
        send_to_char("  stat prog <mobprog>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "room")) {
        do_rstat(ch, string);
        return;
    }

    if (!str_cmp(arg, "obj")) {
        do_ostat(ch, string);
        return;
    }

    if (!str_cmp(arg, "char") || !str_cmp(arg, "mob")) {
        do_mstat(ch, string);
        return;
    }

    if (!str_cmp(arg, "skills")) {
        do_mskills(ch, string);
        return;
    }
    if (!str_cmp(arg, "affects")) {
        do_maffects(ch, string);
        return;
    }
    if (!str_cmp(arg, "pracs")) {
        do_mpracs(ch, string);
        return;
    }
    if (!str_cmp(arg, "info")) {
        do_minfo(ch, string);
        return;
    }
    if (!str_cmp(arg, "spells")) {
        do_mspells(ch, string);
        return;
    }

    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    if (!str_cmp(arg, "prog")) {
        do_mpstat(ch, string); /* in mob_commands.c */
        return;
    }

    /* do it the old way */

    obj = get_obj_world(ch, argument);
    if (obj != nullptr) {
        do_ostat(ch, argument);
        return;
    }

    victim = get_char_world(ch, argument);
    if (victim != nullptr) {
        do_mstat(ch, argument);
        return;
    }

    location = find_location(ch, argument);
    if (location != nullptr) {
        do_rstat(ch, argument);
        return;
    }

    send_to_char("Nothing by that name found anywhere.\n\r", ch);
}

void do_rstat(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;

    one_argument(argument, arg);
    location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
    if (location == nullptr) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (ch->in_room != location && room_is_private(location) && ch->get_trust() < IMPLEMENTOR) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    bug_snprintf(buf, sizeof(buf), "Name: '%s.'\n\rArea: '%s'.\n\r", location->name, location->area->name);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Vnum: %d.  Sector: %d.  Light: %d.\n\r", location->vnum, location->sector_type,
                 location->light);
    send_to_char(buf, ch);
    send_to_char("Flags: ", ch);
    display_flags(ROOM_FLAGS, ch, location->room_flags);
    bug_snprintf(buf, sizeof(buf), "Description:\n\r%s", location->description);
    send_to_char(buf, ch);

    if (location->extra_descr != nullptr) {
        EXTRA_DESCR_DATA *ed;

        send_to_char("Extra description keywords: '", ch);
        for (ed = location->extra_descr; ed; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != nullptr)
                send_to_char(" ", ch);
        }
        send_to_char("'.\n\r", ch);
    }

    send_to_char("Characters:", ch);
    for (rch = location->people; rch; rch = rch->next_in_room) {
        if (can_see(ch, rch)) {
            send_to_char(" ", ch);
            one_argument(rch->name, buf);
            send_to_char(buf, ch);
        }
    }

    send_to_char(".\n\rObjects:   ", ch);
    for (obj = location->contents; obj; obj = obj->next_content) {
        send_to_char(" ", ch);
        one_argument(obj->name, buf);
        send_to_char(buf, ch);
    }
    send_to_char(".\n\r", ch);

    for (auto door : all_directions) {
        if (auto *pexit = location->exit[door]) {
            ch->send_to("Door: {}.  To: {}.  Key: {}.  Exit flags: {}.\n\rKeyword: '{}'.  Description: {}"_format(
                to_string(door), (pexit->u1.to_room == nullptr ? -1 : pexit->u1.to_room->vnum), pexit->key,
                pexit->exit_info, pexit->keyword, pexit->description[0] != '\0' ? pexit->description : "(none).\n\r"));
        }
    }
}

void do_ostat(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    sh_int dam_type;
    int vnum;

    /* this will help prevent major memory allocations - a crash bug!
     --Faramir */
    if (strlen(argument) < 2 && !isdigit(argument[0])) {
        send_to_char("Please be more specific.\n\r", ch);
        return;
    }
    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat what?\n\r", ch);
        return;
    }

    if (isdigit(argument[0])) {
        vnum = atoi(argument);
        if ((pObjIndex = get_obj_index(vnum)) == nullptr) {
            send_to_char("Nothing like that in hell, earth, or heaven.\n\r", ch);
            return;
        }
        send_to_char("Template of object:\n\r", ch);
        bug_snprintf(buf, sizeof(buf), "Name(s): %s\n\r", pObjIndex->name);
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Vnum: %d  Type: %s\n\r", pObjIndex->vnum, item_index_type_name(pObjIndex));
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Short description: %s\n\rLong description: %s\n\r", pObjIndex->short_descr,
                     pObjIndex->description);
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Wear bits: %s\n\rExtra bits: %s\n\r", wear_bit_name(pObjIndex->wear_flags),
                     extra_bit_name(pObjIndex->extra_flags));
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Wear string: %s\n\r", pObjIndex->wear_string);
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Weight: %d\n\r", pObjIndex->weight);
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Level: %d  Cost: %d  Condition: %d\n\r", pObjIndex->level, pObjIndex->cost,
                     pObjIndex->condition);
        send_to_char(buf, ch);

        bug_snprintf(buf, sizeof(buf), "Values: %d %d %d %d %d\n\r", pObjIndex->value[0], pObjIndex->value[1],
                     pObjIndex->value[2], pObjIndex->value[3], pObjIndex->value[4]);
        send_to_char(buf, ch);
        send_to_char("Please load this object if you need to know more about it.\n\r", ch);
        return;
    }

    if ((obj = get_obj_world(ch, argument)) == nullptr) {
        send_to_char("Nothing like that in hell, earth, or heaven.\n\r", ch);
        return;
    }

    bug_snprintf(buf, sizeof(buf), "Name(s): %s\n\r", obj->name);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Vnum: %d  Type: %s  Resets: %d\n\r", obj->pIndexData->vnum, item_type_name(obj),
                 obj->pIndexData->reset_num);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Short description: %s\n\rLong description: %s\n\r", obj->short_descr,
                 obj->description);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Wear bits: %s\n\rExtra bits: %s\n\r", wear_bit_name(obj->wear_flags),
                 extra_bit_name(obj->extra_flags));
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Wear string: %s\n\r", obj->wear_string);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Number: %d/%d  Weight: %d/%d\n\r", 1, get_obj_number(obj), obj->weight,
                 get_obj_weight(obj));
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Level: %d  Cost: %d  Condition: %d  Timer: %d\n\r", obj->level, obj->cost,
                 obj->condition, obj->timer);
    send_to_char(buf, ch);

    bug_snprintf(
        buf, sizeof(buf), "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
        obj->in_room == nullptr ? 0 : obj->in_room->vnum, obj->in_obj == nullptr ? "(none)" : obj->in_obj->short_descr,
        obj->carried_by == nullptr ? "(none)" : can_see(ch, obj->carried_by) ? obj->carried_by->name : "someone",
        obj->wear_loc);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Values: %d %d %d %d %d\n\r", obj->value[0], obj->value[1], obj->value[2],
                 obj->value[3], obj->value[4]);
    send_to_char(buf, ch);

    /* now give out vital statistics as per identify */

    switch (obj->item_type) {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_BOMB:
        bug_snprintf(buf, sizeof(buf), "Level %d spells of:", obj->value[0]);
        send_to_char(buf, ch);

        if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL) {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->value[1]].name, ch);
            send_to_char("'", ch);
        }

        if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL) {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->value[2]].name, ch);
            send_to_char("'", ch);
        }

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->value[3]].name, ch);
            send_to_char("'", ch);
        }

        if ((obj->value[4] >= 0 && obj->value[4] < MAX_SKILL) && obj->item_type == ITEM_BOMB) {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->value[4]].name, ch);
            send_to_char("'", ch);
        }

        send_to_char(".\n\r", ch);
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        bug_snprintf(buf, sizeof(buf), "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0]);
        send_to_char(buf, ch);

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->value[3]].name, ch);
            send_to_char("'", ch);
        }

        send_to_char(".\n\r", ch);
        break;

    case ITEM_WEAPON:
        send_to_char("Weapon type is ", ch);
        switch (obj->value[0]) {
        case (WEAPON_EXOTIC): send_to_char("exotic\n\r", ch); break;
        case (WEAPON_SWORD): send_to_char("sword\n\r", ch); break;
        case (WEAPON_DAGGER): send_to_char("dagger\n\r", ch); break;
        case (WEAPON_SPEAR): send_to_char("spear/staff\n\r", ch); break;
        case (WEAPON_MACE): send_to_char("mace/club\n\r", ch); break;
        case (WEAPON_AXE): send_to_char("axe\n\r", ch); break;
        case (WEAPON_FLAIL): send_to_char("flail\n\r", ch); break;
        case (WEAPON_WHIP): send_to_char("whip\n\r", ch); break;
        case (WEAPON_POLEARM): send_to_char("polearm\n\r", ch); break;
        default: send_to_char("unknown\n\r", ch); break;
        }
        bug_snprintf(buf, sizeof(buf), "Damage is %dd%d (average %d)\n\r", obj->value[1], obj->value[2],
                     (1 + obj->value[2]) * obj->value[1] / 2);
        send_to_char(buf, ch);

        if (obj->value[4]) /* weapon flags */
        {
            bug_snprintf(buf, sizeof(buf), "Weapons flags: %s\n\r", weapon_bit_name(obj->value[4]));
            send_to_char(buf, ch);
        }

        dam_type = attack_table[obj->value[3]].damage;
        send_to_char("Damage type is ", ch);
        switch (dam_type) {
        case (DAM_NONE): bug_snprintf(buf, sizeof(buf), "none.\n\r"); break;
        case (DAM_BASH): bug_snprintf(buf, sizeof(buf), "bash.\n\r"); break;
        case (DAM_PIERCE): bug_snprintf(buf, sizeof(buf), "pierce.\n\r"); break;
        case (DAM_SLASH): bug_snprintf(buf, sizeof(buf), "slash.\n\r"); break;
        case (DAM_FIRE): bug_snprintf(buf, sizeof(buf), "fire.\n\r"); break;
        case (DAM_COLD): bug_snprintf(buf, sizeof(buf), "cold.\n\r"); break;
        case (DAM_LIGHTNING): bug_snprintf(buf, sizeof(buf), "lightning.\n\r"); break;
        case (DAM_ACID): bug_snprintf(buf, sizeof(buf), "acid.\n\r"); break;
        case (DAM_POISON): bug_snprintf(buf, sizeof(buf), "poison.\n\r"); break;
        case (DAM_NEGATIVE): bug_snprintf(buf, sizeof(buf), "negative.\n\r"); break;
        case (DAM_HOLY): bug_snprintf(buf, sizeof(buf), "holy.\n\r"); break;
        case (DAM_ENERGY): bug_snprintf(buf, sizeof(buf), "energy.\n\r"); break;
        case (DAM_MENTAL): bug_snprintf(buf, sizeof(buf), "mental.\n\r"); break;
        case (DAM_DISEASE): bug_snprintf(buf, sizeof(buf), "disease.\n\r"); break;
        case (DAM_DROWNING): bug_snprintf(buf, sizeof(buf), "drowning.\n\r"); break;
        case (DAM_LIGHT): bug_snprintf(buf, sizeof(buf), "light.\n\r"); break;
        case (DAM_OTHER): bug_snprintf(buf, sizeof(buf), "other.\n\r"); break;
        case (DAM_HARM): bug_snprintf(buf, sizeof(buf), "harm.\n\r"); break;
        default:
            bug_snprintf(buf, sizeof(buf), "unknown!!!!\n\r");
            bug("ostat: Unknown damage type %d", dam_type);
            break;
        }
        send_to_char(buf, ch);
        break;

    case ITEM_ARMOR:
        bug_snprintf(buf, sizeof(buf), "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
                     obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
        send_to_char(buf, ch);
        break;

    case ITEM_PORTAL:
        bug_snprintf(buf, sizeof(buf), "Portal to %s (%d).\n\r", obj->destination->name, obj->destination->vnum);
        send_to_char(buf, ch);
        break;
    }

    if (obj->extra_descr != nullptr || obj->pIndexData->extra_descr != nullptr) {
        EXTRA_DESCR_DATA *ed;

        send_to_char("Extra description keywords: '", ch);

        for (ed = obj->extra_descr; ed != nullptr; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != nullptr)
                send_to_char(" ", ch);
        }

        for (ed = obj->pIndexData->extra_descr; ed != nullptr; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != nullptr)
                send_to_char(" ", ch);
        }

        send_to_char("'\n\r", ch);
    }

    for (paf = obj->affected; paf != nullptr; paf = paf->next) {
        bug_snprintf(buf, sizeof(buf), "Affects %s by %d, level %d.\n\r", affect_loc_name(paf->location), paf->modifier,
                     paf->level);
        send_to_char(buf, ch);
    }

    if (!obj->enchanted)
        for (paf = obj->pIndexData->affected; paf != nullptr; paf = paf->next) {
            bug_snprintf(buf, sizeof(buf), "Affects %s by %d, level %d.\n\r", affect_loc_name(paf->location),
                         paf->modifier, paf->level);
            send_to_char(buf, ch);
        }
}

void do_mskills(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char skill_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO];
    int sn, lev;
    bool found = false;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat skills whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc())
        return;

    bug_snprintf(buf, sizeof(buf), "Skill list for %s:\n\r", victim->name);
    send_to_char(buf, ch);

    /* initilize data */
    for (lev = 0; lev < LEVEL_HERO; lev++) {
        skill_columns[lev] = 0;
        skill_list[lev][0] = '\0';
    }

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (get_skill_level(victim, sn) < LEVEL_HERO && skill_table[sn].spell_fun == spell_null
            && victim->pcdata->learned[sn] > 0) // NOT get_skill_learned
        {
            found = true;
            lev = get_skill_level(victim, sn);
            if (victim->level < lev)
                bug_snprintf(buf, sizeof(buf), "%-18s  n/a      ", skill_table[sn].name);
            else
                bug_snprintf(buf, sizeof(buf), "%-18s %3d%%      ", skill_table[sn].name,
                             victim->pcdata->learned[sn]); // NOT get_skill_

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
        send_to_char("They know no skills.\n\r", ch);
        return;
    }

    for (lev = 0; lev < LEVEL_HERO; lev++)
        if (skill_list[lev][0] != '\0')
            send_to_char(skill_list[lev], ch);
    send_to_char("\n\r", ch);
}

/* Corrected 28/8/96 by Oshea to give correct list of spell costs. */
void do_mspells(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char spell_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO];
    int sn, lev, mana;
    bool found = false;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat spells whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc())
        return;

    bug_snprintf(buf, sizeof(buf), "Spell list for %s:\n\r", victim->name);
    send_to_char(buf, ch);

    /* initilize data */
    for (lev = 0; lev < LEVEL_HERO; lev++) {
        spell_columns[lev] = 0;
        spell_list[lev][0] = '\0';
    }

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;

        if (get_skill_level(victim, sn) < LEVEL_HERO && skill_table[sn].spell_fun != spell_null
            && victim->pcdata->learned[sn] > 0) // NOT get_skill_learned
        {
            found = true;
            lev = get_skill_level(victim, sn);
            if (victim->level < lev)
                bug_snprintf(buf, sizeof(buf), "%-18s   n/a      ", skill_table[sn].name);
            else {
                mana = UMAX(skill_table[sn].min_mana, 100 / (2 + victim->level - lev));
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
        send_to_char("They know no spells.\n\r", ch);
        return;
    }

    for (lev = 0; lev < LEVEL_HERO; lev++)
        if (spell_list[lev][0] != '\0')
            send_to_char(spell_list[lev], ch);
    send_to_char("\n\r", ch);
}

void do_maffects(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    AFFECT_DATA *paf;
    int flag = 0;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat affects whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    bug_snprintf(buf, sizeof(buf), "Affect list for %s:\n\r", victim->name);
    send_to_char(buf, ch);

    if (victim->affected != nullptr) {
        for (paf = victim->affected; paf != nullptr; paf = paf->next) {
            if ((paf->type == gsn_sneak) || (paf->type == gsn_ride)) {
                bug_snprintf(buf, sizeof(buf), "Skill: '%s'", skill_table[paf->type].name);
                flag = 1;
            } else {
                bug_snprintf(buf, sizeof(buf), "Spell: '%s'", skill_table[paf->type].name);
                flag = 0;
            }
            send_to_char(buf, ch);

            if (flag == 0) {
                bug_snprintf(buf, sizeof(buf), " modifies %s by %d for %d hours", affect_loc_name(paf->location),
                             paf->modifier, paf->duration);
                send_to_char(buf, ch);
            }

            send_to_char(".\n\r", ch);
        }
    } else {
        send_to_char("Nothing.\n\r", ch);
    }
}

/* Corrected 28/8/96 by Oshea to give correct list of spells/skills. */
void do_mpracs(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int sn;
    int col;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat pracs whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc())
        return;

    bug_snprintf(buf, sizeof(buf), "Practice list for %s:\n\r", victim->name);
    send_to_char(buf, ch);

    col = 0;
    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        if (victim->level < get_skill_level(victim, sn)
            || victim->pcdata->learned[sn] < 1 /* skill is not known NOT get_skill_learned */)
            continue;

        bug_snprintf(buf, sizeof(buf), "%-18s %3d%%  ", skill_table[sn].name, victim->pcdata->learned[sn]);
        send_to_char(buf, ch);
        if (++col % 3 == 0)
            send_to_char("\n\r", ch);
    }

    if (col % 3 != 0)
        send_to_char("\n\r", ch);

    bug_snprintf(buf, sizeof(buf), "They have %d practice sessions left.\n\r", victim->practice);
    send_to_char(buf, ch);
}

/* Correct on 28/8/96 by Oshea to give correct cp's */
void do_minfo(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int gn, col;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat info whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc())
        return;

    bug_snprintf(buf, sizeof(buf), "Info list for %s:\n\r", victim->name);

    col = 0;

    /* show all groups */

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;
        if (victim->pcdata->group_known[gn]) {
            bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].name);
            send_to_char(buf, ch);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", victim->pcdata->points);

    send_to_char(buf, ch);
}

void do_mstat(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;

    /* this will help prevent major memory allocations */
    if (strlen(argument) < 2) {
        send_to_char("Please be more specific.\n\r", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Stat whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    bug_snprintf(buf, sizeof(buf), "Name: %s     Clan: %s     Rank: %s.\n\r", victim->name,
                 victim->clan() ? victim->clan()->name : "(none)",
                 victim->pc_clan() ? victim->pc_clan()->level_name() : "(none)");
    send_to_char(buf, ch);

    do {
        if (snprintf(buf, sizeof(buf), "Vnum: %d  Format: %s  Race: %s  Sex: %s  Room: %d\n\r",
                     victim->is_npc() ? victim->pIndexData->vnum : 0, victim->is_npc() ? ".are" : "pc",
                     race_table[victim->race].name, victim->sex == 1 ? "male" : victim->sex == 2 ? "female" : "neutral",
                     victim->in_room == nullptr ? 0 : victim->in_room->vnum)
            < 0)
            bug("Buffer too small at "
                "_file_name_"
                ":"
                "1296"
                " - message was truncated");
    } while (0);
    send_to_char(buf, ch);

    if (victim->is_npc()) {
        bug_snprintf(buf, sizeof(buf), "Count: %d  Killed: %d\n\r", victim->pIndexData->count,
                     victim->pIndexData->killed);
        send_to_char(buf, ch);
    }

    bug_snprintf(buf, sizeof(buf), "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
                 victim->perm_stat[Stat::Str], get_curr_stat(victim, Stat::Str), victim->perm_stat[Stat::Int],
                 get_curr_stat(victim, Stat::Int), victim->perm_stat[Stat::Wis], get_curr_stat(victim, Stat::Wis),
                 victim->perm_stat[Stat::Dex], get_curr_stat(victim, Stat::Dex), victim->perm_stat[Stat::Con],
                 get_curr_stat(victim, Stat::Con));
    send_to_char(buf, ch);

    do {
        if (snprintf(buf, sizeof(buf), "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d\n\r", victim->hit,
                     victim->max_hit, victim->mana, victim->max_mana, victim->move, victim->max_move,
                     ch->is_npc() ? 0 : victim->practice)
            < 0)
            bug("Buffer too small at "
                "_file_name_"
                ":"
                "1314"
                " - message was truncated");
    } while (0);
    send_to_char(buf, ch);

    do {
        if (snprintf(buf, sizeof(buf), "Lv: %d  Class: %s  Align: %d  Gold: %ld  Exp: %ld\n\r", victim->level,
                     victim->is_npc() ? "mobile" : class_table[victim->class_num].name, victim->alignment, victim->gold,
                     victim->exp)
            < 0)
            bug("Buffer too small at "
                "_file_name_"
                ":"
                "1319"
                " - message was truncated");
    } while (0);
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r", GET_AC(victim, AC_PIERCE),
                 GET_AC(victim, AC_BASH), GET_AC(victim, AC_SLASH), GET_AC(victim, AC_EXOTIC));
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Hit: %d  Dam: %d  Saves: %d  Position: %d  Wimpy: %d\n\r", GET_HITROLL(victim),
                 GET_DAMROLL(victim), victim->saving_throw, victim->position, victim->wimpy);
    send_to_char(buf, ch);

    if (victim->is_npc()) {
        bug_snprintf(buf, sizeof(buf), "Damage: %dd%d  Message:  %s\n\r", victim->damage[DICE_NUMBER],
                     victim->damage[DICE_TYPE], attack_table[victim->dam_type].noun);
        send_to_char(buf, ch);
    }
    bug_snprintf(buf, sizeof(buf), "Fighting: %s\n\r", victim->fighting ? victim->fighting->name : "(none)");
    send_to_char(buf, ch);

    ch->send_to(
        "Sentient 'victim': {}\n\r"_format(victim->sentient_victim.empty() ? "(none)" : victim->sentient_victim));

    if (victim->is_pc()) {
        bug_snprintf(buf, sizeof(buf), "Thirst: %d  Full: %d  Drunk: %d\n\r", victim->pcdata->condition[COND_THIRST],
                     victim->pcdata->condition[COND_FULL], victim->pcdata->condition[COND_DRUNK]);
        send_to_char(buf, ch);
    }

    bug_snprintf(buf, sizeof(buf), "Carry number: %d  Carry weight: %d\n\r", victim->carry_number,
                 victim->carry_weight);
    send_to_char(buf, ch);

    if (victim->is_pc()) {
        using namespace std::chrono;
        bug_snprintf(buf, sizeof(buf), "Age: %d  Played: %ld  Last Level: %d  Timer: %d\n\r", get_age(victim),
                     duration_cast<hours>(victim->total_played()).count(), victim->pcdata->last_level, victim->timer);
        send_to_char(buf, ch);
    }

    bug_snprintf(buf, sizeof(buf), "Act: %s\n\r", (char *)act_bit_name(victim->act));
    send_to_char(buf, ch);

    if (victim->is_pc()) {
        int n;
        bug_snprintf(buf, sizeof(buf), "Extra: ");
        for (n = 0; n < MAX_EXTRA_FLAGS; n++) {
            if (is_set_extra(ch, n)) {
                strcat(buf, flagname_extra[n]);
            }
        }
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
    }

    if (victim->comm) {
        bug_snprintf(buf, sizeof(buf), "Comm: %s\n\r", (char *)comm_bit_name(victim->comm));
        send_to_char(buf, ch);
    }

    if (victim->is_npc() && victim->off_flags) {
        bug_snprintf(buf, sizeof(buf), "Offense: %s\n\r", (char *)off_bit_name(victim->off_flags));
        send_to_char(buf, ch);
    }

    if (victim->imm_flags) {
        bug_snprintf(buf, sizeof(buf), "Immune: %s\n\r", (char *)imm_bit_name(victim->imm_flags));
        send_to_char(buf, ch);
    }

    if (victim->res_flags) {
        bug_snprintf(buf, sizeof(buf), "Resist: %s\n\r", (char *)imm_bit_name(victim->res_flags));
        send_to_char(buf, ch);
    }

    if (victim->vuln_flags) {
        bug_snprintf(buf, sizeof(buf), "Vulnerable: %s\n\r", (char *)imm_bit_name(victim->vuln_flags));
        send_to_char(buf, ch);
    }

    bug_snprintf(buf, sizeof(buf), "Form: %s\n\rParts: %s\n\r", form_bit_name(victim->form),
                 (char *)part_bit_name(victim->parts));
    send_to_char(buf, ch);

    if (victim->affected_by) {
        bug_snprintf(buf, sizeof(buf), "Affected by %s\n\r", (char *)affect_bit_name(victim->affected_by));
        send_to_char(buf, ch);
    }

    bug_snprintf(buf, sizeof(buf), "Master: %s  Leader: %s  Pet: %s\n\r",
                 victim->master ? victim->master->name : "(none)", victim->leader ? victim->leader->name : "(none)",
                 victim->pet ? victim->pet->name : "(none)");
    send_to_char(buf, ch);

    bug_snprintf(buf, sizeof(buf), "Riding: %s  Ridden by: %s\n\r", victim->riding ? victim->riding->name : "(none)",
                 victim->ridden_by ? victim->ridden_by->name : "(none)");
    send_to_char(buf, ch);

    ch->send_to("Short description: {}\n\rLong  description: {}"_format(
        victim->short_descr, victim->long_descr.empty() ? "(none)\n\r" : victim->long_descr));

    if (victim->is_npc() && victim->spec_fun)
        send_to_char("Mobile has special procedure.\n\r", ch);

    if (victim->is_npc() && victim->pIndexData->progtypes) {
        bug_snprintf(buf, sizeof(buf), "Mobile has MOBPROG: view with \"stat prog '%s'\"\n\r", victim->name);
        send_to_char(buf, ch);
    }

    for (paf = victim->affected; paf != nullptr; paf = paf->next) {
        bug_snprintf(buf, sizeof(buf), "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
                     (skill_table[(int)paf->type]).name, affect_loc_name(paf->location), paf->modifier, paf->duration,
                     affect_bit_name(paf->bitvector), paf->level);
        send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    const char *string;

    string = one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  vnum obj <name>\n\r", ch);
        send_to_char("  vnum mob <name>\n\r", ch);
        send_to_char("  vnum skill <skill or spell>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "obj")) {
        do_ofind(ch, string);
        return;
    }

    if (!str_cmp(arg, "mob") || !str_cmp(arg, "char")) {
        do_mfind(ch, string);
        return;
    }

    if (!str_cmp(arg, "skill") || !str_cmp(arg, "spell")) {
        do_slookup(ch, string);
        return;
    }
    /* do both */
    do_mfind(ch, argument);
    do_ofind(ch, argument);
}

void do_mfind(CHAR_DATA *ch, const char *argument) {
    extern int top_mob_index;
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;
    BUFFER *buffer;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Find whom?\n\r", ch);
        return;
    }

    fAll = false; /* !str_cmp( arg, "all" ); */
    found = false;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    buffer = buffer_create();
    for (vnum = 0; nMatch < top_mob_index; vnum++) {
        if ((pMobIndex = get_mob_index(vnum)) != nullptr) {
            nMatch++;
            if (fAll || is_name(argument, pMobIndex->player_name)) {
                found = true;
                buffer_addline_fmt(buffer, "[%5d] %s\n\r", pMobIndex->vnum, pMobIndex->short_descr);
            }
        }
    }

    buffer_send(buffer, ch); /* frees buffer */
    if (!found)
        send_to_char("No mobiles by that name.\n\r", ch);
}

void do_ofind(CHAR_DATA *ch, const char *argument) {
    extern int top_obj_index;
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;
    BUFFER *buffer;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Find what?\n\r", ch);
        return;
    }

    fAll = false; /* !str_cmp( arg, "all" ); */
    found = false;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    buffer = buffer_create();
    for (vnum = 0; nMatch < top_obj_index; vnum++) {
        if ((pObjIndex = get_obj_index(vnum)) != nullptr) {
            nMatch++;
            if (fAll || is_name(argument, pObjIndex->name)) {
                found = true;
                buffer_addline_fmt(buffer, "[%5d] %s\n\r", pObjIndex->vnum, pObjIndex->short_descr);
            }
        }
    }

    buffer_send(buffer, ch); /* NB this frees the buffer */
    if (!found)
        send_to_char("No objects by that name.\n\r", ch);
}

void do_mwhere(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *victim;
    bool found;
    bool findPC = false;
    int number = 0;
    BUFFER *buffer;

    if (argument[0] == '\0') {
        findPC = true;
    } else if (strlen(argument) < 2) {
        send_to_char("Please be more specific.\n\r", ch);
        return;
    }

    found = false;
    number = 0;
    buffer = buffer_create();

    for (victim = char_list; victim != nullptr; victim = victim->next) {
        if ((victim->is_npc() && victim->in_room != nullptr && is_name(argument, victim->name) && findPC == false)
            || (victim->is_pc() && (findPC == true) && can_see(ch, victim))) {
            found = true;
            number++;
            buffer_addline_fmt(
                buffer, "%3d [%5d] %-28.28s [%5d] %20.20s\n\r", number, (findPC == true) ? 0 : victim->pIndexData->vnum,
                (findPC == true) ? victim->name : victim->short_descr, victim->in_room->vnum, victim->in_room->name);
        }
    }
    buffer_send(buffer, ch);

    if (!found)
        act("You didn't find any $T.", ch, nullptr, argument, To::Char);
}

void do_reboo(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    send_to_char("If you want to REBOOT, spell it out.\n\r", ch);
}

void do_reboot(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;

    if (!IS_SET(ch->act, PLR_WIZINVIS)) {
        bug_snprintf(buf, sizeof(buf), "Reboot by %s.", ch->name);
        do_echo(ch, buf);
    }
    do_force(ch, "all save");
    do_save(ch, "");
    merc_down = true;
    // Unlike do_shutdown(), do_reboot() does not explicitly call close_socket()
    // for every connected player. That's because close_socket() actually
    // sends a PACKET_DISCONNECT to doorman, causing the client to get booted and
    // that's not the desired behaviour. Instead, setting the merc_down flag will
    // cause the main process to terminate while doorman holds onto the client's
    // connection and then reconnect them to the mud once it's back up.
}

void do_shutdow(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    send_to_char("If you want to SHUTDOWN, spell it out.\n\r", ch);
}

void do_shutdown(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;

    bug_snprintf(buf, sizeof(buf), "Shutdown by %s.", ch->name);
    append_file(ch, SHUTDOWN_FILE, buf);

    strcat(buf, "\n\r");
    do_echo(ch, buf);
    do_force(ch, "all save");
    do_save(ch, "");
    merc_down = true;
    for (auto &d : descriptors().all())
        d.close();
}

void do_snoop(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (ch->desc == nullptr) {
        // MRG old code was split-brained about checking this. Seems like nothing would have worked if ch->desc was
        // actually null, so bugging out here.
        bug("null ch->desc in do_snoop");
        return;
    }

    if (arg[0] == '\0') {
        send_to_char("Snoop whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (!victim->desc) {
        send_to_char("No descriptor to snoop.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("Cancelling all snoops.\n\r", ch);
        ch->desc->stop_snooping();
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (!ch->desc->try_start_snooping(*victim->desc)) {
        send_to_char("No snoop loops.\n\r", ch);
        return;
    }

    send_to_char("Ok.\n\r", ch);
}

void do_switch(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Switch into whom?\n\r", ch);
        return;
    }

    if (ch->desc == nullptr)
        return;

    if (ch->desc->is_switched()) {
        send_to_char("You are already switched.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (victim->is_pc()) {
        send_to_char("You can only switch into mobiles.\n\r", ch);
        return;
    }

    if (victim->desc != nullptr) {
        send_to_char("Character in use.\n\r", ch);
        return;
    }

    ch->desc->do_switch(victim);
    /* change communications to match */
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    send_to_char("Ok.\n\r", victim);
}

void do_return(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->desc == nullptr)
        return;

    if (!ch->desc->is_switched()) {
        send_to_char("You aren't switched.\n\r", ch);
        return;
    }

    send_to_char("You return to your original body.\n\r", ch);
    ch->desc->do_return();
}

/* trust levels for load and clone */
/* cut out by Faramir but func retained in case of any
   calls I don't know about. */
bool obj_check(CHAR_DATA *ch, OBJ_DATA *obj) {

    if (obj->level > ch->get_trust())
        return false;
    return true;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone) {
    OBJ_DATA *c_obj, *t_obj;

    for (c_obj = obj->contains; c_obj != nullptr; c_obj = c_obj->next_content) {
        if (obj_check(ch, c_obj)) {
            t_obj = create_object(c_obj->pIndexData);
            clone_object(c_obj, t_obj);
            obj_to_obj(t_obj, clone);
            recursive_clone(ch, c_obj, t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    const char *rest;
    CHAR_DATA *mob;
    OBJ_DATA *obj;

    rest = one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Clone what?\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "object")) {
        mob = nullptr;
        obj = get_obj_here(ch, rest);
        if (obj == nullptr) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    } else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
        obj = nullptr;
        mob = get_char_room(ch, rest);
        if (mob == nullptr) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    } else /* find both */
    {
        mob = get_char_room(ch, argument);
        obj = get_obj_here(ch, argument);
        if (mob == nullptr && obj == nullptr) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }

    /* clone an object */
    if (obj != nullptr) {
        OBJ_DATA *clone;

        if (!obj_check(ch, obj)) {
            send_to_char("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_object(obj->pIndexData);
        clone_object(obj, clone);
        if (obj->carried_by != nullptr)
            obj_to_char(clone, ch);
        else
            obj_to_room(clone, ch->in_room);
        recursive_clone(ch, obj, clone);

        act("$n has created $p.", ch, clone, nullptr, To::Room);
        act("You clone $p.", ch, clone, nullptr, To::Char);
        return;
    } else if (mob != nullptr) {
        CHAR_DATA *clone;
        OBJ_DATA *new_obj;

        if (mob->is_pc()) {
            send_to_char("You can only clone mobiles.\n\r", ch);
            return;
        }

        if ((mob->level > 20 && ch->get_trust() < GOD) || (mob->level > 10 && ch->get_trust() < IMMORTAL)
            || (mob->level > 5 && ch->get_trust() < DEMI) || (mob->level > 0 && ch->get_trust() < ANGEL)
            || ch->get_trust() < AVATAR) {
            send_to_char("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_mobile(mob->pIndexData);
        clone_mobile(mob, clone);

        for (obj = mob->carrying; obj != nullptr; obj = obj->next_content) {
            if (obj_check(ch, obj)) {
                new_obj = create_object(obj->pIndexData);
                clone_object(obj, new_obj);
                recursive_clone(ch, obj, new_obj);
                obj_to_char(new_obj, clone);
                new_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room(clone, ch->in_room);
        act("$n has created $N.", ch, nullptr, clone, To::Room);
        act("You clone $N.", ch, nullptr, clone, To::Char);
        return;
    }
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  load mob <vnum>\n\r", ch);
        send_to_char("  load obj <vnum> <level>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "mob") || !str_cmp(arg, "char")) {
        do_mload(ch, argument);
        return;
    }

    if (!str_cmp(arg, "obj")) {
        do_oload(ch, argument);
        return;
    }
    /* echo syntax */
    do_load(ch, "");
}

void do_mload(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char("Syntax: load mob <vnum>.\n\r", ch);
        return;
    }

    if ((pMobIndex = get_mob_index(atoi(arg))) == nullptr) {
        send_to_char("No mob has that vnum.\n\r", ch);
        return;
    }

    victim = create_mobile(pMobIndex);
    char_to_room(victim, ch->in_room);
    act("$n has created $N!", ch, nullptr, victim, To::Room);
    send_to_char("Ok.\n\r", ch);
}

void do_oload(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        send_to_char("Syntax: load obj <vnum>.\n\r", ch);
        return;
    }

    if ((pObjIndex = get_obj_index(atoi(arg1))) == nullptr) {
        send_to_char("No object has that vnum.\n\r", ch);
        return;
    }

    obj = create_object(pObjIndex);
    if (CAN_WEAR(obj, ITEM_TAKE))
        obj_to_char(obj, ch);
    else
        obj_to_room(obj, ch->in_room);
    act("$n has created $p!", ch, obj, nullptr, To::Room);
    send_to_char("Ok.\n\r", ch);
}

void do_purge(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    Descriptor *d;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA *obj_next;

        for (victim = ch->in_room->people; victim != nullptr; victim = vnext) {
            vnext = victim->next_in_room;
            if (victim->is_npc() && !IS_SET(victim->act, ACT_NOPURGE) && victim != ch /* safety precaution */)
                extract_char(victim, true);
        }

        for (obj = ch->in_room->contents; obj != nullptr; obj = obj_next) {
            obj_next = obj->next_content;
            if (!IS_OBJ_STAT(obj, ITEM_NOPURGE))
                extract_obj(obj);
        }

        act("$n purges the room!", ch);
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_pc()) {

        if (ch == victim) {
            send_to_char("Ho ho ho.\n\r", ch);
            return;
        }

        if (ch->get_trust() <= victim->get_trust()) {
            send_to_char("Maybe that wasn't a good idea...\n\r", ch);
            bug_snprintf(buf, sizeof(buf), "%s tried to purge you!\n\r", ch->name);
            send_to_char(buf, victim);
            return;
        }

        act("$n disintegrates $N.", ch, nullptr, victim, To::NotVict);

        if (victim->level > 1)
            save_char_obj(victim);
        d = victim->desc;
        extract_char(victim, true);
        if (d != nullptr)
            d->close();

        return;
    }

    act("$n purges $N.", ch, nullptr, victim, To::NotVict);
    extract_char(victim, true);
}

void do_advance(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;
    int iLevel;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
        send_to_char("Syntax: advance <char> <level>.\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg1)) == nullptr) {
        send_to_char("That player is not here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if ((level = atoi(arg2)) < 1 || level > MAX_LEVEL) {
        char buf[32];
        bug_snprintf(buf, sizeof(buf), "Level must be 1 to %d.\n\r", MAX_LEVEL);

        send_to_char(buf, ch);
        return;
    }

    if (level > ch->get_trust()) {
        send_to_char("Limited to your trust level.\n\r", ch);
        return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if (level <= victim->level && (ch->level > victim->level)) {
        int temp_prac;

        send_to_char("Lowering a player's level!\n\r", ch);
        send_to_char("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim);
        temp_prac = victim->practice;
        victim->level = 1;
        victim->exp = exp_per_level(victim, victim->pcdata->points);
        victim->max_hit = 10;
        victim->max_mana = 100;
        victim->max_move = 100;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        advance_level(victim);
        victim->practice = temp_prac;
    } else {
        send_to_char("Raising a player's level!\n\r", ch);
        send_to_char("|R**** OOOOHHHHHHHHHH  YYYYEEEESSS ****|w\n\r", victim);
    }

    if (ch->level > victim->level) {
        for (iLevel = victim->level; iLevel < level; iLevel++) {
            send_to_char("You raise a level!!  ", victim);
            victim->level += 1;
            advance_level(victim);
        }
        victim->exp = (exp_per_level(victim, victim->pcdata->points) * UMAX(1, victim->level));
        victim->trust = 0;
        save_char_obj(victim);
    }
}

void do_trust(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((arg1[0] == '\0' && arg2[0] == '\0') || (arg2[0] != '\0' && !is_number(arg2))) {
        send_to_char("Syntax: trust <char> <level>.\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg1)) == nullptr) {
        send_to_char("That player is not here.\n\r", ch);
        return;
    }

    if (arg2[0] == '\0') {
        bug_snprintf(buf, sizeof(buf), "%s has a trust of %d.\n\r", victim->name, victim->trust);
        send_to_char(buf, ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPCs.\n\r", ch);
        return;
    }

    if ((level = atoi(arg2)) < 0 || level > 100) {
        send_to_char("Level must be 0 (reset) or 1 to 100.\n\r", ch);
        return;
    }

    if (level > ch->get_trust()) {
        send_to_char("Limited to your trust.\n\r", ch);
        return;
    }

    victim->trust = level;
}

void do_restore(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;

    one_argument(argument, arg);
    if (arg[0] == '\0' || !str_cmp(arg, "room")) {
        /* cure room */

        for (vch = ch->in_room->people; vch != nullptr; vch = vch->next_in_room) {
            affect_strip(vch, gsn_plague);
            affect_strip(vch, gsn_poison);
            affect_strip(vch, gsn_blindness);
            affect_strip(vch, gsn_sleep);
            affect_strip(vch, gsn_curse);

            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            update_pos(vch);
            act("$n has restored you.", ch, nullptr, vch, To::Vict);
        }

        send_to_char("Room restored.\n\r", ch);
        return;
    }

    if (ch->get_trust() >= MAX_LEVEL && !str_cmp(arg, "all")) {
        /* cure all */

        for (auto &d : descriptors().playing()) {
            victim = d.character();

            if (victim->is_npc())
                continue;

            affect_strip(victim, gsn_plague);
            affect_strip(victim, gsn_poison);
            affect_strip(victim, gsn_blindness);
            affect_strip(victim, gsn_sleep);
            affect_strip(victim, gsn_curse);

            victim->hit = victim->max_hit;
            victim->mana = victim->max_mana;
            victim->move = victim->max_move;
            update_pos(victim);
            if (victim->in_room != nullptr)
                act("$n has restored you.", ch, nullptr, victim, To::Vict);
        }
        send_to_char("All active players restored.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    affect_strip(victim, gsn_plague);
    affect_strip(victim, gsn_poison);
    affect_strip(victim, gsn_blindness);
    affect_strip(victim, gsn_sleep);
    affect_strip(victim, gsn_curse);
    victim->hit = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos(victim);
    act("$n has restored you.", ch, nullptr, victim, To::Vict);
    send_to_char("Ok.\n\r", ch);
}

void do_freeze(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Freeze whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->act, PLR_FREEZE)) {
        REMOVE_BIT(victim->act, PLR_FREEZE);
        send_to_char("You can play again.\n\r", victim);
        send_to_char("FREEZE removed.\n\r", ch);
    } else {
        SET_BIT(victim->act, PLR_FREEZE);
        send_to_char("You can't do ANYthing!\n\r", victim);
        send_to_char("FREEZE set.\n\r", ch);
    }

    save_char_obj(victim);
}

void do_log(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Log whom?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        if (fLogAll) {
            fLogAll = false;
            send_to_char("Log ALL off.\n\r", ch);
        } else {
            fLogAll = true;
            send_to_char("Log ALL on.\n\r", ch);
        }
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (IS_SET(victim->act, PLR_LOG)) {
        REMOVE_BIT(victim->act, PLR_LOG);
        send_to_char("LOG removed.\n\r", ch);
    } else {
        SET_BIT(victim->act, PLR_LOG);
        send_to_char("LOG set.\n\r", ch);
    }
}

void do_noemote(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Noemote whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOEMOTE)) {
        REMOVE_BIT(victim->comm, COMM_NOEMOTE);
        send_to_char("You can emote again.\n\r", victim);
        send_to_char("NOEMOTE removed.\n\r", ch);
    } else {
        SET_BIT(victim->comm, COMM_NOEMOTE);
        send_to_char("You can't emote!\n\r", victim);
        send_to_char("NOEMOTE set.\n\r", ch);
    }
}

void do_noshout(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Noshout whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOSHOUT)) {
        REMOVE_BIT(victim->comm, COMM_NOSHOUT);
        send_to_char("You can shout again.\n\r", victim);
        send_to_char("NOSHOUT removed.\n\r", ch);
    } else {
        SET_BIT(victim->comm, COMM_NOSHOUT);
        send_to_char("You can't shout!\n\r", victim);
        send_to_char("NOSHOUT set.\n\r", ch);
    }
}

void do_notell(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Notell whom?", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOTELL)) {
        REMOVE_BIT(victim->comm, COMM_NOTELL);
        send_to_char("You can tell again.\n\r", victim);
        send_to_char("NOTELL removed.\n\r", ch);
    } else {
        SET_BIT(victim->comm, COMM_NOTELL);
        send_to_char("You can't tell!\n\r", victim);
        send_to_char("NOTELL set.\n\r", ch);
    }
}

void do_peace(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    for (auto *rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (rch->fighting)
            stop_fighting(rch, true);
        if (rch->is_npc() && IS_SET(rch->act, ACT_AGGRESSIVE))
            REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
        if (rch->is_npc())
            rch->sentient_victim.clear();
    }

    ch->send_to("Ok.\n\r");
}

void do_awaken(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Awaken whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }
    if (IS_AWAKE(victim)) {
        send_to_char("Duh!  They're not even asleep!\n\r", ch);
        return;
    }

    if (ch == victim) {
        send_to_char("Duh!  If you wanna wake up, get COFFEE!\n\r", ch);
        return;
    }

    REMOVE_BIT(victim->affected_by, AFF_SLEEP);
    victim->position = POS_STANDING;

    act("$n gives $t a kick, and wakes them up.", ch, victim->short_descr, nullptr, To::Room, POS_RESTING);
}

void do_owhere(CHAR_DATA *ch, const char *argument) {
    char target_name[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0;

    found = false;
    number = 0;

    if (argument[0] == '\0') {
        send_to_char("Owhere which object?\n\r", ch);
        return;
    }
    if (strlen(argument) < 2) {
        send_to_char("Please be more specific.\n\r", ch);
        return;
    }
    one_argument(argument, target_name);

    std::string buffer;
    for (obj = object_list; obj != nullptr; obj = obj->next) {
        if (!is_name(target_name, obj->name))
            continue;

        found = true;
        number++;

        for (in_obj = obj; in_obj->in_obj != nullptr; in_obj = in_obj->in_obj)
            ;

        if (in_obj->carried_by != nullptr) {
            buffer += "{:3} {:<25} carried by {:<20} in room {}\n\r"_format(
                number, obj->short_descr, pers(in_obj->carried_by, ch), in_obj->carried_by->in_room->vnum);
        } else if (in_obj->in_room != nullptr) {
            buffer += "{:3} {:<25} in {:<30} [{}]\n\r"_format(number, obj->short_descr, in_obj->in_room->name,
                                                              in_obj->in_room->vnum);
        }
    }

    ch->page_to(buffer);
    if (!found)
        send_to_char("Nothing like that in heaven or earth.\n\r", ch);
}

/* Death's command */
void do_coma(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA af;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Comatoze whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLEEP))
        return;

    if (ch == victim) {
        send_to_char("Duh!  Don't you dare fall asleep on the job!\n\r", ch);
        return;
    }
    if ((ch->get_trust() <= victim->get_trust()) || !((ch->is_immortal()) && victim->is_npc())) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    af.type = 38; /* SLEEP */
    af.level = ch->trust;
    af.duration = 4 + ch->trust;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, &af);

    if (IS_AWAKE(victim)) {
        send_to_char("You feel very sleepy ..... zzzzzz.\n\r", victim);
        act("$n goes to sleep.", victim);
        victim->position = POS_SLEEPING;
    }
}

bool osearch_is_valid_level_range(int min_level, int max_level) {
    if (min_level > MAX_LEVEL || max_level > MAX_LEVEL || min_level > max_level || max_level - min_level > 10) {
        return false;
    } else {
        return true;
    }
}

char *osearch_list_item_types(char *buf) {
    buf[0] = '\0';
    for (int type = 0; item_table[type].name != nullptr;) {
        strcat(buf, item_table[type].name);
        strcat(buf, " ");
        if (++type % 7 == 0) {
            strcat(buf, "\n\r        ");
        }
    }
    strcat(buf, "\n\r");
    return buf;
}

void osearch_display_syntax(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];
    send_to_char("Syntax: osearch [min level] [max level] [item type] optional item name...\n\r", ch);
    send_to_char("        Level range no greater than 10. Item types:\n\r        ", ch);
    send_to_char(osearch_list_item_types(buf), ch);
}

bool osearch_is_item_in_level_range(const OBJ_INDEX_DATA *pIndexData, const int min_level, const int max_level) {
    if (pIndexData == nullptr) {
        return false;
    }
    return pIndexData->level >= min_level && pIndexData->level <= max_level;
}

bool osearch_is_item_type(const OBJ_INDEX_DATA *pIndexData, const sh_int item_type) {
    if (pIndexData == nullptr) {
        return false;
    }
    return pIndexData->item_type == item_type;
}

/**
 * Searches the item database for items of the specified type, level range and
 * optional name. Adds their item vnum, name, level and area name to a new buffer.
 * Caller must release the buffer!
 */
BUFFER *osearch_find_items(const int min_level, const int max_level, const sh_int item_type, char *item_name) {
    BUFFER *buffer = buffer_create();
    for (int i = 0; i < MAX_KEY_HASH; i++) {
        for (OBJ_INDEX_DATA *pIndexData = obj_index_hash[i]; pIndexData != nullptr; pIndexData = pIndexData->next) {
            if (!(osearch_is_item_in_level_range(pIndexData, min_level, max_level)
                  && osearch_is_item_type(pIndexData, item_type))) {
                continue;
            }
            if (item_name[0] != '\0' && !is_name(item_name, pIndexData->name)) {
                continue;
            }
            buffer_addline_fmt(buffer, "%5d %-25.25s|w (%3d) %s\n\r", pIndexData->vnum, pIndexData->short_descr,
                               pIndexData->level, pIndexData->area->filename);
        }
    }
    return buffer;
}

/**
 * osearch: an item search tool for immortals.
 * It searches the item database not the object instances in world!
 * You must provide min/max level range and an item type name.
 * You can optionally filter on the leading part of the object name.
 * As some items are level 0, specifing min level 0 is allowed.
 */
void do_osearch(CHAR_DATA *ch, const char *argument) {
    char min_level_str[MAX_INPUT_LENGTH];
    char max_level_str[MAX_INPUT_LENGTH];
    char item_type_str[MAX_INPUT_LENGTH];
    char item_name[MAX_INPUT_LENGTH];
    int min_level;
    int max_level;
    sh_int item_type;
    BUFFER *buffer;
    if (argument[0] == '\0') {
        osearch_display_syntax(ch);
        return;
    }
    argument = one_argument(argument, min_level_str);
    argument = one_argument(argument, max_level_str);
    argument = one_argument(argument, item_type_str);
    argument = one_argument(argument, item_name);
    if (min_level_str[0] == '\0' || max_level_str[0] == '\0' || item_type_str[0] == '\0' || !is_number(min_level_str)
        || !is_number(max_level_str)) {
        osearch_display_syntax(ch);
        return;
    }
    min_level = atoi(min_level_str);
    max_level = atoi(max_level_str);
    if (!osearch_is_valid_level_range(min_level, max_level)) {
        osearch_display_syntax(ch);
        return;
    }
    item_type = item_lookup_strict(item_type_str);
    if (item_type < 0) {
        osearch_display_syntax(ch);
        return;
    }
    buffer = osearch_find_items(min_level, max_level, item_type, item_name);
    buffer_send(buffer, ch);
}

void do_slookup(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Lookup which skill or spell?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == nullptr)
                break;
            bug_snprintf(buf, sizeof(buf), "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn, skill_table[sn].slot,
                         skill_table[sn].name);
            send_to_char(buf, ch);
        }
    } else {
        if ((sn = skill_lookup(arg)) < 0) {
            send_to_char("No such skill or spell.\n\r", ch);
            return;
        }

        bug_snprintf(buf, sizeof(buf), "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn, skill_table[sn].slot,
                     skill_table[sn].name);
        send_to_char(buf, ch);
    }
}

/* RT set replaces sset, mset, oset, and rset */

void do_set(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set mob   <name> <field> <value>\n\r", ch);
        send_to_char("  set obj   <name> <field> <value>\n\r", ch);
        send_to_char("  set room  <room> <field> <value>\n\r", ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
        do_mset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "skill") || !str_prefix(arg, "spell")) {
        do_sset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "object")) {
        do_oset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "room")) {
        do_rset(ch, argument);
        return;
    }
    /* echo syntax */
    do_set(ch, "");
}

void do_sset(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r", ch);
        send_to_char("  set skill <name> all <value>\n\r", ch);
        send_to_char("   (use the name of the skill, not the number)\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->is_npc()) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    fAll = !str_cmp(arg2, "all");
    sn = 0;
    if (!fAll && (sn = skill_lookup(arg2)) < 0) {
        send_to_char("No such skill or spell.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3)) {
        send_to_char("Value must be numeric.\n\r", ch);
        return;
    }

    value = atoi(arg3);
    if (value < 0 || value > 100) {
        send_to_char("Value range is 0 to 100.\n\r", ch);
        return;
    }

    if (fAll) {
        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != nullptr)
                victim->pcdata->learned[sn] = value;
        }
    } else {
        victim->pcdata->learned[sn] = value;
    }
}

void do_mset(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set char <name> <field> <value>\n\r", ch);
        send_to_char("  Field being one of:\n\r", ch);
        send_to_char("    str int wis dex con sex class level\n\r", ch);
        send_to_char("    race gold hp mana move practice align\n\r", ch);
        send_to_char("    dam hit train thirst drunk full hours\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number(arg3) ? atoi(arg3) : -1;

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "str")) {
        if (value < 3 || value > get_max_train(victim, Stat::Str)) {
            bug_snprintf(buf, sizeof(buf), "Strength range is 3 to %d\n\r.", get_max_train(victim, Stat::Str));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[Stat::Str] = value;
        return;
    }

    if (!str_cmp(arg2, "int")) {
        if (value < 3 || value > get_max_train(victim, Stat::Int)) {
            bug_snprintf(buf, sizeof(buf), "Intelligence range is 3 to %d.\n\r", get_max_train(victim, Stat::Int));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[Stat::Int] = value;
        return;
    }

    if (!str_cmp(arg2, "wis")) {
        if (value < 3 || value > get_max_train(victim, Stat::Wis)) {
            bug_snprintf(buf, sizeof(buf), "Wisdom range is 3 to %d.\n\r", get_max_train(victim, Stat::Wis));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[Stat::Wis] = value;
        return;
    }

    if (!str_cmp(arg2, "dex")) {
        if (value < 3 || value > get_max_train(victim, Stat::Dex)) {
            bug_snprintf(buf, sizeof(buf), "Dexterity ranges is 3 to %d.\n\r", get_max_train(victim, Stat::Dex));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[Stat::Dex] = value;
        return;
    }

    if (!str_cmp(arg2, "con")) {
        if (value < 3 || value > get_max_train(victim, Stat::Con)) {
            bug_snprintf(buf, sizeof(buf), "Constitution range is 3 to %d.\n\r", get_max_train(victim, Stat::Con));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[Stat::Con] = value;
        return;
    }

    if (!str_prefix(arg2, "sex")) {
        if (value < 0 || value > 2) {
            send_to_char("Sex range is 0 to 2.\n\r", ch);
            return;
        }
        victim->sex = value;
        if (victim->is_pc())
            victim->pcdata->true_sex = value;
        return;
    }

    if (!str_prefix(arg2, "class")) {
        int class_num;

        if (victim->is_npc()) {
            send_to_char("Mobiles have no class.\n\r", ch);
            return;
        }

        class_num = class_lookup(arg3);
        if (class_num == -1) {
            char buf[MAX_STRING_LENGTH];

            strcpy(buf, "Possible classes are: ");
            for (class_num = 0; class_num < MAX_CLASS; class_num++) {
                if (class_num > 0)
                    strcat(buf, " ");
                strcat(buf, class_table[class_num].name);
            }
            strcat(buf, ".\n\r");

            send_to_char(buf, ch);
            return;
        }

        victim->class_num = class_num;
        return;
    }

    if (!str_prefix(arg2, "level")) {
        if (victim->is_pc()) {
            send_to_char("Not on PC's.\n\r", ch);
            return;
        }

        if (value < 0 || value > 200) {
            send_to_char("Level range is 0 to 200.\n\r", ch);
            return;
        }
        victim->level = value;
        return;
    }

    if (!str_prefix(arg2, "gold")) {
        victim->gold = value;
        return;
    }

    if (!str_prefix(arg2, "hp")) {
        if (value < 1 || value > 30000) {
            send_to_char("Hp range is 1 to 30,000 hit points.\n\r", ch);
            return;
        }
        victim->max_hit = value;
        if (victim->is_pc())
            victim->pcdata->perm_hit = value;
        return;
    }

    if (!str_prefix(arg2, "mana")) {
        if (value < 0 || value > 30000) {
            send_to_char("Mana range is 0 to 30,000 mana points.\n\r", ch);
            return;
        }
        victim->max_mana = value;
        if (victim->is_pc())
            victim->pcdata->perm_mana = value;
        return;
    }

    if (!str_prefix(arg2, "move")) {
        if (value < 0 || value > 30000) {
            send_to_char("Move range is 0 to 30,000 move points.\n\r", ch);
            return;
        }
        victim->max_move = value;
        if (victim->is_pc())
            victim->pcdata->perm_move = value;
        return;
    }

    if (!str_prefix(arg2, "practice")) {
        if (value < 0 || value > 250) {
            send_to_char("Practice range is 0 to 250 sessions.\n\r", ch);
            return;
        }
        victim->practice = value;
        return;
    }

    if (!str_prefix(arg2, "train")) {
        if (value < 0 || value > 50) {
            send_to_char("Training session range is 0 to 50 sessions.\n\r", ch);
            return;
        }
        victim->train = value;
        return;
    }

    if (!str_prefix(arg2, "align")) {
        if (value < -1000 || value > 1000) {
            send_to_char("Alignment range is -1000 to 1000.\n\r", ch);
            return;
        }
        victim->alignment = value;
        return;
    }

    if (!str_cmp(arg2, "dam")) {
        if (value < 1 || value > 100) {
            bug_snprintf(buf, sizeof(buf), "|RDamroll range is 1 to 100.|w\n\r");
            send_to_char(buf, ch);
            return;
        }

        victim->damroll = value;
        return;
    }

    if (!str_cmp(arg2, "hit")) {
        if (value < 1 || value > 100) {
            bug_snprintf(buf, sizeof(buf), "|RHitroll range is 1 to 100.|w\n\r");
            send_to_char(buf, ch);
            return;
        }

        victim->hitroll = value;
        return;
    }

    if (!str_prefix(arg2, "thirst")) {
        if (victim->is_npc()) {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100) {
            send_to_char("Thirst range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        return;
    }

    if (!str_prefix(arg2, "drunk")) {
        if (victim->is_npc()) {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100) {
            send_to_char("Drunk range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if (!str_prefix(arg2, "full")) {
        if (victim->is_npc()) {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100) {
            send_to_char("Full range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        return;
    }

    if (!str_prefix(arg2, "race")) {
        int race;

        race = race_lookup(arg3);

        if (race == 0) {
            send_to_char("That is not a valid race.\n\r", ch);
            return;
        }

        if (victim->is_pc() && !race_table[race].pc_race) {
            send_to_char("That is not a valid player race.\n\r", ch);
            return;
        }

        victim->race = race;
        return;
    }

    if (!str_cmp(arg2, "hours")) {
        if (value < 1 || value > 999) {
            bug_snprintf(buf, sizeof(buf), "|RHours range is 1 to 999.|w\n\r");
            send_to_char(buf, ch);
            return;
        }

        victim->played = std::chrono::hours(value);
        return;
    }

    /*
     * Generate usage message.
     */
    do_mset(ch, "");
}

void do_string(CHAR_DATA *ch, const char *argument) {
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, type);
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  string char <name> <field> <string>\n\r", ch);
        send_to_char("    fields: name short long desc title spec\n\r", ch);
        send_to_char("  string obj  <name> <field> <string>\n\r", ch);
        send_to_char("    fields: name short long extended wear\n\r", ch);
        return;
    }

    if (!str_prefix(type, "character") || !str_prefix(type, "mobile")) {
        if ((victim = get_char_world(ch, arg1)) == nullptr) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        /* string something */

        if (!str_prefix(arg2, "name")) {
            if (victim->is_pc()) {
                send_to_char("Not on PC's.\n\r", ch);
                return;
            }

            free_string(victim->name);
            victim->name = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "description")) {
            victim->description = arg3;
            return;
        }

        if (!str_prefix(arg2, "short")) {
            free_string(victim->short_descr);
            victim->short_descr = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "long")) {
            victim->long_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "title")) {
            if (victim->is_npc()) {
                send_to_char("Not on NPC's.\n\r", ch);
                return;
            }

            victim->set_title(arg3);
            return;
        }

        if (!str_prefix(arg2, "spec")) {
            if (victim->is_pc()) {
                send_to_char("Not on PC's.\n\r", ch);
                return;
            }

            if ((victim->spec_fun = spec_lookup(arg3)) == 0) {
                send_to_char("No such spec fun.\n\r", ch);
                return;
            }

            return;
        }
    }

    if (!str_prefix(type, "object")) {
        /* string an obj */

        if ((obj = get_obj_world(ch, arg1)) == nullptr) {
            send_to_char("Nothing like that in heaven or earth.\n\r", ch);
            return;
        }

        if (!str_prefix(arg2, "name")) {
            free_string(obj->name);
            obj->name = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "short")) {
            free_string(obj->short_descr);
            obj->short_descr = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "long")) {
            free_string(obj->description);
            obj->description = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "wear")) {
            if (strlen(arg3) > 17) {
                send_to_char("Wear_Strings may not be longer than 17 chars.\n\r", ch);
            } else {
                free_string(obj->wear_string);
                obj->wear_string = str_dup(arg3);
            }
            return;
        }

        if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended")) {
            EXTRA_DESCR_DATA *ed;

            args = one_argument(args, arg3);
            if (args == nullptr) {
                send_to_char("Syntax: oset <object> ed <keyword> <string>\n\r", ch);
                return;
            }

            strcat(args, "\n\r");

            if (extra_descr_free == nullptr) {
                ed = static_cast<EXTRA_DESCR_DATA *>(alloc_perm(sizeof(*ed)));
            } else {
                ed = extra_descr_free;
                extra_descr_free = ed->next;
            }

            ed->keyword = str_dup(arg3);
            ed->description = str_dup(args);
            ed->next = obj->extra_descr;
            obj->extra_descr = ed;
            return;
        }
    }

    /* echo bad use message */
    do_string(ch, "");
}

void do_oset(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set obj <object> <field> <value>\n\r", ch);
        send_to_char("  Field being one of:\n\r", ch);
        send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r", ch);
        send_to_char("    extra wear level weight cost timer nolocate\n\r", ch);
        send_to_char("    (for extra and wear, use set obj <o> <extra/wear> ? to list\n\r", ch);
        return;
    }

    if ((obj = get_obj_world(ch, arg1)) == nullptr) {
        send_to_char("Nothing like that in heaven or earth.\n\r", ch);
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi(arg3);

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "value0") || !str_cmp(arg2, "v0")) {
        obj->value[0] = UMIN(75, value);
        return;
    }

    if (!str_cmp(arg2, "value1") || !str_cmp(arg2, "v1")) {
        obj->value[1] = value;
        return;
    }

    if (!str_cmp(arg2, "value2") || !str_cmp(arg2, "v2")) {
        obj->value[2] = value;
        return;
    }

    if (!str_cmp(arg2, "value3") || !str_cmp(arg2, "v3")) {
        obj->value[3] = value;
        return;
    }

    if (!str_cmp(arg2, "value4") || !str_cmp(arg2, "v4")) {
        obj->value[4] = value;
        return;
    }

    if (!str_prefix(arg2, "extra")) {

        send_to_char("Current extra flags are: \n\r", ch);
        obj->extra_flags = (int)flag_set(ITEM_EXTRA_FLAGS, arg3, obj->extra_flags, ch);
        return;
    }

    if (!str_prefix(arg2, "wear")) {
        send_to_char("Current wear flags are: \n\r", ch);
        obj->wear_flags = (int)flag_set(ITEM_WEAR_FLAGS, arg3, obj->wear_flags, ch);
        return;
    }

    if (!str_prefix(arg2, "level")) {
        obj->level = value;
        return;
    }

    if (!str_prefix(arg2, "weight")) {
        obj->weight = value;
        return;
    }

    if (!str_prefix(arg2, "cost")) {
        obj->cost = value;
        return;
    }

    if (!str_prefix(arg2, "timer")) {
        obj->timer = value;
        return;
    }

    /* NO_LOCATE flag.  0 turns it off, 1 turns it on *WHAHEY!* */
    if (!str_prefix(arg2, "nolocate")) {
        if (value == 0)
            REMOVE_BIT(obj->extra_flags, ITEM_NO_LOCATE);
        else
            SET_BIT(obj->extra_flags, ITEM_NO_LOCATE);
        return;
    }

    /*
     * Generate usage message.
     */
    do_oset(ch, "");
}

void do_rset(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set room <location> <field> <value>\n\r", ch);
        send_to_char("  Field being one of:\n\r", ch);
        send_to_char("    flags sector\n\r", ch);
        send_to_char("  (use set room <location> flags ? to list flags\n\r", ch);
        return;
    }

    if ((location = find_location(ch, arg1)) == nullptr) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!str_prefix(arg2, "flags")) {
        send_to_char("The current room flags are:\n\r", ch);
        location->room_flags = (int)flag_set(ROOM_FLAGS, arg3, location->room_flags, ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3)) {
        send_to_char("Value must be numeric.\n\r", ch);
        return;
    }
    value = atoi(arg3);

    /*
     * Set something.
     */
    if (!str_prefix(arg2, "sector")) {
        location->sector_type = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_rset(ch, "");
}

void do_sockets(CHAR_DATA *ch, const char *argument) {
    std::string buf;
    char arg[MAX_INPUT_LENGTH];
    int count = 0;

    one_argument(argument, arg);
    const auto view_all = arg[0] == '\0';
    for (auto &d : descriptors().all()) {
        const char *name = nullptr;
        if (auto *victim = d.character()) {
            if (!can_see(ch, victim))
                continue;
            if (view_all || is_name(arg, d.character()->name) || is_name(arg, d.person()->name))
                name = d.person()->name;
        } else if (ch->get_trust() == MAX_LEVEL) {
            // log even connections that haven't logged in yet
            // Level 100s only, mind
            name = "(unknown)";
        }
        if (name) {
            count++;
            buf += "[{:3} {:>5}] {}@{}\n\r"_format(d.channel(), short_name_of(d.state()), name, d.host());
        }
    }
    if (count == 0) {
        send_to_char("No one by that name is connected.\n\r", ch);
        return;
    }

    buf += "{} user{}\n\r"_format(count, count == 1 ? "" : "s");
    ch->desc->page_to(buf);
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        send_to_char("Force whom to do what?\n\r", ch);
        return;
    }

    one_argument(argument, arg2);

    if (!str_cmp(arg2, "delete")) {
        send_to_char("That will NOT be done.\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "private")) {
        send_to_char("That will NOT be done.\n\r", ch);
        return;
    }

    bug_snprintf(buf, sizeof(buf), "$n forces you to '%s'.", argument);

    if (!str_cmp(arg, "all")) {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (ch->get_trust() < DEITY) {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != nullptr; vch = vch_next) {
            vch_next = vch->next;

            if (vch->is_pc() && vch->get_trust() < ch->get_trust()) {
                /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
                MOBtrigger = false;
                act(buf, ch, nullptr, vch, To::Vict);
                interpret(vch, argument);
            }
        }
    } else if (!str_cmp(arg, "players")) {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (ch->get_trust() < SUPREME) {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != nullptr; vch = vch_next) {
            vch_next = vch->next;

            if (vch->is_pc() && vch->get_trust() < ch->get_trust() && vch->level < LEVEL_HERO) {
                /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
                MOBtrigger = false;
                act(buf, ch, nullptr, vch, To::Vict);
                interpret(vch, argument);
            }
        }
    } else if (!str_cmp(arg, "gods")) {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (ch->get_trust() < SUPREME) {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != nullptr; vch = vch_next) {
            vch_next = vch->next;

            if (vch->is_pc() && vch->get_trust() < ch->get_trust() && vch->level >= LEVEL_HERO) {
                /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
                MOBtrigger = false;
                act(buf, ch, nullptr, vch, To::Vict);
                interpret(vch, argument);
            }
        }
    } else {
        CHAR_DATA *victim;

        if ((victim = get_char_world(ch, arg)) == nullptr) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch) {
            send_to_char("Aye aye, right away!\n\r", ch);
            return;
        }

        if (victim->get_trust() >= ch->get_trust()) {
            send_to_char("Do it yourself!\n\r", ch);
            return;
        }

        if (victim->is_pc() && ch->get_trust() < DEITY) {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }
        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        MOBtrigger = false;
        act(buf, ch, nullptr, victim, To::Vict);
        interpret(victim, argument);
    }

    send_to_char("Ok.\n\r", ch);
}

/*
 * New routines by Dionysos.
 */
void do_invis(CHAR_DATA *ch, const char *argument) {
    int level;
    char arg[MAX_STRING_LENGTH];

    if (ch->is_npc())
        return;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        /* take the default path */

        if (IS_SET(ch->act, PLR_WIZINVIS)) {
            REMOVE_BIT(ch->act, PLR_WIZINVIS);
            ch->invis_level = 0;
            act("$n slowly fades into existence.", ch);
            send_to_char("You slowly fade back into existence.\n\r", ch);
        } else {
            SET_BIT(ch->act, PLR_WIZINVIS);
            if (IS_SET(ch->act, PLR_PROWL))
                REMOVE_BIT(ch->act, PLR_PROWL);
            ch->invis_level = ch->get_trust();
            act("$n slowly fades into thin air.", ch);
            send_to_char("You slowly vanish into thin air.\n\r", ch);
            if (ch->pet != nullptr) {
                SET_BIT(ch->pet->act, PLR_WIZINVIS);
                if (IS_SET(ch->pet->act, PLR_PROWL))
                    REMOVE_BIT(ch->pet->act, PLR_PROWL);
                ch->pet->invis_level = ch->get_trust();
            }
        }
    else
    /* do the level thing */
    {
        level = atoi(arg);
        if (level < 2 || level > ch->get_trust()) {
            send_to_char("Invis level must be between 2 and your level.\n\r", ch);
            return;
        } else {
            ch->reply = nullptr;
            SET_BIT(ch->act, PLR_WIZINVIS);
            ch->invis_level = level;
            act("$n slowly fades into thin air.", ch);
            send_to_char("You slowly vanish into thin air.\n\r", ch);
        }
    }
}

void do_prowl(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_STRING_LENGTH];
    int level = 0;

    if (ch->is_npc())
        return;

    if (ch->get_trust() < LEVEL_HERO) {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
        if (IS_SET(ch->act, PLR_PROWL)) {
            REMOVE_BIT(ch->act, PLR_PROWL);
            ch->invis_level = 0;
            if (ch->pet != nullptr) {
                REMOVE_BIT(ch->pet->act, PLR_PROWL);
                ch->pet->invis_level = 0;
            }
            act("$n slowly fades into existence.", ch);
            send_to_char("You slowly fade back into existence.\n\r", ch);
            return;
        } else {
            ch->invis_level = ch->get_trust();
            SET_BIT(ch->act, PLR_PROWL);
            if (ch->pet != nullptr) {
                ch->pet->invis_level = ch->get_trust();
                SET_BIT(ch->pet->act, PLR_PROWL);
            }
            act("$n slowly fades into thin air.", ch);
            send_to_char("You slowly vanish into thin air.\n\r", ch);
            REMOVE_BIT(ch->act, PLR_WIZINVIS);
            if (ch->pet != nullptr)
                REMOVE_BIT(ch->pet->act, PLR_WIZINVIS);
            return;
        }
    }

    level = atoi(arg);

    if ((level > ch->get_trust()) || (level < 2)) {
        send_to_char("You must specify a level between 2 and your level.\n\r", ch);
        return;
    }

    if (IS_SET(ch->act, PLR_PROWL)) {
        REMOVE_BIT(ch->act, PLR_PROWL);
        ch->invis_level = 0;
        if (ch->pet != nullptr) {
            REMOVE_BIT(ch->pet->act, PLR_PROWL);
            ch->pet->invis_level = 0;
        }
        act("$n slowly fades into existence.", ch);
        send_to_char("You slowly fade back into existence.\n\r", ch);
        return;
    } else {
        ch->invis_level = level;
        SET_BIT(ch->act, PLR_PROWL);
        if (ch->pet != nullptr) {
            SET_BIT(ch->pet->act, PLR_PROWL);
            ch->pet->invis_level = level;
        }
        act("$n slowly fades into thin air.", ch);
        send_to_char("You slowly vanish into thin air.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_WIZINVIS);
        if (ch->pet != nullptr)
            REMOVE_BIT(ch->pet->act, PLR_WIZINVIS);
        return;
    }
}

void do_holylight(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->is_npc())
        return;

    if (IS_SET(ch->act, PLR_HOLYLIGHT)) {
        REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char("Holy light mode off.\n\r", ch);
    } else {
        SET_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char("Holy light mode on.\n\r", ch);
    }
}

void do_sacname(CHAR_DATA *ch, const char *argument) {

    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        send_to_char("You must tell me who they're gonna sacrifice to!\n\r", ch);
        bug_snprintf(buf, sizeof(buf), "Currently sacrificing to: %s\n\r", deity_name);
        send_to_char(buf, ch);
        return;
    }
    strcpy(deity_name_area, argument);
    bug_snprintf(buf, sizeof(buf), "Players now sacrifice to %s.\n\r", deity_name);
    send_to_char(buf, ch);
}
