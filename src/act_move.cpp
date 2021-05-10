/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_move.c: player and mobile movement                               */
/*                                                                       */
/*************************************************************************/

#include "AFFECT_DATA.hpp"
#include "VnumRooms.hpp"
#include "challenge.hpp"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

#include <cstdio>
#include <cstring>

using namespace std::literals;

constexpr PerSectorType<int> movement_loss{1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6};

void move_char(Char *ch, Direction door) {
    auto *in_room = ch->in_room;
    auto *pexit = in_room->exit[door];
    auto *to_room = pexit ? pexit->u1.to_room : nullptr;
    if (!pexit || !to_room || !ch->can_see(*to_room)) {
        ch->send_line("Alas, you cannot go that way.");
        return;
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED) && ch->is_mortal()) {
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
        ch->send_line("Your mount cannot travel through solid objects.");
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != nullptr && in_room == ch->master->in_room) {
        ch->send_line("What?  And leave your beloved master?");
        return;
    }

    if (room_is_private(to_room) && (ch->get_trust() < IMPLEMENTOR)) {
        ch->send_line("That room is private right now.");
        return;
    }

    if (ch->is_pc()) {
        int iClass, iGuild;
        int move;

        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
                if (iClass != ch->class_num && to_room->vnum == class_table[iClass].guild[iGuild]) {
                    ch->send_line("You aren't allowed in there.");
                    return;
                }
            }
        }

        /* added Faramir 25/6/96 to stop non-clan members walking into room */

        if (ch->is_mortal()) {
            for (auto &clan : clantable) {
                if (to_room->vnum == clan.entrance_vnum && ch->clan() != &clan) {
                    ch->send_line("Only members of the {} may enter there.", clan.name);
                    return;
                }
            }
        }

        if (in_room->sector_type == SectorType::Air || to_room->sector_type == SectorType::Air) {
            if ((!IS_AFFECTED(ch, AFF_FLYING) && ch->is_mortal())
                && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))) {
                ch->send_line("You can't fly.");
                return;
            }
        }

        if ((in_room->sector_type == SectorType::NonSwimmableWater
             || to_room->sector_type == SectorType::NonSwimmableWater)
            && !IS_AFFECTED(ch, AFF_FLYING) && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))
            && !ch->has_boat()) {
            ch->send_line("You need a boat to go there.");
            return;
        }

        move = movement_loss[in_room->sector_type] + movement_loss[to_room->sector_type];

        move /= 2; /* i.e. the average */

        if (ch->riding == nullptr) {
            if (ch->move < move) {
                ch->send_line("You are too exhausted.");
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
        && (ch->is_npc() || !IS_SET(ch->act, PLR_WIZINVIS) || !IS_SET(ch->act, PLR_PROWL))) {
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

    if (!IS_AFFECTED(ch, AFF_SNEAK)
        && (ch->is_npc() || !IS_SET(ch->act, PLR_WIZINVIS) || !IS_SET(ch->act, PLR_PROWL))) {
        if (ch->ridden_by == nullptr) {
            if (ch->riding == nullptr) {
                act("$n has arrived.", ch);
            } else {
                act("$n has arrived, riding $N.", ch, nullptr, ch->riding, To::Room);
            }
        }
    }

    look_auto(ch);

    if (in_room == to_room) /* no circular follows */
        return;

    for (auto *fch : in_room->people) {
        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
            do_stand(fch);

        if (fch->master == ch && fch->position == POS_STANDING) {

            if (IS_SET(ch->in_room->room_flags, ROOM_LAW) && (fch->is_npc() && IS_SET(fch->act, ACT_AGGRESSIVE))) {
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

void do_enter(Char *ch, std::string_view argument) {
    ROOM_INDEX_DATA *to_room, *in_room;
    int count = 0;
    auto &&[number, arg] = number_argument(argument);
    if (ch->riding) {
        ch->send_line("Before entering a portal you must dismount.");
        return;
    }
    in_room = ch->in_room;
    for (auto *obj : ch->in_room->contents) {
        if (can_see_obj(ch, obj) && (is_name(arg, obj->name))) {
            if (++count == number) {
                if (obj->item_type == ITEM_PORTAL) {

                    to_room = obj->destination;

                    /* handle dangling portals, not that they should ever exist*/
                    if (to_room == nullptr) {
                        ch->send_line("That portal is highly unstable and is likely to get you killed!");
                        return;
                    }

                    if (ch->is_npc() && IS_SET(ch->act, ACT_AGGRESSIVE) && IS_SET(to_room->room_flags, ROOM_LAW)) {
                        ch->send_line("Something prevents you from leaving...");
                        return;
                    }

                    if (room_is_private(to_room) && (ch->get_trust() < IMPLEMENTOR)) {
                        ch->send_line("That room is private right now.");
                        return;
                    }

                    if (ch->is_pc()) {
                        int iClass, iGuild;
                        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
                            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
                                if (iClass != ch->class_num && to_room->vnum == class_table[iClass].guild[iGuild]) {
                                    ch->send_line("You aren't allowed in there.");
                                    return;
                                }
                            }
                        }

                        if (ch->is_mortal()) {
                            for (auto &clan : clantable) {
                                if (to_room->vnum == clan.entrance_vnum && ch->clan() != &clan) {
                                    ch->send_line("Only members of the {} may enter there.", clan.name);
                                    return;
                                }
                            }
                        }

                        if (in_room->sector_type == SectorType::Air || to_room->sector_type == SectorType::Air) {
                            if ((!IS_AFFECTED(ch, AFF_FLYING) && ch->is_mortal())
                                && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING))) {
                                ch->send_line("You can't fly.");
                                return;
                            }
                        }

                        if ((in_room->sector_type == SectorType::NonSwimmableWater
                             || to_room->sector_type == SectorType::NonSwimmableWater)
                            && !IS_AFFECTED(ch, AFF_FLYING)
                            && !(ch->riding != nullptr && IS_AFFECTED(ch->riding, AFF_FLYING)) && !ch->has_boat()) {
                            ch->send_line("You need a boat to go there.");
                            return;
                        }
                    }
                    act("$n steps through a portal and vanishes.", ch);
                    ch->send_line("You step through a portal and vanish.");
                    char_from_room(ch);
                    char_to_room(ch, obj->destination);
                    act("$n has arrived through a portal.", ch);
                    look_auto(ch);

                    if (in_room == to_room) /* no circular follows */
                        return;

                    for (auto *fch : in_room->people) {
                        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
                            do_stand(fch);

                        if (fch->master == ch && fch->position == POS_STANDING) {

                            if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
                                && (fch->is_npc() && IS_SET(fch->act, ACT_AGGRESSIVE))) {
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
                    ch->send_line("That's not a portal!");
                    return;
                }
            }
        }
    }
    ch->send_line("You can't see that here.");
}

void do_north(Char *ch) {
    if (ch->in_room->vnum == rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::North);
}

void do_east(Char *ch) {
    if (ch->in_room->vnum == rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::East);
}

void do_south(Char *ch) {
    if (ch->in_room->vnum == rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::South);
}

void do_west(Char *ch) {
    if (ch->in_room->vnum == rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::West);
}

void do_up(Char *ch) {
    if (ch->in_room->vnum == rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::Up);
}

void do_down(Char *ch) {
    if (ch->in_room->vnum == rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::Down);
}

std::optional<Direction> find_door(Char *ch, std::string_view arg) {
    if (auto door = try_parse_direction(arg)) {
        auto *pexit = ch->in_room->exit[*door];
        if (!pexit) {
            act("I see no door $T here.", ch, nullptr, arg, To::Char);
            return {};
        }

        if (!IS_SET(pexit->exit_info, EX_ISDOOR)) {
            ch->send_line("You can't do that.");
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

void do_open(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Open what?");
        return;
    }

    auto arg = args.shift();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'open object' */
        if (obj->item_type != ITEM_CONTAINER) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            ch->send_line("It's already open.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSEABLE)) {
            ch->send_line("You can't do that.");
            return;
        }
        if (IS_SET(obj->value[1], CONT_LOCKED)) {
            ch->send_line("It's locked.");
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_CLOSED);
        ch->send_line("Ok.");
        act("$n opens $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'open door' */

        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            ch->send_line("It's already open.");
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED)) {
            ch->send_line("It's locked.");
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_CLOSED);
        act("$n opens the $d.", ch, nullptr, pexit->keyword, To::Room);
        ch->send_line("Ok.");

        /* open the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {

            REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (auto *rch : to_room->people)
                act("The $d opens.", rch, nullptr, pexit_rev->keyword, To::Char);
        }
    }
}

void do_close(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Close what?");
        return;
    }

    auto arg = args.shift();
    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'close object' */
        if (obj->item_type != ITEM_CONTAINER) {
            ch->send_line("That's not a container.");
            return;
        }
        if (IS_SET(obj->value[1], CONT_CLOSED)) {
            ch->send_line("It's already closed.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSEABLE)) {
            ch->send_line("You can't do that.");
            return;
        }

        SET_BIT(obj->value[1], CONT_CLOSED);
        ch->send_line("Ok.");
        act("$n closes $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'close door' */

        auto *pexit = ch->in_room->exit[door];
        if (IS_SET(pexit->exit_info, EX_CLOSED)) {
            ch->send_line("It's already closed.");
            return;
        }

        SET_BIT(pexit->exit_info, EX_CLOSED);
        act("$n closes the $d.", ch, nullptr, pexit->keyword, To::Room);
        ch->send_line("Ok.");

        /* close the other side */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit_rev;
        if ((to_room = pexit->u1.to_room) && (pexit_rev = to_room->exit[reverse(door)])
            && pexit_rev->u1.to_room == ch->in_room) {
            SET_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (auto *rch : to_room->people)
                act("The $d closes.", rch, nullptr, pexit_rev->keyword, To::Char);
        }
    }
}

void do_lock(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Lock what?");
        return;
    }
    auto arg = args.shift();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'lock object' */
        if (obj->item_type != ITEM_CONTAINER) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (obj->value[2] < 0) {
            ch->send_line("It can't be locked.");
            return;
        }
        if (!ch->carrying_object_vnum(obj->value[2])) {
            ch->send_line("You lack the key.");
            return;
        }
        if (IS_SET(obj->value[1], CONT_LOCKED)) {
            ch->send_line("It's already locked.");
            return;
        }

        SET_BIT(obj->value[1], CONT_LOCKED);
        ch->send_line("*Click*");
        act("$n locks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'lock door' */

        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (pexit->key < 0) {
            ch->send_line("It can't be locked.");
            return;
        }
        if (!ch->carrying_object_vnum(pexit->key)) {
            ch->send_line("You lack the key.");
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED)) {
            ch->send_line("It's already locked.");
            return;
        }

        SET_BIT(pexit->exit_info, EX_LOCKED);
        ch->send_line("*Click*");
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

void do_unlock(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Unlock what?");
        return;
    }

    auto arg = args.shift();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'unlock object' */
        if (obj->item_type != ITEM_CONTAINER) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (obj->value[2] < 0) {
            ch->send_line("It can't be unlocked.");
            return;
        }
        if (!ch->carrying_object_vnum(obj->value[2])) {
            ch->send_line("You lack the key.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_LOCKED)) {
            ch->send_line("It's already unlocked.");
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        ch->send_line("*Click*");
        act("$n unlocks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'unlock door' */
        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (pexit->key < 0) {
            ch->send_line("It can't be unlocked.");
            return;
        }
        if (!ch->carrying_object_vnum(pexit->key)) {
            ch->send_line("You lack the key.");
            return;
        }
        if (!IS_SET(pexit->exit_info, EX_LOCKED)) {
            ch->send_line("It's already unlocked.");
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        ch->send_line("*Click*");
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

void do_pick(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Pick what?");
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_pick_lock].beats);

    /* look for guards */
    for (auto *gch : ch->in_room->people) {
        if (gch->is_npc() && IS_AWAKE(gch) && ch->level + 5 < gch->level) {
            act("$N is standing too close to the lock.", ch, nullptr, gch, To::Char);
            return;
        }
    }

    if (ch->is_pc() && number_percent() > get_skill_learned(ch, gsn_pick_lock)) {
        ch->send_line("You failed.");
        check_improve(ch, gsn_pick_lock, false, 2);
        return;
    }

    auto arg = args.shift();
    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'pick object' */
        if (obj->item_type != ITEM_CONTAINER) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (obj->value[2] < 0) {
            ch->send_line("It can't be unlocked.");
            return;
        }
        if (!IS_SET(obj->value[1], CONT_LOCKED)) {
            ch->send_line("It's already unlocked.");
            return;
        }
        if (IS_SET(obj->value[1], CONT_PICKPROOF)) {
            ch->send_line("You failed.");
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        ch->send_line("*Click*");
        check_improve(ch, gsn_pick_lock, true, 2);
        act("$n picks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_door = find_door(ch, arg)) {
        auto door = *opt_door;
        /* 'pick door' */

        auto *pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED) && ch->is_mortal()) {
            ch->send_line("It's not closed.");
            return;
        }
        if (pexit->key < 0 && ch->is_mortal()) {
            ch->send_line("It can't be picked.");
            return;
        }
        if (!IS_SET(pexit->exit_info, EX_LOCKED)) {
            ch->send_line("It's already unlocked.");
            return;
        }
        if (IS_SET(pexit->exit_info, EX_PICKPROOF) && ch->is_mortal()) {
            ch->send_line("You failed.");
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        ch->send_line("*Click*");
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

void do_stand(Char *ch) {
    if (ch->riding != nullptr) {
        unride_char(ch, ch->riding);
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        if (IS_AFFECTED(ch, AFF_SLEEP)) {
            ch->send_line("You can't wake up!");
            return;
        }

        ch->send_line("You wake and stand up.");
        act("$n wakes and stands up.", ch);
        ch->position = POS_STANDING;
        break;

    case POS_RESTING:
    case POS_SITTING:
        ch->send_line("You stand up.");
        act("$n stands up.", ch);
        ch->position = POS_STANDING;
        break;

    case POS_STANDING: ch->send_line("You are already standing."); break;

    case POS_FIGHTING: ch->send_line("You are already fighting!"); break;
    }
}

void do_rest(Char *ch) {
    if (ch->riding != nullptr) {
        ch->send_line("You cannot rest - the saddle is too uncomfortable!");
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        ch->send_line("You wake up and start resting.");
        act("$n wakes up and starts resting.", ch);
        ch->position = POS_RESTING;
        break;

    case POS_RESTING: ch->send_line("You are already resting."); break;

    case POS_STANDING:
        ch->send_line("You rest.");
        act("$n sits down and rests.", ch);
        ch->position = POS_RESTING;
        break;

    case POS_SITTING:
        ch->send_line("You rest.");
        act("$n rests.", ch);
        ch->position = POS_RESTING;
        break;

    case POS_FIGHTING: ch->send_line("You are already fighting!"); break;
    }
}

void do_sit(Char *ch) {
    if (ch->riding != nullptr) {
        ch->send_line("You're already sitting in a saddle!");
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        ch->send_line("You wake up.");
        act("$n wakes and sits up.\n\r", ch);
        ch->position = POS_SITTING;
        break;
    case POS_RESTING:
        ch->send_line("You stop resting.");
        ch->position = POS_SITTING;
        break;
    case POS_SITTING: ch->send_line("You are already sitting down."); break;
    case POS_FIGHTING: ch->send_line("Maybe you should finish this fight first?"); break;
    case POS_STANDING:
        ch->send_line("You sit down.");
        act("$n sits down on the ground.\n\r", ch);
        ch->position = POS_SITTING;
        break;
    }
}

void do_sleep(Char *ch) {
    if (ch->riding != nullptr) {
        ch->send_line("You can't sleep - it's too uncomfortable in the saddle!");
        return;
    }

    switch (ch->position) {
    case POS_SLEEPING: ch->send_line("You are already sleeping."); break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING:
        ch->send_line("You go to sleep.");
        act("$n goes to sleep.", ch);
        ch->position = POS_SLEEPING;
        break;

    case POS_FIGHTING: ch->send_line("You are already fighting!"); break;
    }
}

void do_wake(Char *ch, ArgParser args) {
    if (args.empty()) {
        /* Changed by rohan so that if you are resting or sitting, when you type
           wake, it will still make you stand up */
        /*      if (IS_AWAKE(ch)) {*/
        if (ch->position != POS_RESTING && ch->position != POS_SITTING && ch->position != POS_SLEEPING) {
            ch->send_line("You are already awake!");
            return;
        } else {
            do_stand(ch);
            return;
        }
    }

    if (!IS_AWAKE(ch)) {
        ch->send_line("You are asleep yourself!");
        return;
    }

    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
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

void do_sneak(Char *ch) {
    ch->send_line("You attempt to move silently.");
    affect_strip(ch, gsn_sneak);

    if (ch->is_npc() || number_percent() < get_skill_learned(ch, gsn_sneak)) {
        check_improve(ch, gsn_sneak, true, 3);
        AFFECT_DATA af;
        af.type = gsn_sneak;
        af.level = ch->level;
        af.duration = ch->level;
        af.bitvector = AFF_SNEAK;
        affect_to_char(ch, af);
    } else
        check_improve(ch, gsn_sneak, false, 3);
}

void do_hide(Char *ch) {
    ch->send_line("You attempt to hide.");

    if (IS_AFFECTED(ch, AFF_HIDE))
        REMOVE_BIT(ch->affected_by, AFF_HIDE);

    if (ch->is_npc() || number_percent() < ch->pcdata->learned[gsn_hide]) {
        SET_BIT(ch->affected_by, AFF_HIDE);
        check_improve(ch, gsn_hide, true, 3);
    } else
        check_improve(ch, gsn_hide, false, 3);
}

/*
 * Contributed by Alander.
 */
void do_visible(Char *ch) {
    affect_strip(ch, gsn_invis);
    affect_strip(ch, gsn_mass_invis);
    affect_strip(ch, gsn_sneak);
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
    REMOVE_BIT(ch->affected_by, AFF_SNEAK);
    ch->send_line("Ok.");
}

void do_recall(Char *ch, ArgParser args) {
    if (ch->is_npc() && !IS_SET(ch->act, ACT_PET)) {
        ch->send_line("Only players can recall.");
        return;
    }

    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n prays for transportation!", ch);

    auto vnum = rooms::MidgaardTemple;
    auto destination = args.shift();
    if (matches(destination, "clan")) {
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
                ch->send_line("You're not a member of a clan.");
                return;
            }
        }
    }

    auto *location = get_room_index(vnum);
    if (!location) {
        ch->send_line("You are completely lost.");
        return;
    }

    if (ch->in_room == location) {
        ch->send_line("You are there already.");
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE)) {
        ch->send_line("{} has forsaken you.", deity_name);
        return;
    }

    if (ch->fighting) {
        int lose, skill;

        if (ch->is_npc())
            skill = 40 + ch->level;
        else
            skill = get_skill_learned(ch, gsn_recall);

        if (number_percent() < 80 * skill / 100) {
            check_improve(ch, gsn_recall, false, 6);
            WAIT_STATE(ch, 4);
            ch->send_line("You failed!");
            return;
        }

        lose = (ch->desc != nullptr) ? 25 : 50;
        gain_exp(ch, 0 - lose);
        check_improve(ch, gsn_recall, true, 4);
        ch->send_line("You recall from combat!  You lose {} exps.", lose);
        stop_fighting(ch, true);
    }

    ch->move /= 2;
    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n disappears.", ch);
    char_from_room(ch);
    char_to_room(ch, location);
    if (!IS_SET(ch->act, PLR_WIZINVIS))
        act("$n appears in the room.", ch);

    look_auto(ch);

    if (ch->is_npc() && ch->ridden_by) {
        act("$n falls to the ground.", ch->ridden_by);
        act("You fall to the ground.", ch->ridden_by, nullptr, nullptr, To::Char);
        fallen_off_mount(ch->ridden_by);
    }

    if (ch->pet && ch->pet->in_room != location) {
        // We construct a new argparser here as the one we have has been modified to shift the arg off.
        do_recall(ch->pet, ArgParser(destination));
    }
}

namespace {
Char *find_trainer(ROOM_INDEX_DATA *room) {
    for (auto *mob : room->people) {
        if (mob->is_npc() && IS_SET(mob->act, ACT_TRAIN))
            return mob;
    }
    return nullptr;
}
}

void do_train(Char *ch, ArgParser args) {
    if (ch->is_npc())
        return;

    /*
     * Check for trainer.
     */
    Char *mob = find_trainer(ch->in_room);
    if (!mob) {
        ch->send_line("You can't do that here.");
        return;
    }

    auto to_train = args.shift();
    if (to_train.empty()) {
        ch->send_line("You have {} training sessions.", ch->train);
    }

    static constexpr auto cost = 1; // See(#222)

    if (auto maybe_stat = try_parse_stat(to_train)) {
        auto stat = *maybe_stat;
        if (ch->perm_stat[stat] >= get_max_train(ch, stat)) {
            act("Your $T is already at maximum.", ch, nullptr, to_long_string(stat), To::Char);
            return;
        }

        if (cost > ch->train) {
            ch->send_line("You don't have enough training sessions.");
            return;
        }

        ch->train -= cost;

        ch->perm_stat[stat] += 1;
        act("|WYour $T increases!|w", ch, nullptr, to_long_string(stat), To::Char);
        act("|W$n's $T increases!|w", ch, nullptr, to_long_string(stat), To::Room);
        return;
    }

    if (matches("hp", to_train)) {
        if (cost > ch->train) {
            ch->send_line("You don't have enough training sessions.");
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

    if (matches("mana", to_train)) {
        if (cost > ch->train) {
            ch->send_line("You don't have enough training sessions.");
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

    const std::array always_trainable = {"hp"sv, "mana"sv};
    ch->send_line("You can train: {}.",
                  fmt::join(ranges::views::concat(all_stats | ranges::views::filter([&](auto stat) {
                                                      return ch->perm_stat[stat] < get_max_train(ch, stat);
                                                  }) | ranges::views::transform(to_short_string),
                                                  always_trainable),
                            " "sv));
}
