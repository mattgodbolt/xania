/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  xania.c: a variety of Xania-specific modifications and new utilities */
/*                                                                       */
/*************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "ArgParser.hpp"
#include "BitsAffect.hpp"
#include "BitsCharAct.hpp"
#include "BitsObjectWear.hpp"
#include "Char.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "MobIndexData.hpp"
#include "Object.hpp"
#include "WearLocation.hpp"
#include "comm.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "mob_prog.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstdlib>

/*
 * Local functions.
 */

Room *find_location(Char *ch, std::string_view arg);

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. It allows the words to show up in mpstat to
 *  make it just a hair bit easier to see what a mob should be doing.
 */

const char *mprog_type_to_name(int type) {
    switch (type) {
    case IN_FILE_PROG: return "in_file_prog";
    case ACT_PROG: return "act_prog";
    case SPEECH_PROG: return "speech_prog";
    case RAND_PROG: return "rand_prog";
    case FIGHT_PROG: return "fight_prog";
    case HITPRCNT_PROG: return "hitprcnt_prog";
    case DEATH_PROG: return "death_prog";
    case ENTRY_PROG: return "entry_prog";
    case GREET_PROG: return "greet_prog";
    case ALL_GREET_PROG: return "all_greet_prog";
    case GIVE_PROG: return "give_prog";
    case BRIBE_PROG: return "bribe_prog";
    default: return "ERROR_PROG";
    }
}

/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MOBprograms which are set.
 */

void do_mpstat(Char *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MPROG_DATA *mprg;
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("MobProg stat whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_pc()) {
        ch->send_line("Only Mobiles can have Programs!");
        return;
    }

    if (!(victim->pIndexData->progtypes)) {
        ch->send_line("That Mobile has no Programs set.");
        return;
    }

    ch->send_line("Name: {}.  Vnum: {}.", victim->name, victim->pIndexData->vnum);

    ch->send_to(fmt::format("Short description:{}.\n\rLong  description: {}", victim->short_descr,
                            victim->long_descr.empty() ? "(none).\n\r" : victim->long_descr));

    snprintf(buf, sizeof(buf), "Hp: %d/%d.  Mana: %d/%d.  Move: %d/%d. \n\r", victim->hit, victim->max_hit,
             victim->mana, victim->max_mana, victim->move, victim->max_move);
    ch->send_to(buf);

    snprintf(buf, sizeof(buf), "Lv: %d.  Class: %d.  Align: %d.   Gold: %ld.  Exp: %ld.\n\r", victim->level,
             victim->class_num, victim->alignment, victim->gold, victim->exp);
    ch->send_to(buf);

    for (mprg = victim->pIndexData->mobprogs; mprg != nullptr; mprg = mprg->next) {
        snprintf(buf, sizeof(buf), ">%s %s\n\r%s\n\r", mprog_type_to_name(mprg->type), mprg->arglist, mprg->comlist);
        ch->send_to(buf);
    }
}

/* prints the argument to all the rooms aroud the mobile */

void do_mpasound(Char *ch, const char *argument) {

    Room *was_in_room;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    if (argument[0] == '\0') {
        bug("Mpasound - No argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    was_in_room = ch->in_room;
    for (auto door : all_directions) {
        Exit *pexit;

        if ((pexit = was_in_room->exit[door]) != nullptr && pexit->u1.to_room != nullptr
            && pexit->u1.to_room != was_in_room) {
            ch->in_room = pexit->u1.to_room;
            MOBtrigger = false;
            act(argument, ch);
        }
    }

    ch->in_room = was_in_room;
}

/* lets the mobile kill any player or mobile without murder*/

void do_mpkill(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        bug("MpKill - No argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        bug("MpKill - Victim not in room from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (victim == ch) {
        bug("MpKill - Bad victim to attack from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        bug("MpKill - Charmed mob attacking master from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (ch->is_pos_fighting()) {
        bug("MpKill - Already fighting from vnum {}", ch->pIndexData->vnum);
        return;
    }

    multi_hit(ch, victim);
}

/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   items using all.xxxxx or just plain all of them */

void do_mpjunk(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        bug("Mpjunk - No argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        auto *obj = get_obj_wear(ch, arg);
        if (obj) {
            unequip_char(ch, obj);
            extract_obj(obj);
            return;
        }
        if ((obj = get_obj_carry(ch, arg)) == nullptr)
            return;
        extract_obj(obj);
    } else {
        for (auto *obj : ch->carrying) {
            if (arg[3] == '\0' || is_name(&arg[4], obj->name)) {
                if (obj->wear_loc != WEAR_NONE)
                    unequip_char(ch, obj);
                extract_obj(obj);
            }
        }
    }
}

/* prints the message to everyone in the room other than the mob and victim */

void do_mpechoaround(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        bug("Mpechoaround - No argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (!(victim = get_char_room(ch, arg))) {
        bug("Mpechoaround - Victim does not exist from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    act(argument, ch, nullptr, victim, To::NotVict);
}

/* prints the message to only the victim */

void do_mpechoat(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("Mpechoat - No argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (!(victim = get_char_room(ch, arg))) {
        bug("Mpechoat - Victim does not exist from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    act(argument, ch, nullptr, victim, To::Vict);
}

/* prints the message to the room at large */

void do_mpecho(Char *ch, const char *argument) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    if (argument[0] == '\0') {
        bug("Mpecho - Called w/o argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    act(argument, ch);
}

/* lets the mobile load an item or mobile.  All items
are loaded into inventory.  you can specify a level with
the load object portion as well. */

void do_mpmload(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    MobIndexData *pMobIndex;
    Char *victim;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        bug("Mpmload - Bad vnum as arg from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if ((pMobIndex = get_mob_index(atoi(arg))) == nullptr) {
        bug("Mpmload - Bad mob vnum from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    victim = create_mobile(pMobIndex);
    char_to_room(victim, ch->in_room);
}

void do_mpoload(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ObjectIndex *objIndex;
    Object *obj;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        bug("Mpoload - Bad syntax from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if ((objIndex = get_obj_index(atoi(arg1))) == nullptr) {
        bug("Mpoload - Bad vnum arg from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    obj = create_object(objIndex);
    if (CAN_WEAR(obj, ITEM_TAKE)) {
        obj_to_char(obj, ch);
    } else {
        obj_to_room(obj, ch->in_room);
    }
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MOBprogram
   otherwise ugly stuff will happen */

void do_mppurge(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* 'purge' */
        for (auto *victim : ch->in_room->people) {
            if (victim->is_npc() && victim != ch)
                extract_char(victim, true);
        }

        for (auto *obj : ch->in_room->contents)
            extract_obj(obj);

        return;
    }

    auto *victim = get_char_room(ch, arg);
    if (!victim) {
        auto *obj = get_obj_here(ch, arg);
        if (obj) {
            extract_obj(obj);
        } else {
            bug("Mppurge - Bad argument from vnum {}.", ch->pIndexData->vnum);
        }
        return;
    }

    if (victim->is_pc()) {
        bug("Mppurge - Purging a PC from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    extract_char(victim, true);
}

/* lets the mobile goto any location it wishes that is not private */

void do_mpgoto(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Room *location;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        bug("Mpgoto - No argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if ((location = find_location(ch, arg)) == nullptr) {
        bug("Mpgoto - No such location from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (ch->fighting != nullptr)
        stop_fighting(ch, true);

    char_from_room(ch);
    char_to_room(ch, location);
}

/* lets the mobile do a command at another location. Very useful */

void do_mpat(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Room *location;
    Room *original;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("Mpat - Bad argument from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if ((location = find_location(ch, arg)) == nullptr) {
        bug("Mpat - No such location from vnum {}.", ch->pIndexData->vnum);
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
    for (auto *wch : char_list) {
        if (wch == ch) {
            char_from_room(ch);
            char_to_room(ch, original);
            break;
        }
    }
}

/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location */

void do_mptransfer(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Room *location;

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        bug("Mptransfer - Bad syntax from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (!str_cmp(arg1, "all")) {
        for (auto &victim :
             descriptors().all_visible_to(*ch) | DescriptorFilter::except(*ch) | DescriptorFilter::to_character()) {
            if (victim.in_room != nullptr) {
                do_transfer(ch, ArgParser(fmt::format("{} {}", victim.name, arg2)));
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
            bug("Mptransfer - No such location from vnum {}.", ch->pIndexData->vnum);
            return;
        }

        if (room_is_private(location)) {
            bug("Mptransfer - Private room from vnum {}.", ch->pIndexData->vnum);
            return;
        }
    }

    Char *victim;
    if ((victim = get_char_world(ch, arg1)) == nullptr) {
        bug("Mptransfer - No such person from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (victim->in_room == nullptr) {
        bug("Mptransfer - Victim in Limbo from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (victim->fighting != nullptr)
        stop_fighting(victim, true);

    char_from_room(victim);
    char_to_room(victim, location);
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

void do_mpforce(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("Mpforce - Bad syntax from vnum {}.", ch->pIndexData->vnum);
        return;
    }

    if (!str_cmp(arg, "all")) {
        for (auto *vch : char_list) {
            if (vch->in_room == ch->in_room && vch->get_trust() < ch->get_trust() && can_see(ch, vch))
                interpret(vch, argument);
        }
    } else {
        Char *victim;

        if ((victim = get_char_room(ch, arg)) == nullptr) {
            bug("Mpforce - No such victim from vnum {}.", ch->pIndexData->vnum);
            return;
        }

        if (victim == ch) {
            bug("Mpforce - Forcing oneself from vnum {}.", ch->pIndexData->vnum);
            return;
        }

        interpret(victim, argument);
    }
}
