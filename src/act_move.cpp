/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_move.c: player and mobile movement                               */
/*                                                                       */
/*************************************************************************/

#include "challeng.h"
#include "interp.h"
#include "merc.h"
#include "olc_room.h"
#include "string_utils.hpp"

#include <cstdio>
#include <cstring>

const char *dir_name[] = {"north", "east", "south", "west", "up", "down"};

const sh_int rev_dir[] = {2, 3, 0, 1, 5, 4};

const sh_int movement_loss[SECT_MAX] = {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6};

/*
 * Local functions.
 */
int find_door(CHAR_DATA *ch, char *arg);
bool has_key(CHAR_DATA *ch, int key);

void move_char(CHAR_DATA *ch, int door) {
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    char buf[MAX_STRING_LENGTH];

    if (door < 0 || door > 5) {
        bug("Do_move: bad door %d.", door);
        return;
    }

    in_room = ch->in_room;
    if ((pexit = in_room->exit[door]) == nullptr || (to_room = pexit->u1.to_room) == nullptr
        || !can_see_room(ch, pexit->u1.to_room)) {
        send_to_char("Alas, you cannot go that way.\n\r", ch);
        return;
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch)) {
        if (IS_SET(pexit->exit_info, EX_PASSPROOF) && IS_AFFECTED(ch, AFF_PASS_DOOR)) {
            act("The $d is protected from trespass by a magical barrier.", ch, nullptr, pexit->keyword, TO_CHAR);
            return;
        } else {
            if (!IS_AFFECTED(ch, AFF_PASS_DOOR)) {
                act("The $d is closed.", ch, nullptr, pexit->keyword, TO_CHAR);
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

    if (room_is_private(to_room) && (get_trust(ch) < IMPLEMENTOR)) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    if (!IS_NPC(ch)) {
        int iClass, iGuild, iClan;
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
            for (iClan = 0; iClan < NUM_CLANS; iClan++) {

                if (to_room->vnum == clantable[iClan].entrance_vnum) {
                    if ((ch->pcdata->pcclan == nullptr) || ch->pcdata->pcclan->clan != &clantable[iClan]) {
                        snprintf(buf, sizeof(buf), "Only members of the %s may enter there.\n\r",
                                 clantable[iClan].name);

                        send_to_char(buf, ch);

                        return;
                    }
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
                act("$N is too exhausted to carry you further.", ch, nullptr, ch->riding, TO_CHAR);
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
                act("$n leaves $T.", ch, nullptr, dir_name[door], TO_ROOM);
            } else {
                act("$n rides $t on $N.", ch, dir_name[door], ch->riding, TO_ROOM);
                act("You ride $t on $N.", ch, dir_name[door], ch->riding, TO_CHAR);
            }
        }
    }

    char_from_room(ch);
    char_to_room(ch, to_room);

    if (!IS_AFFECTED(ch, AFF_SNEAK) && (IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) || !IS_SET(ch->act, PLR_PROWL))) {
        if (ch->ridden_by == nullptr) {
            if (ch->riding == nullptr) {
                act("$n has arrived.", ch, nullptr, nullptr, TO_ROOM);
            } else {
                act("$n has arrived, riding $N.", ch, nullptr, ch->riding, TO_ROOM);
            }
        }
    }

    do_look(ch, "auto");

    if (in_room == to_room) /* no circular follows */
        return;

    for (fch = in_room->people; fch != nullptr; fch = fch_next) {
        fch_next = fch->next_in_room;

        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
            do_stand(fch, "");

        if (fch->master == ch && fch->position == POS_STANDING) {

            if (IS_SET(ch->in_room->room_flags, ROOM_LAW) && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE))) {
                act("$N may not enter here.", ch, nullptr, fch, TO_CHAR);
                act("You aren't allowed to go there.", fch, nullptr, nullptr, TO_CHAR);
                return;
            }

            act("You follow $N.", fch, nullptr, ch, TO_CHAR);
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
    char buf[MAX_STRING_LENGTH];
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

                    if (room_is_private(to_room) && (get_trust(ch) < IMPLEMENTOR)) {
                        send_to_char("That room is private right now.\n\r", ch);
                        return;
                    }

                    if (!IS_NPC(ch)) {
                        int iClass, iGuild, iClan;
                        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
                            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
                                if (iClass != ch->class_num && to_room->vnum == class_table[iClass].guild[iGuild]) {
                                    send_to_char("You aren't allowed in there.\n\r", ch);
                                    return;
                                }
                            }
                        }

                        if (!IS_IMMORTAL(ch)) {
                            for (iClan = 0; iClan < NUM_CLANS; iClan++) {
                                if (to_room->vnum == clantable[iClan].entrance_vnum) {
                                    if ((ch->pcdata->pcclan == nullptr)
                                        || ch->pcdata->pcclan->clan != &clantable[iClan]) {
                                        snprintf(buf, sizeof(buf), "Only members of the %s may enter there.\n\r",
                                                 clantable[iClan].name);

                                        send_to_char(buf, ch);

                                        return;
                                    }
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
                    act("$n steps through a portal and vanishes.", ch, nullptr, nullptr, TO_ROOM);
                    send_to_char("You step through a portal and vanish.\n\r", ch);
                    char_from_room(ch);
                    char_to_room(ch, obj->destination);
                    act("$n has arrived through a portal.", ch, nullptr, nullptr, TO_ROOM);
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
                                act("You can't bring $N into the city.", ch, nullptr, fch, TO_CHAR);
                                act("You aren't allowed in the city.", fch, nullptr, nullptr, TO_CHAR);
                                continue;
                            }

                            act("You follow $N.", fch, nullptr, ch, TO_CHAR);
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

void do_north(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, DIR_NORTH);
}

void do_east(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, DIR_EAST);
}

void do_south(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, DIR_SOUTH);
}

void do_west(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, DIR_WEST);
}

void do_up(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, DIR_UP);
}

void do_down(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->in_room->vnum == CHAL_ROOM)
        do_room_check(ch);
    move_char(ch, DIR_DOWN);
}

int find_door(CHAR_DATA *ch, char *arg) {
    EXIT_DATA *pexit;
    int door;

    if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
        door = 0;
    else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
        door = 1;
    else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
        door = 2;
    else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
        door = 3;
    else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
        door = 4;
    else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
        door = 5;
    else {
        for (door = 0; door <= 5; door++) {
            if ((pexit = ch->in_room->exit[door]) != nullptr && IS_SET(pexit->exit_info, EX_ISDOOR)
                && pexit->keyword != nullptr && is_name(arg, pexit->keyword))
                return door;
        }
        act("I see no $T here.", ch, nullptr, arg, TO_CHAR);
        return -1;
    }

    if ((pexit = ch->in_room->exit[door]) == nullptr) {
        act("I see no door $T here.", ch, nullptr, arg, TO_CHAR);
        return -1;
    }

    if (!IS_SET(pexit->exit_info, EX_ISDOOR)) {
        send_to_char("You can't do that.\n\r", ch);
        return -1;
    }

    return door;
}

void do_open(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Open what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != nullptr) {
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
        act("$n opens $p.", ch, obj, nullptr, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'open door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            send_to_char("It's already open.\n\r", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED)) {
            send_to_char("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_CLOSED);
        act("$n opens the $d.", ch, nullptr, pexit->keyword, TO_ROOM);
        send_to_char("Ok.\n\r", ch);

        /* open the other side */
        if ((to_room = pexit->u1.to_room) != nullptr && (pexit_rev = to_room->exit[rev_dir[door]]) != nullptr
            && pexit_rev->u1.to_room == ch->in_room) {
            CHAR_DATA *rch;

            REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != nullptr; rch = rch->next_in_room)
                act("The $d opens.", rch, nullptr, pexit_rev->keyword, TO_CHAR);
        }
    }
}

void do_close(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Close what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != nullptr) {
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
        act("$n closes $p.", ch, obj, nullptr, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'close door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (IS_SET(pexit->exit_info, EX_CLOSED)) {
            send_to_char("It's already closed.\n\r", ch);
            return;
        }

        SET_BIT(pexit->exit_info, EX_CLOSED);
        act("$n closes the $d.", ch, nullptr, pexit->keyword, TO_ROOM);
        send_to_char("Ok.\n\r", ch);

        /* close the other side */
        if ((to_room = pexit->u1.to_room) != nullptr && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
            && pexit_rev->u1.to_room == ch->in_room) {
            CHAR_DATA *rch;

            SET_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != nullptr; rch = rch->next_in_room)
                act("The $d closes.", rch, nullptr, pexit_rev->keyword, TO_CHAR);
        }
    }
}

bool has_key(CHAR_DATA *ch, int key) {
    OBJ_DATA *obj;

    for (obj = ch->carrying; obj != nullptr; obj = obj->next_content) {
        if (obj->pIndexData->vnum == key)
            return true;
    }

    return false;
}

void do_lock(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Lock what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != nullptr) {
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
        act("$n locks $p.", ch, obj, nullptr, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'lock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
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
        act("$n locks the $d.", ch, nullptr, pexit->keyword, TO_ROOM);

        /* lock the other side */
        if ((to_room = pexit->u1.to_room) != nullptr && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
            && pexit_rev->u1.to_room == ch->in_room) {
            SET_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }
}

void do_unlock(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Unlock what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != nullptr) {
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
        act("$n unlocks $p.", ch, obj, nullptr, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'unlock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
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
        act("$n unlocks the $d.", ch, nullptr, pexit->keyword, TO_ROOM);

        /* unlock the other side */
        if ((to_room = pexit->u1.to_room) != nullptr && (pexit_rev = to_room->exit[rev_dir[door]]) != nullptr
            && pexit_rev->u1.to_room == ch->in_room) {
            REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }
}

void do_pick(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Pick what?\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_pick_lock].beats);

    /* look for guards */
    for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
        if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level) {
            act("$N is standing too close to the lock.", ch, nullptr, gch, TO_CHAR);
            return;
        }
    }

    if (!IS_NPC(ch) && number_percent() > get_skill_learned(ch, gsn_pick_lock)) {
        send_to_char("You failed.\n\r", ch);
        check_improve(ch, gsn_pick_lock, false, 2);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != nullptr) {
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
        act("$n picks $p.", ch, obj, nullptr, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
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
        act("$n picks the $d.", ch, nullptr, pexit->keyword, TO_ROOM);
        check_improve(ch, gsn_pick_lock, true, 2);

        /* pick the other side */
        if ((to_room = pexit->u1.to_room) != nullptr && (pexit_rev = to_room->exit[rev_dir[door]]) != nullptr
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
        act("$n wakes and stands up.", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_STANDING;
        break;

    case POS_RESTING:
    case POS_SITTING:
        send_to_char("You stand up.\n\r", ch);
        act("$n stands up.", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_STANDING;
        break;

    case POS_STANDING: send_to_char("You are already standing.\n\r", ch); break;

    case POS_FIGHTING: send_to_char("You are already fighting!\n\r", ch); break;
    }
}

void do_rest(CHAR_DATA *ch, char *argument) {
    (void)argument;

    if (ch->riding != nullptr) {
        send_to_char("You cannot rest - the saddle is too uncomfortable!\n\r", ch);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        send_to_char("You wake up and start resting.\n\r", ch);
        act("$n wakes up and starts resting.", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_RESTING;
        break;

    case POS_RESTING: send_to_char("You are already resting.\n\r", ch); break;

    case POS_STANDING:
        send_to_char("You rest.\n\r", ch);
        act("$n sits down and rests.", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_RESTING;
        break;

    case POS_SITTING:
        send_to_char("You rest.\n\r", ch);
        act("$n rests.", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_RESTING;
        break;

    case POS_FIGHTING: send_to_char("You are already fighting!\n\r", ch); break;
    }
}

void do_sit(CHAR_DATA *ch, char *argument) {
    (void)argument;

    if (ch->riding != nullptr) {
        send_to_char("You're already sitting in a saddle!", ch);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        send_to_char("You wake up.\n\r", ch);
        act("$n wakes and sits up.\n\r", ch, nullptr, nullptr, TO_ROOM);
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
        act("$n sits down on the ground.\n\r", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_SITTING;
        break;
    }
}

void do_sleep(CHAR_DATA *ch, char *argument) {
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
        act("$n goes to sleep.", ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_SLEEPING;
        break;

    case POS_FIGHTING: send_to_char("You are already fighting!\n\r", ch); break;
    }
}

void do_wake(CHAR_DATA *ch, char *argument) {
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
        act("$N is already awake.", ch, nullptr, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLEEP)) {
        act("You can't wake $M!", ch, nullptr, victim, TO_CHAR);
        return;
    }

    victim->position = POS_STANDING;
    act("You wake $M.", ch, nullptr, victim, TO_CHAR);
    act("$n wakes you.", ch, nullptr, victim, TO_VICT);
}

void do_sneak(CHAR_DATA *ch, char *argument) {
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

void do_hide(CHAR_DATA *ch, char *argument) {
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
void do_visible(CHAR_DATA *ch, char *argument) {
    (void)argument;
    affect_strip(ch, gsn_invis);
    affect_strip(ch, gsn_mass_invis);
    affect_strip(ch, gsn_sneak);
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
    REMOVE_BIT(ch->affected_by, AFF_SNEAK);
    send_to_char("Ok.\n\r", ch);
}

void do_recall(CHAR_DATA *ch, char *argument) {
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
        act("$n prays for transportation!", ch, 0, 0, TO_ROOM);

    if (!str_cmp(argument, "clan")) {
        if (IS_SET(ch->act, ACT_PET)) {
            if (ch->master != nullptr) {
                if (ch->master->pcdata->pcclan)
                    vnum = ch->master->pcdata->pcclan->clan->recall_vnum;
                else
                    return;
            } else
                return;
        } else {
            if (ch->pcdata->pcclan) {
                vnum = ch->pcdata->pcclan->clan->recall_vnum;
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
        act("$n disappears.", ch, nullptr, nullptr, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, location);
    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n appears in the room.", ch, nullptr, nullptr, TO_ROOM);

    do_look(ch, "auto");

    if (IS_NPC(ch) && ch->ridden_by) {
        act("$n falls to the ground.", ch->ridden_by, nullptr, nullptr, TO_ROOM);
        act("You fall to the ground.", ch->ridden_by, nullptr, nullptr, TO_CHAR);
        fallen_off_mount(ch->ridden_by);
    }

    if (ch->pet != nullptr)
        do_recall(ch->pet, argument);
}

void do_train(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    sh_int stat = -1;
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
        if (class_table[ch->class_num].attr_prime == STAT_STR)
            cost = 1;
        stat = STAT_STR;
        pOutput = "strength";
    }

    else if (!str_cmp(argument, "int")) {
        if (class_table[ch->class_num].attr_prime == STAT_INT)
            cost = 1;
        stat = STAT_INT;
        pOutput = "intelligence";
    }

    else if (!str_cmp(argument, "wis")) {
        if (class_table[ch->class_num].attr_prime == STAT_WIS)
            cost = 1;
        stat = STAT_WIS;
        pOutput = "wisdom";
    }

    else if (!str_cmp(argument, "dex")) {
        if (class_table[ch->class_num].attr_prime == STAT_DEX)
            cost = 1;
        stat = STAT_DEX;
        pOutput = "dexterity";
    }

    else if (!str_cmp(argument, "con")) {
        if (class_table[ch->class_num].attr_prime == STAT_CON)
            cost = 1;
        stat = STAT_CON;
        pOutput = "constitution";
    }

    else if (!str_cmp(argument, "hp"))
        cost = 1;

    else if (!str_cmp(argument, "mana"))
        cost = 1;

    else {
        strcpy(buf, "You can train:");
        if (ch->perm_stat[STAT_STR] < get_max_train(ch, STAT_STR))
            strcat(buf, " str");
        if (ch->perm_stat[STAT_INT] < get_max_train(ch, STAT_INT))
            strcat(buf, " int");
        if (ch->perm_stat[STAT_WIS] < get_max_train(ch, STAT_WIS))
            strcat(buf, " wis");
        if (ch->perm_stat[STAT_DEX] < get_max_train(ch, STAT_DEX))
            strcat(buf, " dex");
        if (ch->perm_stat[STAT_CON] < get_max_train(ch, STAT_CON))
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
                ch->sex == SEX_MALE ? "big stud" : ch->sex == SEX_FEMALE ? "hot babe" : "wild thing", TO_CHAR);
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
        act("|WYour durability increases!|w", ch, nullptr, nullptr, TO_CHAR);
        act("|W$n's durability increases!|w", ch, nullptr, nullptr, TO_ROOM);
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
        act("|WYour power increases!|w", ch, nullptr, nullptr, TO_CHAR);
        act("|W$n's power increases!|w", ch, nullptr, nullptr, TO_ROOM);
        return;
    }

    if (ch->perm_stat[stat] >= get_max_train(ch, stat)) {
        act("Your $T is already at maximum.", ch, nullptr, pOutput, TO_CHAR);
        return;
    }

    if (cost > ch->train) {
        send_to_char("You don't have enough training sessions.\n\r", ch);
        return;
    }

    ch->train -= cost;

    ch->perm_stat[stat] += 1;
    act("|WYour $T increases!|w", ch, nullptr, pOutput, TO_CHAR);
    act("|W$n's $T increases!|w", ch, nullptr, pOutput, TO_ROOM);
}
