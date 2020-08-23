/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_move.c: player and mobile movement                               */
/*                                                                       */
/*************************************************************************/

#include "challeng.h"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstring>

using namespace fmt::literals;

const sh_int movement_loss[SECT_MAX] = {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6};

/*
 * Local functions.
 */
int find_door(CHAR_DATA *ch, char *arg);
bool has_key(const CHAR_DATA *ch, int key);

void move_char(CHAR_DATA *ch, Direction door) {
    auto *in_room = ch->in_room;
    auto *pexit = in_room->exit[door];
    auto *to_room = pexit ? pexit->u1.to_room : nullptr;
    if (!pexit || !to_room || !ch->can_see(*to_room)) {
        send_to_char("Alas, you cannot go that way.\n\r", ch);
        return;
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch)) {
        if (IS_SET(pexit->exit_info, EX_PASSPROOF) && IS_AFFECTED(ch, AFF_PASS_DOOR)) {
            act("The $d is protected from trespass by a magical barrier.", ch, nullptr, pexit->keyword, To::Char);
            return;
        } else {
            if (!IS_AFFECTED(ch, AFF_PASS_DOOR)) {
                act("The $d is closed.", ch, nullptr, pexit->keyword, To::Char);
                return;
            }
        }
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED) && IS_AFFECTED(ch, AFF_PASS_DOOR)
        && !(IS_SET(pexit->exit_info, EX_PASSPROOF)) && ch->riding != nullptr
        && !(IS_AFFECTED(ch->riding, AFF_PASS_DOOR))) {
        send_to_char("Your mount cannot travel through solid objects.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != nullptr && in_room == ch->master->in_room) {
        send_to_char("What?  And leave your beloved master?\n\r", ch);
        return;
    }

    if (room_is_private(to_room) && (ch->get_trust() < IMPLEMENTOR)) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    if (!IS_NPC(ch)) {
        int iClass, iGuild;
        int move;

        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
                if (iClass != ch->class_num && to_room->vnum == class_table[iClass].guild[iGuild]) {
                    send_to_char("You aren't allowed in there.\n\r", ch);
                    return;
                }
            }
        }

        /* added Faramir 25/6/96 to stop non-clan members walking into room */

        if (!IS_IMMORTAL(ch)) {
            for (auto &clan : clantable) {
                if (to_room->vnum == clan.entrance_vnum && ch->clan() != &clan) {
                    ch->send_to("Only members of the {} may enter there.\n\r"_format(clan.name));
                    return;
                }
            }
        }

        if (in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR) {
            if ((!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
                && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))) {
                send_to_char("You can't fly.\n\r", ch);
                return;
            }
        }

        if ((in_room->sector_type == SECT_WATER_NOSWIM || to_room->sector_type == SECT_WATER_NOSWIM)
            && !IS_AFFECTED(ch, AFF_FLYING) && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))) {
            OBJ_DATA *obj;
            bool found;

            /*
             * Look for a boat.
             */
            found = false;

            if (IS_IMMORTAL(ch))
                found = true;

            for (obj = ch->carrying; obj != nullptr; obj = obj->next_content) {
                if (obj->item_type == ITEM_BOAT) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                send_to_char("You need a boat to go there.\n\r", ch);
                return;
            }
        }

        move = movement_loss[UMIN(SECT_MAX - 1, in_room->sector_type)]
               + movement_loss[UMIN(SECT_MAX - 1, to_room->sector_type)];

        move /= 2; /* i.e. the average */

        if (ch->riding == nullptr) {
            if (ch->move < move) {
                send_to_char("You are too exhausted.\n\r", ch);
                return;
            }

            WAIT_STATE(ch, 1);
            ch->move -= move;
        } else {
            move /= 2; /* Horses are better at moving ... well probably */
            if (ch->riding->move < move) {
                act("$N is too exhausted to carry you further.", ch, nullptr, ch->riding, To::Char);
                return;
            }
            WAIT_STATE(ch->riding, 1);
            ch->riding->move -= move;
        }
    }

    /* Check to see if mount is about to leave you behind */
    if (ch->ridden_by != nullptr) {
        if (ch->ridden_by->in_room != to_room) {
            /* Oh dear ... */
            /* MrG - shouldn't happen - this is the paranoia code */
            fallen_off_mount(ch);
        }
    }

    if (!(IS_AFFECTED(ch, AFF_SNEAK) && (ch->riding != nullptr))
        && (IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) || !IS_SET(ch->act, PLR_PROWL))) {
        if (ch->ridden_by == nullptr) {
            if (ch->riding == nullptr) {
                act("$n leaves $T.", ch, nullptr, to_string(door), To::Room);
            } else {
                act("$n rides $t on $N.", ch, to_string(door), ch->riding, To::Room);
                act("You ride $t on $N.", ch, to_string(door), ch->riding, To::Char);
            }
        }
    }

    char_from_room(ch);
    char_to_room(ch, to_room);

    if (!IS_AFFECTED(ch, AFF_SNEAK) && (IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) || !IS_SET(ch->act, PLR_PROWL))) {
        if (ch->ridden_by == nullptr) {
            if (ch->riding == nullptr) {
                act("$n has arrived.", ch);
            } else {
                act("$n has arrived, riding $N.", ch, nullptr, ch->riding, To::Room);
            }
        }
    }

    do_look(ch, "auto");

    if (in_room == to_room) /* no circular follows */
        return;

    decltype(in_room->people) fch_next{};
    for (auto *fch = in_room->people; fch != nullptr; fch = fch_next) {
        fch_next = fch->next_in_room;

        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
            do_stand(fch, "");

        if (fch->master == ch && fch->position == POS_STANDING) {

            if (IS_SET(ch->in_room->room_flags, ROOM_LAW) && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE))) {
                act("$N may not enter here.", ch, nullptr, fch, To::Char);
                act("You aren't allowed to go there.", fch, nullptr, nullptr, To::Char);
                return;
            }

            act("You follow $N.", fch, nullptr, ch, To::Char);
            move_char(fch, door);
        }
    }

    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */

    mprog_entry_trigger(ch);
    mprog_greet_trigger(ch);
}

void do_enter(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *to_room, *in_room;
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    int count = 0, number = number_argument(argument, arg);
    if (ch->riding) {
        send_to_char("Before entering a portal you must dismount.\n\r", ch);
        return;
    }
    in_room = ch->in_room;
    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        if (can_see_obj(ch, obj) && (is_name(arg, obj->name))) {
            if (++count == number) {
                if (obj->item_type == ITEM_PORTAL) {

                    to_room = obj->destination;

                    /* handle dangling portals, not that they should ever exist*/
                    if (to_room == nullptr) {
                        send_to_char("That portal is highly unstable and is likely to get you killed!", ch);
                        return;
                    }

                    if (IS_NPC(ch) && IS_SET(ch->act, ACT_AGGRESSIVE) && IS_SET(to_room->room_flags, ROOM_LAW)) {
                        send_to_char("Something prevents you from leaving...\n\r", ch);
                        return;
                    }

                    if (room_is_private(to_room) && (ch->get_trust() < IMPLEMENTOR)) {
                        send_to_char("That room is private right now.\n\r", ch);
                        return;
                    }

                    if (!IS_NPC(ch)) {
                        int iClass, iGuild;
                        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
                            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
                                if (iClass != ch->class_num && to_room->vnum == class_table[iClass].guild[iGuild]) {
                                    send_to_char("You aren't allowed in there.\n\r", ch);
                                    return;
                                }
                            }
                        }

                        if (!IS_IMMORTAL(ch)) {
                            for (auto &clan : clantable) {
                                if (to_room->vnum == clan.entrance_vnum && ch->clan() != &clan) {
                                    ch->send_to("Only members of the {} may enter there.\n\r"_format(clan.name));
                                    return;
                                }
                            }
                        }

                        if (in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR) {
                            if ((!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
                                && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))) {
                                send_to_char("You can't fly.\n\r", ch);
                                return;
                            }
                        }

                        if ((in_room->sector_type == SECT_WATER_NOSWIM || to_room->sector_type == SECT_WATER_NOSWIM)
                            && !IS_AFFECTED(ch, AFF_FLYING)
                            && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))) {
                            OBJ_DATA *obj;
                            bool found;

                            /*
                             * Look for a boat.
                             */
                            found = false;

                            if (IS_IMMORTAL(ch))
                                found = true;

                            for (obj = ch->carrying; obj != nullptr; obj = obj->next_content) {
                                if (obj->item_type == ITEM_BOAT) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                send_to_char("You need a boat to go there.\n\r", ch);
                                return;
                            }
                        }
                    }
                    act("$n steps through a portal and vanishes.", ch);
                    send_to_char("You step through a portal and vanish.\n\r", ch);
                    char_from_room(ch);
                    char_to_room(ch, obj->destination);
                    act("$n has arrived through a portal.", ch);
                    do_look(ch, "auto");

                    if (in_room == to_room) /* no circular follows */
                        return;

                    for (fch = in_room->people; fch != nullptr; fch = fch_next) {
                        fch_next = fch->next_in_room;

                        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
                            do_stand(fch, "");

                        if (fch->master == ch && fch->position == POS_STANDING) {

                            if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
                                && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE))) {
                                act("You can't bring $N into the city.", ch, nullptr, fch, To::Char);
                                act("You aren't allowed in the city.", fch, nullptr, nullptr, To::Char);
                                continue;
                            }

                            act("You follow $N.", fch, nullptr, ch, To::Char);
                            fch->in_room->light++; /* allows follower in dark to enter */
                            do_enter(fch, argument);
                            in_room->light--;
                        }
                    }

                    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */

                    mprog_entry_trigger(ch);
                    mprog_greet_trigger(ch);

                    return;
                } else {
                    send_to_char("That's not a portal!\n\r", ch);
                    return;
                }
            }
        }
    }
    send_to_char("You can't see that here.\n\r", ch);
}

void do_north(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, Direction::North);
}

void do_east(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, Direction::East);
}

void do_south(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, Direction::South);
}

void do_west(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, Direction::West);
}

void do_up(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, Direction::Up);
}

void do_down(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, Direction::Down);
}

std::optional<Direction> find_door(CHAR_DATA *ch, std::string_view arg) {
    if (auto door = try_parse_direction(arg)) {
        auto *pexit = ch->in_room->exit[*door];
        if (!pexit) {
            act("I see no door $T here.", ch, nullptr, arg, To::Char);
            return {};
        }

        if (!IS_SET(pexit->exit_info, EX_ISDOOR)) {
            send_to_char("You can't do that.\n\r", ch);
            return {};
        }

        return door;
    }

    for (auto door : all_directions) {
        if (auto *pexit = ch->in_room->exit[door];
            pexit && IS_SET(pexit->exit_info, EX_ISDOOR) && pexit->keyword != nullptr && is_name(arg, pexit->keyword))
            return door;
    }
    act("I see no $T here.", ch, nullptr, arg, To::Char);
    return {};
}

void do_open(CHAR_DATA *ch, const char *argument) {
    ArgParser args(argument);
    if (args.empty()) {
        send_to_char("Open what?\n\r", ch);
        return;
    }

    auto arg = args.pop_argument();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'open object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            send_to_char("It's already open.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSEABLE)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_LOCKED)) {
            send_to_char("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_CLOSED);
        send_to_char("Ok.\n\r", ch);
        act("$n opens $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'open door' */

        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            send_to_char("It's already open.\n\r", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED)) {
            send_to_char("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_CLOSED);
        act("$n opens the $d.", ch, nullptr, pexit->keyword, To::Room);
        send_to_char("Ok.\n\r", ch);

        /* open the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {
            CHAR_DATA *rch;

            REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != nullptr; rch = rch->next_in_room)
                act("The $d opens.", rch, nullptr, pexit_rev->keyword, To::Char);
        }
    }
}

void do_close(CHAR_DATA *ch, const char *argument) {
    ArgParser args(argument);
    if (args.empty()) {
        send_to_char("Close what?\n\r", ch);
        return;
    }

    auto arg = args.pop_argument();
    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'close object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_CLOSED)) {
            send_to_char("It's already closed.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSEABLE)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }

        SET_BIT(obj->value[1], CONT_CLOSED);
        send_to_char("Ok.\n\r", ch);
        act("$n closes $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'close door' */

        auto *pexit = ch->in_room->exit[door];
        if (IS_SET(pexit->exit_info, EX_CLOSED)) {
            send_to_char("It's already closed.\n\r", ch);
            return;
        }

        SET_BIT(pexit->exit_info, EX_CLOSED);
        act("$n closes the $d.", ch, nullptr, pexit->keyword, To::Room);
        send_to_char("Ok.\n\r", ch);

        /* close the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {
            CHAR_DATA *rch;

            SET_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != nullptr; rch = rch->next_in_room)
                act("The $d closes.", rch, nullptr, pexit_rev->keyword, To::Char);
        }
    }
}

bool has_key(const CHAR_DATA *ch, int key) {
    for (auto *obj = ch->carrying; obj != nullptr; obj = obj->next_content) {
        if (obj->pIndexData->vnum == key)
            return true;
    }

    return false;
}

void do_lock(CHAR_DATA *ch, const char *argument) {
    ArgParser args(argument);
    if (args.empty()) {
        send_to_char("Lock what?\n\r", ch);
        return;
    }
    auto arg = args.pop_argument();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'lock object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (obj->value[2] < 0) {
            send_to_char("It can't be locked.\n\r", ch);
            return;
        }
        if (!has_key(ch, obj->value[2])) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_LOCKED)) {
            send_to_char("It's already locked.\n\r", ch);
            return;
        }

        SET_BIT(obj->value[1], CONT_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n locks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'lock door' */

        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (pexit->key < 0) {
            send_to_char("It can't be locked.\n\r", ch);
            return;
        }
        if (!has_key(ch, pexit->key)) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED)) {
            send_to_char("It's already locked.\n\r", ch);
            return;
        }

        SET_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n locks the $d.", ch, nullptr, pexit->keyword, To::Room);

        /* lock the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {
            SET_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }
}

void do_unlock(CHAR_DATA *ch, const char *argument) {
    ArgParser args(argument);
    if (args.empty()) {
        send_to_char("Unlock what?\n\r", ch);
        return;
    }

    auto arg = args.pop_argument();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'unlock object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (obj->value[2] < 0) {
            send_to_char("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!has_key(ch, obj->value[2])) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n unlocks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'unlock door' */
        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (pexit->key < 0) {
            send_to_char("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!has_key(ch, pexit->key)) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (!IS_SET(pexit->exit_info, EX_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n unlocks the $d.", ch, nullptr, pexit->keyword, To::Room);

        /* unlock the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {
            REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }
}

void do_pick(CHAR_DATA *ch, const char *argument) {
    ArgParser args(argument);
    if (args.empty()) {
        send_to_char("Pick what?\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_pick_lock].beats);

    /* look for guards */
    for (auto *gch = ch->in_room->people; gch; gch = gch->next_in_room) {
        if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level) {
            act("$N is standing too close to the lock.", ch, nullptr, gch, To::Char);
            return;
        }
    }

    if (!IS_NPC(ch) && number_percent() > get_skill_learned(ch, gsn_pick_lock)) {
        send_to_char("You failed.\n\r", ch);
        check_improve(ch, gsn_pick_lock, false, 2);
        return;
    }

    auto arg = args.pop_argument();
    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'pick object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (obj->value[2] < 0) {
            send_to_char("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_PICKPROOF)) {
            send_to_char("You failed.\n\r", ch);
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        send_to_char("*Click*\n\r", ch);
        check_improve(ch, gsn_pick_lock, true, 2);
        act("$n picks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'pick door' */

        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (pexit->key < 0 && !IS_IMMORTAL(ch)) {
            send_to_char("It can't be picked.\n\r", ch);
            return;
        }
        if (!IS_SET(pexit->exit_info, EX_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch)) {
            send_to_char("You failed.\n\r", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n picks the $d.", ch, nullptr, pexit->keyword, To::Room);
        check_improve(ch, gsn_pick_lock, true, 2);

        /* pick the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {
            REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }
}

void do_stand(CHAR_DATA *ch, const char *arg) {
    (void)arg;

    if (ch->riding != nullptr) {
        unride_char(ch, ch->riding);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        if (IS_AFFECTED(ch, AFF_SLEEP)) {
            send_to_char("You can't wake up!\n\r", ch);
            return;
        }

        send_to_char("You wake and stand up.\n\r", ch);
        act("$n wakes and stands up.", ch);
        ch->position = POS_STANDING;
        break;

    case POS_RESTING:
    case POS_SITTING:
        send_to_char("You stand up.\n\r", ch);
        act("$n stands up.", ch);
        ch->position = POS_STANDING;
        break;

    case POS_STANDING: send_to_char("You are already standing.\n\r", ch); break;

    case POS_FIGHTING: send_to_char("You are already fighting!\n\r", ch); break;
    }
}

void do_rest(CHAR_DATA *ch, const char *argument) {
    (void)argument;

    if (ch->riding != nullptr) {
        send_to_char("You cannot rest - the saddle is too uncomfortable!\n\r", ch);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        send_to_char("You wake up and start resting.\n\r", ch);
        act("$n wakes up and starts resting.", ch);
        ch->position = POS_RESTING;
        break;

    case POS_RESTING: send_to_char("You are already resting.\n\r", ch); break;

    case POS_STANDING:
        send_to_char("You rest.\n\r", ch);
        act("$n sits down and rests.", ch);
        ch->position = POS_RESTING;
        break;

    case POS_SITTING:
        send_to_char("You rest.\n\r", ch);
        act("$n rests.", ch);
        ch->position = POS_RESTING;
        break;

    case POS_FIGHTING: send_to_char("You are already fighting!\n\r", ch); break;
    }
}

void do_sit(CHAR_DATA *ch, const char *argument) {
    (void)argument;

    if (ch->riding != nullptr) {
        send_to_char("You're already sitting in a saddle!", ch);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        send_to_char("You wake up.\n\r", ch);
        act("$n wakes and sits up.\n\r", ch);
        ch->position = POS_SITTING;
        break;
    case POS_RESTING:
        send_to_char("You stop resting.\n\r", ch);
        ch->position = POS_SITTING;
        break;
    case POS_SITTING: send_to_char("You are already sitting down.\n\r", ch); break;
    case POS_FIGHTING: send_to_char("Maybe you should finish this fight first?\n\r", ch); break;
    case POS_STANDING:
        send_to_char("You sit down.\n\r", ch);
        act("$n sits down on the ground.\n\r", ch);
        ch->position = POS_SITTING;
        break;
    }
}

void do_sleep(CHAR_DATA *ch, const char *argument) {
    (void)argument;

    if (ch->riding != nullptr) {
        send_to_char("You can't sleep - it's too uncomfortable in the saddle!\n\r", ch);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING: send_to_char("You are already sleeping.\n\r", ch); break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING:
        send_to_char("You go to sleep.\n\r", ch);
        act("$n goes to sleep.", ch);
        ch->position = POS_SLEEPING;
        break;

    case POS_FIGHTING: send_to_char("You are already fighting!\n\r", ch); break;
    }
}

void do_wake(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        /* Changed by rohan so that if you are resting or sitting, when you type
           wake, it will still make you stand up */
        /*      if (IS_AWAKE(ch)) {*/
        if (ch->position != POS_RESTING && ch->position != POS_SITTING && ch->position != POS_SLEEPING) {
            send_to_char("You are already awake!\n\r", ch);
            return;
        } else {
            do_stand(ch, argument);
            return;
        }
    }

    if (!IS_AWAKE(ch)) {
        send_to_char("You are asleep yourself!\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AWAKE(victim)) {
        act("$N is already awake.", ch, nullptr, victim, To::Char);
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLEEP)) {
        act("You can't wake $M!", ch, nullptr, victim, To::Char);
        return;
    }

    victim->position = POS_STANDING;
    act("You wake $M.", ch, nullptr, victim, To::Char);
    act("$n wakes you.", ch, nullptr, victim, To::Vict);
}

void do_sneak(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    AFFECT_DATA af;

    send_to_char("You attempt to move silently.\n\r", ch);
    affect_strip(ch, gsn_sneak);

    if (IS_NPC(ch) || number_percent() < get_skill_learned(ch, gsn_sneak)) {
        check_improve(ch, gsn_sneak, true, 3);
        af.type = gsn_sneak;
        af.level = ch->level;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_char(ch, &af);
    } else
        check_improve(ch, gsn_sneak, false, 3);
}

void do_hide(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    send_to_char("You attempt to hide.\n\r", ch);

    if (IS_AFFECTED(ch, AFF_HIDE))
        REMOVE_BIT(ch->affected_by, AFF_HIDE);

    if (IS_NPC(ch) || number_percent() < ch->pcdata->learned[gsn_hide]) {
        SET_BIT(ch->affected_by, AFF_HIDE);
        check_improve(ch, gsn_hide, true, 3);
    } else
        check_improve(ch, gsn_hide, false, 3);
}

/*
 * Contributed by Alander.
 */
void do_visible(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    affect_strip(ch, gsn_invis);
    affect_strip(ch, gsn_mass_invis);
    affect_strip(ch, gsn_sneak);
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
    REMOVE_BIT(ch->affected_by, AFF_SNEAK);
    send_to_char("Ok.\n\r", ch);
}

void do_recall(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;
    int vnum;

    if (IS_NPC(ch) && !IS_SET(ch->act, ACT_PET)) {
        send_to_char("Only players can recall.\n\r", ch);
        return;
    }

    /* Is this kidding???? .  Death. */
    /* if (ch->invis_level < HERO) */

    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n prays for transportation!", ch);

    if (!str_cmp(argument, "clan")) {
        if (IS_SET(ch->act, ACT_PET)) {
            if (ch->master != nullptr) {
                if (ch->master->clan())
                    vnum = ch->master->clan()->recall_vnum;
                else
                    return;
            } else
                return;
        } else {
            if (ch->clan()) {
                vnum = ch->clan()->recall_vnum;
            } else {
                send_to_char("You're not a member of a clan.\n\r", ch);
                return;
            }
        }
    } else {
        vnum = ROOM_VNUM_TEMPLE;
    }

    if ((location = get_room_index(vnum)) == nullptr) {
        send_to_char("You are completely lost.\n\r", ch);
        return;
    }

    if (ch->in_room == location) {
        send_to_char("You are there already.\n\r", ch);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE)) {
        snprintf(buf, sizeof(buf), "%s has forsaken you.\n\r", deity_name);
        send_to_char(buf, ch);
        return;
    }

    if ((victim = ch->fighting) != nullptr) {
        int lose, skill;

        if (IS_NPC(ch))
            skill = 40 + ch->level;
        else
            skill = get_skill_learned(ch, gsn_recall);

        if (number_percent() < 80 * skill / 100) {
            check_improve(ch, gsn_recall, false, 6);
            WAIT_STATE(ch, 4);
            snprintf(buf, sizeof(buf), "You failed!.\n\r");
            send_to_char(buf, ch);
            return;
        }

        lose = (ch->desc != nullptr) ? 25 : 50;
        gain_exp(ch, 0 - lose);
        check_improve(ch, gsn_recall, true, 4);
        snprintf(buf, sizeof(buf), "You recall from combat!  You lose %d exps.\n\r", lose);
        send_to_char(buf, ch);
        stop_fighting(ch, true);
    }

    ch->move /= 2;
    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n disappears.", ch);
    char_from_room(ch);
    char_to_room(ch, location);
    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n appears in the room.", ch);

    do_look(ch, "auto");

    if (IS_NPC(ch) && ch->ridden_by) {
        act("$n falls to the ground.", ch->ridden_by);
        act("You fall to the ground.", ch->ridden_by, nullptr, nullptr, To::Char);
        fallen_off_mount(ch->ridden_by);
    }

    if (ch->pet != nullptr)
        do_recall(ch->pet, argument);
}

void do_train(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    Stat stat = Stat::Str;
    const char *pOutput = nullptr;
    int cost;

    if (IS_NPC(ch))
        return;

    /*
     * Check for trainer.
     */
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room) {
        if (IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN))
            break;
    }

    if (mob == nullptr) {
        send_to_char("You can't do that here.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        snprintf(buf, sizeof(buf), "You have %d training sessions.\n\r", ch->train);
        send_to_char(buf, ch);
        argument = "foo";
    }

    cost = 1;

    if (!str_cmp(argument, "str")) {
        if (class_table[ch->class_num].attr_prime == Stat::Str)
            cost = 1;
        stat = Stat::Str;
        pOutput = "strength";
    }

    else if (!str_cmp(argument, "int")) {
        if (class_table[ch->class_num].attr_prime == Stat::Int)
            cost = 1;
        stat = Stat::Int;
        pOutput = "intelligence";
    }

    else if (!str_cmp(argument, "wis")) {
        if (class_table[ch->class_num].attr_prime == Stat::Wis)
            cost = 1;
        stat = Stat::Wis;
        pOutput = "wisdom";
    }

    else if (!str_cmp(argument, "dex")) {
        if (class_table[ch->class_num].attr_prime == Stat::Dex)
            cost = 1;
        stat = Stat::Dex;
        pOutput = "dexterity";
    }

    else if (!str_cmp(argument, "con")) {
        if (class_table[ch->class_num].attr_prime == Stat::Con)
            cost = 1;
        stat = Stat::Con;
        pOutput = "constitution";
    }

    else if (!str_cmp(argument, "hp"))
        cost = 1;

    else if (!str_cmp(argument, "mana"))
        cost = 1;

    else {
        strcpy(buf, "You can train:");
        if (ch->perm_stat[Stat::Str] < get_max_train(ch, Stat::Str))
            strcat(buf, " str");
        if (ch->perm_stat[Stat::Int] < get_max_train(ch, Stat::Int))
            strcat(buf, " int");
        if (ch->perm_stat[Stat::Wis] < get_max_train(ch, Stat::Wis))
            strcat(buf, " wis");
        if (ch->perm_stat[Stat::Dex] < get_max_train(ch, Stat::Dex))
            strcat(buf, " dex");
        if (ch->perm_stat[Stat::Con] < get_max_train(ch, Stat::Con))
            strcat(buf, " con");
        strcat(buf, " hp mana");

        if (buf[strlen(buf) - 1] != ':') {
            strcat(buf, ".\n\r");
            send_to_char(buf, ch);
        } else {
            /*
             * This message dedicated to Jordan ... you big stud!
             */
            act("You have nothing left to train, you $T!", ch, nullptr,
                ch->sex == SEX_MALE ? "big stud" : ch->sex == SEX_FEMALE ? "hot babe" : "wild thing", To::Char);
        }

        return;
    }

    if (!str_cmp("hp", argument)) {
        if (cost > ch->train) {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_hit += 10;
        ch->max_hit += 10;
        ch->hit += 10;
        act("|WYour durability increases!|w", ch, nullptr, nullptr, To::Char);
        act("|W$n's durability increases!|w", ch);
        return;
    }

    if (!str_cmp("mana", argument)) {
        if (cost > ch->train) {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_mana += 10;
        ch->max_mana += 10;
        ch->mana += 10;
        act("|WYour power increases!|w", ch, nullptr, nullptr, To::Char);
        act("|W$n's power increases!|w", ch);
        return;
    }

    if (ch->perm_stat[stat] >= get_max_train(ch, stat)) {
        act("Your $T is already at maximum.", ch, nullptr, pOutput, To::Char);
        return;
    }

    if (cost > ch->train) {
        send_to_char("You don't have enough training sessions.\n\r", ch);
        return;
    }

    ch->train -= cost;

    ch->perm_stat[stat] += 1;
    act("|WYour $T increases!|w", ch, nullptr, pOutput, To::Char);
    act("|W$n's $T increases!|w", ch, nullptr, pOutput, To::Room);
}
