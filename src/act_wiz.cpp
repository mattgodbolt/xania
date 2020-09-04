/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_wiz.c: commands for immortals                                    */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
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

static const char ROOM_FLAGS[] = "dark * nomob indoors * * * * * private safe solitary petshop norecall 100imponly "
                                 "92godonly heroonly newbieonly law";

void do_mskills(Char *ch, const char *argument);
void do_maffects(Char *ch, const char *argument);
void do_mpracs(Char *ch, const char *argument);
void do_minfo(Char *ch, const char *argument);
void do_mspells(Char *ch, const char *argument);

/*
 * Local functions.
 */
ROOM_INDEX_DATA *find_location(Char *ch, std::string_view arg);

/* Permits or denies a player from playing the Mud from a PERMIT banned site */
void do_permit(Char *ch, const char *argument) {
    Char *victim;
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
        ch->send_line("Permit whom?");
        return;
    }
    if (flag) {
        set_extra(victim, EXTRA_PERMIT);
    } else {
        remove_extra(victim, EXTRA_PERMIT);
    }
    ch->send_line("PERMIT flag {} for {}.", flag ? "set" : "removed", victim->name);
}

/* equips a character */
void do_outfit(Char *ch, const char *argument) {
    (void)argument;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];

    if (ch->level > 5 || ch->is_npc()) {
        ch->send_line("Find it yourself!");
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
    ch->send_to(buf);
}

/* RT nochannels command, for those spammers */
void do_nochannels(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Nochannel whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (IS_SET(victim->comm, COMM_NOCHANNELS)) {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        victim->send_line("The gods have restored your channel privileges.");
        ch->send_line("NOCHANNELS removed.");
    } else {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        victim->send_line("The gods have revoked your channel privileges.");
        ch->send_line("NOCHANNELS set.");
    }
}

void do_bamfin(Char *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfin = smash_tilde(argument);

    if (bamfin.empty()) {
        ch->send_line("Your poofin is {}", ch->pcdata->bamfin);
        return;
    }

    if (!matches_inside(ch->name, bamfin)) {
        ch->send_line("You must include your name.");
        return;
    }

    ch->pcdata->bamfin = bamfin;
    ch->send_line("Your poofin is now {}", ch->pcdata->bamfin);
}

void do_bamfout(Char *ch, const char *argument) {
    if (ch = ch->player(); !ch)
        return;
    auto bamfout = smash_tilde(argument);

    if (bamfout.empty()) {
        ch->send_line("Your poofout is {}", ch->pcdata->bamfout);
        return;
    }

    if (!matches_inside(ch->name, bamfout)) {
        ch->send_line("You must include your name.");
        return;
    }

    ch->pcdata->bamfout = bamfout;
    ch->send_line("Your poofout is now {}", ch->pcdata->bamfout);
}

void do_deny(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Deny whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    SET_BIT(victim->act, PLR_DENY);
    victim->send_line("You are denied access!");
    ch->send_line("OK.");
    save_char_obj(victim);
    do_quit(victim, "");
}

void do_disconnect(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Disconnect whom?");
        return;
    }

    if (argument[0] == '+') {
        argument++;
        for (auto &d : descriptors().all()) {
            if (d.character() && matches(d.character()->name, argument)) {
                d.close();
                ch->send_line("Ok.");
                return;
            }
        }
        ch->send_line("Couldn't find a matching descriptor.");
        return;
    } else {
        if ((victim = get_char_world(ch, arg)) == nullptr) {
            ch->send_line("They aren't here.");
            return;
        }

        if (victim->desc == nullptr) {
            act("$N doesn't have a descriptor.", ch, nullptr, victim, To::Char);
            return;
        }

        for (auto &d : descriptors().all()) {
            if (&d == victim->desc) {
                d.close();
                ch->send_line("Ok.");
                return;
            }
        }

        bug("Do_disconnect: desc not found.");
        ch->send_line("Descriptor not found!");
        return;
    }
}

void do_pardon(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        ch->send_line("Syntax: pardon <character> <killer|thief>.");
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (!str_cmp(arg2, "killer")) {
        if (IS_SET(victim->act, PLR_KILLER)) {
            REMOVE_BIT(victim->act, PLR_KILLER);
            ch->send_line("Killer flag removed.");
            victim->send_line("You are no longer a KILLER.");
        }
        return;
    }

    if (!str_cmp(arg2, "thief")) {
        if (IS_SET(victim->act, PLR_THIEF)) {
            REMOVE_BIT(victim->act, PLR_THIEF);
            ch->send_line("Thief flag removed.");
            victim->send_line("You are no longer a THIEF.");
        }
        return;
    }

    ch->send_line("Syntax: pardon <character> <killer|thief>.");
}

void do_echo(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("Global echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::to_character()) {
        victim.send_line("{}{}", victim.get_trust() >= ch->get_trust() ? "global> " : "", argument);
    }
}

void do_recho(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Local echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_room(*ch) | DescriptorFilter::to_character()) {
        victim.send_to(fmt::format(
            "{}{}\n\r",
            victim.get_trust() >= ch->get_trust() && ch->in_room->vnum != CHAL_VIEWING_GALLERY ? "local> " : "",
            argument));
    }
}

void do_zecho(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Zone echo what?");
        return;
    }

    for (auto &victim : descriptors().playing() | DescriptorFilter::same_area(*ch) | DescriptorFilter::to_character()) {
        victim.send_line("{}{}", victim.get_trust() >= ch->get_trust() ? "zone> " : "", argument);
    }
}

void do_pecho(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0') {
        ch->send_line("Personal echo what?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("Target not found.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust() && ch->get_trust() != MAX_LEVEL)
        victim->send_to("personal> ");

    victim->send_line("{}", argument);
    ch->send_line("personal> {}", argument);
}

ROOM_INDEX_DATA *find_location(Char *ch, std::string_view arg) {
    Char *victim;
    OBJ_DATA *obj;

    if (is_number(arg))
        return get_room_index(parse_number(arg));

    if (matches(arg, "here"))
        return ch->in_room;

    if ((victim = get_char_world(ch, arg)) != nullptr)
        return victim->in_room;

    if ((obj = get_obj_world(ch, arg)) != nullptr)
        return obj->in_room;

    return nullptr;
}

void do_transfer(Char *ch, std::string_view argument) {
    ArgParser args(argument);
    if (args.empty()) {
        ch->send_line("Transfer whom (and where)?");
        return;
    }

    auto whom = args.shift();
    auto where = args.shift();
    if (matches(whom, "all")) {
        for (auto &victim :
             descriptors().all_visible_to(*ch) | DescriptorFilter::except(*ch) | DescriptorFilter::to_character()) {
            if (victim.in_room != nullptr)
                do_transfer(ch, fmt::format("{} {}", victim.name, where));
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    ROOM_INDEX_DATA *location{};
    if (where.empty()) {
        location = ch->in_room;
    } else {
        if ((location = find_location(ch, where)) == nullptr) {
            ch->send_line("No such location.");
            return;
        }

        if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
            ch->send_line("That room is private right now.");
            return;
        }
    }

    auto *victim = get_char_world(ch, whom);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    transfer(ch, victim, location);
}

void transfer(const Char *imm, Char *victim, ROOM_INDEX_DATA *location) {
    if (victim->in_room == nullptr) {
        imm->send_line("They are in limbo.");
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
    if (imm != victim)
        act("$n has transferred you.", imm, nullptr, victim, To::Vict);
    do_look(victim, "auto");
    imm->send_line("Ok.");
}

void do_wizlock(Char *ch, const char *argument) {
    (void)argument;
    extern bool wizlock;
    wizlock = !wizlock;

    if (wizlock)
        ch->send_line("Game wizlocked.");
    else
        ch->send_line("Game un-wizlocked.");
}

/* RT anti-newbie code */

void do_newlock(Char *ch, const char *argument) {
    (void)argument;
    extern bool newlock;
    newlock = !newlock;

    if (newlock)
        ch->send_line("New characters have been locked out.");
    else
        ch->send_line("Newlock removed.");
}

void do_at(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    Char *wch;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        ch->send_line("At where what?");
        return;
    }

    if ((location = find_location(ch, arg)) == nullptr) {
        ch->send_line("No such location.");
        return;
    }
    if ((ch->in_room != nullptr) && location == ch->in_room) {
        ch->send_line("But that's in here.......");
        return;
    }
    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        ch->send_line("That room is private right now.");
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

void do_goto(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Goto where?");
        return;
    }

    ROOM_INDEX_DATA *location;
    if ((location = find_location(ch, argument)) == nullptr) {
        ch->send_line("No such location.");
        return;
    }

    if (room_is_private(location) && (ch->get_trust() < IMPLEMENTOR)) {
        ch->send_line("That room is private right now.");
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

void do_stat(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    const char *string;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *location;
    Char *victim;

    string = one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  stat <name>");
        ch->send_line("  stat obj <name>");
        ch->send_line("  stat mob <name>");
        ch->send_line("  stat room <number>");
        ch->send_line("  stat <skills/spells/info/pracs/affects> <name>");
        ch->send_line("  stat prog <mobprog>");
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

    ch->send_line("Nothing by that name found anywhere.");
}

void do_rstat(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    Char *rch;

    one_argument(argument, arg);
    location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
    if (location == nullptr) {
        ch->send_line("No such location.");
        return;
    }

    if (ch->in_room != location && room_is_private(location) && ch->get_trust() < IMPLEMENTOR) {
        ch->send_line("That room is private right now.");
        return;
    }

    bug_snprintf(buf, sizeof(buf), "Name: '%s.'\n\rArea: '%s'.\n\r", location->name, location->area->name);
    ch->send_to(buf);

    bug_snprintf(buf, sizeof(buf), "Vnum: %d.  Sector: %d.  Light: %d.\n\r", location->vnum, location->sector_type,
                 location->light);
    ch->send_to(buf);
    ch->send_line("Flags: ");
    display_flags(ROOM_FLAGS, ch, location->room_flags);
    bug_snprintf(buf, sizeof(buf), "Description:\n\r%s", location->description);
    ch->send_to(buf);

    if (location->extra_descr != nullptr) {
        EXTRA_DESCR_DATA *ed;

        ch->send_line("Extra description keywords: '");
        for (ed = location->extra_descr; ed; ed = ed->next) {
            ch->send_to(ed->keyword);
            if (ed->next != nullptr)
                ch->send_line(" ");
        }
        ch->send_line("'.");
    }

    ch->send_line("Characters:");
    for (rch = location->people; rch; rch = rch->next_in_room) {
        if (can_see(ch, rch)) {
            ch->send_line(" ");
            ch->send_to(ArgParser(rch->name).shift());
        }
    }

    ch->send_line(".\n\rObjects:   ");
    for (obj = location->contents; obj; obj = obj->next_content) {
        ch->send_line(" ");
        one_argument(obj->name, buf);
        ch->send_to(buf);
    }
    ch->send_line(".");

    for (auto door : all_directions) {
        if (auto *pexit = location->exit[door]) {
            ch->send_to(fmt::format("Door: {}.  To: {}.  Key: {}.  Exit flags: {}.\n\rKeyword: '{}'.  Description: {}",
                                    to_string(door), (pexit->u1.to_room == nullptr ? -1 : pexit->u1.to_room->vnum),
                                    pexit->key, pexit->exit_info, pexit->keyword,
                                    pexit->description[0] != '\0' ? pexit->description : "(none).\n\r"));
        }
    }
}

void do_ostat(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    sh_int dam_type;
    int vnum;

    /* this will help prevent major memory allocations - a crash bug!
     --Faramir */
    if (strlen(argument) < 2 && !isdigit(argument[0])) {
        ch->send_line("Please be more specific.");
        return;
    }
    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat what?");
        return;
    }

    if (isdigit(argument[0])) {
        vnum = atoi(argument);
        if ((pObjIndex = get_obj_index(vnum)) == nullptr) {
            ch->send_line("Nothing like that in hell, earth, or heaven.");
            return;
        }
        ch->send_line("Template of object:");
        bug_snprintf(buf, sizeof(buf), "Name(s): %s\n\r", pObjIndex->name);
        ch->send_to(buf);

        bug_snprintf(buf, sizeof(buf), "Vnum: %d  Type: %s\n\r", pObjIndex->vnum, item_index_type_name(pObjIndex));
        ch->send_to(buf);

        ch->send_line("Short description: {}", pObjIndex->short_descr);
        ch->send_line("Long description: {}", pObjIndex->description);

        bug_snprintf(buf, sizeof(buf), "Wear bits: %s\n\rExtra bits: %s\n\r", wear_bit_name(pObjIndex->wear_flags),
                     extra_bit_name(pObjIndex->extra_flags));
        ch->send_to(buf);

        ch->send_line("Wear string: {}", pObjIndex->wear_string);

        bug_snprintf(buf, sizeof(buf), "Weight: %d\n\r", pObjIndex->weight);
        ch->send_to(buf);

        bug_snprintf(buf, sizeof(buf), "Level: %d  Cost: %d  Condition: %d\n\r", pObjIndex->level, pObjIndex->cost,
                     pObjIndex->condition);
        ch->send_to(buf);

        bug_snprintf(buf, sizeof(buf), "Values: %d %d %d %d %d\n\r", pObjIndex->value[0], pObjIndex->value[1],
                     pObjIndex->value[2], pObjIndex->value[3], pObjIndex->value[4]);
        ch->send_to(buf);
        ch->send_line("Please load this object if you need to know more about it.");
        return;
    }

    if ((obj = get_obj_world(ch, argument)) == nullptr) {
        ch->send_line("Nothing like that in hell, earth, or heaven.");
        return;
    }

    bug_snprintf(buf, sizeof(buf), "Name(s): %s\n\r", obj->name);
    ch->send_to(buf);

    bug_snprintf(buf, sizeof(buf), "Vnum: %d  Type: %s  Resets: %d\n\r", obj->pIndexData->vnum, item_type_name(obj),
                 obj->pIndexData->reset_num);
    ch->send_to(buf);

    ch->send_line("Short description: {}", obj->short_descr);
    ch->send_line("Long description: {}", obj->description);

    bug_snprintf(buf, sizeof(buf), "Wear bits: %s\n\rExtra bits: %s\n\r", wear_bit_name(obj->wear_flags),
                 extra_bit_name(obj->extra_flags));
    ch->send_to(buf);

    ch->send_line("Wear string: {}", obj->wear_string);

    bug_snprintf(buf, sizeof(buf), "Number: %d/%d  Weight: %d/%d\n\r", 1, get_obj_number(obj), obj->weight,
                 get_obj_weight(obj));
    ch->send_to(buf);

    bug_snprintf(buf, sizeof(buf), "Level: %d  Cost: %d  Condition: %d  Timer: %d\n\r", obj->level, obj->cost,
                 obj->condition, obj->timer);
    ch->send_to(buf);

    ch->send_to(fmt::format(
        "In room: {}  In object: {}  Carried by: {}  Wear_loc: {}\n\r",
        obj->in_room == nullptr ? 0 : obj->in_room->vnum, obj->in_obj == nullptr ? "(none)" : obj->in_obj->short_descr,
        obj->carried_by == nullptr ? "(none)" : can_see(ch, obj->carried_by) ? obj->carried_by->name : "someone",
        obj->wear_loc));

    bug_snprintf(buf, sizeof(buf), "Values: %d %d %d %d %d\n\r", obj->value[0], obj->value[1], obj->value[2],
                 obj->value[3], obj->value[4]);
    ch->send_to(buf);

    /* now give out vital statistics as per identify */

    switch (obj->item_type) {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_BOMB:
        bug_snprintf(buf, sizeof(buf), "Level %d spells of:", obj->value[0]);
        ch->send_to(buf);

        if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[1]].name);
            ch->send_line("'");
        }

        if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[2]].name);
            ch->send_line("'");
        }

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[3]].name);
            ch->send_line("'");
        }

        if ((obj->value[4] >= 0 && obj->value[4] < MAX_SKILL) && obj->item_type == ITEM_BOMB) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[4]].name);
            ch->send_line("'");
        }

        ch->send_line(".");
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        bug_snprintf(buf, sizeof(buf), "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0]);
        ch->send_to(buf);

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[3]].name);
            ch->send_line("'");
        }

        ch->send_line(".");
        break;

    case ITEM_WEAPON:
        ch->send_line("Weapon type is ");
        switch (obj->value[0]) {
        case (WEAPON_EXOTIC): ch->send_line("exotic"); break;
        case (WEAPON_SWORD): ch->send_line("sword"); break;
        case (WEAPON_DAGGER): ch->send_line("dagger"); break;
        case (WEAPON_SPEAR): ch->send_line("spear/staff"); break;
        case (WEAPON_MACE): ch->send_line("mace/club"); break;
        case (WEAPON_AXE): ch->send_line("axe"); break;
        case (WEAPON_FLAIL): ch->send_line("flail"); break;
        case (WEAPON_WHIP): ch->send_line("whip"); break;
        case (WEAPON_POLEARM): ch->send_line("polearm"); break;
        default: ch->send_line("unknown"); break;
        }
        bug_snprintf(buf, sizeof(buf), "Damage is %dd%d (average %d)\n\r", obj->value[1], obj->value[2],
                     (1 + obj->value[2]) * obj->value[1] / 2);
        ch->send_to(buf);

        if (obj->value[4]) /* weapon flags */
        {
            bug_snprintf(buf, sizeof(buf), "Weapons flags: %s\n\r", weapon_bit_name(obj->value[4]));
            ch->send_to(buf);
        }

        dam_type = attack_table[obj->value[3]].damage;
        ch->send_line("Damage type is ");
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
            bug("ostat: Unknown damage type {}", dam_type);
            break;
        }
        ch->send_to(buf);
        break;

    case ITEM_ARMOR:
        bug_snprintf(buf, sizeof(buf), "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
                     obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
        ch->send_to(buf);
        break;

    case ITEM_PORTAL:
        bug_snprintf(buf, sizeof(buf), "Portal to %s (%d).\n\r", obj->destination->name, obj->destination->vnum);
        ch->send_to(buf);
        break;
    }

    if (obj->extra_descr != nullptr || obj->pIndexData->extra_descr != nullptr) {
        EXTRA_DESCR_DATA *ed;

        ch->send_line("Extra description keywords: '");

        for (ed = obj->extra_descr; ed != nullptr; ed = ed->next) {
            ch->send_to(ed->keyword);
            if (ed->next != nullptr)
                ch->send_line(" ");
        }

        for (ed = obj->pIndexData->extra_descr; ed != nullptr; ed = ed->next) {
            ch->send_to(ed->keyword);
            if (ed->next != nullptr)
                ch->send_line(" ");
        }

        ch->send_line("'");
    }

    for (auto &af : obj->affected)
        ch->send_line("Affects {}.", af.describe_item_effect(true));

    if (!obj->enchanted)
        for (auto &af : obj->pIndexData->affected)
            ch->send_line("Affects {}.", af.describe_item_effect(true));
}

void do_mskills(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    char skill_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO];
    int sn, lev;
    bool found = false;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat skills whom?");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc())
        return;

    ch->send_line("Skill list for {}:", victim->name);

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
        ch->send_line("They know no skills.");
        return;
    }

    for (lev = 0; lev < LEVEL_HERO; lev++)
        if (skill_list[lev][0] != '\0')
            ch->send_to(skill_list[lev]);
    ch->send_line("");
}

/* Corrected 28/8/96 by Oshea to give correct list of spell costs. */
void do_mspells(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    char spell_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO];
    int sn, lev, mana;
    bool found = false;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat spells whom?");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc())
        return;

    ch->send_line("Spell list for {}:", victim->name);

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
        ch->send_line("They know no spells.");
        return;
    }

    for (lev = 0; lev < LEVEL_HERO; lev++)
        if (spell_list[lev][0] != '\0')
            ch->send_to(spell_list[lev]);
    ch->send_line("");
}

void do_maffects(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat affects whom?");
        return;
    }

    Char *victim = get_char_world(ch, argument);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    ch->send_line("Affect list for {}:", victim->name);

    if (victim->affected.empty()) {
        ch->send_line("Nothing.");
        return;
    }
    for (auto &af : victim->affected)
        ch->send_to(fmt::format("{}: '{}'{}.\n\r", af.is_skill() ? "Skill" : "Spell", skill_table[af.type].name,
                                af.describe_char_effect(true)));
}

/* Corrected 28/8/96 by Oshea to give correct list of spells/skills. */
void do_mpracs(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    int sn;
    int col;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat pracs whom?");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc())
        return;

    ch->send_line("Practice list for {}:", victim->name);

    col = 0;
    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        if (victim->level < get_skill_level(victim, sn)
            || victim->pcdata->learned[sn] < 1 /* skill is not known NOT get_skill_learned */)
            continue;

        bug_snprintf(buf, sizeof(buf), "%-18s %3d%%  ", skill_table[sn].name, victim->pcdata->learned[sn]);
        ch->send_to(buf);
        if (++col % 3 == 0)
            ch->send_line("");
    }

    if (col % 3 != 0)
        ch->send_line("");

    bug_snprintf(buf, sizeof(buf), "They have %d practice sessions left.\n\r", victim->practice);
    ch->send_to(buf);
}

/* Correct on 28/8/96 by Oshea to give correct cp's */
void do_minfo(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    int gn, col;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat info whom?");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc())
        return;

    ch->send_line("Info list for {}:", victim->name);

    col = 0;

    /* show all groups */

    for (gn = 0; gn < MAX_GROUP; gn++) {
        if (group_table[gn].name == nullptr)
            break;
        if (victim->pcdata->group_known[gn]) {
            bug_snprintf(buf, sizeof(buf), "%-20s ", group_table[gn].name);
            ch->send_to(buf);
            if (++col % 3 == 0)
                ch->send_line("");
        }
    }
    if (col % 3 != 0)
        ch->send_line("");
    bug_snprintf(buf, sizeof(buf), "Creation points: %d\n\r", victim->pcdata->points);

    ch->send_to(buf);
}

void do_mstat(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    /* this will help prevent major memory allocations */
    if (strlen(argument) < 2) {
        ch->send_line("Please be more specific.");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Stat whom?");
        return;
    }

    if ((victim = get_char_world(ch, argument)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    ch->send_to(fmt::format("Name: {}     Clan: {}     Rank: {}.\n\r", victim->name,
                            victim->clan() ? victim->clan()->name : "(none)",
                            victim->pc_clan() ? victim->pc_clan()->level_name() : "(none)"));

    bug_snprintf(buf, sizeof(buf), "Vnum: %d  Format: %s  Race: %s  Sex: %s  Room: %d\n\r",
                 victim->is_npc() ? victim->pIndexData->vnum : 0, victim->is_npc() ? ".are" : "pc",
                 race_table[victim->race].name, victim->sex == 1 ? "male" : victim->sex == 2 ? "female" : "neutral",
                 victim->in_room == nullptr ? 0 : victim->in_room->vnum);
    ch->send_to(buf);

    if (victim->is_npc()) {
        bug_snprintf(buf, sizeof(buf), "Count: %d  Killed: %d\n\r", victim->pIndexData->count,
                     victim->pIndexData->killed);
        ch->send_to(buf);
    }

    bug_snprintf(buf, sizeof(buf), "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
                 victim->perm_stat[Stat::Str], get_curr_stat(victim, Stat::Str), victim->perm_stat[Stat::Int],
                 get_curr_stat(victim, Stat::Int), victim->perm_stat[Stat::Wis], get_curr_stat(victim, Stat::Wis),
                 victim->perm_stat[Stat::Dex], get_curr_stat(victim, Stat::Dex), victim->perm_stat[Stat::Con],
                 get_curr_stat(victim, Stat::Con));
    ch->send_to(buf);

    ch->send_line("Hp: {}/{}  Mana: {}/{}  Move: {}/{}  Practices: {}", victim->hit, victim->max_hit, victim->mana,
                  victim->max_mana, victim->move, victim->max_move, ch->is_npc() ? 0 : victim->practice);

    ch->send_line("Lv: {}  Class: {}  Align: {}  Gold: {}d  Exp: {}", victim->level,
                  victim->is_npc() ? "mobile" : class_table[victim->class_num].name, victim->alignment, victim->gold,
                  victim->exp);

    bug_snprintf(buf, sizeof(buf), "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r", GET_AC(victim, AC_PIERCE),
                 GET_AC(victim, AC_BASH), GET_AC(victim, AC_SLASH), GET_AC(victim, AC_EXOTIC));
    ch->send_to(buf);

    bug_snprintf(buf, sizeof(buf), "Hit: %d  Dam: %d  Saves: %d  Position: %d  Wimpy: %d\n\r", GET_HITROLL(victim),
                 GET_DAMROLL(victim), victim->saving_throw, victim->position, victim->wimpy);
    ch->send_to(buf);

    if (victim->is_npc()) {
        bug_snprintf(buf, sizeof(buf), "Damage: %dd%d  Message:  %s\n\r", victim->damage[DICE_NUMBER],
                     victim->damage[DICE_TYPE], attack_table[victim->dam_type].noun);
        ch->send_to(buf);
    }
    ch->send_line("Fighting: {}", victim->fighting ? victim->fighting->name : "(none)");

    ch->send_to(
        fmt::format("Sentient 'victim': {}\n\r", victim->sentient_victim.empty() ? "(none)" : victim->sentient_victim));

    if (victim->is_pc()) {
        bug_snprintf(buf, sizeof(buf), "Thirst: %d  Full: %d  Drunk: %d\n\r", victim->pcdata->condition[COND_THIRST],
                     victim->pcdata->condition[COND_FULL], victim->pcdata->condition[COND_DRUNK]);
        ch->send_to(buf);
    }

    bug_snprintf(buf, sizeof(buf), "Carry number: %d  Carry weight: %d\n\r", victim->carry_number,
                 victim->carry_weight);
    ch->send_to(buf);

    if (victim->is_pc()) {
        using namespace std::chrono;
        bug_snprintf(buf, sizeof(buf), "Age: %d  Played: %ld  Last Level: %d  Timer: %d\n\r", get_age(victim),
                     duration_cast<hours>(victim->total_played()).count(), victim->pcdata->last_level, victim->timer);
        ch->send_to(buf);
    }

    bug_snprintf(buf, sizeof(buf), "Act: %s\n\r", (char *)act_bit_name(victim->act));
    ch->send_to(buf);

    if (victim->is_pc()) {
        int n;
        bug_snprintf(buf, sizeof(buf), "Extra: ");
        for (n = 0; n < MAX_EXTRA_FLAGS; n++) {
            if (is_set_extra(ch, n)) {
                strcat(buf, flagname_extra[n]);
            }
        }
        strcat(buf, "\n\r");
        ch->send_to(buf);
    }

    if (victim->comm) {
        bug_snprintf(buf, sizeof(buf), "Comm: %s\n\r", (char *)comm_bit_name(victim->comm));
        ch->send_to(buf);
    }

    if (victim->is_npc() && victim->off_flags) {
        bug_snprintf(buf, sizeof(buf), "Offense: %s\n\r", (char *)off_bit_name(victim->off_flags));
        ch->send_to(buf);
    }

    if (victim->imm_flags) {
        bug_snprintf(buf, sizeof(buf), "Immune: %s\n\r", (char *)imm_bit_name(victim->imm_flags));
        ch->send_to(buf);
    }

    if (victim->res_flags) {
        bug_snprintf(buf, sizeof(buf), "Resist: %s\n\r", (char *)imm_bit_name(victim->res_flags));
        ch->send_to(buf);
    }

    if (victim->vuln_flags) {
        bug_snprintf(buf, sizeof(buf), "Vulnerable: %s\n\r", (char *)imm_bit_name(victim->vuln_flags));
        ch->send_to(buf);
    }

    bug_snprintf(buf, sizeof(buf), "Form: %s\n\rParts: %s\n\r", form_bit_name(victim->form),
                 (char *)part_bit_name(victim->parts));
    ch->send_to(buf);

    if (victim->affected_by) {
        ch->send_line("Affected by {}", affect_bit_name(victim->affected_by));
    }

    ch->send_to(fmt::format("Master: {}  Leader: {}  Pet: {}\n\r", victim->master ? victim->master->name : "(none)",
                            victim->leader ? victim->leader->name : "(none)",
                            victim->pet ? victim->pet->name : "(none)"));

    ch->send_to(fmt::format("Riding: {}  Ridden by: {}\n\r", victim->riding ? victim->riding->name : "(none)",
                            victim->ridden_by ? victim->ridden_by->name : "(none)"));

    ch->send_to(fmt::format("Short description: {}\n\rLong  description: {}", victim->short_descr,
                            victim->long_descr.empty() ? "(none)\n\r" : victim->long_descr));

    if (victim->is_npc() && victim->spec_fun)
        ch->send_line("Mobile has special procedure.");

    if (victim->is_npc() && victim->pIndexData->progtypes) {
        ch->send_line("Mobile has MOBPROG: view with \"stat prog '{}'\"", victim->name);
    }

    for (const auto &af : victim->affected)
        ch->send_to(fmt::format("{}: '{}'{}.\n\r", af.is_skill() ? "Skill" : "Spell", skill_table[af.type].name,
                                af.describe_char_effect(true)));
    ch->send_line("");
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_mfind(Char *ch, const char *argument) {
    extern int top_mob_index;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Find whom?");
        return;
    }

    bool found = false;

    // Yeah, so iterating over all vnum's takes 10,000 loops.
    // Get_mob_index is fast, and I don't feel like threading another link.
    // Do you?
    // -- Furey
    std::string buffer;
    int nMatch = 0;
    for (int vnum = 0; nMatch < top_mob_index; vnum++) {
        if (auto *pMobIndex = get_mob_index(vnum)) {
            nMatch++;
            if (is_name(argument, pMobIndex->player_name)) {
                found = true;
                buffer += fmt::format("[{:5}] {}\n\r", pMobIndex->vnum, pMobIndex->short_descr);
            }
        }
    }

    if (found)
        ch->page_to(buffer);
    else
        ch->send_line("No mobiles by that name.");
}

void do_ofind(Char *ch, const char *argument) {
    extern int top_obj_index;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Find what?");
        return;
    }

    int nMatch = 0;

    // Yeah, so iterating over all vnum's takes 10,000 loops.
    // Get_obj_index is fast, and I don't feel like threading another link.
    // Do you?
    // -- Furey
    std::string buffer;
    for (int vnum = 0; nMatch < top_obj_index; vnum++) {
        if (const auto *pObjIndex = get_obj_index(vnum)) {
            nMatch++;
            if (is_name(argument, pObjIndex->name))
                buffer += fmt::format("[{:5}] {}\n\r", pObjIndex->vnum, pObjIndex->short_descr);
        }
    }

    if (buffer.empty())
        ch->send_line("No objects by that name.");
    else
        ch->page_to(buffer);
}

void do_vnum(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    const char *string;

    string = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  vnum obj <name>");
        ch->send_line("  vnum mob <name>");
        ch->send_line("  vnum skill <skill or spell>");
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

void do_mwhere(Char *ch, const char *argument) {
    bool find_pc = false;
    if (argument[0] == '\0') {
        find_pc = true;
    } else if (strlen(argument) < 2) {
        ch->send_line("Please be more specific.");
        return;
    }

    bool found = false;
    int number = 0;
    std::string buffer;
    for (auto *victim = char_list; victim != nullptr; victim = victim->next) {
        if ((victim->is_npc() && victim->in_room != nullptr && is_name(argument, victim->name) && !find_pc)
            || (victim->is_pc() && find_pc && can_see(ch, victim))) {
            found = true;
            number++;
            buffer +=
                fmt::format("{:3} [{:5}] {:<28} [{:5}] {:>20}\n\r", number, find_pc ? 0 : victim->pIndexData->vnum,
                            victim->short_name(), victim->in_room->vnum, victim->in_room->name);
        }
    }
    ch->page_to(buffer);

    if (!found)
        act("You didn't find any $T.", ch, nullptr, argument, To::Char); // Poor Arthur.
}

void do_reboo(Char *ch, const char *argument) {
    (void)argument;
    ch->send_line("If you want to REBOOT, spell it out.");
}

void do_reboot(Char *ch, const char *argument) {
    (void)argument;
    extern bool merc_down;

    if (!IS_SET(ch->act, PLR_WIZINVIS)) {
        do_echo(ch, fmt::format("Reboot by {}.", ch->name));
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

void do_shutdow(Char *ch, const char *argument) {
    (void)argument;
    ch->send_line("If you want to SHUTDOWN, spell it out.");
}

void do_shutdown(Char *ch, const char *argument) {
    (void)argument;
    extern bool merc_down;

    auto buf = fmt::format("Shutdown by {}.", ch->name.c_str());
    append_file(ch, SHUTDOWN_FILE, buf.c_str());

    do_echo(ch, buf + "\n\r");
    do_force(ch, "all save");
    do_save(ch, "");
    merc_down = true;
    for (auto &d : descriptors().all())
        d.close();
}

void do_snoop(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (ch->desc == nullptr) {
        // MRG old code was split-brained about checking this. Seems like nothing would have worked if ch->desc was
        // actually null, so bugging out here.
        bug("null ch->desc in do_snoop");
        return;
    }

    if (arg[0] == '\0') {
        ch->send_line("Snoop whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (!victim->desc) {
        ch->send_line("No descriptor to snoop.");
        return;
    }

    if (victim == ch) {
        ch->send_line("Cancelling all snoops.");
        ch->desc->stop_snooping();
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (!ch->desc->try_start_snooping(*victim->desc)) {
        ch->send_line("No snoop loops.");
        return;
    }

    ch->send_line("Ok.");
}

void do_switch(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Switch into whom?");
        return;
    }

    if (ch->desc == nullptr)
        return;

    if (ch->desc->is_switched()) {
        ch->send_line("You are already switched.");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim == ch) {
        ch->send_line("Ok.");
        return;
    }

    if (victim->is_pc()) {
        ch->send_line("You can only switch into mobiles.");
        return;
    }

    if (victim->desc != nullptr) {
        ch->send_line("Character in use.");
        return;
    }

    ch->desc->do_switch(victim);
    /* change communications to match */
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    victim->send_line("Ok.");
}

void do_return(Char *ch, const char *argument) {
    (void)argument;
    if (ch->desc == nullptr)
        return;

    if (!ch->desc->is_switched()) {
        ch->send_line("You aren't switched.");
        return;
    }

    ch->send_line("You return to your original body.");
    ch->desc->do_return();
}

/* trust levels for load and clone */
/* cut out by Faramir but func retained in case of any
   calls I don't know about. */
bool obj_check(Char *ch, OBJ_DATA *obj) { return ch->get_trust() >= obj->level; }

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(Char *ch, OBJ_DATA *obj, OBJ_DATA *clone) {
    for (OBJ_DATA *c_obj = obj->contains; c_obj != nullptr; c_obj = c_obj->next_content) {
        if (obj_check(ch, c_obj)) {
            OBJ_DATA *t_obj = create_object(c_obj->pIndexData);
            clone_object(c_obj, t_obj);
            obj_to_obj(t_obj, clone);
            recursive_clone(ch, c_obj, t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *mob = nullptr;
    OBJ_DATA *obj = nullptr;
    const char *rest = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Clone what?");
        return;
    }

    if (!str_prefix(arg, "object")) {
        obj = get_obj_here(ch, rest);
    } else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
        mob = get_char_room(ch, rest);
    } else { /* find both */
        mob = get_char_room(ch, argument);
        obj = get_obj_here(ch, argument);
    }

    /* clone an object */
    if (obj != nullptr) {
        OBJ_DATA *clone;

        if (!obj_check(ch, obj)) {
            ch->send_line("Your powers are not great enough for such a task.");
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
    } else if (mob != nullptr) {
        Char *clone;
        OBJ_DATA *new_obj;

        if (mob->is_pc()) {
            ch->send_line("You can only clone mobiles.");
            return;
        }

        if ((mob->level > 20 && ch->get_trust() < GOD) || (mob->level > 10 && ch->get_trust() < IMMORTAL)
            || (mob->level > 5 && ch->get_trust() < DEMI) || (mob->level > 0 && ch->get_trust() < ANGEL)
            || ch->get_trust() < AVATAR) {
            ch->send_line("Your powers are not great enough for such a task.");
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
    } else {
        ch->send_line("You don't see that here.");
    }
}

/* RT to replace the two load commands */

void do_mload(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        ch->send_line("Syntax: load mob <vnum>.");
        return;
    }

    if ((pMobIndex = get_mob_index(atoi(arg))) == nullptr) {
        ch->send_line("No mob has that vnum.");
        return;
    }

    Char *victim = create_mobile(pMobIndex);
    char_to_room(victim, ch->in_room);
    act("$n has created $N!", ch, nullptr, victim, To::Room);
    ch->send_line("Ok.");
}

void do_oload(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        ch->send_line("Syntax: load obj <vnum>.");
        return;
    }

    if ((pObjIndex = get_obj_index(atoi(arg1))) == nullptr) {
        ch->send_line("No object has that vnum.");
        return;
    }

    obj = create_object(pObjIndex);
    if (CAN_WEAR(obj, ITEM_TAKE))
        obj_to_char(obj, ch);
    else
        obj_to_room(obj, ch->in_room);
    act("$n has created $p!", ch, obj, nullptr, To::Room);
    ch->send_line("Ok.");
}

void do_load(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  load mob <vnum>");
        ch->send_line("  load obj <vnum> <level>");
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

void do_purge(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    OBJ_DATA *obj;
    Descriptor *d;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* 'purge' */
        Char *vnext;
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
        ch->send_line("Ok.");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_pc()) {

        if (ch == victim) {
            ch->send_line("Ho ho ho.");
            return;
        }

        if (ch->get_trust() <= victim->get_trust()) {
            ch->send_line("Maybe that wasn't a good idea...");
            victim->send_line("{} tried to purge you!", ch->name);
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

void do_advance(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    int level;
    int iLevel;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
        ch->send_line("Syntax: advance <char> <level>.");
        return;
    }

    if ((victim = get_char_room(ch, arg1)) == nullptr) {
        ch->send_line("That player is not here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if ((level = atoi(arg2)) < 1 || level > MAX_LEVEL) {
        char buf[32];
        bug_snprintf(buf, sizeof(buf), "Level must be 1 to %d.\n\r", MAX_LEVEL);

        ch->send_to(buf);
        return;
    }

    if (level > ch->get_trust()) {
        ch->send_line("Limited to your trust level.");
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

        ch->send_line("Lowering a player's level!");
        victim->send_line("**** OOOOHHHHHHHHHH  NNNNOOOO ****");
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
        ch->send_line("Raising a player's level!");
        victim->send_line("|R**** OOOOHHHHHHHHHH  YYYYEEEESSS ****|w");
    }

    if (ch->level > victim->level) {
        for (iLevel = victim->level; iLevel < level; iLevel++) {
            victim->send_line("You raise a level!!  ");
            victim->level += 1;
            advance_level(victim);
        }
        victim->exp = (exp_per_level(victim, victim->pcdata->points) * UMAX(1, victim->level));
        victim->trust = 0;
        save_char_obj(victim);
    }
}

void do_trust(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    int level;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((arg1[0] == '\0' && arg2[0] == '\0') || (arg2[0] != '\0' && !is_number(arg2))) {
        ch->send_line("Syntax: trust <char> <level>.");
        return;
    }

    if ((victim = get_char_room(ch, arg1)) == nullptr) {
        ch->send_line("That player is not here.");
        return;
    }

    if (arg2[0] == '\0') {
        ch->send_line("{} has a trust of {}.", victim->name, victim->trust);
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPCs.");
        return;
    }

    if ((level = atoi(arg2)) < 0 || level > 100) {
        ch->send_line("Level must be 0 (reset) or 1 to 100.");
        return;
    }

    if (level > ch->get_trust()) {
        ch->send_line("Limited to your trust.");
        return;
    }

    victim->trust = level;
}

void do_restore(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    Char *vch;

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

        ch->send_line("Room restored.");
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
        ch->send_line("All active players restored.");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
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
    ch->send_line("Ok.");
}

void do_freeze(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Freeze whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (IS_SET(victim->act, PLR_FREEZE)) {
        REMOVE_BIT(victim->act, PLR_FREEZE);
        victim->send_line("You can play again.");
        ch->send_line("FREEZE removed.");
    } else {
        SET_BIT(victim->act, PLR_FREEZE);
        victim->send_line("You can't do ANYthing!");
        ch->send_line("FREEZE set.");
    }

    save_char_obj(victim);
}

void do_log(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Log whom?");
        return;
    }

    if (!str_cmp(arg, "all")) {
        if (fLogAll) {
            fLogAll = false;
            ch->send_line("Log ALL off.");
        } else {
            fLogAll = true;
            ch->send_line("Log ALL on.");
        }
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (IS_SET(victim->act, PLR_LOG)) {
        REMOVE_BIT(victim->act, PLR_LOG);
        ch->send_line("LOG removed.");
    } else {
        SET_BIT(victim->act, PLR_LOG);
        ch->send_line("LOG set.");
    }
}

void do_noemote(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Noemote whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (IS_SET(victim->comm, COMM_NOEMOTE)) {
        REMOVE_BIT(victim->comm, COMM_NOEMOTE);
        victim->send_line("You can emote again.");
        ch->send_line("NOEMOTE removed.");
    } else {
        SET_BIT(victim->comm, COMM_NOEMOTE);
        victim->send_line("You can't emote!");
        ch->send_line("NOEMOTE set.");
    }
}

void do_noshout(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Noshout whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (IS_SET(victim->comm, COMM_NOSHOUT)) {
        REMOVE_BIT(victim->comm, COMM_NOSHOUT);
        victim->send_line("You can shout again.");
        ch->send_line("NOSHOUT removed.");
    } else {
        SET_BIT(victim->comm, COMM_NOSHOUT);
        victim->send_line("You can't shout!");
        ch->send_line("NOSHOUT set.");
    }
}

void do_notell(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Notell whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->get_trust() >= ch->get_trust()) {
        ch->send_line("You failed.");
        return;
    }

    if (IS_SET(victim->comm, COMM_NOTELL)) {
        REMOVE_BIT(victim->comm, COMM_NOTELL);
        victim->send_line("You can tell again.");
        ch->send_line("NOTELL removed.");
    } else {
        SET_BIT(victim->comm, COMM_NOTELL);
        victim->send_line("You can't tell!");
        ch->send_line("NOTELL set.");
    }
}

void do_peace(Char *ch, const char *argument) {
    (void)argument;
    for (auto *rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (rch->fighting)
            stop_fighting(rch, true);
        if (rch->is_npc() && IS_SET(rch->act, ACT_AGGRESSIVE))
            REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
        if (rch->is_npc())
            rch->sentient_victim.clear();
    }

    ch->send_line("Ok.");
}

void do_awaken(Char *ch, const char *argument) {
    Char *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Awaken whom?");
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }
    if (IS_AWAKE(victim)) {
        ch->send_line("Duh!  They're not even asleep!");
        return;
    }

    if (ch == victim) {
        ch->send_line("Duh!  If you wanna wake up, get COFFEE!");
        return;
    }

    REMOVE_BIT(victim->affected_by, AFF_SLEEP);
    victim->position = POS_STANDING;

    act("$n gives $t a kick, and wakes them up.", ch, victim->short_descr, nullptr, To::Room, POS_RESTING);
}

void do_owhere(Char *ch, const char *argument) {
    char target_name[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0;

    found = false;
    number = 0;

    if (argument[0] == '\0') {
        ch->send_line("Owhere which object?");
        return;
    }
    if (strlen(argument) < 2) {
        ch->send_line("Please be more specific.");
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
            buffer += fmt::format("{:3} {:<25} carried by {:<20} in room {}\n\r", number, obj->short_descr,
                                  pers(in_obj->carried_by, ch), in_obj->carried_by->in_room->vnum);
        } else if (in_obj->in_room != nullptr) {
            buffer += fmt::format("{:3} {:<25} in {:<30} [{}]\n\r", number, obj->short_descr, in_obj->in_room->name,
                                  in_obj->in_room->vnum);
        }
    }

    ch->page_to(buffer);
    if (!found)
        ch->send_line("Nothing like that in heaven or earth.");
}

/* Death's command */
void do_coma(Char *ch, const char *argument) {
    Char *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Comatoze whom?");
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLEEP))
        return;

    if (ch == victim) {
        ch->send_line("Duh!  Don't you dare fall asleep on the job!");
        return;
    }
    if ((ch->get_trust() <= victim->get_trust()) || !((ch->is_immortal()) && victim->is_npc())) {
        ch->send_line("You failed.");
        return;
    }

    AFFECT_DATA af;
    af.type = 38; /* SLEEP */ // TODO 38? really?
    af.level = ch->trust;
    af.duration = 4 + ch->trust;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, af);

    if (IS_AWAKE(victim)) {
        victim->send_line("You feel very sleepy ..... zzzzzz.");
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

void osearch_display_syntax(Char *ch) {
    char buf[MAX_STRING_LENGTH];
    ch->send_line("Syntax: osearch [min level] [max level] [item type] optional item name...");
    ch->send_line("        Level range no greater than 10. Item types:\n\r        ");
    ch->send_to(osearch_list_item_types(buf));
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
std::string osearch_find_items(const int min_level, const int max_level, const sh_int item_type, char *item_name) {
    std::string buffer;
    for (int i = 0; i < MAX_KEY_HASH; i++) {
        for (OBJ_INDEX_DATA *pIndexData = obj_index_hash[i]; pIndexData != nullptr; pIndexData = pIndexData->next) {
            if (!(osearch_is_item_in_level_range(pIndexData, min_level, max_level)
                  && osearch_is_item_type(pIndexData, item_type))) {
                continue;
            }
            if (item_name[0] != '\0' && !is_name(item_name, pIndexData->name)) {
                continue;
            }
            buffer += fmt::format("{:5} {:<27}|w ({:3}) {}\n\r", pIndexData->vnum, pIndexData->short_descr,
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
void do_osearch(Char *ch, const char *argument) {
    char min_level_str[MAX_INPUT_LENGTH];
    char max_level_str[MAX_INPUT_LENGTH];
    char item_type_str[MAX_INPUT_LENGTH];
    char item_name[MAX_INPUT_LENGTH];
    int min_level;
    int max_level;
    sh_int item_type;
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
    ch->page_to(osearch_find_items(min_level, max_level, item_type, item_name));
}

void do_slookup(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Lookup which skill or spell?");
        return;
    }

    if (!str_cmp(arg, "all")) {
        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name == nullptr)
                break;
            bug_snprintf(buf, sizeof(buf), "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn, skill_table[sn].slot,
                         skill_table[sn].name);
            ch->send_to(buf);
        }
    } else {
        if ((sn = skill_lookup(arg)) < 0) {
            ch->send_line("No such skill or spell.");
            return;
        }

        bug_snprintf(buf, sizeof(buf), "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn, skill_table[sn].slot,
                     skill_table[sn].name);
        ch->send_to(buf);
    }
}

/* RT set replaces sset, mset, oset, and rset */

void do_set(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set mob   <name> <field> <value>");
        ch->send_line("  set obj   <name> <field> <value>");
        ch->send_line("  set room  <room> <field> <value>");
        ch->send_line("  set skill <name> <spell or skill> <value>");
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

void do_sset(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Char *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set skill <name> <spell or skill> <value>");
        ch->send_line("  set skill <name> all <value>");
        ch->send_line("   (use the name of the skill, not the number)");
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_npc()) {
        ch->send_line("Not on NPC's.");
        return;
    }

    fAll = !str_cmp(arg2, "all");
    sn = 0;
    if (!fAll && (sn = skill_lookup(arg2)) < 0) {
        ch->send_line("No such skill or spell.");
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3)) {
        ch->send_line("Value must be numeric.");
        return;
    }

    value = atoi(arg3);
    if (value < 0 || value > 100) {
        ch->send_line("Value range is 0 to 100.");
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

void do_mset(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[100];
    Char *victim;
    int value;

    char smash_tilded[MAX_INPUT_LENGTH];
    strncpy(smash_tilded, smash_tilde(argument).c_str(),
            MAX_INPUT_LENGTH - 1); // TODO to minimize changes during refactor
    auto *args = smash_tilded;
    args = one_argument(args, arg1);
    args = one_argument(args, arg2);
    strcpy(arg3, args);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
        ch->send_line("Syntax:");
        ch->send_line("  set char <name> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    str int wis dex con sex class level");
        ch->send_line("    race gold hp mana move practice align");
        ch->send_line("    dam hit train thirst drunk full hours");
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        ch->send_line("They aren't here.");
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
            ch->send_to(buf);
            return;
        }

        victim->perm_stat[Stat::Str] = value;
        return;
    }

    if (!str_cmp(arg2, "int")) {
        if (value < 3 || value > get_max_train(victim, Stat::Int)) {
            bug_snprintf(buf, sizeof(buf), "Intelligence range is 3 to %d.\n\r", get_max_train(victim, Stat::Int));
            ch->send_to(buf);
            return;
        }

        victim->perm_stat[Stat::Int] = value;
        return;
    }

    if (!str_cmp(arg2, "wis")) {
        if (value < 3 || value > get_max_train(victim, Stat::Wis)) {
            bug_snprintf(buf, sizeof(buf), "Wisdom range is 3 to %d.\n\r", get_max_train(victim, Stat::Wis));
            ch->send_to(buf);
            return;
        }

        victim->perm_stat[Stat::Wis] = value;
        return;
    }

    if (!str_cmp(arg2, "dex")) {
        if (value < 3 || value > get_max_train(victim, Stat::Dex)) {
            bug_snprintf(buf, sizeof(buf), "Dexterity ranges is 3 to %d.\n\r", get_max_train(victim, Stat::Dex));
            ch->send_to(buf);
            return;
        }

        victim->perm_stat[Stat::Dex] = value;
        return;
    }

    if (!str_cmp(arg2, "con")) {
        if (value < 3 || value > get_max_train(victim, Stat::Con)) {
            bug_snprintf(buf, sizeof(buf), "Constitution range is 3 to %d.\n\r", get_max_train(victim, Stat::Con));
            ch->send_to(buf);
            return;
        }

        victim->perm_stat[Stat::Con] = value;
        return;
    }

    if (!str_prefix(arg2, "sex")) {
        if (value < 0 || value > 2) {
            ch->send_line("Sex range is 0 to 2.");
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
            ch->send_line("Mobiles have no class.");
            return;
        }

        class_num = class_lookup(arg3);
        if (class_num == -1) {
            strcpy(buf, "Possible classes are: ");
            for (class_num = 0; class_num < MAX_CLASS; class_num++) {
                if (class_num > 0)
                    strcat(buf, " ");
                strcat(buf, class_table[class_num].name);
            }
            strcat(buf, ".\n\r");

            ch->send_to(buf);
            return;
        }

        victim->class_num = class_num;
        return;
    }

    if (!str_prefix(arg2, "level")) {
        if (victim->is_pc()) {
            ch->send_line("Not on PC's.");
            return;
        }

        if (value < 0 || value > 200) {
            ch->send_line("Level range is 0 to 200.");
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
            ch->send_line("Hp range is 1 to 30,000 hit points.");
            return;
        }
        victim->max_hit = value;
        if (victim->is_pc())
            victim->pcdata->perm_hit = value;
        return;
    }

    if (!str_prefix(arg2, "mana")) {
        if (value < 0 || value > 30000) {
            ch->send_line("Mana range is 0 to 30,000 mana points.");
            return;
        }
        victim->max_mana = value;
        if (victim->is_pc())
            victim->pcdata->perm_mana = value;
        return;
    }

    if (!str_prefix(arg2, "move")) {
        if (value < 0 || value > 30000) {
            ch->send_line("Move range is 0 to 30,000 move points.");
            return;
        }
        victim->max_move = value;
        if (victim->is_pc())
            victim->pcdata->perm_move = value;
        return;
    }

    if (!str_prefix(arg2, "practice")) {
        if (value < 0 || value > 250) {
            ch->send_line("Practice range is 0 to 250 sessions.");
            return;
        }
        victim->practice = value;
        return;
    }

    if (!str_prefix(arg2, "train")) {
        if (value < 0 || value > 50) {
            ch->send_line("Training session range is 0 to 50 sessions.");
            return;
        }
        victim->train = value;
        return;
    }

    if (!str_prefix(arg2, "align")) {
        if (value < -1000 || value > 1000) {
            ch->send_line("Alignment range is -1000 to 1000.");
            return;
        }
        victim->alignment = value;
        return;
    }

    if (!str_cmp(arg2, "dam")) {
        if (value < 1 || value > 100) {
            bug_snprintf(buf, sizeof(buf), "|RDamroll range is 1 to 100.|w\n\r");
            ch->send_to(buf);
            return;
        }

        victim->damroll = value;
        return;
    }

    if (!str_cmp(arg2, "hit")) {
        if (value < 1 || value > 100) {
            bug_snprintf(buf, sizeof(buf), "|RHitroll range is 1 to 100.|w\n\r");
            ch->send_to(buf);
            return;
        }

        victim->hitroll = value;
        return;
    }

    if (!str_prefix(arg2, "thirst")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }

        if (value < -1 || value > 100) {
            ch->send_line("Thirst range is -1 to 100.");
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        return;
    }

    if (!str_prefix(arg2, "drunk")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }

        if (value < -1 || value > 100) {
            ch->send_line("Drunk range is -1 to 100.");
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if (!str_prefix(arg2, "full")) {
        if (victim->is_npc()) {
            ch->send_line("Not on NPC's.");
            return;
        }

        if (value < -1 || value > 100) {
            ch->send_line("Full range is -1 to 100.");
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        return;
    }

    if (!str_prefix(arg2, "race")) {
        int race;

        race = race_lookup(arg3);

        if (race == 0) {
            ch->send_line("That is not a valid race.");
            return;
        }

        if (victim->is_pc() && !race_table[race].pc_race) {
            ch->send_line("That is not a valid player race.");
            return;
        }

        victim->race = race;
        return;
    }

    if (!str_cmp(arg2, "hours")) {
        if (value < 1 || value > 999) {
            bug_snprintf(buf, sizeof(buf), "|RHours range is 1 to 999.|w\n\r");
            ch->send_to(buf);
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

void do_string(Char *ch, const char *argument) {
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Char *victim;
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
        ch->send_line("Syntax:");
        ch->send_line("  string char <name> <field> <string>");
        ch->send_line("    fields: name short long desc title spec");
        ch->send_line("  string obj  <name> <field> <string>");
        ch->send_line("    fields: name short long extended wear");
        return;
    }

    if (!str_prefix(type, "character") || !str_prefix(type, "mobile")) {
        if ((victim = get_char_world(ch, arg1)) == nullptr) {
            ch->send_line("They aren't here.");
            return;
        }

        /* string something */

        if (!str_prefix(arg2, "name")) {
            if (victim->is_pc()) {
                ch->send_line("Not on PC's.");
                return;
            }

            victim->name = arg3;
            return;
        }

        if (!str_prefix(arg2, "description")) {
            victim->description = arg3;
            return;
        }

        if (!str_prefix(arg2, "short")) {
            victim->short_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "long")) {
            victim->long_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "title")) {
            if (victim->is_npc()) {
                ch->send_line("Not on NPC's.");
                return;
            }

            victim->set_title(arg3);
            return;
        }

        if (!str_prefix(arg2, "spec")) {
            if (victim->is_pc()) {
                ch->send_line("Not on PC's.");
                return;
            }

            if ((victim->spec_fun = spec_lookup(arg3)) == 0) {
                ch->send_line("No such spec fun.");
                return;
            }

            return;
        }
    }

    if (!str_prefix(type, "object")) {
        /* string an obj */

        if ((obj = get_obj_world(ch, arg1)) == nullptr) {
            ch->send_line("Nothing like that in heaven or earth.");
            return;
        }

        if (!str_prefix(arg2, "name")) {
            free_string(obj->name);
            obj->name = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "short")) {
            obj->short_descr = arg3;
            return;
        }

        if (!str_prefix(arg2, "long")) {
            obj->description = arg3;
            return;
        }

        if (!str_prefix(arg2, "wear")) {
            if (strlen(arg3) > 17) {
                ch->send_line("Wear_Strings may not be longer than 17 chars.");
            } else {
                obj->wear_string = arg3;
            }
            return;
        }

        if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended")) {
            EXTRA_DESCR_DATA *ed;

            args = one_argument(args, arg3);
            if (args == nullptr) {
                ch->send_line("Syntax: oset <object> ed <keyword> <string>");
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

void do_oset(Char *ch, const char *argument) {
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
        ch->send_line("Syntax:");
        ch->send_line("  set obj <object> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    value0 value1 value2 value3 value4 (v1-v4)");
        ch->send_line("    extra wear level weight cost timer nolocate");
        ch->send_line("    (for extra and wear, use set obj <o> <extra/wear> ? to list");
        return;
    }

    if ((obj = get_obj_world(ch, arg1)) == nullptr) {
        ch->send_line("Nothing like that in heaven or earth.");
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

        ch->send_line("Current extra flags are: ");
        obj->extra_flags = (int)flag_set(ITEM_EXTRA_FLAGS, arg3, obj->extra_flags, ch);
        return;
    }

    if (!str_prefix(arg2, "wear")) {
        ch->send_line("Current wear flags are: ");
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

void do_rset(Char *ch, const char *argument) {
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
        ch->send_line("Syntax:");
        ch->send_line("  set room <location> <field> <value>");
        ch->send_line("  Field being one of:");
        ch->send_line("    flags sector");
        ch->send_line("  (use set room <location> flags ? to list flags");
        return;
    }

    if ((location = find_location(ch, arg1)) == nullptr) {
        ch->send_line("No such location.");
        return;
    }

    if (!str_prefix(arg2, "flags")) {
        ch->send_line("The current room flags are:");
        location->room_flags = (int)flag_set(ROOM_FLAGS, arg3, location->room_flags, ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3)) {
        ch->send_line("Value must be numeric.");
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

void do_sockets(Char *ch, const char *argument) {
    std::string buf;
    char arg[MAX_INPUT_LENGTH];
    int count = 0;

    one_argument(argument, arg);
    const auto view_all = arg[0] == '\0';
    for (auto &d : descriptors().all()) {
        std::string_view name;
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
        if (!name.empty()) {
            count++;
            buf += fmt::format("[{:3} {:>5}] {}@{}\n\r", d.channel(), short_name_of(d.state()), name, d.host());
        }
    }
    if (count == 0) {
        ch->send_line("No one by that name is connected.");
        return;
    }

    buf += fmt::format("{} user{}\n\r", count, count == 1 ? "" : "s");
    ch->desc->page_to(buf);
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        ch->send_line("Force whom to do what?");
        return;
    }

    one_argument(argument, arg2);

    if (!str_cmp(arg2, "delete")) {
        ch->send_line("That will NOT be done.");
        return;
    }

    if (!str_cmp(arg2, "private")) {
        ch->send_line("That will NOT be done.");
        return;
    }

    bug_snprintf(buf, sizeof(buf), "$n forces you to '%s'.", argument);

    if (!str_cmp(arg, "all")) {
        Char *vch;
        Char *vch_next;

        if (ch->get_trust() < DEITY) {
            ch->send_line("Not at your level!");
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
        Char *vch;
        Char *vch_next;

        if (ch->get_trust() < SUPREME) {
            ch->send_line("Not at your level!");
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
        Char *vch;
        Char *vch_next;

        if (ch->get_trust() < SUPREME) {
            ch->send_line("Not at your level!");
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
        Char *victim;

        if ((victim = get_char_world(ch, arg)) == nullptr) {
            ch->send_line("They aren't here.");
            return;
        }

        if (victim == ch) {
            ch->send_line("Aye aye, right away!");
            return;
        }

        if (victim->get_trust() >= ch->get_trust()) {
            ch->send_line("Do it yourself!");
            return;
        }

        if (victim->is_pc() && ch->get_trust() < DEITY) {
            ch->send_line("Not at your level!");
            return;
        }
        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        MOBtrigger = false;
        act(buf, ch, nullptr, victim, To::Vict);
        interpret(victim, argument);
    }

    ch->send_line("Ok.");
}

/*
 * New routines by Dionysos.
 */
void do_invis(Char *ch, const char *argument) {
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
            ch->send_line("You slowly fade back into existence.");
        } else {
            SET_BIT(ch->act, PLR_WIZINVIS);
            if (IS_SET(ch->act, PLR_PROWL))
                REMOVE_BIT(ch->act, PLR_PROWL);
            ch->invis_level = ch->get_trust();
            act("$n slowly fades into thin air.", ch);
            ch->send_line("You slowly vanish into thin air.");
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
            ch->send_line("Invis level must be between 2 and your level.");
            return;
        } else {
            ch->reply = nullptr;
            SET_BIT(ch->act, PLR_WIZINVIS);
            ch->invis_level = level;
            act("$n slowly fades into thin air.", ch);
            ch->send_line("You slowly vanish into thin air.");
        }
    }
}

void do_prowl(Char *ch, const char *argument) {
    char arg[MAX_STRING_LENGTH];
    int level = 0;

    if (ch->is_npc())
        return;

    if (ch->get_trust() < LEVEL_HERO) {
        ch->send_line("Huh?");
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
            ch->send_line("You slowly fade back into existence.");
            return;
        } else {
            ch->invis_level = ch->get_trust();
            SET_BIT(ch->act, PLR_PROWL);
            if (ch->pet != nullptr) {
                ch->pet->invis_level = ch->get_trust();
                SET_BIT(ch->pet->act, PLR_PROWL);
            }
            act("$n slowly fades into thin air.", ch);
            ch->send_line("You slowly vanish into thin air.");
            REMOVE_BIT(ch->act, PLR_WIZINVIS);
            if (ch->pet != nullptr)
                REMOVE_BIT(ch->pet->act, PLR_WIZINVIS);
            return;
        }
    }

    level = atoi(arg);

    if ((level > ch->get_trust()) || (level < 2)) {
        ch->send_line("You must specify a level between 2 and your level.");
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
        ch->send_line("You slowly fade back into existence.");
        return;
    } else {
        ch->invis_level = level;
        SET_BIT(ch->act, PLR_PROWL);
        if (ch->pet != nullptr) {
            SET_BIT(ch->pet->act, PLR_PROWL);
            ch->pet->invis_level = level;
        }
        act("$n slowly fades into thin air.", ch);
        ch->send_line("You slowly vanish into thin air.");
        REMOVE_BIT(ch->act, PLR_WIZINVIS);
        if (ch->pet != nullptr)
            REMOVE_BIT(ch->pet->act, PLR_WIZINVIS);
        return;
    }
}

void do_holylight(Char *ch, const char *argument) {
    (void)argument;
    if (ch->is_npc())
        return;

    if (IS_SET(ch->act, PLR_HOLYLIGHT)) {
        REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
        ch->send_line("Holy light mode off.");
    } else {
        SET_BIT(ch->act, PLR_HOLYLIGHT);
        ch->send_line("Holy light mode on.");
    }
}

void do_sacname(Char *ch, const char *argument) {

    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        ch->send_line("You must tell me who they're gonna sacrifice to!");
        bug_snprintf(buf, sizeof(buf), "Currently sacrificing to: %s\n\r", deity_name);
        ch->send_to(buf);
        return;
    }
    strcpy(deity_name_area, argument);
    bug_snprintf(buf, sizeof(buf), "Players now sacrifice to %s.\n\r", deity_name);
    ch->send_to(buf);
}
