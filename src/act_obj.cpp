/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_obj.c: interaction with objects                                  */
/*                                                                       */
/*************************************************************************/

#include "Pronouns.hpp"
#include "TimeInfoData.hpp"
#include "buffer.h"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace fmt::literals;

extern const char *target_name; /* Included from magic.c */
extern void handle_corpse_summoner(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj);

/*
 * Local functions.
 */
bool remove_obj(CHAR_DATA *ch, int iWear, bool fReplace);
void wear_obj(CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace);
CHAR_DATA *find_keeper(CHAR_DATA *ch);
int get_cost(CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy);

void explode_bomb(OBJ_DATA *bomb, CHAR_DATA *ch, CHAR_DATA *thrower);

/* RT part of the corpse looting code */

bool can_loot(const CHAR_DATA *ch, const OBJ_DATA *obj) {
    if (ch->is_immortal())
        return true;

    if (!obj->owner || obj->owner == nullptr)
        return true;

    const CHAR_DATA *owner = nullptr;
    for (const auto *wch = char_list; wch != nullptr; wch = wch->next)
        if (matches(wch->name, obj->owner))
            owner = wch;

    if (owner == nullptr)
        return true;

    if (matches(ch->name, owner->name))
        return true;

    if (owner->is_pc() && IS_SET(owner->act, PLR_CANLOOT))
        return true;

    if (is_same_group(ch, owner))
        return true;

    return false;
}

void get_obj(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container) {
    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];

    if (!CAN_WEAR(obj, ITEM_TAKE)) {
        send_to_char("You can't take that.\n\r", ch);
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

    if (container != nullptr) {
        if (container->pIndexData->vnum == OBJ_VNUM_PIT && ch->get_trust() < obj->level) {
            send_to_char("You are not powerful enough to use it.\n\r", ch);
            return;
        }

        if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(container, ITEM_TAKE) && obj->timer)
            obj->timer = 0;
        act("You get $p from $P.", ch, obj, container, To::Char);
        act("$n gets $p from $P.", ch, obj, container, To::Room);
        obj_from_obj(obj);
    } else {
        act("You get $p.", ch, obj, container, To::Char);
        act("$n gets $p.", ch, obj, container, To::Room);
        obj_from_room(obj);
    }

    if (obj->item_type == ITEM_MONEY) {
        ch->gold += obj->value[0];
        if (IS_SET(ch->act, PLR_AUTOSPLIT)) { /* AUTOSPLIT code */
            members = 0;
            for (gch = ch->in_room->people; gch != nullptr; gch = gch->next_in_room) {
                if (is_same_group(gch, ch))
                    members++;
            }

            if (members > 1 && obj->value[0] > 1) {
                snprintf(buffer, sizeof(buffer), "%d", obj->value[0]);
                do_split(ch, buffer);
            }
        }

        extract_obj(obj);
    } else {
        obj_to_char(obj, ch);
    }
}

void do_get(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "from"))
        argument = one_argument(argument, arg2);

    /* Get type. */
    if (arg1[0] == '\0') {
        send_to_char("Get what?\n\r", ch);
        return;
    }

    if (arg2[0] == '\0') {
        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            /* 'get obj' */
            obj = get_obj_list(ch, arg1, ch->in_room->contents);
            if (obj == nullptr) {
                act("I see no $T here.", ch, nullptr, arg1, To::Char);
                return;
            }

            get_obj(ch, obj, nullptr);
        } else {
            /* 'get all' or 'get all.obj' */
            found = false;
            for (obj = ch->in_room->contents; obj != nullptr; obj = obj_next) {
                obj_next = obj->next_content;
                if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj)) {
                    found = true;
                    get_obj(ch, obj, nullptr);
                }
            }

            if (!found) {
                if (arg1[3] == '\0')
                    send_to_char("I see nothing here.\n\r", ch);
                else
                    act("I see no $T here.", ch, nullptr, &arg1[4], To::Char);
            }
        }
    } else {
        /* 'get ... container' */
        if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }

        if ((container = get_obj_here(ch, arg2)) == nullptr) {
            act("I see no $T here.", ch, nullptr, arg2, To::Char);
            return;
        }

        switch (container->item_type) {
        default: send_to_char("That's not a container.\n\r", ch); return;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC: break;

        case ITEM_CORPSE_PC: {

            if (!can_loot(ch, container)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }
        }
        }

        if (IS_SET(container->value[1], CONT_CLOSED)) {
            act("The $d is closed.", ch, nullptr, container->name, To::Char);
            return;
        }

        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            /* 'get obj container' */
            obj = get_obj_list(ch, arg1, container->contains);
            if (obj == nullptr) {
                act("I see nothing like that in the $T.", ch, nullptr, arg2, To::Char);
                return;
            }
            get_obj(ch, obj, container);
        } else {
            /* 'get all container' or 'get all.obj container' */
            found = false;
            for (obj = container->contains; obj != nullptr; obj = obj_next) {
                obj_next = obj->next_content;
                if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj)) {
                    found = true;
                    if (container->pIndexData->vnum == OBJ_VNUM_PIT && ch->is_mortal()) {
                        send_to_char("Don't be so greedy!\n\r", ch);
                        return;
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

void do_put(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "in"))
        argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Put what in what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    if ((container = get_obj_here(ch, arg2)) == nullptr) {
        act("I see no $T here.", ch, nullptr, arg2, To::Char);
        return;
    }

    if (container->item_type != ITEM_CONTAINER) {
        send_to_char("That's not a container.\n\r", ch);
        return;
    }

    if (IS_SET(container->value[1], CONT_CLOSED)) {
        act("The $d is closed.", ch, nullptr, container->name, To::Char);
        return;
    }

    if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
        /* put obj container' */
        if ((obj = get_obj_carry(ch, arg1)) == nullptr) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (obj == container) {
            send_to_char("You can't fold it into itself.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
            return;
        }

        if (get_obj_weight(obj) + get_obj_weight(container) > container->value[0]) {
            send_to_char("It won't fit.\n\r", ch);
            return;
        }

        if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(container, ITEM_TAKE)) {
            if (obj->timer) {
                send_to_char("Only permanent items may go in the pit.\n\r", ch);
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
        for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
            obj_next = obj->next_content;

            if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE
                && obj != container && can_drop_obj(ch, obj)
                && get_obj_weight(obj) + get_obj_weight(container) <= container->value[0]) {
                if (container->pIndexData->vnum == OBJ_VNUM_PIT) {
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

void do_donate(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *pit;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    /* Get command argument and ensure that one has been given */
    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Donate what?\n\r", ch);
        return;
    }

    /* get the pit's OBJ_DATA * */
    for (pit = (get_room_index(ROOM_VNUM_ALTAR))->contents; pit; pit = pit->next_content) {
        if (pit->pIndexData->vnum == OBJ_VNUM_PIT)
            break;
    }

    /* just in case someone should accidentally delete the pit... */
    if (pit == nullptr) {
        send_to_char("The psychic field seems to have lost its alignment.\n\r", ch);
        return;
    }

    /* check to see if the ch is currently cursed */
    if (IS_AFFECTED(ch, AFF_CURSE)) {
        send_to_char("The psychic flux seems to be avoiding you today.\n\r", ch);
        return;
    }

    /* check to see if the ch is in a non-recall room */
    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
        send_to_char("The psychic flux is not strong enough here.\n\r", ch);
        return;
    }

    /* check if 'all' or 'all.' has been used */
    if (str_cmp(arg, "all") && str_prefix("all.", arg)) { /* this returns true if NEITHER matched */

        if ((obj = get_obj_carry(ch, arg)) == nullptr) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
            return;
        }

        if (obj->timer) {
            send_to_char("That would just get torn apart by the psychic vortices.\n\r", ch);
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
        for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
            obj_next = obj->next_content;

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

void do_drop(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Drop what?\n\r", ch);
        return;
    }

    if (is_number(arg)) {
        /* 'drop NNNN coins' */
        int amount;

        amount = atoi(arg);
        argument = one_argument(argument, arg);
        if (amount <= 0 || (str_cmp(arg, "coins") && str_cmp(arg, "coin") && str_cmp(arg, "gold"))) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }

        if (ch->gold < amount) {
            send_to_char("You haven't got that many coins.\n\r", ch);
            return;
        }

        ch->gold -= amount;

        for (obj = ch->in_room->contents; obj != nullptr; obj = obj_next) {
            obj_next = obj->next_content;

            switch (obj->pIndexData->vnum) {
            case OBJ_VNUM_MONEY_ONE:
                amount += 1;
                extract_obj(obj);
                break;

            case OBJ_VNUM_MONEY_SOME:
                amount += obj->value[0];
                extract_obj(obj);
                break;
            }
        }

        obj_to_room(create_money(amount), ch->in_room);
        act("$n drops some gold.", ch);
        send_to_char("OK.\n\r", ch);
        return;
    }

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        /* 'drop obj' */
        if ((obj = get_obj_carry(ch, arg)) == nullptr) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
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
        for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
            obj_next = obj->next_content;

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

void do_give(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Give what to whom?\n\r", ch);
        return;
    }

    if (is_number(arg1)) {
        /* 'give NNNN coins victim' */
        int amount;

        amount = atoi(arg1);
        if (amount <= 0 || (str_cmp(arg2, "coins") && str_cmp(arg2, "coin") && str_cmp(arg2, "gold"))) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }

        argument = one_argument(argument, arg2);
        if (arg2[0] == '\0') {
            send_to_char("Give what to whom?\n\r", ch);
            return;
        }

        if ((victim = get_char_room(ch, arg2)) == nullptr) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (ch->gold < amount) {
            send_to_char("You haven't got that much gold.\n\r", ch);
            return;
        }
        /* added because people were using charmed mobs to do some
           cunning things - fara
           */
        if (victim->carry_weight + 1 > can_carry_w(ch)) {
            send_to_char("They can't carry any more weight.\n\r", ch);
            return;
        }
        if (victim->carry_number + 1 > can_carry_n(ch)) {
            send_to_char("Their hands are full.\n\r", ch);
            return;
        }

        ch->gold -= amount;
        victim->gold += amount;
        snprintf(buf, sizeof(buf), "$n gives you %d gold.", amount);
        act(buf, ch, nullptr, victim, To::Vict);
        act("$n gives $N some gold.", ch, nullptr, victim, To::NotVict);
        snprintf(buf, sizeof(buf), "You give $N %d gold.", amount);
        act(buf, ch, nullptr, victim, To::Char);

        /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
        mprog_bribe_trigger(victim, ch, amount);
        return;
    }

    if ((obj = get_obj_carry(ch, arg1)) == nullptr) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (obj->wear_loc != WEAR_NONE) {
        send_to_char("You must remove it first.\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }
    /* prevent giving to yourself! */
    if (victim == ch) {
        send_to_char("Give something to yourself? What, are you insane?!\n\r", ch);
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim)) {
        act("$N has $S hands full.", ch, nullptr, victim, To::Char);
        return;
    }

    if (victim->carry_weight + get_obj_weight(obj) > can_carry_w(victim)) {
        act("$N can't carry that much weight.", ch, nullptr, victim, To::Char);
        return;
    }

    if (!can_see_obj(victim, obj)) {
        act("$N can't see it.", ch, nullptr, victim, To::Char);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, victim);

    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    MOBtrigger = false;

    act("$n gives $p to $N.", ch, obj, victim, To::NotVict);
    act("$n gives you $p.", ch, obj, victim, To::Vict);
    act("You give $p to $N.", ch, obj, victim, To::Char);

    handle_corpse_summoner(ch, victim, obj);
    mprog_give_trigger(victim, ch, obj);
}

/*
 *  pour for Xania, Faramir 30/6/96
 */

void do_pour(CHAR_DATA *ch, const char *argument) {

    char arg1[MAX_INPUT_LENGTH];
    OBJ_DATA *obj, *target_obj, *obj_next;
    bool found = false;
    CHAR_DATA *victim;
    int pour_volume = 0;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0') {
        send_to_char("What do you wish to pour?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg1)) == nullptr) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_DRINK_CON) {
        send_to_char("You cannot pour that.\n\r", ch);
        return;
    }

    one_argument(argument, arg1);

    if (obj->value[1] == 0) {
        send_to_char("Your liquid container is empty.\n\r", ch);
        return;
    }

    if (arg1[0] == '\0') {

        act("You empty $p out onto the ground.", ch, obj, nullptr, To::Char);
        obj->value[1] = 0;

        return;
    }

    if (!str_prefix(arg1, "in")) {

        for (target_obj = ch->carrying; target_obj != nullptr; target_obj = obj_next) {

            if ((target_obj->item_type == ITEM_DRINK_CON) && (obj != target_obj)) {
                found = true;
                break;
            }
            obj_next = target_obj->next_content;
        }

        if (!found) {
            send_to_char("Pour what into what?!\n\r", ch);
            return;
        }

        if (target_obj->value[2] != obj->value[2]) {
            ch->send_to("{} already contains another type of liquid!\n\r"_format(target_obj->name));
            return;
        }

        do {

            pour_volume++;
            obj->value[1]--;
            target_obj->value[1]++;

        } while ((obj->value[1] > 0) && (target_obj->value[1] < target_obj->value[0]) && (pour_volume < 50));

        ch->send_to("You pour the contents of {} into {}"_format(obj->short_descr, target_obj->short_descr));
        return;
    }

    victim = get_char_room(ch, arg1);

    if (victim == nullptr) {
        send_to_char("They're not here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("Pour it into which of your own containers?\n\rPour <container1> in <container2>\n\r", ch);
        return;
    }

    for (target_obj = victim->carrying; target_obj != nullptr; target_obj = obj_next) {

        if (target_obj->item_type == ITEM_DRINK_CON) {
            found = true;
            break;
        }
        obj_next = target_obj->next_content;
    }

    if (!found) {
        ch->send_to("{} is not carrying an item which can be filled.\n\r"_format(victim->short_name()));
        return;
    }

    if (target_obj->value[2] != obj->value[2]) {
        ch->send_to("{}'s {} appears to contain a different type of liquid!\n\r"_format(victim->short_name(),
                                                                                        target_obj->short_descr));
        return;
    }

    if (target_obj->value[1] >= target_obj->value[0]) {
        ch->send_to("{}'s liquid container is already full to the brim!\n\r"_format(victim->short_name()));
        return;
    }

    do {
        obj->value[1]--;
        target_obj->value[1]++;
        pour_volume++; /* very important, to stop inf.loops with endless containers! */
    } while ((obj->value[1] > 0) && (target_obj->value[1] < target_obj->value[0]) && (pour_volume < 50));

    act("You pour $p into $n's container.", ch, obj, nullptr, To::Char);
    ch->send_to("{} pours liquid into your container.\n\r"_format(victim->short_name()));
}

void do_fill(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain;
    bool found;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Fill what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    found = false;
    for (fountain = ch->in_room->contents; fountain != nullptr; fountain = fountain->next_content) {
        if (fountain->item_type == ITEM_FOUNTAIN) {
            found = true;
            break;
        }
    }

    if (!found) {
        send_to_char("There is no fountain here!\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_DRINK_CON) {
        send_to_char("You can't fill that.\n\r", ch);
        return;
    }

    if (obj->value[1] != 0 && obj->value[2] != 0) {
        send_to_char("There is already another liquid in it.\n\r", ch);
        return;
    }

    if (obj->value[1] >= obj->value[0]) {
        send_to_char("Your container is full.\n\r", ch);
        return;
    }

    act("You fill $p.", ch, obj, nullptr, To::Char);
    obj->value[2] = 0;
    obj->value[1] = obj->value[0];
}

void do_drink(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
            if (obj->item_type == ITEM_FOUNTAIN)
                break;
        }

        if (obj == nullptr) {
            send_to_char("Drink what?\n\r", ch);
            return;
        }
    } else {
        if ((obj = get_obj_here(ch, arg)) == nullptr) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    if (ch->is_pc() && ch->pcdata->condition[COND_DRUNK] > 10) {
        send_to_char("You fail to reach your mouth.  *Hic*\n\r", ch);
        return;
    }

    switch (obj->item_type) {
    default: send_to_char("You can't drink from that.\n\r", ch); break;

    case ITEM_FOUNTAIN:
        if (ch->is_pc())
            ch->pcdata->condition[COND_THIRST] = 48;
        act("$n drinks from $p.", ch, obj, nullptr, To::Room);
        send_to_char("You are no longer thirsty.\n\r", ch);
        break;

    case ITEM_DRINK_CON:
        if (obj->value[1] <= 0) {
            send_to_char("It is already empty.\n\r", ch);
            return;
        }

        if ((liquid = obj->value[2]) >= LIQ_MAX) {
            bug("Do_drink: bad liquid number %d.", liquid);
            liquid = obj->value[2] = 0;
        }

        act("$n drinks $T from $p.", ch, obj, liq_table[liquid].liq_name, To::Room);
        act("You drink $T from $p.", ch, obj, liq_table[liquid].liq_name, To::Char);

        amount = number_range(3, 10);
        amount = UMIN(amount, obj->value[1]);

        gain_condition(ch, COND_DRUNK, amount * liq_table[liquid].liq_affect[COND_DRUNK]);
        gain_condition(ch, COND_FULL, amount * liq_table[liquid].liq_affect[COND_FULL]);
        gain_condition(ch, COND_THIRST, amount * liq_table[liquid].liq_affect[COND_THIRST]);

        if (ch->is_pc() && ch->pcdata->condition[COND_DRUNK] > 10)
            send_to_char("You feel drunk.\n\r", ch);
        if (ch->is_pc() && ch->pcdata->condition[COND_FULL] > 40)
            send_to_char("You are full.\n\r", ch);
        if (ch->is_pc() && ch->pcdata->condition[COND_THIRST] > 40)
            send_to_char("You do not feel thirsty.\n\r", ch);

        if (obj->value[3] != 0) {
            /* The shit was poisoned ! */
            AFFECT_DATA af;

            act("$n chokes and gags.", ch);
            send_to_char("You choke and gag.\n\r", ch);
            af.type = gsn_poison;
            af.level = number_fuzzy(amount);
            af.duration = 3 * amount;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_POISON;
            affect_join(ch, &af);
        }

        obj->value[1] -= amount;
        break;
    }
}

void do_eat(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Eat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (ch->is_mortal()) {
        if (obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL) {
            send_to_char("That's not edible.\n\r", ch);
            return;
        }

        if (ch->is_pc() && ch->pcdata->condition[COND_FULL] > 40) {
            send_to_char("You are too full to eat more.\n\r", ch);
            return;
        }
    }

    act("$n eats $p.", ch, obj, nullptr, To::Room);
    act("You eat $p.", ch, obj, nullptr, To::Char);

    switch (obj->item_type) {

    case ITEM_FOOD:
        if (ch->is_pc()) {
            int condition;

            condition = ch->pcdata->condition[COND_FULL];
            gain_condition(ch, COND_FULL, obj->value[0]);
            if (condition == 0 && ch->pcdata->condition[COND_FULL] > 0)
                send_to_char("You are no longer hungry.\n\r", ch);
            else if (ch->pcdata->condition[COND_FULL] > 40)
                send_to_char("You are full.\n\r", ch);
        }

        if (obj->value[3] != 0) {
            /* The shit was poisoned! */
            AFFECT_DATA af;

            act("|r$n chokes and gags.|w", ch);
            send_to_char("|RYou choke and gag.|w\n\r", ch);

            af.type = gsn_poison;
            af.level = number_fuzzy(obj->value[0]);
            af.duration = UMAX(1, obj->value[0]);
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_POISON;
            affect_join(ch, &af);
        }
        break;

    case ITEM_PILL:
        target_name = "";
        obj_cast_spell(obj->value[1], obj->value[0], ch, ch, nullptr);
        obj_cast_spell(obj->value[2], obj->value[0], ch, ch, nullptr);
        obj_cast_spell(obj->value[3], obj->value[0], ch, ch, nullptr);
        break;
    }

    extract_obj(obj);
}

/*
 * Remove an object.
 */
bool remove_obj(CHAR_DATA *ch, int iWear, bool fReplace) {
    OBJ_DATA *obj;

    if ((obj = get_eq_char(ch, iWear)) == nullptr)
        return true;

    if (!fReplace)
        return false;

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE)) {
        act("|rYou can't remove $p.|w", ch, obj, nullptr, To::Char);
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
void wear_obj(CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace) {
    char buf[MAX_STRING_LENGTH];

    if (ch->level < obj->level) {
        snprintf(buf, sizeof(buf), "You must be level %d to use this object.\n\r", obj->level);
        send_to_char(buf, ch);
        act("$n tries to use $p, but is too inexperienced.", ch, obj, nullptr, To::Room);
        return;
    }

    if (obj->wear_string != nullptr) {
        act("$n uses $p.", ch, obj, nullptr, To::Room);
        act("You use $p.", ch, obj, nullptr, To::Char);
    }

    if (obj->item_type == ITEM_LIGHT) {
        if (!remove_obj(ch, WEAR_LIGHT, fReplace))
            return;

        if (obj->wear_string == nullptr) {
            act("$n lights $p and holds it.", ch, obj, nullptr, To::Room);
            act("You light $p and hold it.", ch, obj, nullptr, To::Char);
        }

        equip_char(ch, obj, WEAR_LIGHT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
        if (get_eq_char(ch, WEAR_FINGER_L) != nullptr && get_eq_char(ch, WEAR_FINGER_R) != nullptr
            && !remove_obj(ch, WEAR_FINGER_L, fReplace) && !remove_obj(ch, WEAR_FINGER_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_FINGER_L) == nullptr) {
            if (obj->wear_string == nullptr) {
                act("$n wears $p on $s left finger.", ch, obj, nullptr, To::Room);
                act("You wear $p on your left finger.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_FINGER_L);
            return;
        }

        if (get_eq_char(ch, WEAR_FINGER_R) == nullptr) {
            if (obj->wear_string == nullptr) {
                act("$n wears $p on $s right finger.", ch, obj, nullptr, To::Room);
                act("You wear $p on your right finger.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_FINGER_R);
            return;
        }

        bug("Wear_obj: no free finger.");
        send_to_char("You already wear two rings.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
        if (get_eq_char(ch, WEAR_NECK_1) != nullptr && get_eq_char(ch, WEAR_NECK_2) != nullptr
            && !remove_obj(ch, WEAR_NECK_1, fReplace) && !remove_obj(ch, WEAR_NECK_2, fReplace))
            return;

        if (get_eq_char(ch, WEAR_NECK_1) == nullptr) {
            if (obj->wear_string == nullptr) {
                act("$n wears $p around $s neck.", ch, obj, nullptr, To::Room);
                act("You wear $p around your neck.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_NECK_1);
            return;
        }

        if (get_eq_char(ch, WEAR_NECK_2) == nullptr) {
            if (obj->wear_string == nullptr) {
                act("$n wears $p around $s neck.", ch, obj, nullptr, To::Room);
                act("You wear $p around your neck.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_NECK_2);
            return;
        }

        bug("Wear_obj: no free neck.");
        send_to_char("You already wear two neck items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
        if (!remove_obj(ch, WEAR_BODY, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p on $s body.", ch, obj, nullptr, To::Room);
            act("You wear $p on your body.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_BODY);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
        if (!remove_obj(ch, WEAR_HEAD, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p on $s head.", ch, obj, nullptr, To::Room);
            act("You wear $p on your head.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_HEAD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
        if (!remove_obj(ch, WEAR_LEGS, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p on $s legs.", ch, obj, nullptr, To::Room);
            act("You wear $p on your legs.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_LEGS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
        if (!remove_obj(ch, WEAR_FEET, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p on $s feet.", ch, obj, nullptr, To::Room);
            act("You wear $p on your feet.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_FEET);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
        if (!remove_obj(ch, WEAR_HANDS, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p on $s hands.", ch, obj, nullptr, To::Room);
            act("You wear $p on your hands.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_HANDS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ARMS)) {
        if (!remove_obj(ch, WEAR_ARMS, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p on $s arms.", ch, obj, nullptr, To::Room);
            act("You wear $p on your arms.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_ARMS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
        if (!remove_obj(ch, WEAR_ABOUT, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p about $s body.", ch, obj, nullptr, To::Room);
            act("You wear $p about your body.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_ABOUT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
        if (!remove_obj(ch, WEAR_WAIST, fReplace))
            return;
        if (obj->wear_string == nullptr) {
            act("$n wears $p about $s waist.", ch, obj, nullptr, To::Room);
            act("You wear $p about your waist.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_WAIST);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
        if (get_eq_char(ch, WEAR_WRIST_L) != nullptr && get_eq_char(ch, WEAR_WRIST_R) != nullptr
            && !remove_obj(ch, WEAR_WRIST_L, fReplace) && !remove_obj(ch, WEAR_WRIST_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_WRIST_L) == nullptr) {
            if (obj->wear_string == nullptr) {
                act("$n wears $p around $s left wrist.", ch, obj, nullptr, To::Room);
                act("You wear $p around your left wrist.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_WRIST_L);
            return;
        }

        if (get_eq_char(ch, WEAR_WRIST_R) == nullptr) {
            if (obj->wear_string == nullptr) {
                act("$n wears $p around $s right wrist.", ch, obj, nullptr, To::Room);
                act("You wear $p around your right wrist.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_WRIST_R);
            return;
        }

        bug("Wear_obj: no free wrist.");
        send_to_char("You already wear two wrist items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
        OBJ_DATA *weapon;

        if (!remove_obj(ch, WEAR_SHIELD, fReplace))
            return;

        weapon = get_eq_char(ch, WEAR_WIELD);
        if (weapon != nullptr && ch->size < SIZE_LARGE && IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)) {
            send_to_char("Your hands are tied up with your weapon!\n\r", ch);
            return;
        }

        if (obj->wear_string == nullptr) {
            act("$n wears $p as a shield.", ch, obj, nullptr, To::Room);
            act("You wear $p as a shield.", ch, obj, nullptr, To::Char);
        }
        equip_char(ch, obj, WEAR_SHIELD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WIELD)) {
        int sn, skill;

        if (!remove_obj(ch, WEAR_WIELD, fReplace))
            return;

        if (ch->is_pc() && get_obj_weight(obj) > str_app[get_curr_stat(ch, Stat::Str)].wield) {
            send_to_char("It is too heavy for you to wield.\n\r", ch);
            return;
        }

        if (ch->is_pc() && ch->size < SIZE_LARGE && IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
            && get_eq_char(ch, WEAR_SHIELD) != nullptr) {
            send_to_char("You need two hands free for that weapon.\n\r", ch);
            return;
        }

        if (obj->wear_string == nullptr) {
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

    if (CAN_WEAR(obj, ITEM_HOLD))
        if (!(ch->is_npc() && IS_SET(ch->act, ACT_PET))) {
            if (!remove_obj(ch, WEAR_HOLD, fReplace))
                return;
            if (obj->wear_string == nullptr) {
                act("$n holds $p in $s hands.", ch, obj, nullptr, To::Room);
                act("You hold $p in your hands.", ch, obj, nullptr, To::Char);
            }
            equip_char(ch, obj, WEAR_HOLD);
            return;
        }

    if (fReplace)
        send_to_char("You can't wear, wield, or hold that.\n\r", ch);
}

void do_wear(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Wear, wield, or hold what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        OBJ_DATA *obj_next;

        for (obj = ch->carrying; obj != nullptr; obj = obj_next) {
            obj_next = obj->next_content;
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
                wear_obj(ch, obj, false);
        }
        return;
    } else {
        if ((obj = get_obj_carry(ch, arg)) == nullptr) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        wear_obj(ch, obj, true);
    }
}

void do_remove(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Remove what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_wear(ch, arg)) == nullptr) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    remove_obj(ch, obj->wear_loc, true);
}

void do_sacrifice(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int gold;

    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];

    one_argument(argument, arg);

    if (arg[0] == '\0' || matches(arg, ch->name)) {
        snprintf(buf, sizeof(buf), "$n offers $mself to %s, who graciously declines.", deity_name);
        act(buf, ch);
        snprintf(buf, sizeof(buf), "%s appreciates your offer and may accept it later.\n\r", deity_name);
        send_to_char(buf, ch);
        return;
    }

    obj = get_obj_list(ch, arg, ch->in_room->contents);
    if (obj == nullptr) {
        send_to_char("You can't find it.\n\r", ch);
        return;
    }

    if (obj->item_type == ITEM_CORPSE_PC) {
        if (obj->contains) {
            snprintf(buf, sizeof(buf), "%s wouldn't like that.\n\r", deity_name);
            send_to_char(buf, ch);
            return;
        }
    }

    if (!CAN_WEAR(obj, ITEM_TAKE)) {
        act("$p is not an acceptable sacrifice.", ch, obj, nullptr, To::Char);
        return;
    }

    gold = UMAX(1, obj->level * 2);

    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
        gold = UMIN(gold, obj->cost);

    switch (gold) {
    default: snprintf(buf, sizeof(buf), "%s gives you %d gold coins for your sacrifice.\n\r", deity_name, gold); break;
    case 0:
        snprintf(buf, sizeof(buf), "%s laughs at your worthless sacrifice. You receive no gold coins.\n\r", deity_name);
        break;
    case 1:
        snprintf(buf, sizeof(buf), "%s is unimpressed by your sacrifice but grants you a single\n\rgold coin.\n\r",
                 deity_name);
        break;
    };
    send_to_char(buf, ch);

    ch->gold += gold;

    if (IS_SET(ch->act, PLR_AUTOSPLIT)) { /* AUTOSPLIT code */
        members = 0;
        for (gch = ch->in_room->people; gch != nullptr; gch = gch->next_in_room) {
            if (is_same_group(gch, ch))
                members++;
        }

        if (members > 1 && gold > 1) {
            snprintf(buffer, sizeof(buffer), "%d", gold);
            do_split(ch, buffer);
        }
    }

    snprintf(buf, sizeof(buf), "$n sacrifices $p to %s.", deity_name);
    act(buf, ch, obj, nullptr, To::Room);
    extract_obj(obj);
}

void do_quaff(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Quaff what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == nullptr) {
        send_to_char("You do not have that potion.\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_POTION) {
        send_to_char("You can quaff only potions.\n\r", ch);
        return;
    }

    if (ch->level < obj->level) {
        send_to_char("This liquid is too powerful for you to drink.\n\r", ch);
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

void do_recite(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((scroll = get_obj_carry(ch, arg1)) == nullptr) {
        send_to_char("You do not have that scroll.\n\r", ch);
        return;
    }

    if (scroll->item_type != ITEM_SCROLL) {
        send_to_char("You can recite only scrolls.\n\r", ch);
        return;
    }

    if (ch->level < scroll->level) {
        send_to_char("This scroll is too complex for you to comprehend.\n\r", ch);
        return;
    }

    target_name = arg2; /* Makes cast 'locate object' xxxxx work */
    obj = nullptr;
    if (arg2[0] == '\0') {
        victim = ch;
    } else {
        if ((victim = get_char_room(ch, arg2)) == nullptr && (obj = get_obj_here(ch, arg2)) == nullptr
            && (skill_table[scroll->value[1]].target != TAR_IGNORE)
            && (skill_table[scroll->value[1]].target != TAR_CHAR_OTHER)) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    act("$n recites $p.", ch, scroll, nullptr, To::Room);
    act("You recite $p.", ch, scroll, nullptr, To::Char);

    if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4 / 5) {
        send_to_char("|rYou mispronounce a syllable.|w\n\r", ch);
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

void do_brandish(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    if ((staff = get_eq_char(ch, WEAR_HOLD)) == nullptr) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (staff->item_type != ITEM_STAFF) {
        send_to_char("You can brandish only with a staff.\n\r", ch);
        return;
    }

    if ((sn = staff->value[3]) < 0 || sn >= MAX_SKILL || skill_table[sn].spell_fun == 0) {
        bug("Do_brandish: bad sn %d.", sn);
        return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (staff->value[2] > 0) {
        act("|W$n brandishes $p.|w", ch, staff, nullptr, To::Room);
        act("|WYou brandish $p.|w", ch, staff, nullptr, To::Char);
        if (ch->level < staff->level || number_percent() >= 20 + get_skill(ch, gsn_staves) * 4 / 5) {
            act("|WYou fail to invoke $p.|w", ch, staff, nullptr, To::Char);
            act("|W...and nothing happens.|w", ch);
            check_improve(ch, gsn_staves, false, 2);
        }

        else
            for (vch = ch->in_room->people; vch; vch = vch_next) {
                vch_next = vch->next_in_room;

                switch (skill_table[sn].target) {
                default: bug("Do_brandish: bad target for sn %d.", sn); return;

                case TAR_IGNORE:
                    if (vch != ch)
                        continue;
                    break;

                case TAR_CHAR_OFFENSIVE:
                    if (ch->is_npc() ? vch->is_npc() : vch->is_pc())
                        continue;
                    break;

                case TAR_CHAR_DEFENSIVE:
                    if (ch->is_npc() ? vch->is_pc() : vch->is_npc())
                        continue;
                    break;

                case TAR_CHAR_SELF:
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

void do_zap(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wand;
    OBJ_DATA *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0' && ch->fighting == nullptr) {
        send_to_char("Zap whom or what?\n\r", ch);
        return;
    }

    if ((wand = get_eq_char(ch, WEAR_HOLD)) == nullptr) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (wand->item_type != ITEM_WAND) {
        send_to_char("You can zap only with a wand.\n\r", ch);
        return;
    }

    obj = nullptr;
    if (arg[0] == '\0') {
        if (ch->fighting != nullptr) {
            victim = ch->fighting;
        } else {
            send_to_char("Zap whom or what?\n\r", ch);
            return;
        }
    } else {
        if ((victim = get_char_room(ch, arg)) == nullptr && (victim = get_char_world(ch, arg)) == nullptr
            && (obj = get_obj_here(ch, arg)) == nullptr && (skill_table[wand->value[3]].target) != TAR_CHAR_OTHER
            && (skill_table[wand->value[3]].target) != TAR_IGNORE) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
        if (skill_table[wand->value[3]].target == TAR_CHAR_OTHER || skill_table[wand->value[3]].target == TAR_IGNORE) {
            target_name = arg;
        } else {
            target_name = "";
        }

        if ((victim != nullptr && victim != ch && victim->in_room->vnum != ch->in_room->vnum)
            && skill_table[wand->value[3]].target == TAR_CHAR_OFFENSIVE) {
            act("You attempt to zap {}....."_format(get_char_room(ch, arg) ? victim->short_descr : "someone"), ch,
                nullptr, nullptr, To::Char);
            act("$n attempts to zap something....", ch);
            send_to_char("...but the |cgrand Iscarian magi|w outlawed interplanar combat millenia ago!\n\r", ch);
            act("$n appears to have been foiled by the law, and looks slightly annoyed!", ch, nullptr, nullptr,
                To::Room);
            return;
        }
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

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

void do_steal(CHAR_DATA *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int percent;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Steal what from whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("That's pointless.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->position == POS_FIGHTING) {
        send_to_char("You'd better not -- you might get hit.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_steal].beats);
    percent = number_percent() + (IS_AWAKE(victim) ? 10 : -50);

    if (ch->level + 5 < victim->level || victim->position == POS_FIGHTING || victim->is_pc()
        || (ch->is_pc() && percent > get_skill_learned(ch, gsn_steal))) {
        /*
         * Failure.
         */
        send_to_char("Oops.\n\r", ch);
        act("|W$n tried to steal from you.|w\n\r", ch, nullptr, victim, To::Vict);
        act("|W$n tried to steal from $N.|w\n\r", ch, nullptr, victim, To::NotVict);
        std::string buf;
        switch (number_range(0, 3)) {
        case 0: buf = "{} is a lousy thief!"_format(ch->name); break;
        case 1: buf = "{} couldn't rob {} way out of a paper bag!"_format(ch->name, possessive(*ch)); break;
        case 2: buf = "{} tried to rob me!"_format(ch->name); break;
        case 3: buf = "Keep your hands out of there, {}!"_format(ch->name); break;
        }
        victim->yell(buf);
        if (ch->is_pc()) {
            if (victim->is_npc()) {
                check_improve(ch, gsn_steal, false, 2);
                multi_hit(victim, ch, TYPE_UNDEFINED);
            } else {
                log_string(buf);
                if (!IS_SET(ch->act, PLR_THIEF)) {
                    SET_BIT(ch->act, PLR_THIEF);
                    send_to_char("|R*** You are now a THIEF!! ***|r\n\r", ch);
                    save_char_obj(ch);
                }
            }
        }

        return;
    }

    if (!str_cmp(arg1, "coin") || !str_cmp(arg1, "coins") || !str_cmp(arg1, "gold")) {
        int amount;

        amount = (int)(victim->gold * number_range(1, 10) / 100);
        if (amount <= 0) {
            ch->send_to("You couldn't get any gold.\n\r");
            return;
        }

        ch->gold += amount;
        victim->gold -= amount;
        ch->send_to("Bingo!  You got {} gold coins.\n\r"_format(amount));
        check_improve(ch, gsn_steal, true, 2);
        return;
    }

    if ((obj = get_obj_carry(victim, arg1)) == nullptr) {
        send_to_char("You can't find it.\n\r", ch);
        return;
    }

    if (!can_drop_obj(ch, obj) || IS_SET(obj->extra_flags, ITEM_INVENTORY) || obj->level > ch->level) {
        send_to_char("You can't pry it away.\n\r", ch);
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
        send_to_char("You have your hands full.\n\r", ch);
        return;
    }

    if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
        send_to_char("You can't carry that much weight.\n\r", ch);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, ch);
    check_improve(ch, gsn_steal, true, 2);
}

/*
 * Shopping commands.
 */

namespace {
CHAR_DATA *shopkeeper_in(const ROOM_INDEX_DATA &room) {
    for (auto *maybe_keeper = room.people; maybe_keeper; maybe_keeper = maybe_keeper->next_in_room) {
        if (maybe_keeper->is_npc() && maybe_keeper->pIndexData->pShop)
            return maybe_keeper;
    }
    return nullptr;
}
}

CHAR_DATA *find_keeper(CHAR_DATA *ch) {
    auto *keeper = shopkeeper_in(*ch->in_room);
    if (!keeper) {
        ch->send_to("You can't do that here.\n\r");
        return nullptr;
    }

    // Undesirables.
    if (ch->is_player_killer()) {
        keeper->say("Killers are not welcome!");
        keeper->yell("{} the KILLER is over here!\n\r"_format(ch->name));
        return nullptr;
    }

    if (ch->is_player_thief()) {
        keeper->say("Thieves are not welcome!");
        keeper->yell("{} the THIEF is over here!\n\r"_format(ch->name));
        return nullptr;
    }

    // Shop hours.
    const auto *pShop = keeper->pIndexData->pShop;
    if (time_info.hour() < pShop->open_hour) {
        keeper->say("Sorry, I am closed. Come back later.");
        return nullptr;
    }

    if (time_info.hour() > pShop->close_hour) {
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

int get_cost(CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy) {
    SHOP_DATA *pShop;
    int cost;

    if (obj == nullptr || (pShop = keeper->pIndexData->pShop) == nullptr)
        return 0;

    if (fBuy) {
        cost = obj->cost * pShop->profit_buy / 100;
    } else {
        OBJ_DATA *obj2;
        int itype;

        cost = 0;
        for (itype = 0; itype < MAX_TRADE; itype++) {
            if (obj->item_type == pShop->buy_type[itype]) {
                cost = obj->cost * pShop->profit_sell / 100;
                break;
            }
        }

        for (obj2 = keeper->carrying; obj2; obj2 = obj2->next_content) {
            if (obj->pIndexData == obj2->pIndexData) {
                cost = 0;
                break;
            }
        }
    }

    if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND)
        cost = cost * obj->value[2] / obj->value[1];

    return cost;
}

void do_buy(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    int cost, roll;

    if (argument[0] == '\0') {
        send_to_char("Buy what?\n\r", ch);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        char arg[MAX_INPUT_LENGTH];
        CHAR_DATA *pet;
        ROOM_INDEX_DATA *pRoomIndexNext;
        ROOM_INDEX_DATA *in_room;

        if (ch->is_npc())
            return;

        argument = one_argument(argument, arg);

        pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
        if (pRoomIndexNext == nullptr) {
            bug("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
            send_to_char("Sorry, you can't buy that here.\n\r", ch);
            return;
        }

        in_room = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet = get_char_room(ch, arg);
        ch->in_room = in_room;

        if (pet == nullptr || !IS_SET(pet->act, ACT_PET)) {
            send_to_char("Sorry, you can't buy that here.\n\r", ch);
            return;
        }

        if (ch->pet != nullptr) {
            send_to_char("You already own a pet.\n\r", ch);
            return;
        }

        cost = 10 * pet->level * pet->level;

        if (ch->gold < cost) {
            send_to_char("You can't afford it.\n\r", ch);
            return;
        }

        if (ch->level < pet->level) {
            send_to_char("You're not powerful enough to master this pet.\n\r", ch);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (ch->is_pc() && roll < get_skill_learned(ch, gsn_haggle)) {
            cost -= cost / 2 * roll / 100;
            snprintf(buf, sizeof(buf), "You haggle the price down to %d coins.\n\r", cost);
            send_to_char(buf, ch);
            check_improve(ch, gsn_haggle, true, 4);
        }

        ch->gold -= cost;
        pet = create_mobile(pet->pIndexData);
        SET_BIT(ch->act, PLR_BOUGHT_PET);
        SET_BIT(pet->act, ACT_PET);
        SET_BIT(pet->affected_by, AFF_CHARM);
        pet->comm = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;

        argument = one_argument(argument, arg);
        if (arg[0] != '\0')
            pet->name = smash_tilde("{} {}"_format(pet->name, arg));

        pet->description = "{}A neck tag says 'I belong to {}'."_format(pet->description, ch->name);

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        pet->leader = ch;
        ch->pet = pet;
        send_to_char("Enjoy your pet.\n\r", ch);
        act("$n bought $N as a pet.", ch, nullptr, pet, To::Room);
        return;
    } else {
        CHAR_DATA *keeper;
        OBJ_DATA *obj;

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
            send_to_char("You can't carry that many items.\n\r", ch);
            return;
        }

        if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
            send_to_char("You can't carry that much weight.\n\r", ch);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (ch->is_pc() && roll < get_skill_learned(ch, gsn_haggle)) {
            cost -= obj->cost / 2 * roll / 100;
            snprintf(buf, sizeof(buf), "You haggle the price down to %d coins.\n\r", cost);
            send_to_char(buf, ch);
            check_improve(ch, gsn_haggle, true, 4);
        }

        act("$n buys $p.", ch, obj, nullptr, To::Room);
        act("You buy $p.", ch, obj, nullptr, To::Char);
        ch->gold -= cost;
        keeper->gold += cost;

        if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
            obj = create_object(obj->pIndexData);
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

void do_list(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        ROOM_INDEX_DATA *pRoomIndexNext;
        CHAR_DATA *pet;
        bool found;

        pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
        if (pRoomIndexNext == nullptr) {
            bug("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
            send_to_char("You can't do that here.\n\r", ch);
            return;
        }

        found = false;
        for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room) {
            if (IS_SET(pet->act, ACT_PET)) {
                if (!found) {
                    found = true;
                    ch->send_to("Pets for sale:\n\r");
                }
                ch->send_to("[{:2}] {:8} - {}\n\r"_format(pet->level, 10 * pet->level * pet->level, pet->short_descr));
            }
        }
        if (!found)
            ch->send_to("Sorry, we're out of pets right now.\n\r");
        return;
    } else {
        CHAR_DATA *keeper;
        OBJ_DATA *obj;
        BUFFER *buffer;
        int cost;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == nullptr)
            return;
        one_argument(argument, arg);

        buffer = buffer_create();
        found = false;
        for (obj = keeper->carrying; obj; obj = obj->next_content) {
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj) && (cost = get_cost(keeper, obj, true)) > 0
                && (arg[0] == '\0' || is_name(arg, obj->name))) {
                if (!found) {
                    found = true;
                    buffer_addline(buffer, "[Lv Price] Item\n\r");
                }

                snprintf(buf, sizeof(buf), "[%2d %5d] %s.\n\r", obj->level, cost, obj->short_descr);
                buffer_addline(buffer, buf);
            }
        }

        buffer_send(buffer, ch); /* Doesn't matter if it's blank */

        if (!found)
            send_to_char("You can't buy anything here.\n\r", ch);
        return;
    }
}

void do_sell(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost, roll;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Sell what?\n\r", ch);
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
        send_to_char("You can't let go of it.\n\r", ch);
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
    if (ch->is_pc() && roll < get_skill_learned(ch, gsn_haggle)) {
        send_to_char("You haggle with the shopkeeper.\n\r", ch);
        cost += obj->cost / 2 * roll / 100;
        cost = UMIN(cost, 95 * get_cost(keeper, obj, true) / 100);
        cost = UMIN(cost, (int)keeper->gold);
        check_improve(ch, gsn_haggle, true, 4);
    }
    snprintf(buf, sizeof(buf), "You sell $p for %d gold piece%s.", cost, cost == 1 ? "" : "s");
    act(buf, ch, obj, nullptr, To::Char);
    ch->gold += cost;
    keeper->gold -= cost;
    if (keeper->gold < 0)
        keeper->gold = 0;

    if (obj->item_type == ITEM_TRASH) {
        extract_obj(obj);
    } else {
        obj_from_char(obj);
        obj->timer = number_range(200, 400);
        obj_to_char(obj, keeper);
    }
}

void do_value(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Value what?\n\r", ch);
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
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if ((cost = get_cost(keeper, obj, false)) <= 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, To::Vict);
        return;
    }

    snprintf(buf, sizeof(buf), "$n tells you 'I'll give you %d gold coins for $p'.", cost);
    act(buf, keeper, obj, ch, To::Vict);
    ch->reply = keeper;
}

void do_throw(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *bomb;
    int chance;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Throw at whom?\n\r", ch);
        return;
    }

    if ((bomb = get_eq_char(ch, WEAR_HOLD)) == nullptr) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (bomb->item_type != ITEM_BOMB) {
        send_to_char("You can throw only bombs.\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("You can't see them here.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    chance = get_skill_learned(ch, gsn_throw);

    chance += URANGE(-20, (ch->level - victim->level), 20) - number_percent();

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
    multi_hit(ch, victim, TYPE_UNDEFINED);
}

/* hailcorpse for getting out of sticky situations ooeer --Fara */

void do_hailcorpse(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    ROOM_INDEX_DATA *current_place;
    EXIT_DATA *pexit;
    bool foundit = false;
    OBJ_DATA *current_obj;
    if (is_switched(ch)) {
        send_to_char("You cannot hail NPC corpses.\n\r", ch);
        return;
    }

    if (ch->is_immortal()) {
        send_to_char("Those who cannot be slain may not pray for the return of their corpse.\n\r", ch);
        return;
    }

    act("$n falls to $s knees and incants a garbled verse.", ch, nullptr, ch, To::Room);
    send_to_char("You incant the sacred verse of Necrosis and are overcome with nausea.\n\r", ch);

    /* make them wait a bit, help prevent abuse */
    WAIT_STATE(ch, 25);

    /* first thing is to check the ch room to see if it's already here */
    for (current_obj = ch->in_room->contents; current_obj != nullptr; current_obj = current_obj->next_content) {
        if (current_obj->item_type == ITEM_CORPSE_PC && matches_inside(ch->name, current_obj->short_descr)) {
            act("$n's corpse glows momentarily!", ch);
            send_to_char("Your corpse appears to be in the room already!\n\r", ch);
            return;
        }
    }
    /* if not here then check all the rooms adjacent to this one */
    for (auto direction : all_directions) {
        /* No exits in that direction */
        if (foundit)
            break;
        current_place = ch->in_room;

        if ((pexit = current_place->exit[direction]) == nullptr || (current_place = pexit->u1.to_room) == nullptr
            || !can_see_room(ch, pexit->u1.to_room))
            continue;

        for (current_obj = current_place->contents; current_obj != nullptr; current_obj = current_obj->next_content) {
            if ((current_obj->item_type == ITEM_CORPSE_PC) && matches_inside(ch->name, current_obj->short_descr)) {
                foundit = true;
                break;
            }
        }
    }
    if (foundit) {
        obj_from_room(current_obj);
        obj_to_room(current_obj, ch->in_room);
        act("The corpse of $n materialises and floats gently before you!", ch);
        act("Your corpse materialises through a dark portal and floats to your feet!", ch, nullptr, nullptr, To::Char);
        return;
    } else {
        act("$n's prayers for assistance are ignored by the Gods.", ch);
        act("Your prayers for assistance are ignored. Your corpse cannot be found.", ch, nullptr, nullptr, To::Char);
    }
}
