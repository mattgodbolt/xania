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

#include "AffectFlag.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "Logging.hpp"
#include "MobIndexData.hpp"
#include "Object.hpp"
#include "ObjectWearFlag.hpp"
#include "Wear.hpp"
#include "comm.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "string_utils.hpp"

#include <fmt/format.h>

/*
 * Local functions.
 */

Room *find_location(Char *ch, std::string_view arg);

/* prints the argument to all the rooms aroud the mobile */

void do_mpasound(Char *ch, std::string_view argument) {

    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    if (argument.empty()) {
        bug("mpasound: No argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }

    auto *was_in_room = ch->in_room;
    for (auto direction : all_directions) {
        if (const auto &exit = was_in_room->exits[direction];
            exit && exit->u1.to_room != nullptr && exit->u1.to_room != was_in_room) {
            ch->in_room = exit->u1.to_room;
            act(argument, ch, To::Char, MobTrig::No);
        }
    }

    ch->in_room = was_in_room;
}

/* lets the mobile kill any player or mobile without murder*/

void do_mpkill(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpkill: No argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        bug("mpkill: Victim not in room from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    if (victim == ch) {
        bug("mpkill: Bad victim to attack from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    if (ch->is_aff_charm() && ch->master == victim) {
        bug("mpkill: Charmed mob attacking master from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    if (ch->is_pos_fighting()) {
        bug("mpkill: Already fighting from vnum {}", ch->mobIndex->vnum);
        return;
    }
    multi_hit(ch, victim);
}

/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   items using all.xxxxx or just plain all of them */

void do_mpjunk(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpjunk: No argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto obj_name = args.shift();
    const auto is_all = matches(obj_name, "all");
    const auto is_all_named = matches_start("all.", obj_name);
    if (is_all_named) {
        obj_name.remove_prefix(4);
    }
    if (!(is_all || is_all_named)) {
        // mpjunk obj
        auto *obj = ch->find_worn(obj_name);
        if (obj) {
            unequip_char(ch, obj);
            extract_obj(obj);
            return;
        }
        obj = ch->find_in_inventory(obj_name);
        if (obj)
            extract_obj(obj);
    } else {
        // mpjunk all,  or mpjunk all.obj
        for (auto *obj : ch->carrying) {
            if (is_all || is_name(obj_name, obj->name)) {
                if (obj->wear_loc != Wear::None)
                    unequip_char(ch, obj);
                extract_obj(obj);
            }
        }
    }
}

/* prints the message to everyone in the room other than the mob and victim */

void do_mpechoaround(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpechoaround: No argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto name = args.shift();
    auto *victim = get_char_room(ch, name);
    if (!victim) {
        bug("mpechoaround: Victim does not exist from vnum {}, arg: {}", ch->mobIndex->vnum, name);
        return;
    }
    act(args.remaining(), ch, nullptr, victim, To::NotVict);
}

/* prints the message to only the victim */

void do_mpechoat(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpechoat: No argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto name = args.shift();
    auto *victim = get_char_room(ch, name);
    if (!victim) {
        bug("mpechoat: Victim does not exist from vnum {}, arg: {}", ch->mobIndex->vnum, name);
        return;
    }
    act(args.remaining(), ch, nullptr, victim, To::Vict);
}

/* prints the message to the room at large */

void do_mpecho(Char *ch, std::string_view argument) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }

    if (argument.empty()) {
        bug("mpecho: Called w/o argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    act(argument, ch);
}

/* lets the mobile load an item or mobile.  All items
are loaded into inventory. */

void do_mpmload(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    const auto opt_vnum = args.try_shift_number();
    if (!opt_vnum) {
        bug("mpmload: Bad vnum as arg from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *mob_index = get_mob_index(*opt_vnum);
    if (!mob_index) {
        bug("mpmload: Bad mob vnum from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *victim = create_mobile(mob_index);
    char_to_room(victim, ch->in_room);
}

void do_mpoload(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    const auto opt_vnum = args.try_shift_number();
    if (!opt_vnum) {
        bug("mpoload: Bad syntax from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *obj_index = get_obj_index(*opt_vnum);
    if (!obj_index) {
        bug("mpoload: Bad vnum arg from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *obj = Object::create(obj_index);
    if (obj->is_takeable()) {
        obj_to_char(obj, ch);
    } else {
        obj_to_room(obj, ch->in_room);
    }
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MOBprogram
   otherwise ugly stuff will happen */

void do_mppurge(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        for (auto *victim : ch->in_room->people) {
            if (victim->is_npc() && victim != ch)
                extract_char(victim, true);
        }

        for (auto *obj : ch->in_room->contents)
            extract_obj(obj);

        return;
    }
    auto target = args.shift();
    auto *victim = get_char_room(ch, target);
    if (!victim) {
        auto *obj = get_obj_here(ch, target);
        if (obj) {
            extract_obj(obj);
        } else {
            bug("mppurge: Bad argument from vnum {}.", ch->mobIndex->vnum);
        }
    } else if (victim->is_npc()) {
        extract_char(victim, true);
    }
}

/* lets the mobile goto any location it wishes that is not private */

void do_mpgoto(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpgoto: No argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *location = find_location(ch, args.shift());
    if (!location) {
        bug("mpgoto: No such location from vnum {}.", ch->mobIndex->vnum);
        return;
    }

    if (ch->fighting)
        stop_fighting(ch, true);

    char_from_room(ch);
    char_to_room(ch, location);
}

/* lets the mobile do a command at another location. Very useful */

void do_mpat(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpat: Bad argument from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *location = find_location(ch, args.shift());
    if (!location) {
        bug("mpat: No such location from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto *original = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    interpret(ch, args.remaining());
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

void do_mptransfer(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("Mptransfer - Bad syntax from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto whom = args.shift();
    auto where = args.shift();
    if (matches(whom, "all")) {
        for (auto &victim :
             descriptors().all_visible_to(*ch) | DescriptorFilter::except(*ch) | DescriptorFilter::to_character()) {
            if (victim.in_room != nullptr) {
                do_transfer(ch, ArgParser(fmt::format("{} {}", victim.name, where)));
            }
        }
        return;
    }
    Room *location{};
    if (where.empty()) {
        location = ch->in_room;
    } else {
        if (!(location = find_location(ch, where))) {
            bug("mptransfer: No such location from vnum {}.", ch->mobIndex->vnum);
            return;
        }

        if (room_is_private(location)) {
            bug("mptransfer: Private room from vnum {}.", ch->mobIndex->vnum);
            return;
        }
    }
    auto *victim = get_char_world(ch, whom);
    if (!victim) {
        bug("mptransfer: No such person from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    if (!victim->in_room) {
        bug("mptransfer: Victim in Limbo from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    if (victim->fighting)
        stop_fighting(victim, true);

    char_from_room(victim);
    char_to_room(victim, location);
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

void do_mpforce(Char *ch, ArgParser args) {
    if (ch->is_pc()) {
        ch->send_line("Huh?");
        return;
    }
    if (args.empty()) {
        bug("mpforce: Bad syntax from vnum {}.", ch->mobIndex->vnum);
        return;
    }
    auto target = args.shift();
    if (matches(target, "all")) {
        for (auto *vch : char_list) {
            if (vch->in_room == ch->in_room && vch->get_trust() < ch->get_trust() && ch->can_see(*vch)) {
                interpret(vch, args.remaining());
            }
        }
    } else {
        auto *victim = get_char_room(ch, target);
        if (!victim) {
            bug("mpforce: No such victim from vnum {}.", ch->mobIndex->vnum);
            return;
        }
        if (victim == ch) {
            bug("mpforce: Forcing oneself from vnum {}.", ch->mobIndex->vnum);
            return;
        }
        interpret(victim, args.remaining());
    }
}
