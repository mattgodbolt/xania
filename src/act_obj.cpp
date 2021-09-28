/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_obj.cpp: interaction with objects                                */
/*                                                                       */
/*************************************************************************/

#include "act_obj.hpp"
#include "AFFECT_DATA.hpp"
#include "BitsAffect.hpp"
#include "BitsCharAct.hpp"
#include "BitsCommChannel.hpp"
#include "BitsContainerState.hpp"
#include "BitsDamageTolerance.hpp"
#include "BitsObjectExtra.hpp"
#include "BitsObjectWear.hpp"
#include "BitsPlayerAct.hpp"
#include "BitsRoomState.hpp"
#include "BitsWeaponFlag.hpp"
#include "BodySize.hpp"
#include "Exit.hpp"
#include "Logging.hpp"
#include "Materials.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "Pronouns.hpp"
#include "Room.hpp"
#include "Shop.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "Target.hpp"
#include "TimeInfoData.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "WearLocation.hpp"
#include "act_comm.hpp"
#include "act_wiz.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "magic.h"
#include "mob_prog.hpp"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/find.hpp>

#include <set>

extern const char *target_name; /* Included from magic.c */

/* RT part of the corpse looting code */

bool can_loot(const Char *ch, const Object *obj) {
    if (ch->is_immortal())
        return true;

    if (obj->owner.empty())
        return true;

    const Char *owner = nullptr;
    for (const auto *wch : char_list)
        if (matches(wch->name, obj->owner))
            owner = wch;

    if (owner == nullptr)
        return true;

    if (matches(ch->name, owner->name))
        return true;

    if (owner->is_pc() && check_bit(owner->act, PLR_CANLOOT))
        return true;

    if (is_same_group(ch, owner))
        return true;

    return false;
}

void get_obj(Char *ch, Object *obj, Object *container) {
    if (!obj->is_takeable()) {
        ch->send_line("You can't take that.");
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
        act("$d: you can't carry that many items.", ch, nullptr, obj->name, To::Char);
        return;
    }

    if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
        act("$d: you can't carry that much weight.", ch, nullptr, obj->name, To::Char);
        return;
    }

    if (!can_loot(ch, obj)) {
        act("|rCorpse looting is not permitted.|w", ch, nullptr, nullptr, To::Char);
        return;
    }

    if (obj_move_violates_uniqueness(container != nullptr ? container->carried_by : nullptr, ch, obj, ch->carrying)) {
        act(deity_name + " forbids you from possessing more than one $p.", ch, obj, nullptr, To::Char);
        return;
    }
    if (container != nullptr) {

        if (container->objIndex->vnum == objects::Pit && ch->get_trust() < obj->level) {
            ch->send_line("You are not powerful enough to use it.");
            return;
        }

        if (container->objIndex->vnum == objects::Pit && !container->is_takeable() && obj->timer)
            obj->timer = 0;
        act("You get $p from $P.", ch, obj, container, To::Char);
        act("$n gets $p from $P.", ch, obj, container, To::Room);
        obj_from_obj(obj);
    } else {
        act("You get $p.", ch, obj, container, To::Char);
        act("$n gets $p.", ch, obj, container, To::Room);
        obj_from_room(obj);
    }

    if (obj->type == ObjectType::Money) {
        ch->gold += obj->value[0];
        if (check_bit(ch->act, PLR_AUTOSPLIT)) { /* AUTOSPLIT code */
            if (ch->num_group_members_in_room() > 1 && obj->value[0] > 1) {
                split_coins(ch, obj->value[0]);
            }
        }

        extract_obj(obj);
    } else {
        obj_to_char(obj, ch);
    }
}

namespace {

/*
 * Remove an object.
 */
bool remove_obj(Char *ch, int iWear, bool fReplace) {
    Object *obj;

    if ((obj = get_eq_char(ch, iWear)) == nullptr)
        return true;

    if (!fReplace)
        return false;

    if (obj->is_no_remove()) {
        act("|rYou can't remove $p because it is cursed. Seek one who is skilled in benedictions.|w", ch, obj, nullptr,
            To::Char);
        return false;
    }

    unequip_char(ch, obj);
    act("$n stops using $p.", ch, obj, nullptr, To::Room);
    act("You stop using $p.", ch, obj, nullptr, To::Char);
    return true;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj(Char *ch, Object *obj, bool fReplace) {
    if (ch->level < obj->level) {
        ch->send_line("You must be level {} to use this object.", obj->level);
        act("$n tries to use $p, but is too inexperienced.", ch, obj, nullptr, To::Room);
        return;
    }

    bool use_default_message = obj->wear_string.empty();
    if (!use_default_message) {
        act("$n uses $p.", ch, obj, nullptr, To::Room);
        act("You use $p.", ch, obj, nullptr, To::Char);
    }

    if (obj->type == ObjectType::Light) {
        if (!remove_obj(ch, WEAR_LIGHT, fReplace))
            return;

        if (use_default_message) {
            act("$n lights $p and holds it.", ch, obj, nullptr, To::Room);
            act("You light $p and hold it.", ch, obj, nullptr, To::Char);
        }

        equip_char(ch, obj, WEAR_LIGHT);
        return;
    }

    if (obj->is_wear_finger()) {
        if (get_eq_char(ch, WEAR_FINGER_L) != nullptr && get_eq_char(ch, WEAR_FINGER_R) != nullptr
            && !remove_obj(ch, WEAR_FINGER_L, fReplace) && !remove_obj(ch, WEAR_FINGER_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_FINGER_L) == nullptr) {
            if (use_default_message) {
                act("$n wears $p on $s left finger.", ch, obj, nullptr, To::Room);
                act("You wear $p on your left finger.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_FINGER_L);
            return;
        }

        if (get_eq_char(ch, WEAR_FINGER_R) == nullptr) {
            if (use_default_message) {
                act("$n wears $p on $s right finger.", ch, obj, nullptr, To::Room);
                act("You wear $p on your right finger.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_FINGER_R);
            return;
        }

        bug("Wear_obj: no free finger.");
        ch->send_line("You already wear two rings.");
        return;
    }

    if (obj->is_wear_neck()) {
        if (get_eq_char(ch, WEAR_NECK_1) != nullptr && get_eq_char(ch, WEAR_NECK_2) != nullptr
            && !remove_obj(ch, WEAR_NECK_1, fReplace) && !remove_obj(ch, WEAR_NECK_2, fReplace))
            return;

        if (get_eq_char(ch, WEAR_NECK_1) == nullptr) {
            if (use_default_message) {
                act("$n wears $p around $s neck.", ch, obj, nullptr, To::Room);
                act("You wear $p around your neck.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_NECK_1);
            return;
        }

        if (get_eq_char(ch, WEAR_NECK_2) == nullptr) {
            if (use_default_message) {
                act("$n wears $p around $s neck.", ch, obj, nullptr, To::Room);
                act("You wear $p around your neck.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_NECK_2);
            return;
        }

        bug("Wear_obj: no free neck.");
        ch->send_line("You already wear two neck items.");
        return;
    }

    if (obj->is_wear_body()) {
        if (!remove_obj(ch, WEAR_BODY, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s body.", ch, obj, nullptr, To::Room);
            act("You wear $p on your body.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_BODY);
        return;
    }

    if (obj->is_wear_head()) {
        if (!remove_obj(ch, WEAR_HEAD, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s head.", ch, obj, nullptr, To::Room);
            act("You wear $p on your head.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_HEAD);
        return;
    }

    if (obj->is_wear_legs()) {
        if (!remove_obj(ch, WEAR_LEGS, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s legs.", ch, obj, nullptr, To::Room);
            act("You wear $p on your legs.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_LEGS);
        return;
    }

    if (obj->is_wear_feet()) {
        if (!remove_obj(ch, WEAR_FEET, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s feet.", ch, obj, nullptr, To::Room);
            act("You wear $p on your feet.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_FEET);
        return;
    }

    if (obj->is_wear_hands()) {
        if (!remove_obj(ch, WEAR_HANDS, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s hands.", ch, obj, nullptr, To::Room);
            act("You wear $p on your hands.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_HANDS);
        return;
    }

    if (obj->is_wear_arms()) {
        if (!remove_obj(ch, WEAR_ARMS, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s arms.", ch, obj, nullptr, To::Room);
            act("You wear $p on your arms.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_ARMS);
        return;
    }

    if (obj->is_wear_about()) {
        if (!remove_obj(ch, WEAR_ABOUT, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p about $s body.", ch, obj, nullptr, To::Room);
            act("You wear $p about your body.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_ABOUT);
        return;
    }

    if (obj->is_wear_waist()) {
        if (!remove_obj(ch, WEAR_WAIST, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p about $s waist.", ch, obj, nullptr, To::Room);
            act("You wear $p about your waist.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_WAIST);
        return;
    }

    if (obj->is_wear_wrist()) {
        if (get_eq_char(ch, WEAR_WRIST_L) != nullptr && get_eq_char(ch, WEAR_WRIST_R) != nullptr
            && !remove_obj(ch, WEAR_WRIST_L, fReplace) && !remove_obj(ch, WEAR_WRIST_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_WRIST_L) == nullptr) {
            if (use_default_message) {
                act("$n wears $p around $s left wrist.", ch, obj, nullptr, To::Room);
                act("You wear $p around your left wrist.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_WRIST_L);
            return;
        }

        if (get_eq_char(ch, WEAR_WRIST_R) == nullptr) {
            if (use_default_message) {
                act("$n wears $p around $s right wrist.", ch, obj, nullptr, To::Room);
                act("You wear $p around your right wrist.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_WRIST_R);
            return;
        }

        bug("Wear_obj: no free wrist.");
        ch->send_line("You already wear two wrist items.");
        return;
    }

    if (obj->is_wear_shield()) {
        Object *weapon;

        if (!remove_obj(ch, WEAR_SHIELD, fReplace))
            return;

        weapon = get_eq_char(ch, WEAR_WIELD);
        if (weapon != nullptr && ch->body_size < BodySize::Large && weapon->is_weapon_two_handed()) {
            ch->send_line("Your hands are tied up with your weapon!");
            return;
        }

        if (use_default_message) {
            act("$n wears $p as a shield.", ch, obj, nullptr, To::Room);
            act("You wear $p as a shield.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_SHIELD);
        return;
    }

    if (obj->is_wieldable()) {
        int sn, skill;

        if (!remove_obj(ch, WEAR_WIELD, fReplace))
            return;

        if (ch->is_pc() && get_obj_weight(obj) > str_app[get_curr_stat(ch, Stat::Str)].wield) {
            ch->send_line("It is too heavy for you to wield.");
            return;
        }

        if (ch->is_pc() && ch->body_size < BodySize::Large && obj->is_weapon_two_handed()
            && get_eq_char(ch, WEAR_SHIELD) != nullptr) {
            ch->send_line("You need two hands free for that weapon.");
            return;
        }

        if (use_default_message) {
            act("$n wields $p.", ch, obj, nullptr, To::Room);
            act("You wield $p.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_WIELD);

        sn = get_weapon_sn(ch);

        if (sn == gsn_hand_to_hand)
            return;

        skill = get_weapon_skill(ch, sn);

        if (skill >= 100)
            act("$p feels like a part of you!", ch, obj, nullptr, To::Char);
        else if (skill > 85)
            act("You feel quite confident with $p.", ch, obj, nullptr, To::Char);
        else if (skill > 70)
            act("You are skilled with $p.", ch, obj, nullptr, To::Char);
        else if (skill > 50)
            act("Your skill with $p is adequate.", ch, obj, nullptr, To::Char);
        else if (skill > 25)
            act("$p feels a little clumsy in your hands.", ch, obj, nullptr, To::Char);
        else if (skill > 1)
            act("You fumble and almost drop $p.", ch, obj, nullptr, To::Char);
        else
            act("You don't even know which is end is up on $p.", ch, obj, nullptr, To::Char);

        return;
    }

    if (obj->is_holdable())
        if (!(ch->is_npc() && check_bit(ch->act, ACT_PET))) {
            if (!remove_obj(ch, WEAR_HOLD, fReplace))
                return;
            if (use_default_message) {
                act("$n holds $p in $s hands.", ch, obj, nullptr, To::Room);
                act("You hold $p in your hands.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_HOLD);
            return;
        }

    if (obj->is_wear_ears()) {
        if (!remove_obj(ch, WEAR_EARS, fReplace))
            return;
        if (use_default_message) {
            act("$n wears $p on $s ears.", ch, obj, nullptr, To::Room);
            act("You wear $p on your ears.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_EARS);
        return;
    }

    if (fReplace)
        ch->send_line("You can't wear, wield, or hold that.");
}

bool is_mass_looting_npc_undroppable_obj(const Object *obj, const Object *container, const char looting_all_item_dot) {
    return container->type == ObjectType::Npccorpse && looting_all_item_dot == '\0'
           && (obj->is_no_drop() || obj->is_no_remove());
}

Char *shopkeeper_in(const Room &room) {
    for (auto *maybe_keeper : room.people) {
        if (maybe_keeper->is_npc() && maybe_keeper->mobIndex->shop)
            return maybe_keeper;
    }
    return nullptr;
}

Char *find_keeper(Char *ch) {
    auto *keeper = shopkeeper_in(*ch->in_room);
    if (!keeper) {
        ch->send_line("You can't do that here.");
        return nullptr;
    }

    // Undesirables.
    if (ch->is_player_killer()) {
        keeper->say("Killers are not welcome!");
        keeper->yell(fmt::format("{} the KILLER is over here!", ch->name));
        return nullptr;
    }

    if (ch->is_player_thief()) {
        keeper->say("Thieves are not welcome!");
        keeper->yell(fmt::format("{} the THIEF is over here!", ch->name));
        return nullptr;
    }

    // Shop hours.
    const auto *shop = keeper->mobIndex->shop;
    if (time_info.hour() < shop->open_hour) {
        keeper->say("Sorry, I am closed. Come back later.");
        return nullptr;
    }

    if (time_info.hour() > shop->close_hour) {
        keeper->say("Sorry, I am closed. Come back tomorrow.");
        return nullptr;
    }

    // Invisible or hidden people.
    if (!keeper->can_see(*ch)) {
        keeper->say("I don't trade with folks I can't see.");
        return nullptr;
    }

    return keeper;
}

int get_cost(Char *keeper, Object *obj, bool fBuy) {
    Shop *shop;
    int cost;

    if (obj == nullptr || (shop = keeper->mobIndex->shop) == nullptr)
        return 0;

    if (fBuy) {
        cost = obj->cost * shop->profit_buy / 100;
    } else {
        uint itype;
        cost = 0;
        for (itype = 0; itype < MaxTrade; itype++) {
            if (obj->type == shop->buy_type[itype]) {
                cost = obj->cost * shop->profit_sell / 100;
                break;
            }
        }

        for (auto *obj2 : keeper->carrying) {
            if (obj->objIndex == obj2->objIndex) {
                cost = 0;
                break;
            }
        }
    }

    if (obj->type == ObjectType::Staff || obj->type == ObjectType::Wand)
        cost = cost * obj->value[2] / obj->value[1];

    return cost;
}

}

void do_get(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object *container;
    bool found;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "from"))
        argument = one_argument(argument, arg2);

    /* Get type. */
    if (arg1[0] == '\0') {
        ch->send_line("Get what?");
        return;
    }

    if (arg2[0] == '\0') {
        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            /* 'get obj' */
            auto *obj = get_obj_list(ch, arg1, ch->in_room->contents);
            if (!obj) {
                act("I see no $T here.", ch, nullptr, arg1, To::Char);
                return;
            }

            get_obj(ch, obj, nullptr);
        } else {
            /* 'get all' or 'get all.obj' */
            found = false;
            for (auto *obj : ch->in_room->contents) {
                if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj)) {
                    found = true;
                    get_obj(ch, obj, nullptr);
                }
            }

            if (!found) {
                if (arg1[3] == '\0')
                    ch->send_line("I see nothing here.");
                else
                    act("I see no $T here.", ch, nullptr, &arg1[4], To::Char);
            }
        }
    } else {
        /* 'get ... container' */
        if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
            ch->send_line("You can't do that.");
            return;
        }

        if ((container = get_obj_here(ch, arg2)) == nullptr) {
            act("I see no $T here.", ch, nullptr, arg2, To::Char);
            return;
        }

        switch (container->type) {
        default: ch->send_line("That's not a container."); return;

        case ObjectType::Container:
        case ObjectType::Npccorpse: break;

        case ObjectType::Pccorpse: {

            if (!can_loot(ch, container)) {
                ch->send_line("You can't do that.");
                return;
            }
        }
        }

        if (check_bit(container->value[1], CONT_CLOSED)) {
            act("The $d is closed.", ch, nullptr, container->name, To::Char);
            return;
        }

        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            /* 'get obj container' */
            auto *obj = get_obj_list(ch, arg1, container->contains);
            if (!obj) {
                act("I see nothing like that in the $T.", ch, nullptr, arg2, To::Char);
                return;
            }
            get_obj(ch, obj, container);
        } else {
            /* 'get all container' or 'get all.obj container' */
            found = false;
            for (auto *obj : container->contains) {
                if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj)) {
                    found = true;
                    if (container->objIndex->vnum == objects::Pit && ch->is_mortal()) {
                        ch->send_line("Don't be so greedy!");
                        return;
                    }
                    if (is_mass_looting_npc_undroppable_obj(obj, container, arg1[3])) {
                        act("You refrain from looting $p as it appears to be undroppable. You can still 'get' it using "
                            "its name.",
                            ch, obj, nullptr, To::Char);
                        act("$n refrains from looting $p as it appears to be undroppable.", ch, obj, nullptr, To::Room);
                        continue;
                    }

                    get_obj(ch, obj, container);
                }
            }

            if (!found) {
                if (arg1[3] == '\0')
                    act("I see nothing in the $T.", ch, nullptr, arg2, To::Char);
                else
                    act("I see nothing like that in the $T.", ch, nullptr, arg2, To::Char);
            }
        }
    }
}

void do_put(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object *container;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "in"))
        argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        ch->send_line("Put what in what?");
        return;
    }

    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
        ch->send_line("You can't do that.");
        return;
    }

    if ((container = get_obj_here(ch, arg2)) == nullptr) {
        act("I see no $T here.", ch, nullptr, arg2, To::Char);
        return;
    }

    if (container->type != ObjectType::Container) {
        ch->send_line("That's not a container.");
        return;
    }

    if (check_bit(container->value[1], CONT_CLOSED)) {
        act("The $d is closed.", ch, nullptr, container->name, To::Char);
        return;
    }

    if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
        /* put obj container' */
        auto *obj = get_obj_carry(ch, arg1);
        if (!obj) {
            ch->send_line("You do not have that item.");
            return;
        }

        if (obj == container) {
            ch->send_line("You can't fold it into itself.");
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            ch->send_line("You can't let go of it.");
            return;
        }

        if (obj_move_violates_uniqueness(ch, container->carried_by, obj, container)) {
            act(deity_name + " forbids you from putting more than one $p there, even cheap replicas.", ch, obj, nullptr,
                To::Char);
            return;
        }

        if (get_obj_weight(obj) + get_obj_weight(container) > container->value[0]) {
            ch->send_line("It won't fit.");
            return;
        }

        if (container->objIndex->vnum == objects::Pit && !container->is_takeable()) {
            if (obj->timer) {
                ch->send_line("Only permanent items may go in the pit.");
                return;
            } else
                obj->timer = number_range(100, 200);
        }

        obj_from_char(obj);
        obj_to_obj(obj, container);
        act("$n puts $p in $P.", ch, obj, container, To::Room);
        act("You put $p in $P.", ch, obj, container, To::Char);
    } else {
        /* 'put all container' or 'put all.obj container' */
        for (auto *obj : ch->carrying) {
            if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE
                && obj != container && can_drop_obj(ch, obj)
                && get_obj_weight(obj) + get_obj_weight(container) <= container->value[0]) {

                if (obj_move_violates_uniqueness(ch, container->carried_by, obj, container)) {
                    act(deity_name + " forbids you from putting more than one $p there, even cheap replicas.", ch, obj,
                        nullptr, To::Char);
                    continue;
                }
                if (container->objIndex->vnum == objects::Pit) {
                    if (obj->timer)
                        continue;
                    else
                        obj->timer = number_range(100, 200);
                }

                obj_from_char(obj);
                obj_to_obj(obj, container);
                act("$n puts $p in $P.", ch, obj, container, To::Room);
                act("You put $p in $P.", ch, obj, container, To::Char);
            }
        }
    }
}

void do_donate(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    /* Get command argument and ensure that one has been given */
    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Donate what?");
        return;
    }

    /* get the pit's Object * */
    auto *altar = get_room(rooms::MidgaardAltar);

    auto pit_it = ranges::find(altar->contents, objects::Pit, [](auto *obj) { return obj->objIndex->vnum; });
    if (pit_it == altar->contents.end()) {
        /* just in case someone should accidentally delete the pit... */
        ch->send_line("The psychic field seems to have lost its alignment.");
        return;
    }
    auto *pit = *pit_it;

    /* check to see if the ch is currently cursed */
    if (ch->is_aff_curse()) {
        ch->send_line("The psychic flux seems to be avoiding you today.");
        return;
    }

    /* check to see if the ch is in a non-recall room */
    if (check_bit(ch->in_room->room_flags, ROOM_NO_RECALL)) {
        ch->send_line("The psychic flux is not strong enough here.");
        return;
    }

    /* check if 'all' or 'all.' has been used */
    if (str_cmp(arg, "all") && str_prefix("all.", arg)) { /* this returns true if NEITHER matched */

        auto *obj = get_obj_carry(ch, arg);
        if (!obj) {
            ch->send_line("You do not have that item.");
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            ch->send_line("You can't let go of it.");
            return;
        }

        if (obj->timer) {
            ch->send_line("That would just get torn apart by the psychic vortices.");
            return;
        }

        obj->timer = number_range(100, 200);

        /* move the item, and echo the relevant message to witnesses */
        obj_from_char(obj);
        obj_to_obj(obj, pit);
        if (pit->in_room->vnum == ch->in_room->vnum) {
            act("$n raises $p in $s hands, and in a whirl of psychic flux, it flies into the pit.", ch, obj, pit,
                To::Room);
            act("$p rockets into the pit, nearly taking your arm with it.", ch, obj, nullptr, To::Char);
        } else {
            act("$p disappears in a whirl of psychic flux.", ch, obj, nullptr, To::Char);
            act("$n raises $p in $s hands, and it disappears in a whirl of psychic flux.", ch, obj, pit, To::Room);
            act("$p appears in a whirl of psychic flux and drops into the pit.", ch, obj, pit->in_room, To::GivenRoom);
        }
    } else {
        /* 'put all container' or 'put all.obj container' */
        for (auto *obj : ch->carrying) {
            if ((arg[3] == '\0' || is_name(&arg[4], obj->name)) && can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE
                && can_drop_obj(ch, obj) && obj->timer == 0) {
                obj->timer = number_range(100, 200);
                obj_from_char(obj);
                obj_to_obj(obj, pit);
                act("$p disappears in a whirl of psychic flux.", ch, obj, nullptr, To::Char);
                act("$n raises $p in $s hands, and it disappears in a whirl of psychic flux.", ch, obj, pit, To::Room);
                act("$p appears in a whirl of psychic flux and drops into the pit.", ch, obj, pit->in_room,
                    To::GivenRoom);
            }
        }
    }
}

void do_drop(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    bool found;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Drop what?");
        return;
    }

    if (is_number(arg)) {
        /* 'drop NNNN coins' */
        int amount;

        amount = atoi(arg);
        argument = one_argument(argument, arg);
        if (amount <= 0 || (str_cmp(arg, "coins") && str_cmp(arg, "coin") && str_cmp(arg, "gold"))) {
            ch->send_line("Sorry, you can't do that.");
            return;
        }

        if (ch->gold < amount) {
            ch->send_line("You haven't got that many coins.");
            return;
        }

        ch->gold -= amount;

        for (auto *obj : ch->in_room->contents) {
            switch (obj->objIndex->vnum) {
            case objects::MoneyOne:
                amount += 1;
                extract_obj(obj);
                break;

            case objects::MoneySome:
                amount += obj->value[0];
                extract_obj(obj);
                break;
            }
        }

        obj_to_room(create_money(amount), ch->in_room);
        act("$n drops some gold.", ch);
        ch->send_line("OK.");
        return;
    }

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        /* 'drop obj' */
        auto *obj = get_obj_carry(ch, arg);
        if (!obj) {
            ch->send_line("You do not have that item.");
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            ch->send_line("You can't let go of it. Use 'trash' if you want to destroy it.");
            return;
        }

        obj_from_char(obj);
        if (!check_sub_issue(obj, ch)) {
            obj_to_room(obj, ch->in_room);
            act("$n drops $p.", ch, obj, nullptr, To::Room);
            act("You drop $p.", ch, obj, nullptr, To::Char);
        }
    } else {
        /* 'drop all' or 'drop all.obj' */
        found = false;
        for (auto *obj : ch->carrying) {
            if ((arg[3] == '\0' || is_name(&arg[4], obj->name)) && can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE
                && can_drop_obj(ch, obj)) {
                found = true;
                obj_from_char(obj);
                if (!check_sub_issue(obj, ch)) {
                    obj_to_room(obj, ch->in_room);
                    act("$n drops $p.", ch, obj, nullptr, To::Room);
                    act("You drop $p.", ch, obj, nullptr, To::Char);
                }
            }
        }

        if (!found) {
            if (arg[3] == '\0')
                act("You are not carrying anything.", ch, nullptr, arg, To::Char);
            else
                act("You are not carrying any $T.", ch, nullptr, &arg[4], To::Char);
        }
    }
}

namespace {
void do_give_coins(Char *ch, std::string_view num_coins, ArgParser args) {
    /* 'give NNNN coins victim' */
    const auto amount = parse_number(num_coins);
    const auto noun = args.shift();
    if (amount <= 0 || !(matches(noun, "coins") || matches(noun, "coin") || matches(noun, "gold"))) {
        ch->send_line("Sorry, you can't do that.");
        return;
    }

    const auto victim_name = args.shift();
    if (victim_name.empty()) {
        ch->send_line("Give what to whom?");
        return;
    }

    auto *victim = get_char_room(ch, victim_name);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (ch->gold < amount) {
        ch->send_line("You haven't got that much gold.");
        return;
    }
    // added because people were using charmed mobs to do some cunning things - fara
    if (victim->carry_weight + 1 > can_carry_w(ch)) {
        ch->send_line("They can't carry any more weight.");
        return;
    }
    if (victim->carry_number + 1 > can_carry_n(ch)) {
        ch->send_line("Their hands are full.");
        return;
    }

    ch->gold -= amount;
    victim->gold += amount;
    act(fmt::format("$n gives you {} gold.", amount), ch, nullptr, victim, To::Vict);
    act("$n gives $N some gold.", ch, nullptr, victim, To::NotVict);
    act(fmt::format("You give $N {} gold.", amount), ch, nullptr, victim, To::Char);
    mprog_bribe_trigger(victim, ch, amount);
}
}

void do_give(Char *ch, ArgParser args) {
    const auto item_name = args.shift();
    if (is_number(item_name)) {
        do_give_coins(ch, item_name, args);
        return;
    }

    const auto victim_name = args.shift();
    if (item_name.empty() || victim_name.empty()) {
        ch->send_line("Give what to whom?");
        return;
    }

    auto *obj = ch->find_in_inventory(item_name);
    if (!obj) {
        ch->send_line("You do not have that item.");
        return;
    }

    auto *victim = get_char_room(ch, victim_name);
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }
    // prevent giving to yourself!
    if (victim == ch) {
        ch->send_line("Give something to yourself? What, are you insane?!");
        return;
    }

    ch->try_give_item_to(obj, victim);
}

namespace {

Object *find_pour_target(GenericList<Object *> &list, Object *obj) {
    for (auto *target_obj : list) {
        if (target_obj->type == ObjectType::Drink && obj != target_obj) {
            return target_obj;
        }
    }
    return nullptr;
}

void pour_from_to(Object *obj, Object *target_obj) {
    int pour_volume = 0;
    do {
        pour_volume++;
        obj->value[1]--;
        target_obj->value[1]++;
    } while ((obj->value[1] > 0) && (target_obj->value[1] < target_obj->value[0]) && (pour_volume < 50));
}

// Recursively collect the object index data of all in the source object list having the ITEM_UNIQUE flag.
void collect_unique_obj_indexes(const GenericList<Object *> &objects, std::set<ObjectIndex *> &unique_obj_idxs) {

    for (const auto object : objects) {
        if (check_bit(object->extra_flags, ITEM_UNIQUE)) {
            unique_obj_idxs.insert(object->objIndex);
        }
        // This is a small optimization as all Objects have a contains list but only these types are valid containers.
        if (object->type == ObjectType::Container || object->type == ObjectType::Npccorpse
            || object->type == ObjectType::Pccorpse) {
            collect_unique_obj_indexes(object->contains, unique_obj_idxs);
        }
    }
}

}

/*
 *  pour for Xania, Faramir 30/6/96
 */

void do_pour(Char *ch, const char *argument) {

    char arg1[MAX_INPUT_LENGTH];
    Char *victim;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0') {
        ch->send_line("What do you wish to pour?");
        return;
    }

    auto *obj = get_obj_carry(ch, arg1);
    if (!obj) {
        ch->send_line("You do not have that item.");
        return;
    }

    if (obj->type != ObjectType::Drink) {
        ch->send_line("You cannot pour that.");
        return;
    }

    one_argument(argument, arg1);

    if (obj->value[1] == 0) {
        ch->send_line("Your liquid container is empty.");
        return;
    }

    if (arg1[0] == '\0') {

        act("You empty $p out onto the ground.", ch, obj, nullptr, To::Char);
        obj->value[1] = 0;

        return;
    }

    if (!str_prefix(arg1, "in")) {
        auto *target_obj = find_pour_target(ch->carrying, obj);
        if (!target_obj) {
            ch->send_line("Pour what into what?!");
            return;
        }

        if (target_obj->value[2] != obj->value[2]) {
            ch->send_line("{} already contains another type of liquid!", target_obj->name);
            return;
        }

        pour_from_to(obj, target_obj);

        ch->send_to("You pour the contents of {} into {}", obj->short_descr, target_obj->short_descr);
        return;
    }

    victim = get_char_room(ch, arg1);

    if (victim == nullptr) {
        ch->send_line("They're not here.");
        return;
    }

    if (victim == ch) {
        ch->send_line("Pour it into which of your own containers?\n\rPour <container1> in <container2>");
        return;
    }

    auto *target_obj = find_pour_target(victim->carrying, nullptr);
    if (!target_obj) {
        ch->send_line("{} is not carrying an item which can be filled.", victim->short_name());
        return;
    }

    if (target_obj->value[2] != obj->value[2]) {
        ch->send_line(fmt::format("{}'s {} appears to contain a different type of liquid!", victim->short_name(),
                                  target_obj->short_descr));
        return;
    }

    if (target_obj->value[1] >= target_obj->value[0]) {
        ch->send_line("{}'s liquid container is already full to the brim!", victim->short_name());
        return;
    }

    pour_from_to(obj, target_obj);

    act("You pour $p into $n's container.", ch, obj, nullptr, To::Char);
    ch->send_line("{} pours liquid into your container.", victim->short_name());
}

namespace {
Object *find_fountain(GenericList<Object *> &list) {
    for (auto *fountain : list)
        if (fountain->type == ObjectType::Fountain)
            return fountain;
    return nullptr;
}
}

void do_fill(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Fill what?");
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        ch->send_line("You do not have that item.");
        return;
    }

    auto fountain = find_fountain(ch->in_room->contents);
    if (!fountain) {
        ch->send_line("There is no fountain here!");
        return;
    }

    if (obj->type != ObjectType::Drink) {
        ch->send_line("You can't fill that.");
        return;
    }

    if (obj->value[1] != 0 && obj->value[2] != 0) {
        ch->send_line("There is already another liquid in it.");
        return;
    }

    if (obj->value[1] >= obj->value[0]) {
        ch->send_line("Your container is full.");
        return;
    }

    act("You fill $p.", ch, obj, nullptr, To::Char);
    obj->value[2] = 0;
    obj->value[1] = obj->value[0];
}

void do_drink(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    int amount;
    one_argument(argument, arg);

    if (arg[0] == '\0') {
        obj = find_fountain(ch->in_room->contents);
        if (!obj) {
            ch->send_line("Drink what?");
            return;
        }
    } else {
        if ((obj = get_obj_here(ch, arg)) == nullptr) {
            ch->send_line("You can't find it.");
            return;
        }
    }

    if (ch->is_inebriated()) {
        ch->send_line("You fail to reach your mouth.  *Hic*");
        return;
    }

    switch (obj->type) {
    default: ch->send_line("You can't drink from that."); break;

    case ObjectType::Fountain:
        act("$n drinks from $p.", ch, obj, nullptr, To::Room);
        act("You drink from $p.", ch, obj, nullptr, To::Char);
        if (const auto opt_message = ch->delta_thirst(48)) {
            ch->send_line(*opt_message);
        }
        break;

    case ObjectType::Drink:
        if (obj->value[1] <= 0) {
            ch->send_line("It is already empty.");
            return;
        }
        const auto *liquid = Liquid::get_by_index(obj->value[2]);
        if (!liquid) {
            bug("{} attempted to drink a liquid having an unknown type: {} {} -> {}", ch->name, obj->objIndex->vnum,
                obj->short_descr, obj->value[2]);
            return;
        }
        act("$n drinks $T from $p.", ch, obj, liquid->name, To::Room);
        act("You drink $T from $p.", ch, obj, liquid->name, To::Char);
        // Liquids are more complex than foods because they can have variable bonuses to all three nutrition
        // types including negative ones.
        amount = number_range(3, 10);
        amount = std::min(amount, obj->value[1]);
        if (const auto opt_message =
                ch->delta_inebriation(amount * liquid->affect[Nutrition::LiquidAffect::Inebriation])) {
            ch->send_line(*opt_message);
        }
        if (const auto opt_message = ch->delta_hunger(amount * liquid->affect[Nutrition::LiquidAffect::Satiation])) {
            ch->send_line(*opt_message);
        }
        if (const auto opt_message = ch->delta_thirst(amount * liquid->affect[Nutrition::LiquidAffect::Hydration])) {
            ch->send_line(*opt_message);
        }

        if (obj->value[3] != 0) {
            /* The shit was poisoned ! */

            act("$n chokes and gags.", ch);
            ch->send_line("You choke and gag.");
            AFFECT_DATA af;
            af.type = gsn_poison;
            af.level = number_fuzzy(amount);
            af.duration = 3 * amount;
            af.bitvector = AFF_POISON;
            affect_join(ch, af);
        }

        obj->value[1] -= amount;
        break;
    }
}

void do_eat(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Eat what?");
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        ch->send_line("You do not have that item.");
        return;
    }

    if (ch->is_mortal()) {
        if (obj->type != ObjectType::Food && obj->type != ObjectType::Pill) {
            ch->send_line("That's not edible.");
            return;
        }

        if (!ch->is_hungry()) {
            ch->send_line("You are too full to eat more.");
            return;
        }
    }

    act("$n eats $p.", ch, obj, nullptr, To::Room);
    act("You eat $p.", ch, obj, nullptr, To::Char);

    switch (obj->type) {

    case ObjectType::Food:
        if (const auto opt_message = ch->delta_hunger(obj->value[0])) {
            ch->send_line(*opt_message);
        }
        if (obj->value[3] != 0) {
            /* The shit was poisoned! */

            act("|r$n chokes and gags.|w", ch);
            ch->send_line("|RYou choke and gag.|w");

            AFFECT_DATA af;
            af.type = gsn_poison;
            af.level = number_fuzzy(obj->value[0]);
            af.duration = std::max(1, obj->value[0]);
            af.bitvector = AFF_POISON;
            affect_join(ch, af);
        }
        break;

    case ObjectType::Pill:
        target_name = "";
        obj_cast_spell(obj->value[1], obj->value[0], ch, ch, nullptr);
        obj_cast_spell(obj->value[2], obj->value[0], ch, ch, nullptr);
        obj_cast_spell(obj->value[3], obj->value[0], ch, ch, nullptr);
        break;
    default:;
    }

    extract_obj(obj);
}

void do_wear(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Wear, wield, or hold what?");
        return;
    }

    if (!str_cmp(arg, "all")) {
        for (auto *obj : ch->carrying)
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
                wear_obj(ch, obj, false);
    } else {
        auto *obj = get_obj_carry(ch, arg);
        if (!obj) {
            ch->send_line("You do not have that item.");
            return;
        }

        wear_obj(ch, obj, true);
    }
}

void do_remove(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Remove what?");
        return;
    }

    if ((obj = get_obj_wear(ch, arg)) == nullptr) {
        ch->send_line("You do not have that item.");
        return;
    }

    remove_obj(ch, obj->wear_loc, true);
}

void do_sacrifice(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    int gold;

    one_argument(argument, arg);

    if (arg[0] == '\0' || matches(arg, ch->name)) {
        act(fmt::format("$n offers $r to {}, who graciously declines.", deity_name), ch);
        ch->send_line("{} appreciates your offer and may accept it later.", deity_name);
        return;
    }

    obj = get_obj_list(ch, arg, ch->in_room->contents);
    if (obj == nullptr) {
        ch->send_line("You can't find it.");
        return;
    }

    if (obj->type == ObjectType::Pccorpse) {
        if (!obj->contains.empty()) {
            ch->send_line("{} wouldn't like that.", deity_name);
            return;
        }
    }

    if (!obj->is_takeable()) {
        act("$p is not an acceptable sacrifice.", ch, obj, nullptr, To::Char);
        return;
    }

    gold = std::max(1, obj->level * 2);

    if (obj->type != ObjectType::Npccorpse && obj->type != ObjectType::Pccorpse)
        gold = std::min(gold, obj->cost);

    switch (gold) {
    default: ch->send_line("{} gives you {} gold coins for your sacrifice.", deity_name, gold); break;
    case 0: ch->send_line("{} laughs at your worthless sacrifice. You receive no gold coins.", deity_name); break;
    case 1: ch->send_line("{} is unimpressed by your sacrifice but grants you a single gold coin.", deity_name); break;
    };

    ch->gold += gold;

    if (check_bit(ch->act, PLR_AUTOSPLIT)) { /* AUTOSPLIT code */
        if (ch->num_group_members_in_room() > 1 && gold > 1) {
            split_coins(ch, gold);
        }
    }

    act(fmt::format("$n sacrifices $p to {}.", deity_name), ch, obj, nullptr, To::Room);
    extract_obj(obj);
}

void do_tras(Char *ch, ArgParser args) {
    (void)args;
    ch->send_line("To trash an item please type the full command!");
}

void do_trash(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Which item do you want to destroy?");
        return;
    }
    auto objname = args.remaining();
    auto *obj = ch->find_in_inventory(objname);
    if (!obj) {
        ch->send_line("You do not have that item.");
        return;
    }
    // Disallow trashing of containers having contents because we don't want a player
    // to shoot themselves in the foot.
    if (obj->type == ObjectType::Container && !obj->contains.empty()) {
        ch->send_line("To trash a container please empty it first.");
        return;
    }
    ch->send_line("You trash {}.", obj->short_descr);
    act("$n trashes $p.", ch, obj, nullptr, To::Room);
    extract_obj(obj);
}

void do_quaff(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Quaff what?");
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        ch->send_line("You do not have that potion.");
        return;
    }

    if (obj->type != ObjectType::Potion) {
        ch->send_line("You can quaff only potions.");
        return;
    }

    if (ch->level < obj->level) {
        ch->send_line("This liquid is too powerful for you to drink.");
        return;
    }

    act("$n quaffs $p.", ch, obj, nullptr, To::Room);
    act("You quaff $p.", ch, obj, nullptr, To::Char);

    target_name = "";
    obj_cast_spell(obj->value[1], obj->value[0], ch, ch, nullptr);
    obj_cast_spell(obj->value[2], obj->value[0], ch, ch, nullptr);
    obj_cast_spell(obj->value[3], obj->value[0], ch, ch, nullptr);

    extract_obj(obj);
}

void do_recite(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    Object *scroll;
    Object *obj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((scroll = get_obj_carry(ch, arg1)) == nullptr) {
        ch->send_line("You do not have that scroll.");
        return;
    }

    if (scroll->type != ObjectType::Scroll) {
        ch->send_line("You can recite only scrolls.");
        return;
    }

    if (ch->level < scroll->level) {
        ch->send_line("This scroll is too complex for you to comprehend.");
        return;
    }

    target_name = arg2; /* Makes cast 'locate object' xxxxx work */
    obj = nullptr;
    if (arg2[0] == '\0') {
        victim = ch;
    } else {
        if ((victim = get_char_room(ch, arg2)) == nullptr && (obj = get_obj_here(ch, arg2)) == nullptr
            && (skill_table[scroll->value[1]].target != Target::Ignore)
            && (skill_table[scroll->value[1]].target != Target::CharOther)) {
            ch->send_line("You can't find it.");
            return;
        }
    }

    act("$n recites $p.", ch, scroll, nullptr, To::Room);
    act("You recite $p.", ch, scroll, nullptr, To::Char);

    if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4 / 5) {
        ch->send_line("|rYou mispronounce a syllable.|w");
        check_improve(ch, gsn_scrolls, false, 2);
    }

    else {
        if (target_name == nullptr)
            target_name = "";
        obj_cast_spell(scroll->value[1], scroll->value[0], ch, victim, obj);
        obj_cast_spell(scroll->value[2], scroll->value[0], ch, victim, obj);
        obj_cast_spell(scroll->value[3], scroll->value[0], ch, victim, obj);
        check_improve(ch, gsn_scrolls, true, 2);
    }

    extract_obj(scroll);
}

void do_brandish(Char *ch) {
    Object *staff;
    int sn;

    if ((staff = get_eq_char(ch, WEAR_HOLD)) == nullptr) {
        ch->send_line("You hold nothing in your hand.");
        return;
    }

    if (staff->type != ObjectType::Staff) {
        ch->send_line("You can brandish only with a staff.");
        return;
    }

    if ((sn = staff->value[3]) < 0 || sn >= MAX_SKILL || skill_table[sn].spell_fun == 0) {
        bug("Do_brandish: bad sn {}.", sn);
        return;
    }

    ch->wait_state(2 * PULSE_VIOLENCE);

    if (staff->value[2] > 0) {
        act("|W$n brandishes $p.|w", ch, staff, nullptr, To::Room);
        act("|WYou brandish $p.|w", ch, staff, nullptr, To::Char);
        if (ch->level < staff->level || number_percent() >= 20 + get_skill(ch, gsn_staves) * 4 / 5) {
            act("|WYou fail to invoke $p.|w", ch, staff, nullptr, To::Char);
            act("|W...and nothing happens.|w", ch);
            check_improve(ch, gsn_staves, false, 2);
        }

        else
            for (auto *vch : ch->in_room->people) {
                switch (skill_table[sn].target) {
                default: bug("Do_brandish: bad target for sn {}.", sn); return;

                case Target::Ignore:
                    if (vch != ch)
                        continue;
                    break;

                case Target::CharOffensive:
                    if (ch->is_npc() ? vch->is_npc() : vch->is_pc())
                        continue;
                    break;

                case Target::CharDefensive:
                    if (ch->is_npc() ? vch->is_pc() : vch->is_npc())
                        continue;
                    break;

                case Target::CharSelf:
                    if (vch != ch)
                        continue;
                    break;
                }

                target_name = "";
                obj_cast_spell(staff->value[3], staff->value[0], ch, vch, nullptr);
                check_improve(ch, gsn_staves, true, 2);
            }
    }

    if (--staff->value[2] <= 0) {
        act("$n's $p blazes bright and is gone.", ch, staff, nullptr, To::Room);
        act("Your $p blazes bright and is gone.", ch, staff, nullptr, To::Char);
        extract_obj(staff);
    }
}

/*
 * zap changed by Faramir 7/8/96 because it was crap.
 * eg. being able to kill mobs far away with a wand of acid blast.
 */

void do_zap(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    Object *wand;
    Object *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0' && ch->fighting == nullptr) {
        ch->send_line("Zap whom or what?");
        return;
    }

    if ((wand = get_eq_char(ch, WEAR_HOLD)) == nullptr) {
        ch->send_line("You hold nothing in your hand.");
        return;
    }

    if (wand->type != ObjectType::Wand) {
        ch->send_line("You can zap only with a wand.");
        return;
    }

    obj = nullptr;
    if (arg[0] == '\0') {
        if (ch->fighting != nullptr) {
            victim = ch->fighting;
        } else {
            ch->send_line("Zap whom or what?");
            return;
        }
    } else {
        if ((victim = get_char_room(ch, arg)) == nullptr && (victim = get_char_world(ch, arg)) == nullptr
            && (obj = get_obj_here(ch, arg)) == nullptr && (skill_table[wand->value[3]].target) != Target::CharOther
            && (skill_table[wand->value[3]].target) != Target::Ignore) {
            ch->send_line("You can't find it.");
            return;
        }
        if (skill_table[wand->value[3]].target == Target::CharOther
            || skill_table[wand->value[3]].target == Target::Ignore) {
            target_name = arg;
        } else {
            target_name = "";
        }

        if ((victim != nullptr && victim != ch && victim->in_room->vnum != ch->in_room->vnum)
            && skill_table[wand->value[3]].target == Target::CharOffensive) {
            act(fmt::format("You attempt to zap {}.....", get_char_room(ch, arg) ? victim->short_descr : "someone"), ch,
                nullptr, nullptr, To::Char);
            act("$n attempts to zap something....", ch);
            ch->send_line("...but the |cgrand Iscarian magi|w outlawed interplanar combat millenia ago!");
            act("$n appears to have been foiled by the law, and looks slightly annoyed!", ch, nullptr, nullptr,
                To::Room);
            return;
        }
    }

    ch->wait_state(2 * PULSE_VIOLENCE);

    if (wand->value[2] > 0) {
        if (victim != nullptr) {
            if (victim->in_room->vnum == ch->in_room->vnum) {
                act("$n zaps $N with $p.", ch, wand, victim, To::Room);
                act("You zap $N with $p.", ch, wand, victim, To::Char);
            }
            if (victim != nullptr && victim->in_room->vnum != ch->in_room->vnum) {
                act("Plucking up all great magical fortitude, $n attemps to zap someone or\n\rsomething far away with "
                    "$p...",
                    ch, wand, nullptr, To::Room);
                act("You attempt to zap $N with $p.....", ch, wand, victim, To::Char);
            }
        }
        if (obj != nullptr) {
            act("$n zaps $P with $p.", ch, wand, obj, To::Room);
            act("You zap $P with $p.", ch, wand, obj, To::Char);
        }
        if (ch->level < wand->level || number_percent() >= 20 + get_skill(ch, gsn_wands) * 4 / 5) {
            act("Your efforts with $p produce only smoke and sparks.", ch, wand, nullptr, To::Char);
            act("$n's efforts with $p produce only smoke and sparks.", ch, wand, nullptr, To::Room);
            check_improve(ch, gsn_wands, false, 2);
        } else {
            obj_cast_spell(wand->value[3], wand->value[0], ch, victim, obj);
            check_improve(ch, gsn_wands, true, 2);
        }
    }

    if (--wand->value[2] <= 0) {
        act("$n's $p explodes into fragments.", ch, wand, nullptr, To::Room);
        act("Your $p explodes into fragments.", ch, wand, nullptr, To::Char);
        extract_obj(wand);
    }
}

void do_steal(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    Object *obj;
    int percent;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        ch->send_line("Steal what from whom?");
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (victim == ch) {
        ch->send_line("That's pointless.");
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->is_pos_fighting()) {
        ch->send_line("You'd better not, you might get hit.");
        return;
    }

    ch->wait_state(skill_table[gsn_steal].beats);
    percent = number_percent() + (victim->is_pos_awake() ? 10 : -50);

    if (ch->level + 5 < victim->level || victim->is_pos_fighting() || victim->is_pc()
        || (ch->is_pc() && percent > ch->get_skill(gsn_steal))) {
        /*
         * Failure.
         */
        ch->send_line("Oops.");
        act("|W$n tried to steal from you.|w", ch, nullptr, victim, To::Vict);
        act("|W$n tried to steal from $N.|w", ch, nullptr, victim, To::NotVict);
        std::string buf;
        switch (number_range(0, 3)) {
        case 0: buf = fmt::format("{} is a lousy thief!", ch->name); break;
        case 1: buf = fmt::format("{} couldn't rob {} way out of a paper bag!", ch->name, possessive(*ch)); break;
        case 2: buf = fmt::format("{} tried to rob me!", ch->name); break;
        case 3: buf = fmt::format("Keep your hands out of there, {}!", ch->name); break;
        }
        victim->yell(buf);
        if (ch->is_pc()) {
            if (victim->is_npc()) {
                check_improve(ch, gsn_steal, false, 2);
                multi_hit(victim, ch);
            } else {
                log_string(buf);
                if (!check_bit(ch->act, PLR_THIEF)) {
                    set_bit(ch->act, PLR_THIEF);
                    ch->send_line("|R*** You are now a THIEF!! ***|r");
                    save_char_obj(ch);
                }
            }
        }

        return;
    }

    if (!str_cmp(arg1, "coin") || !str_cmp(arg1, "coins") || !str_cmp(arg1, "gold")) {
        int amount;

        amount = static_cast<int>(victim->gold * number_range(1, 10) / 100);
        if (amount <= 0) {
            ch->send_line("You couldn't get any gold.");
            return;
        }

        ch->gold += amount;
        victim->gold -= amount;
        ch->send_line("Bingo!  You got {} gold coins.", amount);
        check_improve(ch, gsn_steal, true, 2);
        return;
    }

    if ((obj = get_obj_carry(victim, arg1)) == nullptr) {
        ch->send_line("You can't find it.");
        return;
    }

    if (!can_drop_obj(ch, obj) || check_bit(obj->extra_flags, ITEM_INVENTORY) || obj->level > ch->level) {
        ch->send_line("You can't pry it away.");
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
        ch->send_line("You have your hands full.");
        return;
    }

    if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
        ch->send_line("You can't carry that much weight.");
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, ch);
    check_improve(ch, gsn_steal, true, 2);
}

void do_buy(Char *ch, const char *argument) {
    int cost, roll;

    if (argument[0] == '\0') {
        ch->send_line("Buy what?");
        return;
    }

    if (check_bit(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        char arg[MAX_INPUT_LENGTH];
        Char *pet;
        Room *roomNext;
        Room *in_room;

        if (ch->is_npc())
            return;

        argument = one_argument(argument, arg);

        roomNext = get_room(ch->in_room->vnum + 1);
        if (roomNext == nullptr) {
            bug("Do_buy: bad pet shop at vnum {}.", ch->in_room->vnum);
            ch->send_line("Sorry, you can't buy that here.");
            return;
        }

        in_room = ch->in_room;
        ch->in_room = roomNext;
        pet = get_char_room(ch, arg);
        ch->in_room = in_room;

        if (pet == nullptr || !check_bit(pet->act, ACT_PET)) {
            ch->send_line("Sorry, you can't buy that here.");
            return;
        }

        if (ch->pet != nullptr) {
            ch->send_line("You already own a pet.");
            return;
        }

        cost = 10 * pet->level * pet->level;

        if (ch->gold < cost) {
            ch->send_line("You can't afford it.");
            return;
        }

        if (ch->level < pet->level) {
            ch->send_line("You're not powerful enough to master this pet.");
            return;
        }

        /* haggle */
        roll = number_percent();
        if (ch->is_pc() && roll < ch->get_skill(gsn_haggle)) {
            cost -= cost / 2 * roll / 100;
            ch->send_line("You haggle the price down to {} coins.", cost);
            check_improve(ch, gsn_haggle, true, 4);
        }

        ch->gold -= cost;
        pet = create_mobile(pet->mobIndex);
        set_bit(ch->act, PLR_BOUGHT_PET);
        set_bit(pet->act, ACT_PET);
        set_bit(pet->affected_by, AFF_CHARM);
        pet->comm = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;

        argument = one_argument(argument, arg);
        if (arg[0] != '\0')
            pet->name = smash_tilde(fmt::format("{} {}", pet->name, arg));

        pet->description = fmt::format("{}A neck tag says 'I belong to {}'.", pet->description, ch->name);

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        pet->leader = ch;
        ch->pet = pet;
        ch->send_line("Enjoy your pet.");
        act("$n bought $N as a pet.", ch, nullptr, pet, To::Room);
        return;
    } else {
        Char *keeper;
        Object *obj;

        if ((keeper = find_keeper(ch)) == nullptr)
            return;

        obj = get_obj_carry(keeper, argument);
        cost = get_cost(keeper, obj, true);

        if (cost <= 0 || !can_see_obj(ch, obj)) {
            act("$n tells you 'I don't sell that -- try 'list''.", keeper, nullptr, ch, To::Vict);
            ch->reply = keeper;
            return;
        }

        if (ch->gold < cost) {
            act("$n tells you 'You can't afford to buy $p'.", keeper, obj, ch, To::Vict);
            ch->reply = keeper;
            return;
        }

        if (obj->level > ch->level) {
            act("$n tells you 'You can't use $p yet'.", keeper, obj, ch, To::Vict);
            ch->reply = keeper;
            return;
        }

        if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
            ch->send_line("You can't carry that many items.");
            return;
        }

        if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
            ch->send_line("You can't carry that much weight.");
            return;
        }

        if (obj_move_violates_uniqueness(keeper, ch, obj, ch->carrying)) {
            act(deity_name + " forbids you from possessing more than one $p.", ch, obj, nullptr, To::Char);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (ch->is_pc() && roll < ch->get_skill(gsn_haggle)) {
            cost -= obj->cost / 2 * roll / 100;
            ch->send_line("You haggle the price down to {} coins.", cost);
            check_improve(ch, gsn_haggle, true, 4);
        }

        act("$n buys $p.", ch, obj, nullptr, To::Room);
        act("You buy $p.", ch, obj, nullptr, To::Char);
        ch->gold -= cost;
        keeper->gold += cost;

        if (check_bit(obj->extra_flags, ITEM_INVENTORY))
            obj = create_object(obj->objIndex);
        else
            obj_from_char(obj);

        if (obj->timer > 0)
            obj->timer = 0;
        obj_to_char(obj, ch);
        if (cost < obj->cost)
            obj->cost = cost;
        return;
    }
}

void do_list(Char *ch, const char *argument) {
    if (check_bit(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        Room *roomNext;
        bool found;

        roomNext = get_room(ch->in_room->vnum + 1);
        if (roomNext == nullptr) {
            bug("Do_list: bad pet shop at vnum {}.", ch->in_room->vnum);
            ch->send_line("You can't do that here.");
            return;
        }

        found = false;
        for (auto *pet : roomNext->people) {
            if (check_bit(pet->act, ACT_PET)) {
                if (!found) {
                    found = true;
                    ch->send_line("Pets for sale:");
                }
                ch->send_line("[{:2}] {:8} - {}", pet->level, 10 * pet->level * pet->level, pet->short_descr);
            }
        }
        if (!found)
            ch->send_line("Sorry, we're out of pets right now.");
    } else {
        Char *keeper;
        int cost;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == nullptr)
            return;
        one_argument(argument, arg);

        std::string buffer;
        for (auto *obj : keeper->carrying) {
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj) && (cost = get_cost(keeper, obj, true)) > 0
                && (arg[0] == '\0' || is_name(arg, obj->name))) {
                if (!found) {
                    found = true;
                    buffer += "[Lv Price] Item\n\r";
                }

                buffer += fmt::format("[{:2} {:5}] {}.\n\r", obj->level, cost, obj->short_descr);
            }
        }

        if (buffer.empty())
            ch->send_line("You can't buy anything here.");
        else
            ch->page_to(buffer);
    }
}

void do_sell(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *keeper;
    Object *obj;
    int cost, roll;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Sell what?");
        return;
    }

    if ((keeper = find_keeper(ch)) == nullptr)
        return;

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        act("$n tells you 'You don't have that item'.", keeper, nullptr, ch, To::Vict);
        ch->reply = keeper;
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        ch->send_line("You can't let go of it.");
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, nullptr, ch, To::Vict);
        return;
    }

    /* won't buy rotting goods */
    if (obj->timer || (cost = get_cost(keeper, obj, false)) <= 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, To::Vict);
        return;
    }

    if (cost > keeper->gold) {
        act("$n tells you 'I'm afraid I don't have enough gold to buy $p.", keeper, obj, ch, To::Vict);
        return;
    }

    act("$n sells $p.", ch, obj, nullptr, To::Room);
    /* haggle */
    roll = number_percent();
    if (ch->is_pc() && roll < ch->get_skill(gsn_haggle)) {
        ch->send_line("You haggle with the shopkeeper.");
        cost += obj->cost / 2 * roll / 100;
        cost = std::min(cost, 95 * get_cost(keeper, obj, true) / 100);
        cost = std::min(cost, static_cast<int>(keeper->gold));
        check_improve(ch, gsn_haggle, true, 4);
    }
    act(fmt::format("You sell $p for {} gold piece{}.", cost, cost == 1 ? "" : "s"), ch, obj, nullptr, To::Char);
    ch->gold += cost;
    keeper->gold -= cost;
    if (keeper->gold < 0)
        keeper->gold = 0;

    if (obj->type == ObjectType::Trash) {
        extract_obj(obj);
    } else {
        obj_from_char(obj);
        obj->timer = number_range(200, 400);
        obj_to_char(obj, keeper);
    }
}

void do_value(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *keeper;
    Object *obj;
    int cost;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("Value what?");
        return;
    }

    if ((keeper = find_keeper(ch)) == nullptr)
        return;

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        act("$n tells you 'You don't have that item'.", keeper, nullptr, ch, To::Vict);
        ch->reply = keeper;
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, nullptr, ch, To::Vict);
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        ch->send_line("You can't let go of it.");
        return;
    }

    if ((cost = get_cost(keeper, obj, false)) <= 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, To::Vict);
        return;
    }

    act(fmt::format("$n tells you 'I'll give you {} gold coins for $p'."), keeper, obj, ch, To::Vict);
    ch->reply = keeper;
}

void do_throw(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;
    Object *bomb;
    int chance;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Throw at whom?");
        return;
    }

    if ((bomb = get_eq_char(ch, WEAR_HOLD)) == nullptr) {
        ch->send_line("You hold nothing in your hand.");
        return;
    }

    if (bomb->type != ObjectType::Bomb) {
        ch->send_line("You can throw only bombs.");
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("You can't see them here.");
        return;
    }

    ch->wait_state(2 * PULSE_VIOLENCE);

    chance = ch->get_skill(gsn_throw);

    chance += std::clamp((ch->level - victim->level), -20, 20) - number_percent();

    if (chance >= 0) {
        act("$n throws a bomb at $N!", ch, nullptr, victim, To::NotVict);
        act("$n throws a bomb at you!", ch, nullptr, victim, To::Vict);
        act("You throw your bomb at $N!", ch, nullptr, victim, To::Char);
        explode_bomb(bomb, victim, ch);
        check_improve(ch, gsn_throw, true, 2);
    } else {
        if (chance > -25) {
            act("$n throws a bomb at $N - but misses!", ch, nullptr, victim, To::NotVict);
            act("You throw your bomb at $N but miss!", ch, nullptr, victim, To::Char);
            act("$n throws a bomb at you but misses!", ch, nullptr, victim, To::Vict);
            check_improve(ch, gsn_throw, false, 1);
            obj_from_char(bomb);
            obj_to_room(bomb, ch->in_room);
        } else {
            act("$n's incompetent fumblings cause $m bomb to go off!", ch, nullptr, victim, To::Room);
            act("Your incompetent fumblings cause your bomb to explode!", ch, nullptr, victim, To::Char);
            check_improve(ch, gsn_throw, false, 1);
            explode_bomb(bomb, ch, ch);
        }
    }
    multi_hit(ch, victim);
}

/* hailcorpse for getting out of sticky situations ooeer --Fara */
namespace {
Object *find_corpse(Char *ch, GenericList<Object *> &list) {
    for (auto *current_obj : list)
        if (current_obj->type == ObjectType::Pccorpse && matches_inside(ch->name, current_obj->short_descr))
            return current_obj;
    return nullptr;
}
}
void do_hailcorpse(Char *ch) {
    if (ch->is_switched()) {
        ch->send_line("You cannot hail NPC corpses.");
        return;
    }

    if (ch->is_immortal()) {
        ch->send_line("Those who cannot be slain may not pray for the return of their corpse.");
        return;
    }

    act("$n falls to $s knees and incants a garbled verse.", ch, nullptr, ch, To::Room);
    ch->send_line("You incant the sacred verse of Necrosis and are overcome with nausea.");

    /* make them wait a bit, help prevent abuse */
    ch->wait_state(25);

    /* first thing is to check the ch room to see if it's already here */
    if (find_corpse(ch, ch->in_room->contents)) {
        act("$n's corpse glows momentarily!", ch);
        ch->send_line("Your corpse appears to be in the room already!");
        return;
    }
    /* if not here then check all the rooms adjacent to this one */
    for (auto direction : all_directions) {
        auto *pexit = ch->in_room->exit[direction];
        if (!pexit)
            continue;
        auto adjacent_room = pexit->u1.to_room;
        if (!adjacent_room || !can_see_room(ch, adjacent_room))
            continue;

        if (auto *corpse = find_corpse(ch, adjacent_room->contents)) {
            obj_from_room(corpse);
            obj_to_room(corpse, ch->in_room);
            act("The corpse of $n materialises and floats gently before you!", ch);
            act("Your corpse materialises through a dark portal and floats to your feet!", ch, nullptr, nullptr,
                To::Char);
            return;
        }
    }
    act("$n's prayers for assistance are ignored by the Gods.", ch);
    act("Your prayers for assistance are ignored. Your corpse cannot be found.", ch, nullptr, nullptr, To::Char);
}

bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, Object *moving_obj, Object *obj_to) {
    GenericList<Object *> objs_to;
    objs_to.add_back(obj_to);
    return obj_move_violates_uniqueness(source_char, dest_char, moving_obj, objs_to);
}

bool obj_move_violates_uniqueness(Char *source_char, Char *dest_char, Object *moving_obj,
                                  GenericList<Object *> &objs_to) {
    // If we know in advance the object(s) being moved aren't changing ownership we can take a shortcut.
    // This simplifies things like: "get innerbag outerbag", where they are carrying outerbag, and innerbag
    // contains a unique item. Shopkeepers are permitted to be given unique items, they can resell them.
    if (source_char == dest_char || (dest_char && dest_char->is_shopkeeper())) {
        return false;
    }

    std::set<ObjectIndex *> moving_unique_obj_idxs;
    std::set<ObjectIndex *> to_unique_obj_idxs;

    GenericList<Object *> moving_objects;
    moving_objects.add_back(moving_obj);

    collect_unique_obj_indexes(moving_objects, moving_unique_obj_idxs);
    // The most common case is a non-unique non-container is being moved.
    if (moving_unique_obj_idxs.empty()) {
        return false;
    }
    collect_unique_obj_indexes(objs_to, to_unique_obj_idxs);
    std::vector<ObjectIndex *> intersection;
    std::set_intersection(moving_unique_obj_idxs.begin(), moving_unique_obj_idxs.end(), to_unique_obj_idxs.begin(),
                          to_unique_obj_idxs.end(), std::back_inserter(intersection));
    return !intersection.empty();
}

bool check_material_vulnerability(Char *ch, Object *object) {
    switch (object->material) {
    case Material::Type::Wood: return check_bit(ch->vuln_flags, DMG_TOL_WOOD);
    case Material::Type::Silver: return check_bit(ch->vuln_flags, DMG_TOL_SILVER);
    case Material::Type::Iron: return check_bit(ch->vuln_flags, DMG_TOL_IRON);
    default: return false;
    }
}
