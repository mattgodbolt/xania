/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_move.cpp: player and mobile movement                             */
/*                                                                       */
/*************************************************************************/

#include "act_move.hpp"
#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Classes.hpp"
#include "ContainerFlag.hpp"
#include "Exit.hpp"
#include "ExitFlag.hpp"
#include "MProg.hpp"
#include "Object.hpp"
#include "ObjectType.hpp"
#include "PlayerActFlag.hpp"
#include "Room.hpp"
#include "RoomFlag.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "VnumRooms.hpp"
#include "act_info.hpp"
#include "act_wiz.hpp"
#include "challenge.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "ride.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "update.hpp"

#include <fmt/format.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

using namespace std::literals;

constexpr PerSectorType<int> movement_loss{1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6};

void move_char(Char *ch, Direction direction) {
    auto *in_room = ch->in_room;
    const auto &exit = in_room->exits[direction];
    auto *to_room = exit ? exit->u1.to_room : nullptr;
    if (!exit || !to_room || !ch->can_see(*to_room)) {
        ch->send_line("Alas, you cannot go that way.");
        return;
    }

    if (check_enum_bit(exit->exit_info, ExitFlag::Closed) && ch->is_mortal()) {
        if (check_enum_bit(exit->exit_info, ExitFlag::PassProof) && ch->is_aff_pass_door()) {
            act("The $d is protected from trespass by a magical barrier.", ch, nullptr, exit->keyword, To::Char);
            return;
        } else {
            if (!ch->is_aff_pass_door()) {
                act("The $d is closed.", ch, nullptr, exit->keyword, To::Char);
                return;
            }
        }
    }

    if (check_enum_bit(exit->exit_info, ExitFlag::Closed) && ch->is_aff_pass_door()
        && !(check_enum_bit(exit->exit_info, ExitFlag::PassProof)) && ch->riding != nullptr
        && !ch->riding->is_aff_pass_door()) {
        ch->send_line("Your mount cannot travel through solid objects.");
        return;
    }

    if (ch->is_aff_charm() && ch->master != nullptr && in_room == ch->master->in_room) {
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
            if ((!ch->is_aff_fly() && ch->is_mortal()) && !(ch->riding != nullptr && ch->riding->is_aff_fly())) {
                ch->send_line("You can't fly.");
                return;
            }
        }

        if ((in_room->sector_type == SectorType::NonSwimmableWater
             || to_room->sector_type == SectorType::NonSwimmableWater)
            && !ch->is_aff_fly() && !(ch->riding != nullptr && ch->riding->is_aff_fly()) && !ch->has_boat()) {
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

            ch->wait_state(1);
            ch->move -= move;
        } else {
            move /= 2; /* Horses are better at moving ... well probably */
            if (ch->riding->move < move) {
                act("$N is too exhausted to carry you further.", ch, nullptr, ch->riding, To::Char);
                return;
            }
            ch->riding->wait_state(1);
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

    if (!(ch->is_aff_sneak() && (ch->riding != nullptr))
        && (ch->is_npc() || !ch->is_wizinvis() || !ch->is_prowlinvis())) {
        if (ch->ridden_by == nullptr) {
            if (ch->riding == nullptr) {
                act("$n leaves $T.", ch, nullptr, to_string(direction), To::Room);
            } else {
                act("$n rides $t on $N.", ch, to_string(direction), ch->riding, To::Room);
                act("You ride $t on $N.", ch, to_string(direction), ch->riding, To::Char);
            }
        }
    }

    char_from_room(ch);
    char_to_room(ch, to_room);

    if (!ch->is_aff_sneak() && (ch->is_npc() || !ch->is_wizinvis() || !ch->is_prowlinvis())) {
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
        if (fch->master == ch && fch->is_aff_charm() && fch->is_pos_preoccupied())
            do_stand(fch);

        if (fch->master == ch && fch->is_pos_standing()) {

            if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Law)
                && (fch->is_npc() && check_enum_bit(fch->act, CharActFlag::Aggressive))) {
                act("$N may not enter here.", ch, nullptr, fch, To::Char);
                act("You aren't allowed to go there.", fch, nullptr, nullptr, To::Char);
                return;
            }

            act("You follow $N.", fch, nullptr, ch, To::Char);
            move_char(fch, direction);
        }
    }

    MProg::entry_trigger(ch);
    MProg::greet_trigger(ch);
}

void do_enter(Char *ch, std::string_view argument) {
    Room *to_room, *in_room;
    int count = 0;
    auto &&[number, arg] = number_argument(argument);
    if (ch->riding) {
        ch->send_line("Before entering a portal you must dismount.");
        return;
    }
    in_room = ch->in_room;
    for (auto *obj : ch->in_room->contents) {
        if (ch->can_see(*obj) && (is_name(arg, obj->name))) {
            if (++count == number) {
                if (obj->type == ObjectType::Portal) {

                    to_room = obj->destination;

                    /* handle dangling portals, not that they should ever exist*/
                    if (to_room == nullptr) {
                        ch->send_line("That portal is highly unstable and is likely to get you killed!");
                        return;
                    }

                    if (ch->is_npc() && check_enum_bit(ch->act, CharActFlag::Aggressive)
                        && check_enum_bit(to_room->room_flags, RoomFlag::Law)) {
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
                            if ((!ch->is_aff_fly() && ch->is_mortal())
                                && !(ch->riding != nullptr && ch->riding->is_aff_fly())) {
                                ch->send_line("You can't fly.");
                                return;
                            }
                        }

                        if ((in_room->sector_type == SectorType::NonSwimmableWater
                             || to_room->sector_type == SectorType::NonSwimmableWater)
                            && !ch->is_aff_fly() && !(ch->riding != nullptr && ch->riding->is_aff_fly())
                            && !ch->has_boat()) {
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
                        if (fch->master == ch && fch->is_aff_charm() && fch->is_pos_preoccupied())
                            do_stand(fch);

                        if (fch->master == ch && fch->is_pos_standing()) {

                            if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Law)
                                && (fch->is_npc() && check_enum_bit(fch->act, CharActFlag::Aggressive))) {
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

                    MProg::entry_trigger(ch);
                    MProg::greet_trigger(ch);

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
    if (ch->in_room->vnum == Rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::North);
}

void do_east(Char *ch) {
    if (ch->in_room->vnum == Rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::East);
}

void do_south(Char *ch) {
    if (ch->in_room->vnum == Rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::South);
}

void do_west(Char *ch) {
    if (ch->in_room->vnum == Rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::West);
}

void do_up(Char *ch) {
    if (ch->in_room->vnum == Rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::Up);
}

void do_down(Char *ch) {
    if (ch->in_room->vnum == Rooms::ChallengeArena)
        do_room_check(ch);
    move_char(ch, Direction::Down);
}

std::optional<Direction> find_door(Char *ch, std::string_view arg) {
    if (auto direction = try_parse_direction(arg)) {
        const auto &exit = ch->in_room->exits[*direction];
        if (!exit) {
            act("I see no door $T here.", ch, nullptr, arg, To::Char);
            return {};
        }

        if (!check_enum_bit(exit->exit_info, ExitFlag::IsDoor)) {
            ch->send_line("You can't do that.");
            return {};
        }

        return direction;
    }

    for (auto direction : all_directions) {
        if (const auto &exit = ch->in_room->exits[direction]; exit && check_enum_bit(exit->exit_info, ExitFlag::IsDoor)
                                                              && !exit->keyword.empty() && is_name(arg, exit->keyword))
            return direction;
    }
    act("I see no $T here.", ch, nullptr, arg, To::Char);
    return {};
}

namespace {

// Performs an action on the reverse side of an Exit, if the Exit leads back to the origin room.
void update_reverse_exit(const Room *room, const auto &direction, const auto exit_action) {
    auto &exit = room->exits[direction];
    if (Room *to_room = exit->u1.to_room) {
        if (auto &exit_rev = to_room->exits[reverse(direction)]; exit_rev && exit_rev->u1.to_room == room) {
            exit_action(to_room, exit_rev);
        }
    }
}
}

void do_open(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Open what?");
        return;
    }

    auto arg = args.shift();

    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'open object' */
        if (obj->type != ObjectType::Container) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Closed)) {
            ch->send_line("It's already open.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Closeable)) {
            ch->send_line("You can't do that.");
            return;
        }
        if (check_enum_bit(obj->value[1], ContainerFlag::Locked)) {
            ch->send_line("It's locked.");
            return;
        }

        clear_enum_bit(obj->value[1], ContainerFlag::Closed);
        ch->send_line("Ok.");
        act("$n opens $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_direction = find_door(ch, arg)) {
        auto direction = *opt_direction;
        /* 'open door' */

        auto &exit = ch->in_room->exits[direction];
        if (!check_enum_bit(exit->exit_info, ExitFlag::Closed)) {
            ch->send_line("It's already open.");
            return;
        }
        if (check_enum_bit(exit->exit_info, ExitFlag::Locked)) {
            ch->send_line("It's locked.");
            return;
        }

        clear_enum_bit(exit->exit_info, ExitFlag::Closed);
        act("$n opens the $d.", ch, nullptr, exit->keyword, To::Room);
        ch->send_line("Ok.");

        const auto open_exit = [](const auto *to_room, auto &exit_rev) {
            clear_enum_bit(exit_rev->exit_info, ExitFlag::Closed);
            for (auto *rch : to_room->people)
                act("The $d opens.", rch, nullptr, exit_rev->keyword, To::Char);
        };
        update_reverse_exit(ch->in_room, direction, open_exit);
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
        if (obj->type != ObjectType::Container) {
            ch->send_line("That's not a container.");
            return;
        }
        if (check_enum_bit(obj->value[1], ContainerFlag::Closed)) {
            ch->send_line("It's already closed.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Closeable)) {
            ch->send_line("You can't do that.");
            return;
        }

        set_enum_bit(obj->value[1], ContainerFlag::Closed);
        ch->send_line("Ok.");
        act("$n closes $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_direction = find_door(ch, arg)) {
        auto direction = *opt_direction;
        /* 'close door' */

        auto &exit = ch->in_room->exits[direction];
        if (check_enum_bit(exit->exit_info, ExitFlag::Closed)) {
            ch->send_line("It's already closed.");
            return;
        }

        set_enum_bit(exit->exit_info, ExitFlag::Closed);
        act("$n closes the $d.", ch, nullptr, exit->keyword, To::Room);
        ch->send_line("Ok.");

        const auto close_exit = [](const auto *to_room, auto &exit_rev) {
            set_enum_bit(exit_rev->exit_info, ExitFlag::Closed);
            for (auto *rch : to_room->people)
                act("The $d closes.", rch, nullptr, exit_rev->keyword, To::Char);
        };
        update_reverse_exit(ch->in_room, direction, close_exit);
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
        if (obj->type != ObjectType::Container) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Closed)) {
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
        if (check_enum_bit(obj->value[1], ContainerFlag::Locked)) {
            ch->send_line("It's already locked.");
            return;
        }

        set_enum_bit(obj->value[1], ContainerFlag::Locked);
        ch->send_line("*Click*");
        act("$n locks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_direction = find_door(ch, arg)) {
        auto direction = *opt_direction;
        /* 'lock door' */

        auto &exit = ch->in_room->exits[direction];
        if (!check_enum_bit(exit->exit_info, ExitFlag::Closed)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (exit->key < 0) {
            ch->send_line("It can't be locked.");
            return;
        }
        if (!ch->carrying_object_vnum(exit->key)) {
            ch->send_line("You lack the key.");
            return;
        }
        if (check_enum_bit(exit->exit_info, ExitFlag::Locked)) {
            ch->send_line("It's already locked.");
            return;
        }

        set_enum_bit(exit->exit_info, ExitFlag::Locked);
        ch->send_line("*Click*");
        act("$n locks the $d.", ch, nullptr, exit->keyword, To::Room);

        const auto lock_exit = [](const auto *to_room, auto &exit_rev) {
            set_enum_bit(exit_rev->exit_info, ExitFlag::Locked);
            for (auto *rch : to_room->people)
                act("You hear a faint clicking sound.", rch, nullptr, exit_rev->keyword, To::Char);
        };
        update_reverse_exit(ch->in_room, direction, lock_exit);
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
        if (obj->type != ObjectType::Container) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Closed)) {
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
        if (!check_enum_bit(obj->value[1], ContainerFlag::Locked)) {
            ch->send_line("It's already unlocked.");
            return;
        }

        clear_enum_bit(obj->value[1], ContainerFlag::Locked);
        ch->send_line("*Click*");
        act("$n unlocks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_direction = find_door(ch, arg)) {
        auto direction = *opt_direction;
        /* 'unlock door' */
        auto &exit = ch->in_room->exits[direction];
        if (!check_enum_bit(exit->exit_info, ExitFlag::Closed)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (exit->key < 0) {
            ch->send_line("It can't be unlocked.");
            return;
        }
        if (!ch->carrying_object_vnum(exit->key)) {
            ch->send_line("You lack the key.");
            return;
        }
        if (!check_enum_bit(exit->exit_info, ExitFlag::Locked)) {
            ch->send_line("It's already unlocked.");
            return;
        }

        clear_enum_bit(exit->exit_info, ExitFlag::Locked);
        ch->send_line("*Click*");
        act("$n unlocks the $d.", ch, nullptr, exit->keyword, To::Room);

        const auto unlock_exit = []([[maybe_unused]] const auto *to_room, auto &exit_rev) {
            clear_enum_bit(exit_rev->exit_info, ExitFlag::Locked);
        };
        update_reverse_exit(ch->in_room, direction, unlock_exit);
    }
}

void do_pick(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Pick what?");
        return;
    }

    ch->wait_state(skill_table[gsn_pick_lock].beats);

    /* look for guards */
    for (auto *gch : ch->in_room->people) {
        if (gch->is_npc() && gch->is_pos_awake() && ch->level + 5 < gch->level) {
            act("$N is standing too close to the lock.", ch, nullptr, gch, To::Char);
            return;
        }
    }

    if (ch->is_pc() && number_percent() > ch->get_skill(gsn_pick_lock)) {
        ch->send_line("You failed.");
        check_improve(ch, gsn_pick_lock, false, 2);
        return;
    }

    auto arg = args.shift();
    if (auto *obj = get_obj_here(ch, arg)) {
        /* 'pick object' */
        if (obj->type != ObjectType::Container) {
            ch->send_line("That's not a container.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Closed)) {
            ch->send_line("It's not closed.");
            return;
        }
        if (obj->value[2] < 0) {
            ch->send_line("It can't be unlocked.");
            return;
        }
        if (!check_enum_bit(obj->value[1], ContainerFlag::Locked)) {
            ch->send_line("It's already unlocked.");
            return;
        }
        if (check_enum_bit(obj->value[1], ContainerFlag::PickProof)) {
            ch->send_line("You failed.");
            return;
        }

        clear_enum_bit(obj->value[1], ContainerFlag::Locked);
        ch->send_line("*Click*");
        check_improve(ch, gsn_pick_lock, true, 2);
        act("$n picks $p.", ch, obj, nullptr, To::Room);
        return;
    }

    if (auto opt_direction = find_door(ch, arg)) {
        auto direction = *opt_direction;
        /* 'pick door' */

        auto &exit = ch->in_room->exits[direction];
        if (!check_enum_bit(exit->exit_info, ExitFlag::Closed) && ch->is_mortal()) {
            ch->send_line("It's not closed.");
            return;
        }
        if (exit->key < 0 && ch->is_mortal()) {
            ch->send_line("It can't be picked.");
            return;
        }
        if (!check_enum_bit(exit->exit_info, ExitFlag::Locked)) {
            ch->send_line("It's already unlocked.");
            return;
        }
        if (check_enum_bit(exit->exit_info, ExitFlag::PickProof) && ch->is_mortal()) {
            ch->send_line("You failed.");
            return;
        }

        clear_enum_bit(exit->exit_info, ExitFlag::Locked);
        ch->send_line("*Click*");
        act("$n picks the $d.", ch, nullptr, exit->keyword, To::Room);
        check_improve(ch, gsn_pick_lock, true, 2);

        const auto unlock_exit = []([[maybe_unused]] const auto *to_room, auto &exit_rev) {
            clear_enum_bit(exit_rev->exit_info, ExitFlag::Locked);
        };
        update_reverse_exit(ch->in_room, direction, unlock_exit);
    }
}

void do_stand(Char *ch) {
    if (ch->riding != nullptr) {
        unride_char(ch, ch->riding);
        return;
    }

    switch (ch->position) {
    case Position::Type::Dead: ch->send_line("You're dead tired and can't get up!"); break;
    case Position::Type::Mortal:
    case Position::Type::Incap: ch->send_line("You're mortally wounded and can't stand up!"); break;
    case Position::Type::Stunned: ch->send_line("You're too disorientated to stand up!"); break;
    case Position::Type::Sleeping:
        if (ch->is_aff_sleep()) {
            ch->send_line("You can't wake up!");
            return;
        }

        ch->send_line("You wake and stand up.");
        act("$n wakes and stands up.", ch);
        ch->position = Position::Type::Standing;
        break;

    case Position::Type::Resting:
    case Position::Type::Sitting:
        ch->send_line("You stand up.");
        act("$n stands up.", ch);
        ch->position = Position::Type::Standing;
        break;

    case Position::Type::Standing: ch->send_line("You are already standing."); break;

    case Position::Type::Fighting: ch->send_line("You are already fighting!"); break;
    }
}

void do_rest(Char *ch) {
    if (ch->riding != nullptr) {
        ch->send_line("You cannot rest, the saddle is too uncomfortable!");
        return;
    }

    switch (ch->position) {
    case Position::Type::Sleeping:
        ch->send_line("You wake up and start resting.");
        act("$n wakes up and starts resting.", ch);
        ch->position = Position::Type::Resting;
        break;

    case Position::Type::Resting: ch->send_line("You are already resting."); break;

    case Position::Type::Standing:
        ch->send_line("You rest.");
        act("$n sits down and rests.", ch);
        ch->position = Position::Type::Resting;
        break;

    case Position::Type::Sitting:
        ch->send_line("You rest.");
        act("$n rests.", ch);
        ch->position = Position::Type::Resting;
        break;

    case Position::Type::Fighting: ch->send_line("You are already fighting!"); break;
    case Position::Type::Dead: ch->send_line("You're dead tired and can't get comfortable!"); break;
    case Position::Type::Mortal:
    case Position::Type::Incap: ch->send_line("You're mortally wounded and in too much pain to rest!"); break;
    case Position::Type::Stunned: ch->send_line("You're too disorientated to rest!"); break;
    }
}

void do_sit(Char *ch) {
    if (ch->riding != nullptr) {
        ch->send_line("You're already sitting in a saddle!");
        return;
    }

    switch (ch->position) {
    case Position::Type::Sleeping:
        ch->send_line("You wake up.");
        act("$n wakes and sits up.\n\r", ch);
        ch->position = Position::Type::Sitting;
        break;
    case Position::Type::Resting:
        ch->send_line("You stop resting.");
        ch->position = Position::Type::Sitting;
        break;
    case Position::Type::Sitting: ch->send_line("You are already sitting down."); break;
    case Position::Type::Fighting: ch->send_line("Maybe you should finish this fight first?"); break;
    case Position::Type::Standing:
        ch->send_line("You sit down.");
        act("$n sits down on the ground.\n\r", ch);
        ch->position = Position::Type::Sitting;
        break;
    case Position::Type::Dead: ch->send_line("You're dead tired and can't get comfortable!"); break;
    case Position::Type::Mortal:
    case Position::Type::Incap: ch->send_line("You're mortally wounded and in too much pain to sit!"); break;
    case Position::Type::Stunned: ch->send_line("You're too disorientated to sit!"); break;
    }
}

void do_sleep(Char *ch) {
    if (ch->riding != nullptr) {
        ch->send_line("You can't sleep - it's too uncomfortable in the saddle!");
        return;
    }

    switch (ch->position) {
    case Position::Type::Sleeping: ch->send_line("You are already sleeping."); break;

    case Position::Type::Resting:
    case Position::Type::Sitting:
    case Position::Type::Standing:
        ch->send_line("You go to sleep.");
        act("$n goes to sleep.", ch);
        ch->position = Position::Type::Sleeping;
        break;

    case Position::Type::Fighting: ch->send_line("You are already fighting!"); break;
    case Position::Type::Dead: ch->send_line("You're dead tired and can't get comfortabe!"); break;
    case Position::Type::Mortal:
    case Position::Type::Incap: ch->send_line("You're mortally wounded and in too much pain to sleep!"); break;
    case Position::Type::Stunned: ch->send_line("You're too disorientated to sleep!"); break;
    }
}

void do_wake(Char *ch, ArgParser args) {
    if (args.empty()) {
        /* o.g. changed by rohan so that if you are resting or sitting, when you type
           wake, it will still make you stand up. And 'wake' cannot be used when in any incapacitated state. */
        if (!ch->is_pos_relaxing()) {
            ch->send_line("You are already awake!");
            return;
        } else {
            do_stand(ch);
            return;
        }
    }

    if (!ch->is_pos_awake()) {
        ch->send_line("You are asleep yourself!");
        return;
    }

    auto *victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim->is_pos_awake()) {
        act("$N is already awake.", ch, nullptr, victim, To::Char);
        return;
    }

    if (victim->is_aff_sleep()) {
        act("You can't wake $M!", ch, nullptr, victim, To::Char);
        return;
    }

    victim->position = Position::Type::Standing;
    act("You wake $M.", ch, nullptr, victim, To::Char);
    act("$n wakes you.", ch, nullptr, victim, To::Vict);
}

void do_sneak(Char *ch) {
    ch->send_line("You attempt to move silently.");
    affect_strip(ch, gsn_sneak);

    if (number_percent() < ch->get_skill(gsn_sneak)) {
        check_improve(ch, gsn_sneak, true, 3);
        AFFECT_DATA af;
        af.type = gsn_sneak;
        af.level = ch->level;
        af.duration = ch->level;
        af.bitvector = to_int(AffectFlag::Sneak);
        affect_to_char(ch, af);
    } else
        check_improve(ch, gsn_sneak, false, 3);
}

void do_hide(Char *ch) {
    ch->send_line("You attempt to hide.");

    if (ch->is_aff_hide())
        clear_enum_bit(ch->affected_by, AffectFlag::Hide);

    if (ch->is_npc() || number_percent() < ch->pcdata->learned[gsn_hide]) {
        set_enum_bit(ch->affected_by, AffectFlag::Hide);
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
    clear_enum_bit(ch->affected_by, AffectFlag::Hide);
    clear_enum_bit(ch->affected_by, AffectFlag::Invisible);
    clear_enum_bit(ch->affected_by, AffectFlag::Sneak);
    ch->send_line("You slowly fade back into existence.");
    act("$n fades into existence.", ch);
}

void do_recall(Char *ch, ArgParser args) {
    if (ch->is_npc() && !check_enum_bit(ch->act, CharActFlag::Pet)) {
        ch->send_line("Only players can recall.");
        return;
    }

    if (!check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis))
        act("$n prays for transportation!", ch);

    auto vnum = Rooms::MidgaardTemple;
    auto destination = args.shift();
    if (matches(destination, "clan")) {
        if (check_enum_bit(ch->act, CharActFlag::Pet)) {
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

    auto *location = get_room(vnum);
    if (!location) {
        ch->send_line("You are completely lost.");
        return;
    }

    if (ch->in_room == location) {
        ch->send_line("You are there already.");
        return;
    }

    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::NoRecall) || ch->is_aff_curse()) {
        ch->send_line("{} has forsaken you.", deity_name);
        return;
    }

    if (ch->fighting) {
        const auto skill = ch->get_skill(gsn_recall);
        if (number_percent() < 80 * skill / 100) {
            check_improve(ch, gsn_recall, false, 6);
            ch->wait_state(4);
            ch->send_line("You failed!");
            return;
        }

        const auto lose_exp = (ch->desc != nullptr) ? 25 : 50;
        gain_exp(ch, 0 - lose_exp);
        check_improve(ch, gsn_recall, true, 4);
        ch->send_line("You recall from combat!  You lose {} exps.", lose_exp);
        stop_fighting(ch, true);
    }

    ch->move /= 2;
    if (!check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis))
        act("$n disappears.", ch);
    char_from_room(ch);
    char_to_room(ch, location);
    if (!check_enum_bit(ch->act, PlayerActFlag::PlrWizInvis))
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
Char *find_trainer(Room *room) {
    for (auto *mob : room->people) {
        if (mob->is_npc() && check_enum_bit(mob->act, CharActFlag::Train))
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
