/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  olc_obj.c:  Faramir's OLC object editor                              */
/*                                                                       */
/*************************************************************************/

#include "olc_obj.h"
#include "buffer.h"
#include "db.h"
#include "flags.h"
#include "merc.h"
#include "olc_ctrl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

/* COMM 24576 */

/*
 * This system is based on a player being able to enter an overall
 * OLC editing mode, given that they have permission to do so.
 * If they can, then they may use the object, mobile or room command
 * to enter a separate mode for each. They then work on one particular
 * object for the duration of being in that mode. They must then exit
 * that mode and the mud will update all aspects of all instantiations
 * of the OLC object they have just modified.
 */

/*
 * the main object command used to enter object mode and then
 * manipulate the current olc object
 */

void do_object(CHAR_DATA *ch, char *argument) {

    char buf[MAX_STRING_LENGTH];

    if (!IS_SET(ch->comm, COMM_OLC_MODE)) {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    if (!olc_user_authority(ch)) {
        send_to_char("OLC: sorry, only Faramir can use OLC commands atm.\n\r", ch);
        return;
    }
    if ((is_set_extra(ch, EXTRA_OLC_OBJ)) && ch->pcdata->olc->current_obj == nullptr) {
        bug("do_object: OLC: error! player %s is in OLC_OBJ mode, but obj is nullptr\n\r", ch->name);
        send_to_char("OLC: error! Your current object is nullptr. Consult an IMP now!\n\r", ch);
        return;
    }

    if (ch->pcdata->olc->current_obj->vnum != 0) {
        if (argument[0] != '\0') {
            olc_edit_obj(ch, argument);
            return;
        }
        send_to_char("Syntax: object [command] ? to list.\n\r", ch);
        return;
    }
    if (is_set_extra(ch, EXTRA_OLC_MOB) || is_set_extra(ch, EXTRA_OLC_ROOM)) {

        send_to_char("OLC: leave your current olc mode and enter object mode first.\n\r", ch);
        return;
    }
    if (is_number(argument)) {
        if (olc_can_edit_obj(ch, argument)) {
            OBJ_INDEX_DATA *new_obj; /* tut...tut, a local local */
            new_obj = get_obj_index(atoi(argument));
            set_extra(ch, EXTRA_OLC_OBJ);
            ch->pcdata->olc->current_obj = new_obj;
            snprintf(buf, sizeof(buf), "OLC: entering object mode to edit existing obj #%d.\n\r", atoi(argument));
            send_to_char(buf, ch);
            send_to_char("Syntax: object [command] ? to list.\n\r", ch);
            return;
        }
        send_to_char("OLC: either that object does not exist, or it cannot be edited.\n\r", ch);
        send_to_char("Syntax: object <vnum> {add <type> <level>}\n\r", ch);
        send_to_char("        (to begin editing an existing, or new object)\n\r", ch);
        return;
    } else {
        olc_parse_obj(ch, argument);
        return;
    }
}

/*
 * deal with parameters for building a new object
 */

void olc_parse_obj(CHAR_DATA *ch, char *argument) {

    char arg1[MAX_INPUT_LENGTH], command[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], levelStr[MAX_INPUT_LENGTH];
    int vnum = 0, level = 0;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, command);
    argument = one_argument(argument, type);
    argument = one_argument(argument, levelStr);

    if (command[0] == '\0' || type[0] == '\0' || levelStr[0] == '\0') {
        send_to_char("Syntax: object <vnum> {add <type> <level>}\n\r", ch);
        return;
    }
    if (is_number(arg1)) {
        if (!str_cmp(command, "add")) {
            if (olc_can_create_obj(ch, arg1)) {
                send_to_char("OLC: attempting to create object ...\n\r", ch);
                vnum = atoi(arg1);
                level = atoi(levelStr);
                if (olc_create_obj(ch, vnum, type, level))
                    ;
                (ch->pcdata->olc->current_obj->vnum) = vnum;
                return;
            }
            send_to_char("OLC: that vnum is already used, or is unavailable.\n\r", ch);
            return;
        }
    }
    send_to_char("Syntax: object <vnum> {add <type> <level>}\n\r", ch);
    return;
}

void olc_edit_obj(CHAR_DATA *ch, char *argument) {

    char command[MAX_INPUT_LENGTH];
    int temp = 0;

    if (argument[0] == '?' || argument[0] == '\0') {
        send_to_char("Syntax:       object <command>\n\r", ch);
        send_to_char("              object <command> ?  (to get help on command)\n\r", ch);
        send_to_char("for editing:  apply cost flags magical material\n\r", ch);
        send_to_char("              string weapon wear weight\n\r", ch);
        send_to_char("control:      destroy end undo\n\r", ch);
        return;
    }
    if (ch->pcdata->olc->current_obj == nullptr) {
        bug("olc_edit_obj: OLC: error! player %s is in OLC_OBJ mode, but obj is nullptr\n\r", ch->name);
        send_to_char("OLC: error! your current object is nullptr, consult an IMP immediately.\n\r", ch);
        return;
    }
    argument = one_argument(argument, command);
    if ((temp = atoi(command)) == (ch->pcdata->olc->current_obj->vnum)) {
        send_to_char("OLC: you are already editing that object!\n\r", ch);
        return;
    }
    if (!str_prefix(command, "apply")) {
        olc_apply_obj(ch, argument);
        return;
    }
    if (!str_prefix(command, "cost")) {
        olc_cost_obj(ch, argument);
        return;
    }
    if (!str_cmp(command, "destroy")) {
        olc_destroy_obj(ch);
        return;
    }
    if (!str_cmp(command, "end")) {
        /*		olc_end_objects( ch ); */
        return;
    }
    if (!str_prefix(command, "flags")) {
        olc_flag_obj(ch, argument);
        return;
    }
    if (!str_prefix(command, "magical")) {
        olc_magical_obj(ch, argument);
        return;
    }
    if (!str_prefix(command, "material")) {
        olc_material_obj(ch, argument);
        return;
    }
    if (!str_prefix(command, "string")) {
        olc_string_obj(ch, argument);
        return;
    }
    if (!str_cmp(command, "update")) {
        /*	olc_update_obj( ch, argument);  */
        return;
    }
    if (!str_prefix(command, "weapon")) {
        olc_weapon_obj(ch, argument);
        return;
    }
    if (!str_prefix(command, "wear")) {
        olc_wear_obj(ch, argument);
        return;
    }
    if (!str_prefix(command, "weight")) {
        olc_weight_obj(ch, argument);
        return;
    }
    if (!str_cmp(command, "undo")) {
        /*		olc_undo_current_obj( ch ); */
        return;
    }
    send_to_char("OLC: unrecognised object command: object ? to list.\n\r", ch);
    return;
}

/*
 * this looks to see if the builder can modify an existing
 * object, by looking at the vnum provided and comparing it
 * with the vnum range for objects in the builders olc data
 * structure. This range will have been defined when the
 * builder entered the OLC mode originally using the edit cmd.
 */

bool olc_can_edit_obj(CHAR_DATA *ch, char *argument) {

    int vnum = 0;

    /* temporary */

    return true;

    vnum = atoi(argument);

    if (vnum < ch->pcdata->olc->obj_vnums[0] || vnum > ch->pcdata->olc->obj_vnums[1])

        return false;

    return true;
}

/*
 * Similar to the function above, this checks if the builder can
 * create a new object using the vnum provided. It checks against
 * existing objects in the mud, and against the master vnum parameters
 * set when the builder enters OLC mode.
 */

bool olc_can_create_obj(CHAR_DATA *ch, char *argument) {

    int vnum = 0;
    vnum = atoi(argument);
    if ((get_obj_world(ch, argument)) != nullptr)
        return false;
    if (vnum < ch->pcdata->olc->master_vnums[0] || vnum > ch->pcdata->olc->master_vnums[1])
        return false;
    return true;
}

/*
 * OK this is the big mamma object creation function. If the parameters
 * passed by the builder are not valid, it returns false and the
 * builders current_obj is not set. Otherwise, the object is built, put
 * into the linked list, one is given to the builder and that becomes
 * his current object. He can then modify as he pleases or use
 * object end to do something else.
 */

bool olc_create_obj(CHAR_DATA *ch, int vnum, char *type, int level) {

    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    int typeRef = 0, iHash;

    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;

    pObjIndex->vnum = vnum;

    if (level > OLC_MAX_OBJECT_LEVEL || level < 1) {
        snprintf(buf, sizeof(buf), "OLC: object level must be between 1 and %d.\n\r", OLC_MAX_OBJECT_LEVEL);
        send_to_char(buf, ch);
        return false;
    }
    pObjIndex->level = level;
    if (!str_prefix(type, "armour"))
        typeRef = ITEM_ARMOR;
    if (!str_prefix(type, "container"))
        typeRef = ITEM_CONTAINER;
    if (!str_prefix(type, "drink"))
        typeRef = ITEM_DRINK_CON;
    if (!str_prefix(type, "food"))
        typeRef = ITEM_FOOD;
    if (!str_prefix(type, "fountain"))
        typeRef = ITEM_FOUNTAIN;
    if (!str_prefix(type, "furniture"))
        typeRef = ITEM_FURNITURE;
    if (!str_prefix(type, "light"))
        typeRef = ITEM_LIGHT;
    if (!str_prefix(type, "potion"))
        typeRef = ITEM_POTION;
    if (!str_prefix(type, "pill"))
        typeRef = ITEM_PILL;
    if (!str_prefix(type, "scroll"))
        typeRef = ITEM_SCROLL;
    if (!str_prefix(type, "staff"))
        typeRef = ITEM_STAFF;
    if (!str_prefix(type, "treasure"))
        typeRef = ITEM_TREASURE;
    if (!str_prefix(type, "wand"))
        typeRef = ITEM_WAND;
    if (!str_prefix(type, "weapon"))
        typeRef = ITEM_WEAPON;

    if (typeRef == 0) {
        send_to_char("OLC: object types: armour container drink food fountain furniture light ...\n\r", ch);
        send_to_char("...  potion pill scroll staff treasure wand weapon\n\r", ch);
        return false;
    }
    pObjIndex = alloc_perm(sizeof(*pObjIndex));
    obj = alloc_perm(sizeof(*obj));

    pObjIndex->new_format = true;
    newobjs++;

    snprintf(buf2, sizeof(buf2), "new object %s", type);
    pObjIndex->name = str_dup(buf2);
    snprintf(buf2, sizeof(buf2), "a new %s", type);
    pObjIndex->short_descr = str_dup(buf2);
    snprintf(buf2, sizeof(buf2), "A brand new, dull %s lies on the ground.", type);
    pObjIndex->description = str_dup(buf2);

    switch (typeRef) {
    case ITEM_LIGHT:
        pObjIndex->material = MATERIAL_GLASS;
        pObjIndex->wear_flags = 1; /* light  */
        pObjIndex->value[0] = 0;
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 999;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 4;
        break;
    case ITEM_CONTAINER:
        pObjIndex->material = MATERIAL_LEATHER;
        pObjIndex->wear_flags = GET_HOLD;
        pObjIndex->value[0] = (int)UMAX(30, level * 1.5);
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 6;
        break;
    case ITEM_SCROLL:
        pObjIndex->material = MATERIAL_VELLUM;
        pObjIndex->wear_flags = GET_HOLD; /* scroll */
        pObjIndex->value[0] = UMAX(1, level - 6);
        pObjIndex->value[1] = 1;
        pObjIndex->value[2] = 1;
        pObjIndex->value[3] = 1;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 4;
        break;
    case ITEM_WAND:
        pObjIndex->material = MATERIAL_WOOD;
        pObjIndex->wear_flags = GET_HOLD; /* wand   */
        pObjIndex->value[0] = UMAX(1, level - 6);
        pObjIndex->value[1] = 5;
        pObjIndex->value[2] = 5;
        pObjIndex->value[3] = 1;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 3;
        break;
    case ITEM_STAFF:
        pObjIndex->material = MATERIAL_WOOD;
        pObjIndex->wear_flags = GET_HOLD; /* staff  */
        pObjIndex->value[0] = UMAX(1, level - 6);
        pObjIndex->value[1] = 5;
        pObjIndex->value[2] = 5;
        pObjIndex->value[3] = 1;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 6;
        break;
    case ITEM_WEAPON:
        pObjIndex->material = MATERIAL_STEEL;
        pObjIndex->wear_flags = GET_WIELD; /* weapon */
        pObjIndex->value[0] = 1; /* sword default */
        pObjIndex->value[1] = 4;
        pObjIndex->value[2] = level / 4;
        pObjIndex->value[3] = 3; /* slash default */
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 10;
        break;
    case ITEM_TREASURE:
        pObjIndex->material = MATERIAL_GOLD;
        pObjIndex->wear_flags = 1; /* treasure */
        pObjIndex->value[0] = 0;
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = (level * 5);
        break;
    case ITEM_ARMOR:
        pObjIndex->material = MATERIAL_STEEL;
        pObjIndex->wear_flags = GET_BODY; /* armour */
        pObjIndex->value[0] = level / 4;
        pObjIndex->value[1] = level / 4;
        pObjIndex->value[2] = level / 4;
        pObjIndex->value[3] = level / 5;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = UMAX(8, level / 2);
        break;
    case ITEM_POTION:
        pObjIndex->material = MATERIAL_GLASS;
        pObjIndex->wear_flags = GET_HOLD; /* potion */
        pObjIndex->value[0] = UMAX(1, level - 6);
        pObjIndex->value[1] = 1;
        pObjIndex->value[2] = 1;
        pObjIndex->value[3] = 1;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 4;
        break;
    case ITEM_FURNITURE:
        pObjIndex->material = MATERIAL_WOOD;
        pObjIndex->wear_flags = 0; /* furnit */
        pObjIndex->value[0] = 0;
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 30;
        break;
    case ITEM_DRINK_CON:
        pObjIndex->material = MATERIAL_LEATHER;
        pObjIndex->wear_flags = GET_HOLD; /* drink container  */
        pObjIndex->value[0] = UMIN(20, level / 2);
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 3;
        break;
    case ITEM_FOOD:
        pObjIndex->material = MATERIAL_FOOD;
        pObjIndex->wear_flags = 1; /* food   */
        pObjIndex->value[0] = UMIN(20, level / 2);
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 1;
        break;
    case ITEM_FOUNTAIN:
        pObjIndex->material = MATERIAL_STONE;
        pObjIndex->wear_flags = 0; /* fount  */
        pObjIndex->value[0] = UMIN(50, level);
        pObjIndex->value[1] = UMIN(50, level);
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 90;
        break;
    case ITEM_PILL:
        pObjIndex->material = MATERIAL_FOOD;
        pObjIndex->wear_flags = 1; /* pill   */
        pObjIndex->value[0] = UMAX(1, level - 6);
        pObjIndex->value[1] = 1;
        pObjIndex->value[2] = 1;
        pObjIndex->value[3] = 1;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 1;
        break;
    default:
        pObjIndex->material = MATERIAL_DEFAULT;
        pObjIndex->wear_flags = 0;
        pObjIndex->value[0] = 0;
        pObjIndex->value[1] = 0;
        pObjIndex->value[2] = 0;
        pObjIndex->value[3] = 0;
        pObjIndex->value[4] = 0;
        pObjIndex->weight = 0;
    }
    pObjIndex->item_type = typeRef;
    pObjIndex->extra_flags = 0;
    pObjIndex->wear_string = nullptr;

    switch (typeRef) {
    case ITEM_TREASURE: pObjIndex->cost = (level * 300); break;
    default: pObjIndex->cost = (level * 200);
    }
    iHash = vnum % MAX_KEY_HASH;
    pObjIndex->next = obj_index_hash[iHash];
    obj_index_hash[iHash] = pObjIndex;
    top_obj_index++;

    obj = create_object(pObjIndex, pObjIndex->level);
    obj_to_char(obj, ch);

    ch->pcdata->olc->obj_template = pObjIndex;
    /* defines the 'undo' version of the obj */

    snprintf(buf2, sizeof(buf2), "$n has created a new %s!", type);
    act(buf2, ch, nullptr, nullptr, TO_ROOM);
    snprintf(buf2, sizeof(buf2), "OLC: new %s created!\n\r", type);
    send_to_char(buf2, ch);
    return true;
}

/*
 *  destroys the current object and removes from linked list.
 */

void olc_destroy_obj(CHAR_DATA *ch) {

    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj = nullptr, *in_obj, *next_obj;
    int count = 0;

    for (obj = object_list; obj != nullptr; obj = next_obj) {
        for (in_obj = obj; in_obj->in_obj != nullptr; in_obj = in_obj->in_obj)
            ;
        next_obj = obj->next;
        if ((ch->pcdata->olc->current_obj->vnum) == in_obj->pIndexData->vnum) {
            extract_obj(in_obj);
            count++;
            if (count > 50)
                break;
        }
    }
    if ((_delete_obj(ch->pcdata->olc->current_obj->vnum)) == 0)
        send_to_char("OLC: warning: object not erased from database.\n\r", ch);

    snprintf(buf, sizeof(buf), "OLC: %d instances of object vnum %d destroyed!\n\r", count,
             ch->pcdata->olc->current_obj->vnum);
    send_to_char(buf, ch);
    return;
}

void olc_magical_obj(CHAR_DATA *ch, char *argument) {

    char command[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    bool q = false, changed = false;
    int count = 0, max_count = 0, initial = 0;
    OBJ_INDEX_DATA *tmp_obj;

    tmp_obj = ch->pcdata->olc->current_obj;
    switch (tmp_obj->item_type) {
    case ITEM_STAFF:
    case ITEM_WAND:
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL: break;
    default: send_to_char("OLC: use object magical only for staves, wands, scrolls or pills.\n\r", ch); return;
    }
    if (argument[0] == '\0') {
        send_to_char("Syntax: object magical [command] ? to list.\n\r", ch);
        return;
    }

    if (argument[0] == '?') {
        send_to_char("Syntax: object magical [charges||power||spells] ? for help on each.\n\r", ch);
        send_to_char("OLC:    object magical is used to modify potions/staves/wands/pills.\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_MAGICAL, ch->pcdata->olc->current_obj);
        return;
    }
    argument = one_argument(argument, command);

    if (argument[0] == '\0')
        q = true;

    if (!str_prefix(command, "spells")) {
        if (q) {
            send_to_char("OLC: object spells <spell list> used set the spells on an object.\n\r", ch);
            send_to_char("     use ? to list all available spells.\n\r", ch);
            return;
        }
        if (argument[0] == '?') {
            olc_slookup_obj(ch);
            return;
        }
        switch (ch->pcdata->olc->current_obj->item_type) {
        case ITEM_STAFF:
        case ITEM_WAND: initial = 3; break;
        default: initial = 1;
        }
        max_count = 4;

        olc_change_spells_obj(ch, argument, initial, max_count);
        return;
    }

    if (!str_prefix(command, "charges")) {

        if (q) {
            send_to_char("OLC: object magical charges <current> <max>\n\r", ch);
            send_to_char("     (available only for wands and staves)\n\r", ch);
            return;
        }
        if (((ch->pcdata->olc->current_obj->item_type) != ITEM_WAND)
            && ((ch->pcdata->olc->current_obj->item_type) != ITEM_STAFF)) {
            send_to_char("OLC: object magical charges only for wands and staves.\n\r", ch);
            return;
        }
        if (arg1[0] == '\0' || arg1[0] == '?') {
            send_to_char("OLC: how many charges do you wish to set?\n\r", ch);
            send_to_char("Syntax: object magical charges <current> <maximum>\n\r", ch);
            return;
        }
        one_argument(argument, arg2);
        count = atoi(arg1);
        max_count = atoi(arg2);
        if (!is_number(arg1) || !is_number(arg2)) {
            send_to_char("Syntax: object magical charges <current> <maximum>\n\r", ch);
            return;
        }
        if (count < 0 || max_count < 1) {
            send_to_char("OLC: use positive integer values.\n\r", ch);
            return;
        }
        if (count > max_count)
            count = max_count;
        if (count > 10)
            count = 10;
        if (count < 1)
            count = 1;
        if (max_count > 10)
            max_count = 10;
        if (max_count < 1)
            max_count = 1;

        ch->pcdata->olc->current_obj->value[1] = max_count;
        ch->pcdata->olc->current_obj->value[2] = count;
        send_to_char("OLC: charge values set.\n\r", ch);
        return;
    }
    if (!str_prefix(command, "power")) {
        if (q) {
            send_to_char("Syntax: object magical power <level>\n\r", ch);
            send_to_char("        (used to set the spell level)\n\r", ch);
            return;
        }
        if (arg1[0] == '\0' || arg1[0] == '?') {
            send_to_char("OLC: what power level do you wish to set?\n\r", ch);
            send_to_char("     (may not me more than six levels higher than the object)\n\r", ch);
            return;
        }
        if (!is_number(arg1)) {
            send_to_char("Syntax: object magical power <level>\n\r", ch);
            return;
        }
        initial = atoi(arg1);
        if (initial < 0) {
            send_to_char("OLC: use a positive integer value.\n\r", ch);
            return;
        }
        if (initial > ((ch->pcdata->olc->current_obj->level - 6))) {
            changed = true;
            initial = (ch->pcdata->olc->current_obj->level);
        }
        ch->pcdata->olc->current_obj->value[0] = initial;
        send_to_char("OLC: power set.\n\r", ch);
        if (changed)
            send_to_char("OLC: the amount you entered was capped.\n\r", ch);
        return;
    }
    send_to_char("OLC: unrecognised object magical command.\n\r", ch);
    send_to_char("OLC: object magical ? (to list)\n\r", ch);
    return;
}

void olc_change_spells_obj(CHAR_DATA *ch, char *argument, int initial, int max_count) {

    char spell_na[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    int slot = 0, count = 0, val = 0;
    bool limit = false, found = false, changed = false, error = false;
    char *sp_list = argument;

    while (*sp_list && !limit) {
        found = false;
        val = initial + count;
        sp_list = one_argument(sp_list, spell_na);
        slot = skill_lookup(spell_na);
        if (slot > 0) {
            found = true;
            changed = true;
            ch->pcdata->olc->current_obj->value[val] = slot;
            slot = 0;
        }
        count++;
        if (!found) {
            snprintf(buf, sizeof(buf), "%s ", spell_na);
            error = true;
        }
        if (max_count <= count)
            limit = true;
    }
    if (!changed)
        send_to_char("OLC: object was not modified.\n\r", ch);
    if (error) {
        send_to_char("OLC:|C ", ch);
        send_to_char(buf, ch);
        send_to_char("|w not recognised.\n\r", ch);
    }
    if (changed)
        send_to_char("OLC: object spells have been modified.\n\r", ch);
    return;
}

/*
 * set what the object is made of
 */

void olc_material_obj(CHAR_DATA *ch, char *argument) {

    char text[MAX_STRING_LENGTH];
    int count = 0;
    bool found = false;

    if (argument[0] == '?' || argument[0] == '\0') {
        olc_list_materials_obj(ch);
        return;
    }

    for (count = 0; material_table[count].material_name != '\0'; count++) {
        if (olc_material_search_obj(ch, argument, count))
            found = true;
    }
    if (found)
        return;

    snprintf(text, sizeof(text), "OLC: %s is not a valid material type.\n\r", argument);
    send_to_char(text, ch);
    send_to_char("     object material (to list types available)\n\r", ch);
    return;
}

void olc_list_materials_obj(CHAR_DATA *ch) {

    char text[MAX_STRING_LENGTH];
    BUFFER *buffer;
    int count = 0, len = 0;
    buffer = new_buf();
    add_buf(buffer, "Materials available:\n\n\r");
    for (count = 0; material_table[count].material_name != '\0'; count++) {
        len += strlen(material_table[count].material_name);
        if (len >= 70) {
            snprintf(text, sizeof(text), "\n\r");
            add_buf(buffer, text);
            len = 0;
        }
        snprintf(text, sizeof(text), "%s ", material_table[count].material_name);
        add_buf(buffer, text);
    }
    buf_to_char(buffer, ch);
    send_to_char("\n\rOLC: object material <material type>\n\r", ch);
    olc_display_obj_vars(ch, OLC_V_MATERIAL, ch->pcdata->olc->current_obj);
    return;
}

bool olc_material_search_obj(CHAR_DATA *ch, char *argument, int count) {

    char text[MAX_STRING_LENGTH];

    if (!str_prefix(argument, material_table[count].material_name)) {
        if ((ch->pcdata->olc->current_obj->material == material_lookup(material_table[count].material_name))) {
            send_to_char("OLC: object is already that material type!\n\r", ch);
            return true;
        }
        ch->pcdata->olc->current_obj->material = count;

        snprintf(text, sizeof(text), "OLC: object: %s is now material %s.\n\r",
                 ch->pcdata->olc->current_obj->short_descr, material_table[count].material_name);
        send_to_char(text, ch);
        return true;
    }
    return false;
}

/*
 * set the value of the item
 */

void olc_cost_obj(CHAR_DATA *ch, char *argument) {

    int amount = 0;
    bool max = false;

    if (argument[0] == '?') {
        send_to_char("OLC: object cost <amount>\n\r", ch);
        send_to_char("     (used to change the object's value)\n\r", ch);
        send_to_char("     normal items: up to 200*level\n\r", ch);
        send_to_char("     treasure:     up to 300*level\n\r", ch);
        send_to_char("     putting 'max' for amount will maximise the amount.\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_COST, ch->pcdata->olc->current_obj);
        return;
    }
    if (!str_prefix(argument, "maximum"))
        max = true;
    if (!is_number(argument) && !max) {
        send_to_char("Syntax: object cost <amount> (? for help)\n\r", ch);
        return;
    }
    amount = atoi(argument);

    if (amount = (ch->pcdata->olc->current_obj->cost)) {
        send_to_char("OLC: object is already that cost!\n\r", ch);
        return;
    }
    if (amount < 0 && !max)
        amount = 0;

    switch (ch->pcdata->olc->current_obj->item_type) {
    case ITEM_TREASURE:
        if (max)
            ch->pcdata->olc->current_obj->cost = (ch->pcdata->olc->current_obj->level * 300);
        else
            ch->pcdata->olc->current_obj->cost = UMIN(amount, (ch->pcdata->olc->current_obj->level * 300));
        break;
    default:
        if (max)
            ch->pcdata->olc->current_obj->cost = (ch->pcdata->olc->current_obj->level * 200);
        else
            ch->pcdata->olc->current_obj->cost = UMIN(amount, (ch->pcdata->olc->current_obj->level * 200));
    }
    send_to_char("OLC: object cost changed.\n\r", ch);
    return;
}

/*
 * to set/remove/toggle the extra flags on the object, eg. glow, hum
 */

void olc_flag_obj(CHAR_DATA *ch, char *argument) {

    if (argument[0] == '?') {
        send_to_char("Syntax: object flags <flag list> (empty list to show all available)\n\r", ch);
        send_to_char("OLC:    for each flag: {+/-}flagname\n\r", ch);
        send_to_char("        (without +/-, OLC will toggle the state of the flag)\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_FLAGS, ch->pcdata->olc->current_obj);
        return;
    }
    send_to_char("OLC: current extra flags are: \n\r", ch);
    ch->pcdata->olc->current_obj->extra_flags =
        flag_set(ITEM_EXTRA_FLAGS, argument, ch->pcdata->olc->current_obj->extra_flags, ch);
    return;
}

/*
 * to set/remove/toggle the wear location flags on the object eg. wrist, neck
 */

void olc_wear_obj(CHAR_DATA *ch, char *argument) {

    if (argument[0] == '?') {
        send_to_char("Syntax: object wear <wear location list>\n\r", ch);
        send_to_char("OLC:    an empty wear location list shows all available\n\r", ch);
        send_to_char("        for each location: {+/-}location\n\r", ch);
        send_to_char("        (without +/-, OLC will toggle the state of the flag)\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_WEAR, ch->pcdata->olc->current_obj);
        return;
    }
    send_to_char("OLC: current wear flags are: \n\r", ch);
    ch->pcdata->olc->current_obj->wear_flags =
        flag_set(ITEM_WEAR_FLAGS, argument, ch->pcdata->olc->current_obj->wear_flags, ch);
    return;
}

/*
 * initialise modifications to a weapon item
 */

void olc_weapon_obj(CHAR_DATA *ch, char *argument) {

    char arg1[MAX_INPUT_LENGTH];

    if (ch->pcdata->olc->current_obj->item_type != ITEM_WEAPON) {
        send_to_char("OLC: object weapon is only for weapon items.\n\r", ch);
        return;
    }
    if (argument[0] == '?') {
        send_to_char("Syntax:   object weapon [command]\n\r", ch);
        send_to_char("Commands: [attack||flags||type]\n\r", ch);
        send_to_char("OLC: proving a command demonstrates syntax\n\r", ch);
        send_to_char("     adding ? after the command lists current settings.\n\r", ch);
        return;
    }
    if (argument[0] == '\0') {
        send_to_char("Syntax: object weapon [command]\n\r", ch);
        send_to_char("        use ? to list commands available.\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg1);

    if (!str_prefix(arg1, "attack")) {
        olc_attack_obj(ch, argument);
        return;
    }
    if (!str_prefix(arg1, "flags")) {
        olc_weapon_flags_obj(ch, argument);
        return;
    }
    if (!str_prefix(arg1, "type")) {
        olc_weapon_type_obj(ch, argument);
        return;
    }
    send_to_char("OLC: unrecognised object weapon command.\n\r", ch);
    send_to_char("     object weapon ? (for help)\n\r", ch);
    return;
}

void olc_attack_obj(CHAR_DATA *ch, char *argument) {

    BUFFER *buffer;
    char text[MAX_STRING_LENGTH];
    int count = 0, len = 0;
    bool found = false;

    if (argument[0] == '\0') {
        send_to_char("Syntax: object weapon attack <attack name>\n\r", ch);
        send_to_char("        use ? to list attack types available.\n\r", ch);
        return;
    }

    if (argument[0] == '?') {
        send_to_char("OLC: attack types available for weapons:\n\r", ch);

        /*
         * OLC: note: $ added to the end of the const.c attack_table for this!
         */
        buffer = new_buf();
        for (count = 1; attack_table[count].name[0] != '$'; count++) {
            len += strlen(olc_stripcolour(attack_table[count].name));
            if (len >= 60) {
                len = 0;
                snprintf(text, sizeof(text), "\n\r");
                add_buf(buffer, text);
            }
            snprintf(text, sizeof(text), "%s ", olc_stripcolour(attack_table[count].name));
            add_buf(buffer, text);
        }
        buf_to_char(buffer, ch);
        send_to_char("\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_WEAPON, ch->pcdata->olc->current_obj);
        return;
    }
    for (count = 1; (attack_table[count].name[0] != '$') && !found; count++) {
        if (!str_prefix(attack_table[count].name, argument)) {
            ch->pcdata->olc->current_obj->value[3] = count;
            found = true;
            snprintf(text, sizeof(text), "OLC: attack type set to %s.\n\r", argument);
            send_to_char(text, ch);
            return;
        }
    }
    if (!found) {
        snprintf(text, sizeof(text), "OLC: %s is not a recognised attack type.\n\r", argument);
        send_to_char(text, ch);
        send_to_char("     object weapon attack (for help)\n\r", ch);
        return;
    }
}

/*
 * set the weapon type flags
 */

void olc_weapon_flags_obj(CHAR_DATA *ch, char *argument) {

    if (argument[0] == '\0') {
        send_to_char("Syntax: object weapon flags <flags list>\n\r", ch);
        send_to_char("OLC:    use ? to list available flags\n\r", ch);
        send_to_char("        for each flags: {+/-}flag\n\r", ch);
        send_to_char("        (without +/-, OLC will toggle the state of the flag)\n\r", ch);
        return;
    }
    send_to_char("OLC: current weapon flags are: \n\r", ch);
    ch->pcdata->olc->current_obj->value[4] =
        flag_set(WEAPON_TYPE_FLAGS, argument, ch->pcdata->olc->current_obj->value[4], ch);
    return;
}

/*
 * set the weapon class, eg. axe, sword etc. Uses the table in olc_ctrl.h
 */

void olc_weapon_type_obj(CHAR_DATA *ch, char *argument) {

    BUFFER *buffer;
    char text[MAX_STRING_LENGTH];
    int count = 0, len = 0;

    if (argument[0] == '\0') {
        send_to_char("Syntax: object weapon type <weapon type>\n\r", ch);
        send_to_char("        use ? to list weapon types available.\n\r", ch);
        return;
    }
    if (argument[0] == '?') {
        buffer = new_buf();
        send_to_char("OLC: types available: \n\r", ch);
        for (count = 0; weapon_class_table[count][0] != '$'; count++) {
            len += strlen(weapon_class_table[count]);
            if (len >= 70) {
                len = 0;
                snprintf(text, sizeof(text), "\n\r");
                add_buf(buffer, text);
            }
            snprintf(text, sizeof(text), "%s ", weapon_class_table[count]);
            add_buf(buffer, text);
        }
        buf_to_char(buffer, ch);
        send_to_char("\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_WEAPON, ch->pcdata->olc->current_obj);
        return;
    }
    for (count = 0; weapon_class_table[count][0] != '$'; count++) {
        if (olc_weapon_type_search_obj(ch, argument, count))
            return;
    }
    send_to_char("OLC: weapon type |W", ch);
    send_to_char(argument, ch);
    send_to_char(" |wnot recognised.\n\r", ch);
    send_to_char("     object weapon type (for help)\n\r", ch);
    return;
}

bool olc_weapon_type_search_obj(CHAR_DATA *ch, char *weapon, int count) {

    if (!str_prefix(weapon, weapon_class_table[count])) {
        if (ch->pcdata->olc->current_obj->value[0] == count) {
            send_to_char("OLC: the weapon is already of that type!\n\r", ch);
            return true;
        }
        ch->pcdata->olc->current_obj->value[0] = count;
        send_to_char("OLC: weapon type set to ", ch);
        send_to_char((char *)weapon_class_table[count], ch);
        send_to_char(".\n\r", ch);
        return true;
    }
    return false;
}

/*
 * might improve this in future to cope with different material types
 */

void olc_weight_obj(CHAR_DATA *ch, char *argument) {

    int amount = 0;

    if (argument[0] == '?') {
        send_to_char("Syntax: object weight <amount>\n\r", ch);
        send_to_char("OLC:    the amount must be a reasonable number.\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_WEIGHT, ch->pcdata->olc->current_obj);
        return;
    }
    if (argument[0] == '\0' || (!is_number(argument))) {
        send_to_char("OLC: how heavy?\n\r", ch);
        return;
    }
    amount = atoi(argument);
    if (amount < 1)
        amount = 1;
    ch->pcdata->olc->current_obj->weight = UMIN(amount, (ch->pcdata->olc->current_obj->level * 2));
    send_to_char("OLC: weight set.\n\r", ch);
    return;
}

void olc_string_obj(CHAR_DATA *ch, char *argument) {

    char arg1[MAX_INPUT_LENGTH];

    if (argument[0] == '?' || argument[0] == '\0') {
        send_to_char("Syntax: object string [name||short||long||extended||list] [clipboard||string]\n\r", ch);
        send_to_char("OLC:    ? to get help on each of the above\n\r", ch);
        send_to_char("        list to show current strings\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg1);

    if (!str_prefix(arg1, "name")) {
        olc_string_name_obj(ch, argument);
        return;
    }
    if (!str_prefix(arg1, "short")) {
        olc_string_short_obj(ch, argument);
        return;
    }
    if (!str_prefix(arg1, "long")) {
        olc_string_long_obj(ch, argument);
        return;
    }
    if (!str_prefix(arg1, "extended")) {
        olc_string_extd_obj(ch, argument);
        return;
    }
    if (!str_prefix(arg1, "list")) {
        olc_display_obj_vars(ch, OLC_V_STRING, ch->pcdata->olc->current_obj);
        return;
    }
    send_to_char("OLC: unrecognised object string command.\n\r", ch);
    send_to_char("     object string ? to list.\n\r", ch);
    return;
}

void olc_string_name_obj(CHAR_DATA *ch, char *argument) {

    char buf[MAX_INPUT_LENGTH], *ptr;

    if (argument[0] == '?' && argument[1] == '\0') {
        send_to_char("Syntax: object string name [clipboard||string]\n\r", ch);
        return;
    }
    if (argument[0] == '\0' && ch->clipboard == nullptr) {
        send_to_char("OLC:    string the name to what?\n\r", ch);
        send_to_char("Syntax: object string name [clipboard||string]\n\r", ch);
        return;
    }
    if (!str_prefix(argument, "clipboard")) {
        argument = ch->clipboard;
        if (argument == nullptr) {
            send_to_char("OLC: you have no clipboard!\n\r", ch);
            return;
        }
    }
    ptr = strcpy(buf, argument);
    for (; *ptr; ptr++)
        if (*ptr == '\n')
            *ptr = '\0';

    free_string(ch->pcdata->olc->current_obj->name);
    ch->pcdata->olc->current_obj->name = str_dup(buf);
    send_to_char("OLC: name changed.\n\r", ch);
    return;
}

void olc_string_short_obj(CHAR_DATA *ch, char *argument) {

    char buf[MAX_INPUT_LENGTH], *ptr;

    if (argument[0] == '?' && argument[1] == '\0') {
        send_to_char("Syntax: object string short [clipboard||string]\n\r", ch);
        return;
    }
    if (argument[0] == '\0' && ch->clipboard == nullptr) {
        send_to_char("OLC:    string the short name to what?\n\r", ch);
        send_to_char("Syntax: object string short [clipboard||string]\n\r", ch);
        return;
    }
    if (!str_prefix(argument, "clipboard")) {
        argument = ch->clipboard;
        if (argument == nullptr) {
            send_to_char("OLC: you have no clipboard!\n\r", ch);
            return;
        }
    }
    ptr = strcpy(buf, argument);
    for (; *ptr; ptr++)
        if (*ptr == '\n')
            *ptr = '\0';
    free_string(ch->pcdata->olc->current_obj->short_descr);
    ch->pcdata->olc->current_obj->short_descr = str_dup(buf);
    send_to_char("OLC: short name changed.\n\r", ch);
    return;
}

void olc_string_long_obj(CHAR_DATA *ch, char *argument) {

    char buf[MAX_INPUT_LENGTH], *ptr;

    if (argument[0] == '?' && argument[1] == '\0') {
        send_to_char("Syntax: object string long [clipboard||string]\n\r", ch);
        return;
    }
    if (argument[0] == '\0' && ch->clipboard == nullptr) {
        send_to_char("OLC:    string the long description to what?\n\r", ch);
        send_to_char("Syntax: object string long [clipboard||string]\n\r", ch);
        return;
    }
    if (!str_prefix(argument, "clipboard")) {

        argument = ch->clipboard;
        if (argument == nullptr) {
            send_to_char("OLC: you have no clipboard!\n\r", ch);
            return;
        }
    }
    ptr = strcpy(buf, argument);
    for (; *ptr; ptr++)
        if (*ptr == '\n')
            *ptr = '\0';
    free_string(ch->pcdata->olc->current_obj->description);
    ch->pcdata->olc->current_obj->description = str_dup(buf);
    send_to_char("OLC: long description changed.\n\r", ch);
    return;
}

void olc_string_extd_obj(CHAR_DATA *ch, char *argument) {

    char key[MAX_INPUT_LENGTH];
    EXTRA_DESCR_DATA *extras;

    if (argument[0] == '?' && argument[1] == '\0') {
        send_to_char("Syntax: object string extended <keyword> clipboard\n\r", ch);
        return;
    }
    if (argument[0] == '\0' && ch->clipboard == nullptr) {
        send_to_char("OLC:    string the extended description to the clipboard?\n\r", ch);
        send_to_char("Syntax: object string extended <keyword> clipboard\n\r", ch);
        return;
    }
    argument = one_argument(argument, key);

    if (ch->clipboard == nullptr) {
        send_to_char("OLC: you have no clipboard!\n\r", ch);
        return;
    }
    if (!str_prefix(argument, "clipboard")) {
        argument = ch->clipboard;
        if (argument == nullptr) {
            send_to_char("OLC: you have no clipboard!\n\r", ch);
            return;
        }
    }
    if (extra_descr_free == nullptr) {
        extras = alloc_perm(sizeof(*extras));
    } else {
        extras = extra_descr_free;
        extra_descr_free = extras->next;
    }
    extras->keyword = str_dup(key);
    extras->description = str_dup(argument);
    extras->next = ch->pcdata->olc->current_obj->extra_descr;
    ch->pcdata->olc->current_obj->extra_descr = extras;
    send_to_char("OLC: extra description added ok.\n\r", ch);
    return;
}

/*
 * big bitchin' function to fiddle an object's bonus apply bits
 * awoo! :)
 */

void olc_apply_obj(CHAR_DATA *ch, char *argument) {

    char arg1[MAX_INPUT_LENGTH], oper[1];
    int count = 0, amount = 0, aff_loc = 30;
    bool found = false;

    if (argument[0] == '?' || argument[0] == '\0') {
        send_to_char("Syntax: object apply <+/-> <apply type> <amount>\n\r", ch);
        olc_display_obj_vars(ch, OLC_V_APPLY, ch->pcdata->olc->current_obj);
        olc_list_apply_table_obj(ch);
        return;
    }
    argument = one_argument(argument, oper);

    if (oper[0] != '+' && oper[0] != '-') {
        send_to_char("OLC:    unrecognised operator.\n\r", ch);
        send_to_char("Syntax: object apply <+/-> <apply type> <amount>\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg1);

    if (arg1[0] == '?' || arg1[0] == '\0' || arg1[0] == '*') {
        send_to_char("Syntax: object apply <+/-> <apply type> <amount>\n\r", ch);
        return;
    }
    for (count = 0; apply_lookup_table[count].name[0] != '$' && !found; count++) {
        if (!str_prefix(arg1, apply_lookup_table[count].name)) {
            aff_loc = apply_lookup_table[count].value;
            found = true;
        }
        if (aff_loc == APPLY_NONE) {
            send_to_char("OLC: apply none?\n\r", ch);
            return;
        }
    }
    if (!found) {
        send_to_char("OLC: apply type |W", ch);
        send_to_char(arg1, ch);
        send_to_char(" |wnot recognised.\n\r", ch);
        send_to_char("Syntax: object apply ? to list.\n\r", ch);
        return;
    }
    if (oper[0] == '+') {
        if (!is_number(argument)) {
            send_to_char("OLC: amount must be numeric.\n\r", ch);
            return;
        }
        amount = atoi(argument);
        if (!olc_check_apply_range(ch, aff_loc, amount))
            return;
    }
    switch (oper[0]) {
    case '+':
        olc_add_affect_obj(ch, aff_loc, amount);
        return;
        break;
    case '-': olc_remove_affect_obj(ch, aff_loc); return;
    }
}

bool olc_check_apply_range(CHAR_DATA *ch, int aff_loc, int amount) {

    char text[MAX_STRING_LENGTH];

    if (amount < (apply_lookup_table[aff_loc].minimum)) {
        snprintf(text, sizeof(text), "OLC: amount %d is below the minimum of %d for apply type '%s'.\n\r", amount,
                 apply_lookup_table[aff_loc].minimum, apply_lookup_table[aff_loc].name);
        send_to_char(text, ch);
        return false;
    }
    if (amount > (apply_lookup_table[aff_loc].maximum)) {
        snprintf(text, sizeof(text), "OLC: amount %d exceeds the maximum of %d for apply type '%s'.\n\r", amount,
                 apply_lookup_table[aff_loc].maximum, apply_lookup_table[aff_loc].name);
        send_to_char(text, ch);
        return false;
    }
    return true;
}

void olc_list_apply_table_obj(CHAR_DATA *ch) {

    BUFFER *buffer = new_buf();
    char text[MAX_STRING_LENGTH];
    int count = 0, len = 0;
    bool end = false;

    while (!end) {

        if (!str_prefix(apply_lookup_table[count].name, "$")) {
            end = true;
            continue;
        }
        if (!str_prefix(apply_lookup_table[count].name, "*")) {
            count++;
            continue;
        } else {
            len += strlen(apply_lookup_table[count].name);
            if (len >= 60) {
                snprintf(text, sizeof(text), "\n\r");
                add_buf(buffer, text);
                len = 0;
            }
            snprintf(text, sizeof(text), "%s ", apply_lookup_table[count].name);
            add_buf(buffer, text);
            count++;
        }
    }

    buf_to_char(buffer, ch);
    send_to_char("\n\rOLC: select apply type from this list.\n\r", ch);
    return;
}

/*
 * funky banana func for showing the flow and the way to go with the coolio
 * objectivo.
 * ok, less of the drugs: this can grab any one or all details about an object
 * and tell the builder.
 */

void olc_display_obj_vars(CHAR_DATA *ch, int info, OBJ_INDEX_DATA *obj) {

    AFFECT_DATA *paf;
    BUFFER *buffer;
    char text[MAX_STRING_LENGTH];
    EXTRA_DESCR_DATA *ptr;

    if (obj == nullptr) {
        send_to_char("OLC: error! nullptr object INFO request!\n\r", ch);
        bug("OLC: error! nullptr object INFO request: ch = %s.", ch->name);
        return;
    }
    switch (info) {
    case OLC_V_COST:
    case OLC_V_WEIGHT:
        snprintf(text, sizeof(text), "INFO: current cost: %d\n\r", obj->cost);
        send_to_char(text, ch);
        snprintf(text, sizeof(text), "      current weight: %d.\n\r", obj->weight);
        send_to_char(text, ch);
        return;
        break;
    case OLC_V_APPLY:
        if (obj->affected == nullptr) {
            send_to_char("INFO: current affects: none.\n\r", ch);
            return;
        }
        buffer = new_buf();
        for (paf = obj->affected; paf != nullptr; paf = paf->next) {
            add_buf(buffer, apply_lookup_table[paf->location].name);
            add_buf(buffer, ": ");
            add_buf(buffer, "%d\n\r", paf->modifier);
        }
        send_to_char("INFO: current affects:\n\r", ch);
        buf_to_char(buffer, ch);
        send_to_char("\n\r", ch);
        return;
        break;
    case OLC_V_FLAGS:
        send_to_char("INFO: current flags:\n\r", ch);
        obj->extra_flags = (int)flag_set(ITEM_EXTRA_FLAGS, "?", obj->extra_flags, ch);
        return;
        break;
    case OLC_V_MAGICAL:
        switch (obj->item_type) {
        case ITEM_STAFF:
        case ITEM_WAND: olc_magical_listvals_obj(ch, obj, true); break;
        default: olc_magical_listvals_obj(ch, obj, false);
        }
        return;
        break;
    case OLC_V_STRING:
        send_to_char("INFO: current strings: \n\r", ch);
        snprintf(text, sizeof(text), "Name:  %s\n\r", obj->name);
        send_to_char(text, ch);
        snprintf(text, sizeof(text), "Short: %s\n\r", obj->short_descr);
        send_to_char(text, ch);
        snprintf(text, sizeof(text), "Long:  %s\n\r", obj->description);
        send_to_char(text, ch);
        ptr = obj->extra_descr;
        if (ptr == nullptr) {
            send_to_char("Extra: (none)\n\r", ch);
            return;
        }
        for (ptr; ptr != nullptr; ptr = ptr->next) {
            snprintf(text, sizeof(text), "Extra description, using keyword(s): %s\n\r%s\n\r", ptr->keyword,
                     ptr->description);
            send_to_char(text, ch);
        }
        return;
        break;
    case OLC_V_WEAPON:
        send_to_char("INFO: current weapon status: \n\r", ch);
        snprintf(text, sizeof(text), "Class: %s \n\rAttack type: %s\n\r", weapon_class_table[obj->value[0]],
                 attack_table[obj->value[3]].name);
        send_to_char(text, ch);
        send_to_char("Weapon flags:\n\r", ch);
        obj->value[4] = (int)flag_set(WEAPON_TYPE_FLAGS, "?", obj->value[4], ch);
        return;
        break;
    case OLC_V_WEAR:
        send_to_char("INFO: current wear flags: \n\r", ch);
        obj->wear_flags = (sh_int)flag_set(ITEM_WEAR_FLAGS, "?", obj->wear_flags, ch);
        return;
        break;
    case OLC_V_MATERIAL:
        send_to_char("INFO: material type: ", ch);
        snprintf(text, sizeof(text), "%s.\n\r", material_table[obj->material].material_name);
        send_to_char(text, ch);
        return;
        break;
    default:
        send_to_char("OLC: error! unrecognised INFO request!\n\r", ch);
        bug("OLC: error! unrecognised INFO request: ch = %s.", ch->name);
        return;
    }
}

void olc_magical_listvals_obj(CHAR_DATA *ch, OBJ_INDEX_DATA *obj, bool stf_wnd) {

    char text[MAX_STRING_LENGTH];

    if (stf_wnd) {

        snprintf(text, sizeof(text), "Power: %d Max_ch: %d Current_ch: %d Spell: %s\n\r", obj->value[0], obj->value[1],
                 obj->value[2], skill_table[obj->value[3]].name);
        send_to_char("INFO: current status: \n\r", ch);
        send_to_char(text, ch);
        return;
    }
    snprintf(text, sizeof(text), "Power: %d Sp1: %s Sp2: %s\n\rSp3: %s\n\r", obj->value[0],
             skill_table[obj->value[1]].name, skill_table[obj->value[2]].name, skill_table[obj->value[3]].name);
    send_to_char("INFO: current status: \n\r", ch);
    send_to_char(text, ch);
    return;
}

void olc_add_affect_obj(CHAR_DATA *ch, int aff_loc, int amount) {

    AFFECT_DATA *paf;
    OBJ_INDEX_DATA *obj = ch->pcdata->olc->current_obj;

    for (; obj->affected != nullptr; obj->affected = obj->affected->next) {
        if (obj->affected->location == aff_loc) {
            send_to_char("OLC: object already has an affect of that type.\n\r", ch);
            send_to_char("     use object apply - to remove it first.\n\r", ch);
            return;
        }
    }
    if (affect_free == nullptr)
        paf = alloc_perm(sizeof(*paf));
    else {
        paf = affect_free;
        affect_free = affect_free->next;
    }
    paf->type = -1;
    paf->level = ch->pcdata->olc->current_obj->level;
    paf->duration = -1;
    paf->location = aff_loc;
    paf->modifier = amount;
    paf->bitvector = 0;
    paf->next = ch->pcdata->olc->current_obj->affected;
    obj->affected = paf;
    top_affect++;
    ch->pcdata->olc->current_obj = obj;
    send_to_char("OLC: object affect applied.\n\r", ch);

    return;
}

void olc_remove_affect_obj(CHAR_DATA *ch, int aff_loc) {

    AFFECT_DATA *paf = nullptr, *paf_next = nullptr;
    OBJ_INDEX_DATA *obj = ch->pcdata->olc->current_obj;
    char buf[MAX_STRING_LENGTH];
    bool found = false;
    int count = 0;

    if (obj->affected == nullptr) {
        send_to_char("OLC: this object has no affects.\n\r", ch);
        return;
    }
    paf = obj->affected;
    for (paf; paf != nullptr; paf = paf_next) {
        paf_next = paf->next;
        if (paf->location == aff_loc) {
            paf->next = affect_free;
            affect_free = paf;
            paf = nullptr;
            found = true;
        }
        if (found)
            count++;
        found = false;
    }
    if (count == 0) {
        send_to_char("OLC: object has no affects of that type.\n\r", ch);
        return;
    }
    ch->pcdata->olc->current_obj = obj;
    snprintf(buf, sizeof(buf), "OLC: %d affects stripped.\n\r", count);
    send_to_char(buf, ch);
    return;
}
